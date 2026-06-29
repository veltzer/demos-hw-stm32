// 01_led_patterns, HAL version. Cylon scanner across the three user LEDs
// (LD1=PB15, LD2=PB9, LD3=PB11). Compare with main_bare.c.
#include "stm32wlxx_hal.h"

int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    const uint16_t leds[] = { GPIO_PIN_15, GPIO_PIN_9, GPIO_PIN_11 };
    const int n = sizeof(leds) / sizeof(leds[0]);

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin   = leds[0] | leds[1] | leds[2];
    HAL_GPIO_Init(GPIOB, &g);

    int i = 0, dir = 1;
    while (1) {
        HAL_GPIO_WritePin(GPIOB, leds[i], GPIO_PIN_SET);
        HAL_Delay(150);
        HAL_GPIO_WritePin(GPIOB, leds[i], GPIO_PIN_RESET);

        i += dir;
        if (i == n - 1 || i == 0) {
            dir = -dir; // bounce at the ends
        }
    }
}
