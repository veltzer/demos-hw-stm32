// 05_proto_os -- M0+ (CPU2) side, bare metal.
//
// The M0+ runs its OWN independent copy of the same cooperative proto-OS as the
// M4 (see main_cm4_bare.c for the full commentary on the setjmp/longjmp
// scheduler). This side schedules three tasks that together blink LD2 (green,
// PB9) at a faster rate than the M4's LD1 -- proof that a second, completely
// separate scheduler is running in parallel on the second core.
//
// Using three tasks here (vs two on the M4) underlines that the kernel is
// generic: the scheduler code does not care how many tasks there are.
#include "stm32wl55xx.h"
#include <setjmp.h>

#define NUM_TASKS 3

typedef struct {
    jmp_buf context;
    int     started;
} tcb_t;

static tcb_t   tasks[NUM_TASKS];
static jmp_buf scheduler_context;
static int     current;

static void delay(volatile uint32_t n) { while (n--); }

static void yield(void) {
    if (setjmp(tasks[current].context) == 0) {
        longjmp(scheduler_context, 1);
    }
}

// Three cooperating tasks. Two toggle the LED, one is pure "compute" (it burns
// a little time and yields) to show that not every task has to do I/O -- a
// scheduler is happy to run a task that just thinks.
static void task_led_on(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS9; // LD2 on
        delay(150000);
        yield();
    }
}

static void task_compute(void) {
    volatile uint32_t acc = 0;
    while (1) {
        acc += 1; // stand-in for real work between yields
        delay(150000);
        yield();
    }
}

static void task_led_off(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BR9; // LD2 off
        delay(150000);
        yield();
    }
}

static void (*const task_entries[NUM_TASKS])(void) = {
    task_led_on,
    task_compute,
    task_led_off,
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
    // LD2 = PB9 push-pull output. Only PB9 is touched here; PB15 is the M4's.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE9_1;
    GPIOB->MODER |= GPIO_MODER_MODE9_0;

    current = -1;
    if (setjmp(scheduler_context) == 0) {
        scheduler();
    }

    while (1); // unreachable
}
