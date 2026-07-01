// 05_proto_os -- M4 (CPU1) side, HAL. Compare with main_cm4_bare.c.
//
// Same cooperative proto-OS (a setjmp/longjmp round-robin scheduler over a task
// table), but the tasks drive the LED and time through the HAL instead of raw
// registers. The kernel itself is pure C -- setjmp/longjmp are not HAL calls --
// so it is byte-for-byte the same idea on both variants.
//   M4 -> LD1 (blue, PB15), two tasks toggling it.
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_pwr.h" // C2BOOT lives at the LL layer on the WL HAL
#include <setjmp.h>

void SysTick_Handler(void) { HAL_IncTick(); } // so HAL_Delay works

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
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // LD1
        HAL_Delay(250);
        yield();
    }
}

static void task_heartbeat(void) {
    // A second task that also toggles LD1: with two cooperating togglers the
    // visible blink rate is the sum of their yields -- a tiny demonstration
    // that behaviour is the product of the whole task set, not any one task.
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
        HAL_Delay(250);
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

    // Boot the M0+ so it can run its own scheduler.
    LL_PWR_EnableBootC2();

    // LD1 = PB15 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
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
