#include "stm32wl55xx.h"

int main(void) {
    // Enable GPIOA clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // Configure PA15 (LD1) as output
    GPIOA->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOA->MODER |= GPIO_MODER_MODE15_0;

    // Configure PA0 (B1) as input (it's the default, but good practice)
    GPIOA->MODER &= ~GPIO_MODER_MODE0; // Clear both bits for input

    // Optional: Enable pull-up resistor for PA0
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD0_0;


    while (1) {
        // Check if button is pressed (PA0 is low)
        if (!(GPIOA->IDR & GPIO_IDR_ID0)) {
            GPIOA->BSRR = GPIO_BSRR_BS15; // Turn LED ON
        } else {
            GPIOA->BRR = GPIO_BRR_BR15; // Turn LED OFF
        }
    }
}
