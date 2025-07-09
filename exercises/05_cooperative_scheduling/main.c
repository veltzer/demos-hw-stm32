#include "stm32wl55xx.h"

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
    HAL_Init(); // Initializes SysTick for HAL_GetTick()

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
