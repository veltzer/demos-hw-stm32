#include "stm32wl55xx.h"

void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    // Enable GPIOB clock (the user LEDs are on port B)
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    // Configure PB15 (LD1), PB9 (LD2), PB11 (LD3) as output
    GPIOB->MODER &= ~(GPIO_MODER_MODE15_1 | GPIO_MODER_MODE9_1 | GPIO_MODER_MODE11_1);
    GPIOB->MODER |= (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE9_0 | GPIO_MODER_MODE11_0);

    // Array of pin masks for the LEDs (LD1=PB15, LD2=PB9, LD3=PB11)
    uint32_t leds[] = {GPIO_ODR_OD15, GPIO_ODR_OD9, GPIO_ODR_OD11};
    int led_index = 0;
    int direction = 1;

    while (1) {
        // Turn on the current LED
        GPIOB->BSRR = leds[led_index];
        delay(200000);
        // Turn it off
        GPIOB->BRR = leds[led_index];

        // Move to the next LED
        led_index += direction;
        if (led_index == 2 || led_index == 0) {
            direction *= -1; // Reverse direction at the ends
        }
    }
}
