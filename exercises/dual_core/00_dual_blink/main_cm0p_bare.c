// 00_dual_blink -- M0+ (CPU2) side, bare metal.
//
// Runs independently once the M4 has set PWR.C2BOOT. Blinks LD2 (green, PB9) at
// a faster rate than the M4's LD1, so you can see both cores running at once.
#include "stm32wl55xx.h"

static void delay(volatile uint32_t n) { while (n--); }

int main(void) {
    // LD2 = PB9 as push-pull output.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE9_1;
    GPIOB->MODER |= GPIO_MODER_MODE9_0;

    // Blink faster than the M4. Atomic BSRR writes -- only PB9 is touched here,
    // PB15 belongs to the M4, so the two cores never collide on the port.
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS9; // on
        delay(250000);
        GPIOB->BSRR = GPIO_BSRR_BR9; // off
        delay(250000);
    }
}
