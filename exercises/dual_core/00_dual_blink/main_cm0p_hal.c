// 00_dual_blink -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
// Runs once the M4 boots it; blinks LD2 (green, PB9) faster than the M4's LD1.
//
// This compiles the HAL for cortex-m0plus (a separate library from the M4's --
// the two cores have different instruction sets, so the machine code differs).
#include "stm32wlxx_hal.h"

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void) {
    HAL_Init();

    // LD2 = PB9 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_9;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
        HAL_Delay(150); // fast
    }
}
