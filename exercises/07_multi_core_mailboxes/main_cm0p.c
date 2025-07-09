#include "stm32wl55xx.h"

int main(void) {
    // M0+ specific initializations...
    // Setup GPIO for an LED (e.g., LD3 on PA10)
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODE10_1;
    GPIOA->MODER |= GPIO_MODER_MODE10_0;

    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    // Enable receive interrupt on IPCC channel 1 for CPU2
    IPCC->C2CR |= IPCC_C2CR_RXOIE;

    while (1) {
        // Check if a message is pending on channel 1 from M4
        if (IPCC->C2SR & IPCC_C2SR_CH1S) {
            // Clear the flag to acknowledge receipt
            IPCC->C2SCR = IPCC_C2SCR_CH1C;
            // Toggle the LED
            GPIOA->ODR ^= GPIO_ODR_OD10;
        }
    }
}
