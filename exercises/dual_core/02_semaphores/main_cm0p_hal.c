// 02_semaphores -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
//
// Mirrors the M4 side but drives the IPCC channel from the CPU2 (RX) direction.
// The LPUART1 is shared -- the M4 already initialised it -- so the M0+ only
// builds a UART handle to transmit on it.
#include "stm32wlxx_hal.h"
#include <string.h>

void SysTick_Handler(void) { HAL_IncTick(); }

static UART_HandleTypeDef lpuart1;
static IPCC_HandleTypeDef hipcc;

static void uart_puts(const char* s) {
    HAL_UART_Transmit(&lpuart1, (uint8_t*)s, strlen(s), HAL_MAX_DELAY);
}

// IPCC channel 2 used as a binary semaphore, from the CPU2 (RX) side.
static void acquire_lock(void) {
    while (HAL_IPCC_GetChannelStatus(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_RX)
           == IPCC_CHANNEL_STATUS_OCCUPIED) {
    }
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_RX);
}
static void release_lock(void) {
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_RX);
}

int main(void) {
    HAL_Init();

    // The UART pins/clock are already set up by the M4 (shared peripheral); we
    // only need a handle to transmit through it.
    lpuart1.Instance        = LPUART1;
    lpuart1.Init.BaudRate   = 9600;
    lpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    lpuart1.Init.StopBits   = UART_STOPBITS_1;
    lpuart1.Init.Parity     = UART_PARITY_NONE;
    lpuart1.Init.Mode       = UART_MODE_TX_RX;
    lpuart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    HAL_UART_Init(&lpuart1);

    __HAL_RCC_IPCC_CLK_ENABLE();
    hipcc.Instance = IPCC;
    HAL_IPCC_Init(&hipcc);

    while (1) {
        acquire_lock();
        uart_puts("Message from Cortex-M0+!\r\n");
        release_lock();
        HAL_Delay(750);
    }
}
