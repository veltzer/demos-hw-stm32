#include "stm32wl55xx.h"
#include <string.h>

// --- UART Functions (from previous exercise) ---
void LPUART1_Init(void) { /* ... Same as before ... */ }
void LPUART1_SendString(const char* str) { /* ... Same as before ... */ }

// --- Semaphore Functions using IPCC Channel 2 ---
#define LOCK_CHANNEL 2
#define LOCK_CHANNEL_MASK_TX (1 << LOCK_CHANNEL)
#define LOCK_CHANNEL_MASK_RX (1 << LOCK_CHANNEL)

void acquire_lock(void) {
    // Loop until we successfully set the flag, meaning we acquired the lock.
    // This is a "test-and-set" operation.
    while (IPCC->C1SCR & LOCK_CHANNEL_MASK_TX); // Wait while channel is busy
    IPCC->C1SCR = LOCK_CHANNEL_MASK_TX; // Set the channel flag (acquire lock)
}

void release_lock(void) {
    // Clear the channel flag by writing to the "clear" register.
    // This signals that the resource is free.
    IPCC->C1SCR = (LOCK_CHANNEL_MASK_TX << 16); // This is how you clear it
}


int main(void) {
    // Standard initializations
    HAL_Init();
    LPUART1_Init();

    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    // Boot the Cortex-M0+ (usually handled in system_stm32wlxx.c)
    
    while (1) {
        acquire_lock();
        LPUART1_SendString("Message from Cortex-M4!\r\n");
        release_lock();
        HAL_Delay(500); // Wait a bit before trying again
    }
}
