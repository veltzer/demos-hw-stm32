// 03_basic_shared_memory -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
//
// Owns the shared integer: defined here in the ".shared" section, pinned to
// SRAM2 base (0x20008000) by the M0+ linker, so the M4 reaches it at the same
// absolute address. The M4 toggles it on button presses; this core prints it
// every 2 seconds. `volatile` is essential -- the value is changed by the OTHER
// core, so it must be re-read from memory each time.
#include "stm32wlxx_hal.h"
#include <string.h>

void SysTick_Handler(void) { HAL_IncTick(); }

// ===========================================================================
// CACHE-COHERENCY DEMO TOGGLE -- flip which of the two lines below is commented
// ===========================================================================
// The STM32WL55 cores have NO hardware data cache, so the only thing that can
// make this core miss the M4's writes is the COMPILER caching the value in a
// register instead of re-reading SRAM. `volatile` is what forbids that.
//
//   * COHERENT   (default): the `volatile` line active  -> M0+ re-reads SRAM,
//                           tracks the M4's toggles, output flips 0<->1.
//   * INCOHERENT (demo)   : the non-volatile line active -> at -O2 the compiler
//                           hoists the read out of the loop (see main()), the
//                           M0+ prints the boot value FOREVER no matter how many
//                           times the M4 toggles the shared integer.
//
// To demo the bug: comment the volatile line, uncomment the plain one, rebuild.
// ---------------------------------------------------------------------------
volatile uint32_t shared_flag __attribute__((section(".shared")));  // COHERENT
//          uint32_t shared_flag __attribute__((section(".shared")));  // INCOHERENT (demo)
// ===========================================================================

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

    lpuart1.Instance        = LPUART1;
    lpuart1.Init.BaudRate   = 9600;
    lpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    lpuart1.Init.StopBits   = UART_STOPBITS_1;
    lpuart1.Init.Parity     = UART_PARITY_NONE;
    lpuart1.Init.Mode       = UART_MODE_TX_RX;
    lpuart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    HAL_UART_Init(&lpuart1);
}

static void uart_puts(const char* s) {
    HAL_UART_Transmit(&lpuart1, (uint8_t*)s, strlen(s), HAL_MAX_DELAY);
}

int main(void) {
    HAL_Init();
    shared_flag = 0;          // defined starting value
    uart_init();              // this core owns the UART here

    while (1) {
        // Snapshot then print. `volatile` forces this load to hit SRAM every
        // iteration (tracks the M4); non-volatile lets -O2 hoist it out of the
        // loop so the boot value prints forever -- the demo. (See toggle above.)
        uint32_t v = shared_flag;
        uart_puts(v ? "shared = 1\r\n" : "shared = 0\r\n");
        HAL_Delay(2000);      // 2 seconds
    }
}
