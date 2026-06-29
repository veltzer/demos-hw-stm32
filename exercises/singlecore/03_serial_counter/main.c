#include "stm32wl55xx.h"
#include <string.h>

void delay(volatile uint32_t count) { while (count--); }

void LPUART1_Init(void) {
    // Enable GPIOA and LPUART1 clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    // Configure PA2 (TX) and PA3 (RX) to alternate function mode (AF8).
    // PA2/PA3 are routed to the ST-LINK virtual COM port (/dev/ttyACM0).
    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    // Set baud rate to 9600 (assuming system clock is ~4MHz LSI).
    // LPUART BRR = 256 * f_ck / baud = 256 * 4000000 / 9600 = 106667
    LPUART1->BRR = 106667;

    // Enable UART transmitter and the UART itself
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void LPUART1_SendChar(char c) {
    // Wait until the transmit data register can accept a byte, then write it.
    // We do NOT enable the TX FIFO (CR1.FIFOEN), so the right flag is
    // TXE_TXFNF (transmit data register empty) -- NOT TXFE, which only asserts
    // in FIFO mode and would otherwise spin here forever.
    while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
    LPUART1->TDR = c;
}

void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        LPUART1_SendChar(str[i]);
    }
}

// Print an unsigned integer in decimal (no stdio / printf on bare metal).
void LPUART1_SendUint(uint32_t value) {
    char buf[11];      // enough for 2^32-1 (10 digits) + NUL
    int pos = 10;
    buf[pos] = '\0';
    do {
        buf[--pos] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0);
    LPUART1_SendString(&buf[pos]);
}

int main(void) {
    LPUART1_Init();

    // Print "i is 4", "i is 5", ... forever, one line per second-ish.
    uint32_t i = 4;
    while (1) {
        LPUART1_SendString("i is ");
        LPUART1_SendUint(i);
        LPUART1_SendString("\r\n");
        i++;
        delay(1000000);
    }
}
