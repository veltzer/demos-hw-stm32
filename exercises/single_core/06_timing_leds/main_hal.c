// 06_timing_leds, HAL version. Blink LD1 (PB15) at a precise 1 Hz.
//
// The bare-metal version (main_bare.c) programs TIM16 to interrupt once a
// second. The HAL way to get a precise time base is HAL_Delay(), which counts
// real milliseconds off the 1 ms SysTick that HAL_Init() starts -- no manual
// prescaler/ARR arithmetic. (The vendored HAL set here does not include the TIM
// driver; SysTick-based timing is the idiomatic HAL approach for this anyway.)
#include "stm32wlxx_hal.h"

// HAL_Delay() counts milliseconds off this interrupt; without it HAL_Delay hangs.
void SysTick_Handler(void) { HAL_IncTick(); }

int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
        HAL_Delay(1000); // exactly once per second
    }
}
