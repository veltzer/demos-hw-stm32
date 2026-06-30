// 04_atomic_increment -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
//
// Both cores hammer the SAME integer (`counter`) in shared SRAM2, each
// incrementing it N times per round. A non-atomic `counter++` loses increments
// to the inter-core load/modify/store interleave; an atomic add does not.
//
//   M4  : drive the round handshake, increment its N-share, watch button B1
//         (PA0, active-low) and toggle RACY<->ATOMIC on each press.
//   M0+ : increment its N-share, then print expected/actual/lost per round.
//
// The M4 owns the bootstrap (PWR.C2BOOT) that releases the M0+. It reaches the
// shared block at the absolute SRAM2 base 0x20008000 through a volatile struct
// pointer; the M0+ image DEFINES it there. The layout must match the M0+'s.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h"

void SysTick_Handler(void) { HAL_IncTick(); }

#define N 1000000u   // increments PER CORE per round; total should reach 2*N

typedef struct {
    volatile uint32_t counter;   // the contended integer (both cores ++)
    volatile uint32_t mode;      // 0 = RACY (non-atomic), 1 = ATOMIC
    volatile uint32_t round;     // bumped by M4 to start each round
    volatile uint32_t m4_done;   // M4 finished its N increments this round
    volatile uint32_t m0p_done;  // M0+ finished its N increments this round
} shared_t;

#define SHARED ((volatile shared_t*)0x20008000UL)

// Do N increments of the shared counter, atomically or not per `atomic`.
static void do_increments(int atomic) {
    if (atomic) {
        for (uint32_t i = 0; i < N; i++)
            __atomic_fetch_add(&SHARED->counter, 1u, __ATOMIC_SEQ_CST);
    } else {
        // Deliberately non-atomic: load; add; store, interruptible by the M0+.
        for (uint32_t i = 0; i < N; i++)
            SHARED->counter = SHARED->counter + 1u;
    }
}

int main(void) {
    HAL_Init();

    // Defined starting state before booting CPU2.
    SHARED->counter  = 0;
    SHARED->mode     = 0;        // start in RACY mode (the interesting one)
    SHARED->round    = 0;
    SHARED->m4_done  = 0;
    SHARED->m0p_done = 0;

    LL_PWR_EnableBootC2();       // release the M0+

    // B1 = PA0 input with pull-up: idle HIGH, LOW while pressed.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_0;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &g);

    GPIO_PinState prev = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    while (1) {
        // ---- start a new round: clear state, THEN bump round -------------
        SHARED->counter  = 0;
        SHARED->m4_done  = 0;
        SHARED->m0p_done = 0;
        SHARED->round   += 1;

        int atomic = SHARED->mode ? 1 : 0;

        // Our share, then announce done and stop touching counter.
        do_increments(atomic);
        SHARED->m4_done = 1;

        // Wait for the M0+ to finish its share; the total is now final.
        while (!SHARED->m0p_done) { /* spin */ }

        // ---- poll the button between rounds, to toggle the mode ----------
        for (int t = 0; t < 50; t++) {
            GPIO_PinState level = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
            if (prev == GPIO_PIN_SET && level == GPIO_PIN_RESET) {
                SHARED->mode ^= 1u;          // RACY <-> ATOMIC
            }
            prev = level;
            HAL_Delay(4);                    // poll / debounce
        }
    }
}
