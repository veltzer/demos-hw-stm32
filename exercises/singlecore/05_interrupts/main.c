#include "stm32wl55xx.h"

// ISR for external interrupt line 0
void EXTI0_IRQHandler(void) {
    // Check if the interrupt is from line 0
    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        // Clear the pending bit by writing a 1 to it
        EXTI->PR1 = EXTI_PR1_PIF0;
        // Toggle LD1 (the blue LED on PB15)
        GPIOB->ODR ^= GPIO_ODR_OD15;
    }
}

int main(void) {
    // Enable GPIOA (button B1 on PA0) and GPIOB (LD1 on PB15) clocks. (SYSCFG
    // has no separate clock-enable bit on the STM32WL55 - it is always clocked -
    // so no RCC line is needed for it.)
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;

    // Configure PB15 (LD1, the blue LED) as output. The user LEDs are on port B.
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Configure PA0 (B1) as input. B1 is ACTIVE-LOW (confirmed via
    // 02_buttons_test): idles high, pulled to ground when pressed. Enable the
    // internal pull-up so the released state is a clean high.
    GPIOA->MODER &= ~GPIO_MODER_MODE0;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD0_0;

    // Connect EXTI Line 0 to PA0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0; // Clear and set to PA0

    // Trigger on the FALLING edge: the press takes PA0 from high to low.
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
