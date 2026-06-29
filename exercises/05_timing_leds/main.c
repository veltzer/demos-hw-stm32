#include "stm32wl55xx.h"

// ISR for TIM16
void TIM16_IRQHandler(void) {
    if (TIM16->SR & TIM_SR_UIF) {
        TIM16->SR &= ~TIM_SR_UIF; // Clear update interrupt flag
        GPIOB->ODR ^= GPIO_ODR_OD15; // Toggle LD1 (blue, PB15)
    }
}

int main(void) {
    // The user LEDs are on port B (LD1 = PB15, the blue LED).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Enable TIM16 clock
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;

    // Configure TIM16: Prescaler and Auto-Reload Register for 1Hz
    // System clock is ~4MHz (LSI). Prescaler = 4000 -> Timer clock = 1kHz
    TIM16->PSC = 3999;
    // Auto-reload value = 1000 -> Interrupt every 1 second (1000 / 1kHz)
    TIM16->ARR = 999;

    // Enable update interrupt
    TIM16->DIER |= TIM_DIER_UIE;
    // Enable counter
    TIM16->CR1 |= TIM_CR1_CEN;

    // Enable TIM16 interrupt in NVIC
    NVIC_SetPriority(TIM16_IRQn, 1);
    NVIC_EnableIRQ(TIM16_IRQn);

    while (1) {
        // Main loop is now free
    }
}
