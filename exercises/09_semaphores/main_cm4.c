#include "stm32wl55xx.h"
#include <string.h>

// CPU1 (Cortex-M4) side of the semaphore exercise.
//
// The M4 and M0+ both want to print over the shared LPUART1. We use an IPCC
// channel as a simple binary semaphore (a hardware mutex): a core "takes" the
// lock by setting the channel-occupied flag and "releases" it by clearing it.
// (This is a teaching simplification -- a real mutex over IPCC also has to
//  cope with the other core racing on the same flag; HSEM is the proper tool.)

void delay(volatile uint32_t count) { while (count--); }

// --- minimal LPUART1 bring-up (PA2 TX @ 9600, routed to the ST-LINK VCP) ---
void LPUART1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    LPUART1->BRR = 106667;                 // 256 * 4MHz / 9600
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        // No TX FIFO enabled -> poll TXE_TXFNF, not TXFE.
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = str[i];
    }
}

// --- IPCC channel 2 used as a binary semaphore, from the CPU1 side ---
void acquire_lock(void) {
    // Spin until channel 2 is free (the CPU1->CPU2 occupied flag is clear),
    // then take it by setting the status.
    while (IPCC->C1TOC2SR & IPCC_C1TOC2SR_CH2F);
    IPCC->C1SCR = IPCC_C1SCR_CH2S;
}

void release_lock(void) {
    // Clear the channel from the CPU1 side -> resource free again.
    IPCC->C1SCR = IPCC_C1SCR_CH2C;
}

int main(void) {
    LPUART1_Init();

    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    while (1) {
        acquire_lock();
        LPUART1_SendString("Message from Cortex-M4!\r\n");
        release_lock();
        delay(500000); // wait a bit before trying again
    }
}
