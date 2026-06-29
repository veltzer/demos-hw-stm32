// 08_preemption, HAL version (conceptual). Preemptive context switching is
// inherently low-level -- there is no HAL API that saves/restores CPU registers
// -- so the real work still lives in a PendSV assembly handler, exactly as in
// main_bare.c. What the HAL gives us here is the periodic tick: instead of
// hand-configuring SysTick, we let HAL_Init() run the 1 ms SysTick and hook
// HAL_SYSTICK_Callback() (called from HAL_IncTick path via SysTick_Handler in
// hal_glue.c) to pend the context switch.
//
// NOTE: kept a stub on purpose. A working scheduler needs per-task stacks, TCBs
// and the PendSV register save/restore -- that is the exercise.
#include "stm32wlxx_hal.h"

void task1_function(void) { while (1) { /* blink LD1 */ } }
void task2_function(void) { while (1) { /* blink LD2 */ } }

// HAL calls this from its SysTick handling once per millisecond. We use it to
// request a context switch by pending the (lower-priority) PendSV exception.
void HAL_SYSTICK_Callback(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

// The actual register save/restore would go here (assembly).
__attribute__((naked)) void PendSV_Handler(void) {
    // 1. Save R4-R11 of the current task to its stack.
    // 2. Save current task's SP into its TCB; load the next task's SP.
    // 3. Restore R4-R11 for the next task.
    // 4. bx lr  (exception return restores PC/xPSR automatically).
    __asm volatile("bx lr");
}

int main(void) {
    HAL_Init(); // starts the 1 ms SysTick that drives HAL_SYSTICK_Callback()

    // 1. Set up per-task stacks.
    // 2. Initialise the Task Control Blocks.
    // 3. Start the first task.
    while (1) {
        // the scheduler (via PendSV) would take over from here
    }
}
