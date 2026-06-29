// 00_dual_blink -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
// Each core blinks its own LED independently; the M4 boots the M0+ once.
//   M4 -> LD1 (blue, PB15), slow.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h" // boot-C2 lives at the LL layer on the WL HAL

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void) {
    HAL_Init();

    // Boot the M0+ (CPU2). The WL HAL has no high-level wrapper for C2BOOT, so
    // use the Low-Layer helper (PWR is always clocked -- no clock enable needed).
    LL_PWR_EnableBootC2();

    // LD1 = PB15 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
        HAL_Delay(500); // slow
    }
}
