#include "stm32wl55xx.h"
#include <string.h>

// 03_basic_shared_memory -- M0+ (CPU2) side, bare metal.
//
// Owns the shared integer: it is defined here in the ".shared" section, which
// the M0+ linker pins to the base of SRAM2 (0x20008000). Both cores reach it at
// that absolute address (the M4 pokes it directly -- see main_cm4_bare.c). The
// M4 toggles it on button presses; this core just prints it every 2 seconds.
//
// `volatile` is essential: the value is changed by the OTHER core, so the
// compiler must re-read it from memory on every loop, not cache it in a
// register.

// The shared flag. Placed at SRAM2 base by the linker's ".shared" section.
volatile uint32_t shared_flag __attribute__((section(".shared")));

static void delay(volatile uint32_t count) { while (count--); }

// --- minimal LPUART1 bring-up (PA2 TX @ 9600, routed to the ST-LINK VCP) ---
// The M0+ runs first to print, and we cannot assume the M4 has set the UART up
// yet, so this core initialises it itself.
static void LPUART1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    LPUART1->BRR = 106667;                 // 256 * 4MHz / 9600
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

static void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = str[i];
    }
}

int main(void) {
    shared_flag = 0;          // defined starting value
    LPUART1_Init();

    while (1) {
        // Re-read the shared flag (volatile) -- the M4 may have toggled it.
        LPUART1_SendString(shared_flag ? "shared = 1\r\n" : "shared = 0\r\n");
        delay(2000000);       // ~2 seconds at the 4MHz default clock
    }
}
