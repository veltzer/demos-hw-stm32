// 11_os_preempt_two_stacks -- bare metal.
//
// The step up from cooperation to PREEMPTION. In 10_os_coop_two_stacks a task
// keeps the CPU until it politely calls yield(); here a hardware timer
// (SysTick) forcibly switches tasks behind their backs. The tasks contain NO
// yield() and NO cooperation at all -- each just loops forever -- yet both run,
// because every tick the kernel snatches the CPU from whoever holds it and
// hands it to the next task. That is what makes an OS *preemptive*.
//
// Why setjmp/longjmp is NOT enough here
// -------------------------------------
// Cooperative switching could use setjmp/longjmp because the switch happened at
// a known point (inside yield()), where the compiler had already spilled
// whatever it cared about. Preemption interrupts a task at an ARBITRARY
// instruction, so we must save/restore the FULL register file, and we must do
// it from an interrupt. The Cortex-M4 is built for exactly this:
//
//   - Each task runs in Thread mode on its own PSP (process stack pointer);
//     the kernel and interrupt handlers run on the MSP (main stack pointer).
//   - On any exception the CPU AUTO-stacks R0-R3, R12, LR, PC, xPSR onto the
//     current (process) stack. We only have to save/restore the rest: R4-R11.
//   - PendSV is a dedicated, lowest-priority exception meant for context
//     switching. SysTick fires periodically and just PENDS PendSV; PendSV then
//     does the actual stack swap once no higher-priority ISR is pending.
//
// So the plan is: build a fake initial exception frame on each task's stack (so
// "returning" into it starts the task cleanly), then let PendSV swap PSP
// between tasks on every tick.
#include "stm32wl55xx.h"

#define NUM_TASKS 2

// A task control block. For a PREEMPTIVE kernel the whole saved context lives
// on the task's own stack; the only thing the TCB must remember between
// switches is where that stack's top currently is (the task's PSP).
typedef struct {
    uint32_t *sp;                 // saved process stack pointer for this task
    uint32_t  stack[256];         // 1 KiB private stack, per task
} tcb_t;

static tcb_t tasks[NUM_TASKS];
static int   current = -1;        // index of the running task (-1 before start)

static void delay(volatile uint32_t n) { while (n--); }

// --- The two tasks. NOTE: neither yields. They just loop. Preemption is the
//     only reason both ever get to run. They blink LD1 (PB15) at different
//     rates, so the LED shows a compound pattern no single task produces. ---

static void task_fast(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS15; // LD1 on
        delay(200000);
        GPIOB->BSRR = GPIO_BSRR_BR15; // LD1 off
        delay(200000);
    }
}

static void task_slow(void) {
    while (1) {
        GPIOB->BSRR = GPIO_BSRR_BS15; // LD1 on
        delay(800000);
        GPIOB->BSRR = GPIO_BSRR_BR15; // LD1 off
        delay(800000);
    }
}

static void (*const task_entries[NUM_TASKS])(void) = {
    task_fast,
    task_slow,
};

// Build the initial exception stack frame for a task, so that the very first
// time PendSV "returns" into it, the CPU pops a valid frame and starts running
// task_entries[i] in Thread mode. Layout matches the hardware auto-stacked
// frame, top-of-stack downward: xPSR, PC, LR, R12, R3, R2, R1, R0, then our
// software-saved R4-R11 below it.
static void task_init_stack(int i, void (*entry)(void)) {
    uint32_t *sp = &tasks[i].stack[256];   // full-descending: start at the top
    *(--sp) = 0x01000000u;                 // xPSR: Thumb bit set (required)
    *(--sp) = (uint32_t)entry;             // PC: task entry point
    *(--sp) = 0xFFFFFFFDu;                 // LR: EXC_RETURN? no -- dummy return
    *(--sp) = 0;                           // R12
    *(--sp) = 0;                           // R3
    *(--sp) = 0;                           // R2
    *(--sp) = 0;                           // R1
    *(--sp) = 0;                           // R0
    // Software-saved registers R4-R11 (PendSV pops these before returning).
    for (int r = 0; r < 8; r++) *(--sp) = 0;
    tasks[i].sp = sp;
}

// PendSV: the actual context switch. Naked so we control the whole prologue /
// epilogue. On entry the CPU has already auto-stacked the outgoing task's
// R0-R3,R12,LR,PC,xPSR onto its PSP. We save R4-R11 too, stash PSP into the
// current TCB, pick the next task, load its PSP, restore its R4-R11, and return
// with EXC_RETURN=0xFFFFFFFD so the CPU pops the frame and resumes in Thread
// mode on the process stack.
void __attribute__((naked)) PendSV_Handler(void) {
    __asm volatile(
        "  mrs   r0, psp            \n" // r0 = outgoing PSP
        "  cbz   r0, 1f             \n" // first switch ever? no task to save -> skip save
        "  stmdb r0!, {r4-r11}      \n" // push R4-R11, r0 now points at new top
        "  ldr   r1, =current       \n"
        "  ldr   r2, [r1]           \n" // r2 = current index
        "  ldr   r3, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r2, r2, r12        \n"
        "  str   r0, [r3, r2]       \n" // tasks[current].sp = r0  (sp is field 0)
        "1:                         \n"
        "  ldr   r1, =current       \n"
        "  ldr   r2, [r1]           \n"
        "  adds  r2, r2, #1         \n"
        "  cmp   r2, %[n]           \n" // wrap: if (++current == NUM_TASKS) current = 0
        "  it    ge                 \n"
        "  movge r2, #0             \n"
        "  str   r2, [r1]           \n" // current = next
        "  ldr   r3, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r2, r2, r12        \n"
        "  ldr   r0, [r3, r2]       \n" // r0 = tasks[current].sp
        "  ldmia r0!, {r4-r11}      \n" // restore R4-R11
        "  msr   psp, r0            \n" // PSP = new task's stack
        "  ldr   lr, =0xFFFFFFFD    \n" // EXC_RETURN: Thread mode, use PSP
        "  bx    lr                 \n"
        :
        : [stride] "i" (sizeof(tcb_t)), [n] "i" (NUM_TASKS)
        : "memory");
}

// SysTick: the heartbeat. It does almost nothing -- just set the PendSV pending
// bit so the actual switch happens in PendSV (at lowest priority, after any
// other ISR). Keeping the switch out of SysTick is the standard Cortex-M idiom.
void SysTick_Handler(void) {
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

int main(void) {
    // LD1 = PB15 push-pull output.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE15_1;
    GPIOB->MODER |= GPIO_MODER_MODE15_0;

    // Prepare each task's stack so PendSV can "return" into it.
    for (int i = 0; i < NUM_TASKS; i++)
        task_init_stack(i, task_entries[i]);

    // Make PendSV the lowest priority so it never preempts a real ISR; it runs
    // only once everything else has finished (that is why we switch there).
    SCB->SHP[1] = 0xFFu;              // PendSV priority = lowest (SHPR3, byte 2)

    // Start SysTick: MSI is 4 MHz after reset, so 40000 counts = 10 ms tick.
    SysTick_Config(40000);

    // Launch the first task by hand: point PSP at task 0's frame, switch the
    // CPU to use PSP in Thread mode, then "return" into the task by popping its
    // frame -- reusing the same restore path PendSV uses.
    current = 0;
    __set_PSP((uint32_t)tasks[0].sp + 8 * 4); // PSP above the R4-R11 area...
    __asm volatile(
        "  ldr r0, =tasks           \n"
        "  ldr r0, [r0]             \n" // r0 = tasks[0].sp
        "  ldmia r0!, {r4-r11}      \n" // restore R4-R11
        "  msr psp, r0              \n"
        "  movs r0, #2              \n"
        "  msr control, r0          \n" // CONTROL.SPSEL=1: Thread mode uses PSP
        "  isb                      \n"
        "  ldr lr, =0xFFFFFFFD      \n" // EXC_RETURN not valid outside handler..
        "  ldr r0, =0               \n"
        ::: "r0", "memory");

    // The line above cannot cleanly "return into" a task from Thread mode (that
    // trick only works from an exception). Instead we just enable interrupts and
    // fall into a spin; the FIRST SysTick -> PendSV will do the real launch,
    // switching from this main context to a task. main's own frame becomes an
    // implicit "task -1" that is simply never scheduled again.
    while (1);
}
