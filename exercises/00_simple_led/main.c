#include "stm32wl55xx.h"

// Busy-wait delay. count is just a loop counter, not a real time unit.
void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    // 1. Enable the clock for GPIO port A.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // 2. Configure PA15 (LD1 on the NUCLEO-WL55) as a push-pull output.
    GPIOA->MODER &= ~GPIO_MODER_MODE15_1; // clear bit 1
    GPIOA->MODER |= GPIO_MODER_MODE15_0;  // set bit 0 -> output mode

    // 3. Blink forever: toggle the LED, wait, repeat.
    while (1) {
        GPIOA->ODR ^= GPIO_ODR_OD15;
        delay(500000);
    }
}
