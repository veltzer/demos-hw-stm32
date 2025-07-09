#include "stm32wl55xx.h"

// ISR for external interrupt line 0
void EXTI0_IRQHandler(void) {
    // Check if the interrupt is from line 0
    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        // Clear the pending bit by writing a 1 to it
        EXTI->PR1 = EXTI_PR1_PIF0;
        // Toggle the LED
        GPIOA->ODR ^= GPIO_ODR_OD15;
    }
}

int main(void) {
    // Enable GPIOA and SYSCFG clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Configure PA15 (LD1) as output
    GPIOA->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOA->MODER |= GPIO_MODER_MODE15_0;

    // Configure PA0 (B1) as input
    GPIOA->MODER &= ~GPIO_MODER_MODE0;

    // Connect EXTI Line 0 to PA0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0; // Clear and set to PA0

    // Configure EXTI line 0 for falling edge trigger
    EXTI->IMR1 |= EXTI_IMR1_IM0;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT0;
    EXTI->FTSR1 |= EXTI_FTSR1_FT0;

    // Enable EXTI0 interrupt in NVIC
    NVIC_SetPriority(EXTI0_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_IRQn);

    while (1) {
        // The CPU can be doing other things or be in a low-power state here
        // The interrupt will wake it up when the button is pressed.
    }
}
