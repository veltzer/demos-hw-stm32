// 05_interrupts, HAL version. A falling edge on B1 (PA0, active-low) fires
// EXTI line 0; the handler toggles LD1 (PB15). Compare with main_bare.c, which
// pokes EXTI/SYSCFG/NVIC directly.
//
// With the HAL: GPIO_MODE_IT_FALLING wires the pin through SYSCFG+EXTI for us,
// the EXTI0 vector calls HAL_GPIO_EXTI_IRQHandler(), and that calls the weak
// HAL_GPIO_EXTI_Callback() which we override below.
#include "stm32wlxx_hal.h"

// Service the 1 ms tick HAL_Init() starts (keeps HAL_GetTick/HAL_Delay working).
void SysTick_Handler(void) { HAL_IncTick(); }

void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); // clears the pending bit, then calls back
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_0) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // toggle LD1
    }
}

int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // LD1 = PB15 output.
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    // B1 = PA0, interrupt on falling edge, pull-up (active-low button).
    GPIO_InitTypeDef btn = {0};
    btn.Pin  = GPIO_PIN_0;
    btn.Mode = GPIO_MODE_IT_FALLING;
    btn.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &btn);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    while (1) {
        // idle; the interrupt does the work
    }
}
