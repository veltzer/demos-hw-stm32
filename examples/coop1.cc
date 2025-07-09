#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>

// jmp_buf is a data type that can store a "snapshot" of the CPU state.
// We'll use these to save the context of each task.
jmp_buf task1_context;
jmp_buf task2_context;
jmp_buf main_context;

void task_one() {
    int stam;
    int counter = 1000;
    // Set a jump point for returning to this task later.
    if (setjmp(task1_context) == 0) {
        // This block runs only the FIRST time setjmp is called.
        // We jump back to the main function to start the scheduler.
        longjmp(main_context, 1);
    }
    
    // This is the task's "forever" loop.
    while (1) {
        printf("Task 1 - Execution #%d\n", ++counter);
        usleep(500000); // 0.5 second delay

        // Save the current context (here in task_one) and jump to task_two.
        if (setjmp(task1_context) == 0) {
            longjmp(task2_context, 1);
        }
    }
}

/**
 * @brief The second "forever" task.
 *
 * This function also contains an infinite loop. It prints its message,
 * then saves its own context and jumps back to task 1, creating a
 * back-and-forth execution flow.
 */
void task_two() {
    int counter1 = 1000;
    int stam;
    // Set a jump point for returning to this task later.
    if (setjmp(task2_context) == 0) {
        // This block runs only the FIRST time setjmp is called.
        // We jump back to the main function to start the scheduler.
        longjmp(main_context, 1);
    }

    // This is the task's "forever" loop.
    while (1) {
        printf("Task 2 - Execution #%d\n", ++counter1);
        usleep(500000); // 0.5 second delay

        // Save the current context (here in task_two) and jump to task_one.
        if (setjmp(task2_context) == 0) {
            longjmp(task1_context, 1);
        }
    }
}

/**
 * @brief The main function which sets up and starts the multitasking.
 */
int main() {
    printf("Starting cooperative multitasking with setjmp/longjmp.\n");
    printf("Press Ctrl+C to exit.\n\n");

    // setjmp returns 0 the first time it's called.
    // It will return a non-zero value if a longjmp directs execution here.
    // We use this to initialize the tasks one by one.
    if (setjmp(main_context) == 0) {
        task_one();
    }
    
    if (setjmp(main_context) == 0) {
        task_two();
    }

    // Start the multitasking by jumping to the first task.
    longjmp(task1_context, 1);

    return 0; // Unreachable
}
