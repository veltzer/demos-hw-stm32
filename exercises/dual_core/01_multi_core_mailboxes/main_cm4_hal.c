// 01_multi_core_mailboxes -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
//
// The M4 boots the M0+, then once a second posts a message on IPCC channel 1.
// The M0+ toggles LD3 each time it receives one. Uses the HAL IPCC driver
// (HAL_IPCC_*) in polling mode.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h" // C2BOOT lives at the LL layer on the WL HAL

void SysTick_Handler(void) { HAL_IncTick(); }

static IPCC_HandleTypeDef hipcc;

int main(void) {
    HAL_Init();

    // Boot the M0+ (CPU2).
    LL_PWR_EnableBootC2();

    // Bring up IPCC.
    __HAL_RCC_IPCC_CLK_ENABLE();
    hipcc.Instance = IPCC;
    HAL_IPCC_Init(&hipcc);

    while (1) {
        // Wait until channel 1 is free (the M0+ has acknowledged the last one),
        // then post a new message by marking the TX channel occupied.
        while (HAL_IPCC_GetChannelStatus(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_TX)
               == IPCC_CHANNEL_STATUS_OCCUPIED) {
        }
        HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_TX);
        HAL_Delay(1000);
    }
}
