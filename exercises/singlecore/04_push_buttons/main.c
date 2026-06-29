#include "stm32wl55xx.h"

int main(void) {
    // Enable GPIOA (button B1 on PA0) and GPIOB (LD1 on PB15) clocks.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;

    // Configure PB15 (LD1, the blue LED) as output. The user LEDs are on port B.
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Configure PA0 (B1) as input (it's the default, but good practice)
    GPIOA->MODER &= ~GPIO_MODER_MODE0; // Clear both bits for input

    // B1 is ACTIVE-LOW (confirmed via 02_buttons_test): the pin idles high and
    // is pulled to ground while pressed. Enable the internal pull-up.
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD0_0;


    while (1) {
        // Button pressed => PA0 reads low.
        if (!(GPIOA->IDR & GPIO_IDR_ID0)) {
            GPIOB->BSRR = GPIO_BSRR_BS15; // Turn LED ON
        } else {
            GPIOB->BRR = GPIO_BRR_BR15; // Turn LED OFF
        }
    }
}
