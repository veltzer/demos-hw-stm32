#include "stm32wl55xx.h"
#include <string.h>

// CPU2 (Cortex-M0+) side of the semaphore exercise. Mirrors main_cm4.c but
// drives the channel from the CPU2 registers (C2*). The LPUART1 peripheral is
// shared: the M4 brings it up, the M0+ just transmits on it -- which is exactly
// why a lock is needed so the two cores' lines don't interleave.

void delay(volatile uint32_t count) { while (count--); }

void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = str[i];
    }
}

// --- IPCC channel 2 used as a binary semaphore, from the CPU2 side ---
void acquire_lock(void) {
    // Spin until channel 2 is free (the CPU2->CPU1 occupied flag is clear),
    // then take it by setting the status from the CPU2 side.
    while (IPCC->C2TOC1SR & IPCC_C2TOC1SR_CH2F);
    IPCC->C2SCR = IPCC_C2SCR_CH2S;
}

void release_lock(void) {
    IPCC->C2SCR = IPCC_C2SCR_CH2C;
}

int main(void) {
    // The UART is already initialized by the M4 -- shared peripheral, so the
    // M0+ does not re-init it. Only the IPCC clock needs enabling here.
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    while (1) {
        acquire_lock();
        LPUART1_SendString("Message from Cortex-M0+!\r\n");
        release_lock();
        delay(750000); // a different delay so the contention is visible
    }
}
