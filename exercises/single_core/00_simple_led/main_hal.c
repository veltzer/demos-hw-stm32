// 00_simple_led, solved with the STM32 HAL. Compare with main_bare.c in this
// same folder, which pokes the registers directly. Same result: blink LD1 (the
// blue LED on PB15) forever.
//
// The HAL way: let HAL_Init() set up the SysTick 1 ms time base, describe the
// pin with a GPIO_InitTypeDef, and use HAL_GPIO_* + HAL_Delay() helpers.
#include "stm32wlxx_hal.h"

int main(void) {
    HAL_Init(); // configures SysTick (1 ms tick) and the NVIC priority grouping

    // Enable the GPIOB peripheral clock (LD1 = PB15; the user LEDs are on port B).
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP; // push-pull output
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
        HAL_Delay(500); // 500 ms, real milliseconds (vs the bare-metal busy loop)
    }
}
