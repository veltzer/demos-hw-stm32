// 05_proto_os -- M0+ (CPU2) side, HAL. Compare with main_cm0p_bare.c.
//
// The M0+ runs its own copy of the cooperative proto-OS (setjmp/longjmp
// round-robin scheduler), driving LD2 (green, PB9) via the HAL. Runs only once
// the M4 has released it with C2BOOT. Blinks faster than the M4's LD1.
#include "stm32wlxx_hal.h"
#include <setjmp.h>

void SysTick_Handler(void) { HAL_IncTick(); }

#define NUM_TASKS 2

typedef struct {
    jmp_buf context;
    int     started;
} tcb_t;

static tcb_t   tasks[NUM_TASKS];
static jmp_buf scheduler_context;
static int     current;

static void yield(void) {
    if (setjmp(tasks[current].context) == 0) {
        longjmp(scheduler_context, 1);
    }
}

static void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9); // LD2
        HAL_Delay(100);
        yield();
    }
}

static void task_heartbeat(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
        HAL_Delay(100);
        yield();
    }
}

static void (*const task_entries[NUM_TASKS])(void) = {
    task_blink,
    task_heartbeat,
};

static void scheduler(void) {
    while (1) {
        current = (current + 1) % NUM_TASKS;
        if (!tasks[current].started) {
            tasks[current].started = 1;
            task_entries[current]();
        } else {
            longjmp(tasks[current].context, 1);
        }
    }
}

int main(void) {
    HAL_Init();

    // LD2 = PB9 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_9;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    current = -1;
    if (setjmp(scheduler_context) == 0) {
        scheduler();
    }

    while (1); // unreachable
}
