#include "stm32wl55xx.h"

// NOTE: this exercise is a stub that COMPILES but does not yet keep real time.
// HAL_GetTick() here is a placeholder free-running counter (not millisecond
// based). Wiring it to a real time base (e.g. configuring SysTick to tick every
// 1 ms and incrementing a counter in SysTick_Handler) is the actual exercise.
static volatile uint32_t fake_tick;
static uint32_t HAL_GetTick(void) { return ++fake_tick; }

// --- Task 1: Blink LD1 slowly ---
void task1_blink_slow(void) {
    static uint32_t last_run = 0;
    if ( (HAL_GetTick() - last_run) > 500 ) { // Using HAL_GetTick for simplicity here
        GPIOA->ODR ^= GPIO_ODR_OD15;
        last_run = HAL_GetTick();
    }
}

// --- Task 2: Blink LD2 quickly ---
void task2_blink_fast(void) {
    static uint32_t last_run = 0;
    if ( (HAL_GetTick() - last_run) > 100 ) {
        GPIOA->ODR ^= GPIO_ODR_OD11;
        last_run = HAL_GetTick();
    }
}

// Array of task function pointers
void (*tasks[])(void) = {
    task1_blink_slow,
    task2_blink_fast,
};
const int NUM_TASKS = sizeof(tasks) / sizeof(void*);

int main(void) {
    // TODO: set up a real 1 ms time base here (SysTick) so HAL_GetTick()
    // returns milliseconds. For now it is just a free-running counter.

    // Setup GPIOs for LD1 and LD2 (as in exercise 2)
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(GPIO_MODER_MODE15_1 | GPIO_MODER_MODE11_1);
    GPIOA->MODER |= (GPIO_MODER_MODE15_0 | GPIO_MODER_MODE11_0);


    while (1) {
        // The "scheduler" loop
        for (int i = 0; i < NUM_TASKS; i++) {
            tasks[i]();
        }
    }
}
