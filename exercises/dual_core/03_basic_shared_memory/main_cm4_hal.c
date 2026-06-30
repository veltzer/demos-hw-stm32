// 02_semaphores -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
//
// Both cores print over the shared LPUART1; an IPCC channel is used as a binary
// semaphore so their lines don't interleave. The M4 brings up the UART, boots
// the M0+, then loops: take the lock, print, release. Uses HAL_UART_* and the
// HAL IPCC driver.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h"
#include <string.h>

void SysTick_Handler(void) { HAL_IncTick(); }

static UART_HandleTypeDef lpuart1;
static IPCC_HandleTypeDef hipcc;

static void uart_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOA, &g);

    lpuart1.Instance        = LPUART1;
    lpuart1.Init.BaudRate   = 9600;
    lpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    lpuart1.Init.StopBits   = UART_STOPBITS_1;
    lpuart1.Init.Parity     = UART_PARITY_NONE;
    lpuart1.Init.Mode       = UART_MODE_TX_RX;
    lpuart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    HAL_UART_Init(&lpuart1);
}

static void uart_puts(const char* s) {
    HAL_UART_Transmit(&lpuart1, (uint8_t*)s, strlen(s), HAL_MAX_DELAY);
}

// IPCC channel 2 used as a binary semaphore, from the CPU1 (TX) side.
static void acquire_lock(void) {
    while (HAL_IPCC_GetChannelStatus(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_TX)
           == IPCC_CHANNEL_STATUS_OCCUPIED) {
    }
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_TX);
}
static void release_lock(void) {
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_TX);
}

int main(void) {
    HAL_Init();
    uart_init();
    LL_PWR_EnableBootC2(); // boot the M0+

    __HAL_RCC_IPCC_CLK_ENABLE();
    hipcc.Instance = IPCC;
    HAL_IPCC_Init(&hipcc);

    while (1) {
        acquire_lock();
        uart_puts("Message from Cortex-M4!\r\n");
        release_lock();
        HAL_Delay(500);
    }
}
