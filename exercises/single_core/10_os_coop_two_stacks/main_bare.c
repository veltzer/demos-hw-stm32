// 10_os_coop_two_stacks -- bare metal.
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
// This single-core version schedules two tasks that blink LD1 (blue, PB15).
// Compare with single_core/07_cooperative_scheduling, which does the same idea
// with a plain task-array loop and no real context switch.
#include "stm32wl55xx.h"
#include <setjmp.h>

#define NUM_TASKS 2

// A task control block. For a cooperative kernel the "context" we save/restore
// once a task is running is a jmp_buf: setjmp snapshots the callee-saved
// registers, stack pointer and return address; longjmp restores them. started
// tells the scheduler whether this task has run its one-time bootstrap yet.
//
// CRUCIAL: every task needs its OWN stack. setjmp/longjmp only saves/restores
// the stack POINTER, not the stack CONTENTS -- so if two suspendable tasks
// shared one stack, resuming task A would land on memory that task B has since
// overwritten (undefined behaviour; in practice the LED sticks). Each task
// therefore gets a private stack here, and we switch SP to it on first entry.
typedef struct {
    jmp_buf  context;
    int      started;
    uint32_t stack[256];          // 1 KiB private stack, per task
} tcb_t;

static tcb_t   tasks[NUM_TASKS];
static jmp_buf scheduler_context; // where a yielding task jumps back to
static int     current;           // index of the task currently running

// Jump into a brand-new task on its OWN stack. We set SP to the top of the
// task's private stack (full-descending, 8-byte aligned) and branch to its
// entry. The entry loops forever and never returns, so we never need to
// restore the scheduler's SP here -- control leaves only via yield()'s longjmp.
static void __attribute__((noreturn))
start_on_stack(void (*entry)(void), const uint32_t *stack_top) {
    __asm volatile(
        "mov sp, %0\n"
        "blx %1\n"
        :
        : "r"(stack_top), "r"(entry)
        : "memory");
    __builtin_unreachable();
}

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
// resume it where it last yielded.
//
// The setjmp(scheduler_context) MUST live here, at the top of the loop -- that
// is the point yield() longjmps back to. Every time a task yields, control
// re-enters right here (setjmp returns non-zero), we fall through, pick the
// next task, and dispatch it. Putting this setjmp in main() before calling
// scheduler() would be wrong: yield() would return into main() *after* the
// scheduler frame, skipping the loop entirely and hanging on the first yield.
static void scheduler(void) {
    setjmp(scheduler_context);        // (re)entry point for every yield()
    while (1) {
        current = (current + 1) % NUM_TASKS;
        if (!tasks[current].started) {
            tasks[current].started = 1;
            // First run: enter the task on its OWN stack (top of the array,
            // full-descending). It loops forever + yields, never returns.
            const uint32_t *top = &tasks[current].stack[256];
            start_on_stack(task_entries[current], top);
        } else {
            longjmp(tasks[current].context, 1); // resume where it yielded
        }
    }
}

int main(void) {
    // LD1 = PB15 push-pull output (user LEDs live on port B).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Start the kernel. scheduler() runs forever, switching between tasks.
    current = -1;                 // so the first (current+1) selects task 0
    scheduler();

    while (1); // unreachable
}
