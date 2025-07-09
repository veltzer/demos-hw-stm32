#include "stm32wl55xx.h"

// Simple delay function
void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    // 1. Enable GPIOA clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // 2. Configure PA15 (connected to LD1) as output
    GPIOA->MODER &= ~GPIO_MODER_MODE15_1; // Clear bit 1 to set to output mode
    GPIOA->MODER |= GPIO_MODER_MODE15_0;  // Set bit 0

    while (1) {
        // 3. Toggle the LED
        GPIOA->ODR ^= GPIO_ODR_OD15;
        delay(500000); // Adjust for desired blink speed
    }
}
