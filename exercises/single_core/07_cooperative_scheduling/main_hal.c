// 07_cooperative_scheduling, HAL version. A tiny round-robin cooperative
// scheduler: two tasks blink LD1 (PB15) and LD2 (PB9) at different rates, each
// yielding back to the loop. Compare with main_bare.c, whose "tick" was a fake
// free-running counter; here HAL_GetTick() gives a real 1 ms time base for free
// (HAL_Init() starts the SysTick that increments it).
#include "stm32wlxx_hal.h"

// HAL_GetTick() reads the millisecond counter this interrupt increments.
void SysTick_Handler(void) { HAL_IncTick(); }

// --- Task 1: blink LD1 (blue, PB15) slowly (every 500 ms) ---
static void task1_blink_slow(void) {
    static uint32_t last = 0;
    if (HAL_GetTick() - last >= 500) {
        last = HAL_GetTick();
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
    }
}

// --- Task 2: blink LD2 (green, PB9) quickly (every 100 ms) ---
static void task2_blink_fast(void) {
    static uint32_t last = 0;
    if (HAL_GetTick() - last >= 100) {
        last = HAL_GetTick();
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
    }
}

static void (*tasks[])(void) = { task1_blink_slow, task2_blink_fast };
static const int NUM_TASKS = sizeof(tasks) / sizeof(tasks[0]);

int main(void) {
    HAL_Init();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_15 | GPIO_PIN_9; // LD1, LD2
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    while (1) {
        for (int i = 0; i < NUM_TASKS; i++) {
            tasks[i](); // each task runs briefly and returns (cooperative)
        }
    }
}
