// 05_proto_os -- M4 (CPU1) side, bare metal.
//
// A tiny "proto operating system": a cooperative round-robin scheduler that
// context-switches between two tasks with setjmp/longjmp -- exactly the trick
// from examples/coop1.cc, now running on real hardware. Each task runs a bit
// of work and then calls yield() to voluntarily hand the CPU to the scheduler,
// which picks the next runnable task and longjmp()s into it.
//
// This is the essence of a cooperative OS kernel:
//   - a table of task control blocks (each is just a saved context here),
//   - a scheduler that resumes the next task,
//   - tasks that cooperate by yielding instead of running to completion.
//
// There is NO preemption and NO timer -- a task keeps the CPU until it yields.
// That is the whole point of "cooperative": correctness depends on every task
// yielding often enough. (Preemption -- a timer forcibly switching tasks -- is
// the next step up, and a much bigger one.)
//
// Both cores run their own independent copy of this proto-OS:
//   M4  (this file) schedules two tasks that blink LD1 (blue,  PB15).
//   M0+ (main_cm0p_bare.c) schedules two tasks that blink LD2 (green, PB9).
// Seeing both LEDs blink proves two independent schedulers running in parallel.
#include "stm32wl55xx.h"
#include <setjmp.h>

#define NUM_TASKS 2

// A task control block. For a cooperative kernel the entire "context" we need
// to save/restore is a jmp_buf: setjmp snapshots the callee-saved registers,
// stack pointer and return address; longjmp restores them. started tells the
// scheduler whether this task has run its one-time setjmp bootstrap yet.
typedef struct {
    jmp_buf context;
    int     started;
} tcb_t;

static tcb_t   tasks[NUM_TASKS];
static jmp_buf scheduler_context; // where a yielding task jumps back to
static int     current;           // index of the task currently running

static void delay(volatile uint32_t n) { while (n--); }

// yield(): called by a task to give up the CPU. Save this task's context; if
// we are returning here via longjmp later, setjmp returns non-zero and we just
// fall through back into the task. On the first pass setjmp returns 0 and we
// jump back into the scheduler so it can run someone else.
static void yield(void) {
    if (setjmp(tasks[current].context) == 0) {
        longjmp(scheduler_context, 1);
    }
}

// --- The two tasks. Each blinks LD1 at its own rate, yielding between edges. ---

static void task_led_on(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS15; // LD1 on
        delay(400000);
        yield();
    }
}

static void task_led_off(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BR15; // LD1 off
        delay(400000);
        yield();
    }
}

static void (*const task_entries[NUM_TASKS])(void) = {
    task_led_on,
    task_led_off,
};

// scheduler(): dead-simple round robin. Pick the next task; if it has never
// run, jump to its entry point (which loops forever, yielding); otherwise
// resume it where it last yielded. Control comes back here each time a task
// calls yield(), because yield() longjmps into scheduler_context.
static void scheduler(void) {
    while (1) {
        current = (current + 1) % NUM_TASKS;
        if (!tasks[current].started) {
            tasks[current].started = 1;
            task_entries[current]();      // never returns (task loops + yields)
        } else {
            longjmp(tasks[current].context, 1); // resume where it yielded
        }
    }
}

int main(void) {
    // Boot the M0+ (CPU2) so it can run its own proto-OS. SBRV already points
    // at flash bank 2; a single C2BOOT write releases the second core.
    PWR->CR4 |= PWR_CR4_C2BOOT;

    // LD1 = PB15 push-pull output (user LEDs live on port B). Atomic BSRR
    // writes below mean we never touch PB9, which belongs to the M0+.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Start the kernel. The scheduler runs forever, switching between tasks;
    // setjmp here just records "the scheduler" so yield() has somewhere to go
    // back to -- though scheduler() never actually returns.
    current = -1;                 // so the first (current+1) selects task 0
    if (setjmp(scheduler_context) == 0) {
        scheduler();
    }

    while (1); // unreachable
}
