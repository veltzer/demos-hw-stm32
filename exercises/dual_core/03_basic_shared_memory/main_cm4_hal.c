// 03_basic_shared_memory -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
//
// The two cores share ONE integer in SRAM2 (reachable from both cores). The M0+
// image defines it in a reserved ".shared" slot pinned to SRAM2 base
// (0x20008000) by its linker; the M4 pokes that absolute address directly.
//
//   M4  : watch button B1 (PA0, active-low); toggle the shared integer per press.
//   M0+ : print the shared integer over the UART every 2 seconds.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h"

void SysTick_Handler(void) { HAL_IncTick(); }

// Base of SRAM2 -- the reserved ".shared" slot. Must match the M0+ linker.
#define SHARED_FLAG (*(volatile uint32_t*)0x20008000UL)

int main(void) {
    HAL_Init();

    SHARED_FLAG = 0;             // defined starting value before booting CPU2
    LL_PWR_EnableBootC2();       // release the M0+

    // B1 = PA0 input with pull-up: idle HIGH, LOW while pressed.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_0;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &g);

    // Edge detect: act once per press, on the HIGH->LOW transition.
    GPIO_PinState prev = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    while (1) {
        GPIO_PinState level = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        if (prev == GPIO_PIN_SET && level == GPIO_PIN_RESET) {
            SHARED_FLAG ^= 1u;   // 0<->1
        }
        prev = level;
        HAL_Delay(20);           // poll / debounce
    }
}
