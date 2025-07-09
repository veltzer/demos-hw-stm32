#include "stm32wl55xx.h"
#include <string.h>

void delay(volatile uint32_t count) { while (count--); }

void LPUART1_Init(void) {
    // Enable GPIOA and LPUART1 clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    // Configure PA2 (TX) and PA3 (RX) to alternate function mode (AF8)
    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    // Set baud rate to 9600 (assuming system clock is ~4MHz LSI)
    // 256 * 4000000 / 9600 = 106667
    LPUART1->BRR = 106667;

    // Enable UART transmitter and receiver, and enable UART itself
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void LPUART1_SendChar(char c) {
    // Wait for transmit data register to be empty
    while (!(LPUART1->ISR & USART_ISR_TXE));
    LPUART1->TDR = c;
}

void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        LPUART1_SendChar(str[i]);
    }
}

int main(void) {
    LPUART1_Init();
    while (1) {
        LPUART1_SendString("Hello!\r\n");
        delay(1000000);
    }
}
