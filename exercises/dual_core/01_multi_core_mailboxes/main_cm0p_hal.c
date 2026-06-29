// 01_multi_core_mailboxes -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
//
// Waits for a message from the M4 on IPCC channel 1; each time one arrives it
// acknowledges it (HAL_IPCC_NotifyCPU on the RX direction frees the channel) and
// toggles LD3 (red, PB11). Polling mode, HAL IPCC driver.
#include "stm32wlxx_hal.h"

void SysTick_Handler(void) { HAL_IncTick(); }

static IPCC_HandleTypeDef hipcc;

int main(void) {
    HAL_Init();

    // LD3 = PB11 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_11;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    // Bring up IPCC.
    __HAL_RCC_IPCC_CLK_ENABLE();
    hipcc.Instance = IPCC;
    HAL_IPCC_Init(&hipcc);

    while (1) {
        // A message from the M4 shows up as the RX channel being occupied.
        if (HAL_IPCC_GetChannelStatus(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_RX)
            == IPCC_CHANNEL_STATUS_OCCUPIED) {
            // Acknowledge: notifying on the RX direction clears the channel.
            HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_RX);
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_11);
        }
    }
}
