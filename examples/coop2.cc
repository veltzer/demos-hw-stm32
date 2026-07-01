#include <stdio.h>
#include <unistd.h>
#include <ucontext.h>

// coop2: same idea as coop1 -- cooperative multitasking -- but the tasks are
// DECOUPLED from each other, and it is done properly with real per-task stacks.
//
// In coop1 each task hard-coded who ran next: task_one did
// longjmp(task2_context) and task_two did longjmp(task1_context). Task_one could
// not exist without task_two, and adding a third task meant editing them all.
//
// Here a task just calls yield(). yield() returns control to a scheduler, and
// the SCHEDULER alone knows the set of tasks and their order. task_one and
// task_two never name each other and are fully interchangeable; adding a task
// means adding one line to the scheduler's table -- the tasks don't change.
//
// coop1 abused setjmp/longjmp to fake coroutines on a single shared stack, which
// only works if no task ever returns and there are essentially two of them
// ping-ponging. To decouple cleanly for any N we give every task its own stack
// with ucontext (makecontext/swapcontext), so tasks never clobber one another
// and the scheduler can jump to whichever task it likes.

#define NUM_TASKS 2
#define STACK_SIZE 65536

ucontext_t scheduler_ctx;              // the scheduler's own context
ucontext_t task_ctx[NUM_TASKS];        // one context (and stack) per task
char task_stack[NUM_TASKS][STACK_SIZE];

int current_task;                      // task the scheduler is running

// A task calls yield() to give up the CPU. It swaps back to the scheduler, which
// decides who runs next. The task names no other task.
void yield() {
    swapcontext(&task_ctx[current_task], &scheduler_ctx);
    // Control returns here when the scheduler resumes this task.
}

void task_one() {
    int counter = 1000;
    while (1) {
        printf("Task 1 - Execution #%d\n", ++counter);
        usleep(500000); // 0.5 second delay
        yield();
    }
}

void task_two() {
    int counter = 1000;
    while (1) {
        printf("Task 2 - Execution #%d\n", ++counter);
        usleep(500000); // 0.5 second delay
        yield();
    }
}

// The scheduler's task table -- the ONLY place that knows the full set of tasks.
void (*task_entry[NUM_TASKS])() = { task_one, task_two };

int main() {
    printf("Starting cooperative multitasking (scheduler-routed tasks).\n");
    printf("Press Ctrl+C to exit.\n\n");

    // Give each task its own stack and entry point. The tasks stay ignorant of
    // one another; only this loop and the scheduler below know the set.
    for (int i = 0; i < NUM_TASKS; i++) {
        getcontext(&task_ctx[i]);
        task_ctx[i].uc_stack.ss_sp = task_stack[i];
        task_ctx[i].uc_stack.ss_size = STACK_SIZE;
        task_ctx[i].uc_link = &scheduler_ctx; // if a task ever returns, go here
        makecontext(&task_ctx[i], task_entry[i], 0);
    }

    // Round-robin scheduler. Resume each task in turn; when it yields, control
    // comes back here and we move on to the next one. This loop never exits.
    int next = 0;
    while (1) {
        current_task = next;
        next = (next + 1) % NUM_TASKS;
        swapcontext(&scheduler_ctx, &task_ctx[current_task]);
    }

    return 0; // Unreachable
}
