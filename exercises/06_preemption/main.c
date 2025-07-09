#include "stm32wl55xx.h"

// This is highly simplified for conceptual understanding.
// A real implementation requires significant assembly code for context switching.

void task1_function(void) { while(1) { /* Blink LED 1 */ } }
void task2_function(void) { while(1) { /* Blink LED 2 */ } }

// SysTick interrupt triggers the scheduler
void SysTick_Handler(void) {
    // 1. Save the context (CPU registers) of the current task.
    // 2. Load the context of the next task.
    // 3. This is usually done by triggering a PendSV exception,
    //    which has a lower priority to allow other interrupts to complete first.
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

// PendSV handler does the actual context switching
__attribute__((naked)) void PendSV_Handler(void) {
    // Assembly code here:
    // 1. Save R4-R11 of the current task to its stack.
    // 2. Get current task's stack pointer and save it.
    // 3. Get next task's stack pointer and load it.
    // 4. Restore R4-R11 for the next task from its stack.
    // 5. Return from exception (which loads PC, LR, etc. automatically).
}


int main(void) {
    // 1. Setup stacks for each task.
    // 2. Initialize the Task Control Blocks (TCBs) for each task.
    // 3. Configure and start the SysTick timer to fire periodically.
    // 4. Start the first task.
    while(1);
}
