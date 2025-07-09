#define _XOPEN_SOURCE // Required for ucontext functions on some systems
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

// Define the stack size for our tasks. 8KB is a common size.
#define STACK_SIZE 8192

// Declare the contexts for our two tasks and the main function
ucontext_t task1_context;
ucontext_t task2_context;
ucontext_t main_context;

// Stacks for our tasks
char task1_stack[STACK_SIZE];
char task2_stack[STACK_SIZE];

/**
 * @brief The first "forever" task.
 *
 * This function runs in its own context with its own stack.
 * It contains an infinite loop that prints a message and then swaps
 * control to the second task.
 */
void task_one() {
    int counter = 0;
    while (1) {
        printf("Task 1 - Execution #%d\n", ++counter);
        usleep(500000); // 0.5 second delay

        // Swap from the current context (task1) to task2's context
        if (swapcontext(&task1_context, &task2_context) == -1) {
            perror("swapcontext");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief The second "forever" task.
 *
 * This function also runs in its own context and on its own stack.
 * Its infinite loop prints a message and then swaps control back
 * to the first task.
 */
void task_two() {
    int counter = 100;
    while (1) {
        printf("Task 2 - Execution #%d\n", ++counter);
        usleep(500000); // 0.5 second delay

        // Swap from the current context (task2) back to task1's context
        if (swapcontext(&task2_context, &task1_context) == -1) {
            perror("swapcontext");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief The main function which sets up the contexts and starts the multitasking.
 */
int main() {
    printf("Starting cooperative multitasking with separate stacks.\n");
    printf("Press Ctrl+C to exit.\n\n");

    // --- Setup for Task 1 ---
    // 1. Get the current context as a template
    if (getcontext(&task1_context) == -1) {
        perror("getcontext");
        exit(EXIT_FAILURE);
    }
    // 2. Modify the context to use our own stack
    task1_context.uc_stack.ss_sp = task1_stack;
    task1_context.uc_stack.ss_size = sizeof(task1_stack);
    // 3. Set the context to return to when the task function finishes
    //    (it won't in this case, but it's good practice)
    task1_context.uc_link = &main_context;
    // 4. Associate the function `task_one` with this context
    makecontext(&task1_context, task_one, 0);


    // --- Setup for Task 2 (same procedure) ---
    if (getcontext(&task2_context) == -1) {
        perror("getcontext");
        exit(EXIT_FAILURE);
    }
    task2_context.uc_stack.ss_sp = task2_stack;
    task2_context.uc_stack.ss_size = sizeof(task2_stack);
    task2_context.uc_link = &main_context;
    makecontext(&task2_context, task_two, 0);


    // Start the multitasking by swapping from the main context to task 1
    // The main context will be saved, and execution will jump to task_one.
    if (swapcontext(&main_context, &task1_context) == -1) {
        perror("swapcontext");
        exit(EXIT_FAILURE);
    }

    // This line will only be reached if the tasks finish and their
    // uc_link points back here. In our infinite loop example, it's unreachable.
    printf("Finished all tasks.\n");

    return 0;
}

