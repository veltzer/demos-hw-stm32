// 00_dual_blink -- M4 (CPU1) side, bare metal.
//
// The simplest dual-core demo: each core blinks its own LED, at its own rate,
// with NO communication between them. The only interaction is the one
// unavoidable bootstrap step -- the M4 must release the M0+ to run (PWR.C2BOOT).
// After that the two cores run completely independently.
//
//   M4  -> LD1 (blue,  PB15), slow
//   M0+ -> LD2 (green, PB9),  fast   (see main_cm0p_bare.c)
#include "stm32wl55xx.h"

static void delay(volatile uint32_t n) { while (n--); }

int main(void) {
    // Boot the M0+: set C2BOOT so CPU2 starts from its reset vector (SBRV
    // already points at flash bank 2, where the M0+ image is linked -- see the
    // linker script / CLAUDE.md). PWR has no clock-enable bit on the STM32WL --
    // it is always clocked -- so we just write CR4 directly.
    PWR->CR4 |= PWR_CR4_C2BOOT;

    // LD1 = PB15 as push-pull output (user LEDs are on port B).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Blink slowly. Use BSRR (atomic set/reset) rather than read-modify-write
    // on ODR, so there is no chance of a race with the M0+ on the shared port.
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS15; // on
        delay(800000);
        GPIOB->BSRR = GPIO_BSRR_BR15; // off
        delay(800000);
    }
}
