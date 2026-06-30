#include "stm32wl55xx.h"

// 03_basic_shared_memory -- M4 (CPU1) side, bare metal.
//
// The two cores share ONE integer that lives in SRAM2 (reachable from both
// cores over the bus matrix). The M0+ image defines it in a reserved ".shared"
// slot pinned to the base of SRAM2 by its linker script (see
// exercises/common/STM32WL55JCIX_FLASH_CM0PLUS.ld), so its address is fixed at
// 0x20008000. The M4 has no symbol for it -- it just pokes that absolute
// address through a volatile pointer.
//
//   M4  : watch user button B1 (PA0, active-low). On each press, toggle the
//         shared integer 0<->1.
//   M0+ : print the shared integer over the UART every 2 seconds.
//
// The M4 owns the bootstrap: it releases the M0+ (PWR.C2BOOT) so CPU2 runs.

// Base of SRAM2 -- the reserved ".shared" slot. Must match the M0+ linker.
#define SHARED_FLAG (*(volatile uint32_t*)0x20008000UL)

static void delay(volatile uint32_t count) { while (count--); }

int main(void) {
    // Make sure the shared flag has a defined starting value before we boot the
    // M0+. (The M0+ also clears it, but doing it here too removes any doubt
    // about who initialises shared state -- the writer/owner side.)
    SHARED_FLAG = 0;

    // Boot the M0+: SBRV already points at flash bank 2 where the M0+ image is
    // linked, so setting C2BOOT starts CPU2 from its reset vector. PWR is always
    // clocked on the STM32WL, so we just write CR4.
    PWR->CR4 |= PWR_CR4_C2BOOT;

    // B1 = PA0 as input with a pull-up: idle HIGH, reads LOW while pressed.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODE0;                 // input
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |=  GPIO_PUPDR_PUPD0_0;               // pull-up

    // Edge detect: remember the previous level so we act once per press, on the
    // HIGH->LOW transition (the moment the button goes down).
    int prev = (GPIOA->IDR & GPIO_IDR_ID0) ? 1 : 0;

    while (1) {
        int level = (GPIOA->IDR & GPIO_IDR_ID0) ? 1 : 0;
        if (prev == 1 && level == 0) {                 // press edge
            SHARED_FLAG ^= 1u;                         // 0<->1
        }
        prev = level;
        delay(20000);                                  // crude debounce / poll
    }
}
