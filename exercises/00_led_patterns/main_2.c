#include "stm32wl55xx.h"

void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    // Enable GPIOA clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // Configure PA15, PA11, PA10 as output
    GPIOA->MODER &= ~(GPIO_MODER_MODE15_1 | GPIO_MODER_MODE11_1 | GPIO_MODER_MODE10_1);
    GPIOA->MODER |= (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE11_0 | GPIO_MODER_MODE10_0);

    // Array of pin masks for the LEDs
    uint32_t leds[] = {GPIO_ODR_OD15, GPIO_ODR_OD11, GPIO_ODR_OD10};
    int led_index = 0;
    int direction = 1;

    while (1) {
        // Turn on the current LED
        GPIOA->BSRR = leds[led_index];
        delay(200000);
        // Turn it off
        GPIOA->BRR = leds[led_index];

        // Move to the next LED
        led_index += direction;
        if (led_index == 2 || led_index == 0) {
            direction *= -1; // Reverse direction at the ends
        }
    }
}
