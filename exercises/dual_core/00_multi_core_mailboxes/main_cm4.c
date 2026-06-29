#include "stm32wl55xx.h"
#include <string.h>

void delay(volatile uint32_t count) { while (count--); }

int main(void) {
    // M4 specific initializations...
    // Enable IPCC clock
    RCC->AHB3ENR |= RCC_AHB3ENR_IPCCEN;

    // Boot the Cortex-M0+ core
    // This is done by the bootloader or M4 startup code.
    // Ensure CM0+ is running its own firmware.

    while (1) {
        // Wait until channel 1 is free again (the M0+ has acknowledged the
        // previous message by clearing it). The CPU1->CPU2 channel-occupied
        // flag lives in C1TOC2SR.CH1F.
        while (IPCC->C1TOC2SR & IPCC_C1TOC2SR_CH1F);

        // Mark channel 1 occupied (set the status) to signal the M0+.
        IPCC->C1SCR = IPCC_C1SCR_CH1S;

        delay(1000000); // Send signal every so often
    }
}
