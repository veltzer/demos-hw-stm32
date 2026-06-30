#include "stm32wl55xx.h"
#include <string.h>

// 03_basic_shared_memory -- M0+ (CPU2) side, bare metal.
//
// Owns the shared integer: it is defined here in the ".shared" section, which
// the M0+ linker pins to the base of SRAM2 (0x20008000). Both cores reach it at
// that absolute address (the M4 pokes it directly -- see main_cm4_bare.c). The
// M4 toggles it on button presses; this core just prints it every 2 seconds.
//
// `volatile` is essential: the value is changed by the OTHER core, so the
// compiler must re-read it from memory on every loop, not cache it in a
// register.

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

static void delay(volatile uint32_t count) { while (count--); }

// --- minimal LPUART1 bring-up (PA2 TX @ 9600, routed to the ST-LINK VCP) ---
// The M0+ runs first to print, and we cannot assume the M4 has set the UART up
// yet, so this core initialises it itself.
static void LPUART1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    LPUART1->BRR = 106667;                 // 256 * 4MHz / 9600
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

static void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = str[i];
    }
}

int main(void) {
    shared_flag = 0;          // defined starting value
    LPUART1_Init();

    while (1) {
        // Snapshot the shared flag, then print it. When `shared_flag` is
        // `volatile` this load is forced to hit SRAM every iteration, so it
        // tracks the M4's toggles. When it is NOT volatile, the compiler proves
        // nothing in this loop writes `shared_flag` and hoists the load out of
        // the loop entirely -- `v` is read once, before the loop, and printed
        // forever: the cache-coherency bug. (See the toggle near the top.)
        uint32_t v = shared_flag;
        LPUART1_SendString(v ? "shared = 1\r\n" : "shared = 0\r\n");
        delay(2000000);       // ~2 seconds at the 4MHz default clock
    }
}
