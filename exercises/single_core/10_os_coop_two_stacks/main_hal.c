// 10_os_coop_two_stacks -- HAL. Compare with main_bare.c.
//
// Same cooperative proto-OS (a setjmp/longjmp round-robin scheduler over a task
// table), but the tasks drive the LED and time through the HAL instead of raw
// registers. The kernel itself is pure C -- setjmp/longjmp are not HAL calls --
// so it is byte-for-byte the same idea on both variants.
//   Two tasks toggle LD1 (blue, PB15).
#include "stm32wlxx_hal.h"
#include <setjmp.h>

void SysTick_Handler(void) { HAL_IncTick(); } // so HAL_Delay works

#define NUM_TASKS 2

// Each task needs its OWN stack: setjmp/longjmp saves the stack POINTER, not
// its CONTENTS, so suspendable tasks sharing one stack would clobber each
// other on resume. See main_bare.c for the full explanation.
typedef struct {
    jmp_buf  context;
    int      started;
    uint32_t stack[256];          // 1 KiB private stack, per task
} tcb_t;

static tcb_t   tasks[NUM_TASKS];
static jmp_buf scheduler_context;
static int     current;

// Enter a new task on its own stack (top of the array, full-descending).
static void __attribute__((noreturn))
start_on_stack(void (*entry)(void), uint32_t *stack_top) {
    __asm volatile(
        "mov sp, %0\n"
        "blx %1\n"
        :
        : "r"(stack_top), "r"(entry)
        : "memory");
    __builtin_unreachable();
}

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

// setjmp(scheduler_context) lives at the top of the loop: it is where yield()
// longjmps back to, so every yield re-enters here and dispatches the next task.
// (See main_bare.c for why it must NOT sit in main() before this call.)
static void scheduler(void) {
    setjmp(scheduler_context);        // (re)entry point for every yield()
    while (1) {
        current = (current + 1) % NUM_TASKS;
        if (!tasks[current].started) {
            tasks[current].started = 1;
            uint32_t *top = &tasks[current].stack[256];
            start_on_stack(task_entries[current], top);
        } else {
            longjmp(tasks[current].context, 1);
        }
    }
}

int main(void) {
    HAL_Init();

    // LD1 = PB15 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    current = -1;
    scheduler();

    while (1); // unreachable
}
