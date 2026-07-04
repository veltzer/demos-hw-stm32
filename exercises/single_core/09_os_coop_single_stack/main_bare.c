// 09_os_coop_single_stack -- bare metal.
//
// A faithful port of examples/coop1.cc to the STM32: cooperative multitasking
// with setjmp/longjmp, but WITHOUT a scheduler and WITHOUT per-task stacks.
// The two tasks jump straight to each other (task1 -> task2 -> task1 ...), and
// they all run on the ONE stack the startup code set up.
//
// This is the naive version. Its sibling, 10_os_coop_two_stacks, adds a real
// round-robin scheduler and a private stack per task. Read the two together:
// this file shows that the coop1.cc trick DOES run on bare metal for simple
// tasks, and the sibling shows the machinery you need once tasks get real.
//
// Why one stack is enough HERE: each task is bootstrapped by main() calling it
// once; the task hits its setjmp and immediately longjmps back to main, so its
// call frame is unwound before the next task is set up. From then on the tasks
// re-enter via longjmp and run their forever-loops with only tiny, equal-depth
// frames that never grow across a yield -- so nothing clobbers anything. Give a
// task a deep or persistent local frame across a yield and this breaks; that is
// exactly what the per-task stacks in 10_os_coop_two_stacks fix.
#include "stm32wl55xx.h"
#include <setjmp.h>

// A saved "snapshot" of CPU state per task, exactly as in coop1.cc.
static jmp_buf task1_context;
static jmp_buf task2_context;
static jmp_buf main_context;

static void delay(volatile uint32_t n) { while (n--); }

// task_one: turn LD1 on, then hand control to task_two. On the first call it
// registers its context and jumps back to main so main can bootstrap task_two.
static void task_one(void) {
    if (setjmp(task1_context) == 0) {
        longjmp(main_context, 1);   // first call only: back to main to init
    }
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS15; // LD1 on
        delay(400000);
        // save where we are, then jump straight into task_two (no scheduler)
        if (setjmp(task1_context) == 0) {
            longjmp(task2_context, 1);
        }
    }
}

// task_two: the mirror image -- turn LD1 off, then jump back to task_one.
static void task_two(void) {
    if (setjmp(task2_context) == 0) {
        longjmp(main_context, 1);   // first call only: back to main to init
    }
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BR15; // LD1 off
        delay(400000);
        if (setjmp(task2_context) == 0) {
            longjmp(task1_context, 1);
        }
    }
}

int main(void) {
    // LD1 = PB15 push-pull output (user LEDs live on port B).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Bootstrap each task one at a time: call it, let it setjmp its context and
    // longjmp back here. setjmp(main_context) returns 0 the first time and
    // non-zero when a task jumps back, so each of these ifs runs its task once.
    if (setjmp(main_context) == 0) {
        task_one();
    }
    if (setjmp(main_context) == 0) {
        task_two();
    }

    // Kick off the ping-pong by entering task_one for real.
    longjmp(task1_context, 1);

    while (1); // unreachable
}
