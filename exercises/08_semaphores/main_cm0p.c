#include "stm32wl55xx.h"
#include <string.h>

// --- UART Functions ---
void LPUART1_Init(void) { /* ... Same as before ... */ }
void LPUART1_SendString(const char* str) { /* ... Same as before ... */ }

// --- Semaphore Functions using IPCC Channel 2 ---
#define LOCK_CHANNEL 2
// The M0+ uses the CPU2 registers (C2)
#define LOCK_CHANNEL_MASK_TX (1 << LOCK_CHANNEL)
#define LOCK_CHANNEL_MASK_RX (1 << LOCK_CHANNEL)

void acquire_lock(void) {
    // Wait until we can set the channel flag from the M0+'s perspective
    while (IPCC->C2SCR & LOCK_CHANNEL_MASK_TX);
    IPCC->C2SCR = LOCK_CHANNEL_MASK_TX; // Acquire lock
}

void release_lock(void) {
    // Clear the flag from the M0+'s perspective
    IPCC->C2SCR = (LOCK_CHANNEL_MASK_TX << 16);
}


int main(void) {
    // M0+ initializations
    HAL_Init(); 
    // The UART is already initialized by the M4, so no need to re-init.
    // This is a key concept of shared peripherals.

    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    while (1) {
        acquire_lock();
        LPUART1_SendString("Message from Cortex-M0+!\r\n");
        release_lock();
        HAL_Delay(750); // Use a different delay to see the contention clearly
    }
}
