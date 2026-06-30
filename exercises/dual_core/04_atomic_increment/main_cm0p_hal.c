// 04_atomic_increment -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
//
// Both cores hammer the SAME integer (`counter`) in shared SRAM2, each
// incrementing it N times per round. A non-atomic `counter++` loses increments
// to the inter-core load/modify/store interleave; serialising the ++ with the
// HSEM hardware semaphore does not. The M4 toggles the mode on a button press;
// this core is the second incrementer AND the printer.
//
// This core OWNS the shared struct: defined here in the ".shared" section,
// pinned to SRAM2 base (0x20008000) by the M0+ linker, so the M4 reaches it at
// the same absolute address. Everything is `volatile` -- the OTHER core mutates
// it -- but note `volatile` does NOT make `counter++` atomic; it only forbids
// register caching. The M0+ (ARMv6-M) has no LDREX/STREX, so genuine M4<->M0+
// mutual exclusion uses the HSEM peripheral. That is the whole point here.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_hsem.h"
#include <stdio.h>
#include <string.h>

void SysTick_Handler(void) { HAL_IncTick(); }

#define N 1000000u   // increments PER CORE per round; total should reach 2*N

// The shared inter-core block. Layout must match the M4's struct pointer.
typedef struct {
    volatile uint32_t counter;   // the contended integer (both cores ++)
    volatile uint32_t mode;      // 0 = RACY (non-atomic), 1 = ATOMIC
    volatile uint32_t round;     // bumped by M4 to start each round
    volatile uint32_t m4_done;   // M4 finished its N increments this round
    volatile uint32_t m0p_done;  // M0+ finished its N increments this round
} shared_t;

volatile shared_t shared __attribute__((section(".shared")));

// HSEM semaphore index used to guard `counter`. Both cores must agree on it.
#define HSEM_ID 0u

static UART_HandleTypeDef lpuart1;

static void uart_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOA, &g);

    lpuart1.Instance = LPUART1;
    lpuart1.Init.BaudRate = 9600;
    lpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    lpuart1.Init.StopBits = UART_STOPBITS_1;
    lpuart1.Init.Parity = UART_PARITY_NONE;
    lpuart1.Init.Mode = UART_MODE_TX_RX;
    lpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&lpuart1);
}

static void uart_puts(const char *s) {
    HAL_UART_Transmit(&lpuart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

// Do N increments of the shared counter, serialised by HSEM or not per `atomic`.
// LL_HSEM_1StepLock returns 0 when the lock is held by us, so spin until 0.
static void do_increments(int atomic) {
    if (atomic) {
        for (uint32_t i = 0; i < N; i++) {
            while (LL_HSEM_1StepLock(HSEM, HSEM_ID)) { /* spin */ }
            shared.counter = shared.counter + 1u;
            LL_HSEM_ReleaseLock(HSEM, HSEM_ID, 0);
        }
    } else {
        // Deliberately unguarded: load; add; store, interruptible by the M4.
        for (uint32_t i = 0; i < N; i++)
            shared.counter = shared.counter + 1u;
    }
}

int main(void) {
    HAL_Init();
    __HAL_RCC_HSEM_CLK_ENABLE();   // this core's AHB3 gate for the HSEM
    uart_init();                   // this core owns the UART

    uint32_t last_round = shared.round;

    while (1) {
        // Wait for the M4 to start a new round (it clears counter + done-flags,
        // then bumps `round`).
        while (shared.round == last_round) { /* spin */ }
        last_round = shared.round;

        int atomic = shared.mode ? 1 : 0;

        // Our share, then announce done and STOP touching counter so we don't
        // keep contending while the M4 is still going.
        do_increments(atomic);
        shared.m0p_done = 1;

        // Wait for the M4's share to finish too; now the total is final.
        while (!shared.m4_done) { /* spin */ }

        char line[80];
        snprintf(line, sizeof(line), "mode=%-6s expected=%lu actual=%lu lost=%lu\r\n",
                 atomic ? "ATOMIC" : "RACY",
                 (unsigned long)(2u * N),
                 (unsigned long)shared.counter,
                 (unsigned long)(2u * N - shared.counter));
        uart_puts(line);

        HAL_Delay(100);  // readable gap; the M4 paces the rounds anyway
    }
}
