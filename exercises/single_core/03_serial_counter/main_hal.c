// 03_serial_counter, HAL version. Prints "i is 4", "i is 5", ... over the
// ST-LINK virtual COM port (LPUART1, PA2/PA3, 9600 8N1). Compare with
// main_bare.c, which drives LPUART1's registers by hand.
#include "stm32wlxx_hal.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef lpuart1;

static void uart_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();

    // PA2 (TX) / PA3 (RX) as alternate function AF8 = LPUART1.
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
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

int main(void) {
    HAL_Init();
    uart_init();

    uint32_t i = 4;
    char line[32];
    while (1) {
        // snprintf is fine here (newlib-nano); the bare version hand-rolls this.
        snprintf(line, sizeof(line), "i is %lu\r\n", (unsigned long)i);
        uart_puts(line);
        i++;
        HAL_Delay(1000);
    }
}
