#include "stm32wl55xx.h"

int main(void) {
    // M0+ specific initializations.
    // Setup GPIO for LD3 (the red LED on PB11; the user LEDs are on port B).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE11_1;
    GPIOB->MODER |= GPIO_MODER_MODE11_0;

    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    // Poll channel 1 here (interrupts are an extension). Unmask channel 1's
    // RX-occupied event on the CPU2 side by clearing its mask bit.
    IPCC->C2MR &= ~IPCC_C2MR_CH1OM;

    while (1) {
        // A message from the M4 shows up as the CPU1->CPU2 occupied flag.
        if (IPCC->C1TOC2SR & IPCC_C1TOC2SR_CH1F) {
            // Acknowledge receipt: clear the channel from the CPU2 side.
            IPCC->C2SCR = IPCC_C2SCR_CH1C;
            // Toggle the LED.
            GPIOB->ODR ^= GPIO_ODR_OD11;
        }
    }
}
