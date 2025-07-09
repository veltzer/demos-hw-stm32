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
        // Wait for IPCC channel 1 to be free on CPU1 side
        while (IPCC->C1SR & IPCC_C1SR_CH1S);

        // Set a flag on IPCC channel 1 to signal the M0+
        IPCC->C1SCR = IPCC_C1SCR_CH1S;

        delay(1000000); // Send signal every so often
    }
}
