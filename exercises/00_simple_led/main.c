#include "stm32wl55xx.h"

// Busy-wait delay. count is just a loop counter, not a real time unit.
void delay(volatile uint32_t count) {
    while (count--);
}

int main(void) {
    // 1. Enable the clock for GPIO port B.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    // 2. Configure PB15 (LD1, the blue LED on the NUCLEO-WL55JC) as a
    //    push-pull output. (The user LEDs are on port B, not port A:
    //    LD1=PB15, LD2=PB9, LD3=PB11.)
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1; // clear bit 1
    GPIOB->MODER |= GPIO_MODER_MODE15_0;  // set bit 0 -> output mode

    // 3. Blink forever: toggle the LED, wait, repeat.
    while (1) {
        GPIOB->ODR ^= GPIO_ODR_OD15;
        delay(500000);
    }
}
