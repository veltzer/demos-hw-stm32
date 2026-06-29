// 04_buttons_and_leds, HAL version. LD1 (PB15) is on while B1 (PA0, active-low)
// is held. Compare with main_bare.c.
#include "stm32wlxx_hal.h"

// Service the 1 ms tick HAL_Init() starts (keeps HAL_GetTick/HAL_Delay working).
void SysTick_Handler(void) { HAL_IncTick(); }

int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // LD1 = PB15, push-pull output.
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    // B1 = PA0, input with pull-up (active-low: pressed reads low).
    GPIO_InitTypeDef btn = {0};
    btn.Pin  = GPIO_PIN_0;
    btn.Mode = GPIO_MODE_INPUT;
    btn.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &btn);

    while (1) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);   // pressed -> on
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // released -> off
        }
    }
}
