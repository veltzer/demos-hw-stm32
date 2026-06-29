// 02_buttons_test, HAL version. Watches all user-button pins and prints each
// level transition over LPUART1 (9600 8N1). Same diagnostic as main_bare.c, but
// using HAL_GPIO_ReadPin + HAL_UART_Transmit.
//
//   B1 = PA0 (default) / PC13 (alt)   B2 = PA1   B3 = PC6
#include "stm32wlxx_hal.h"
#include <string.h>

static UART_HandleTypeDef lpuart1;

static void uart_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();
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

typedef struct {
    const char*    name;
    GPIO_TypeDef*  port;
    uint16_t       pin;
} button_t;

int main(void) {
    HAL_Init();
    uart_init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // All monitored pins: input with pull-up (defined idle = high).
    GPIO_InitTypeDef gA = {0};
    gA.Pin  = GPIO_PIN_0 | GPIO_PIN_1;
    gA.Mode = GPIO_MODE_INPUT;
    gA.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gA);

    GPIO_InitTypeDef gC = {0};
    gC.Pin  = GPIO_PIN_6 | GPIO_PIN_13;
    gC.Mode = GPIO_MODE_INPUT;
    gC.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gC);

    button_t buttons[] = {
        { "B1 (PA0) ",  GPIOA, GPIO_PIN_0  },
        { "B2 (PA1) ",  GPIOA, GPIO_PIN_1  },
        { "B3 (PC6) ",  GPIOC, GPIO_PIN_6  },
        { "B1 (PC13)",  GPIOC, GPIO_PIN_13 },
    };
    const int N = sizeof(buttons) / sizeof(buttons[0]);

    GPIO_PinState last[sizeof(buttons) / sizeof(buttons[0])];
    for (int i = 0; i < N; i++) {
        last[i] = HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin);
    }

    uart_puts("buttons_test (HAL): press B1/B2/B3 -- transitions print below\r\n");

    while (1) {
        for (int i = 0; i < N; i++) {
            GPIO_PinState s = HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin);
            if (s != last[i]) {
                last[i] = s;
                uart_puts(buttons[i].name);
                uart_puts(s == GPIO_PIN_SET ? " HIGH\r\n" : " LOW\r\n");
            }
        }
        HAL_Delay(5); // poll / crude debounce
    }
}
