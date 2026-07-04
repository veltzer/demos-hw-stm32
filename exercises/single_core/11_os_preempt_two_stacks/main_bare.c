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
// instruction, so we must save/restore the FULL register file, and from an
// interrupt. The Cortex-M4 is built for exactly this:
//
//   - Each task runs in Thread mode on its own PSP (process stack pointer);
//     the kernel/ISRs run on the MSP (main stack pointer).
//   - On any exception the CPU AUTO-stacks R0-R3, R12, LR, PC, xPSR onto the
//     current (process) stack. We only save/restore the rest by hand: R4-R11.
//   - PendSV is a dedicated, lowest-priority exception meant for context
//     switching. SysTick fires periodically and just PENDS PendSV; PendSV does
//     the actual stack swap once no higher-priority ISR is pending.
//
// The launch is elegant: main() prepares each task's stack, sets current = -1,
// and starts SysTick. The very first PendSV sees "no outgoing task" (current
// < 0), skips the save step, and simply loads task 0 -- returning into it on
// the PSP. main()'s own context is then abandoned (never scheduled again).
#include "stm32wl55xx.h"

#define NUM_TASKS 2

// A task control block. For a PREEMPTIVE kernel the whole saved context lives
// on the task's own stack; between switches the TCB only needs to remember
// where that stack's top currently is (the task's PSP). sp MUST be field 0 --
// the PendSV assembly indexes tasks[i] and dereferences it as the sp directly.
typedef struct {
    uint32_t *sp;                 // saved process stack pointer for this task
    uint32_t  stack[256];         // 1 KiB private stack, per task
} tcb_t;

// Non-static (external linkage) because the PendSV assembly refers to these by
// symbol name (`ldr rN, =tasks` / `=current`); a static symbol could be
// localized or elided and the assembler reference would not resolve.
tcb_t tasks[NUM_TASKS];
int   current = -1;               // running task index (-1 before first switch)

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

// Build the initial exception stack frame for a task, so the first time PendSV
// "returns" into it, the CPU pops a valid frame and starts running its entry in
// Thread mode. Layout matches the hardware frame, top-of-stack downward:
// xPSR, PC, LR, R12, R3, R2, R1, R0, then our software-saved R4-R11 below.
static void task_init_stack(int i, void (*entry)(void)) {
    uint32_t *sp = &tasks[i].stack[256];   // full-descending: start at the top
    *(--sp) = 0x01000000u;                 // xPSR: Thumb bit set (required)
    *(--sp) = (uint32_t)entry & ~1u;       // PC: task entry (bit0 clear in frame)
    *(--sp) = 0xFFFFFFFFu;                 // LR: tasks never return; trap if they do
    *(--sp) = 0;                           // R12
    *(--sp) = 0;                           // R3
    *(--sp) = 0;                           // R2
    *(--sp) = 0;                           // R1
    *(--sp) = 0;                           // R0
    for (int r = 0; r < 8; r++) *(--sp) = 0; // R4-R11
    tasks[i].sp = sp;
}

// PendSV: the actual context switch. Naked so we own the whole prologue/
// epilogue. On entry the CPU has already auto-stacked the outgoing task's
// R0-R3,R12,LR,PC,xPSR onto its PSP. We save R4-R11 too, stash PSP into the
// current TCB (unless there is no outgoing task yet), advance current round
// robin, load the next task's PSP, restore its R4-R11, and return with
// EXC_RETURN=0xFFFFFFFD so the CPU resumes it in Thread mode on the PSP.
void __attribute__((naked)) PendSV_Handler(void) {
    __asm volatile(
        "  ldr   r3, =current       \n"
        "  ldr   r2, [r3]           \n" // r2 = current
        "  cmp   r2, #0             \n"
        "  blt   1f                 \n" // current < 0 -> first switch, nothing to save
        "  mrs   r0, psp            \n" // r0 = outgoing PSP
        "  stmdb r0!, {r4-r11}      \n" // push R4-R11
        "  ldr   r1, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r12, r2, r12       \n"
        "  str   r0, [r1, r12]      \n" // tasks[current].sp = r0
        "1:                         \n"
        "  adds  r2, r2, #1         \n"
        "  cmp   r2, %[n]           \n"
        "  it    ge                 \n"
        "  movge r2, #0             \n" // wrap round robin
        "  str   r2, [r3]           \n" // current = next
        "  ldr   r1, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r12, r2, r12       \n"
        "  ldr   r0, [r1, r12]      \n" // r0 = tasks[current].sp
        "  ldmia r0!, {r4-r11}      \n" // restore R4-R11
        "  msr   psp, r0            \n" // PSP = new task's stack
        "  ldr   r0, =0xFFFFFFFD    \n"
        "  bx    r0                 \n" // return into Thread mode, using PSP
        :
        : [stride] "i" (sizeof(tcb_t)), [n] "i" (NUM_TASKS)
        : "memory");
}

// SysTick: the heartbeat. It does almost nothing -- just set the PendSV pending
// bit so the actual switch happens in PendSV (lowest priority, after any other
// ISR). Keeping the switch out of SysTick is the standard Cortex-M idiom.
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

    // PendSV at lowest priority: it must never preempt a real ISR; it runs only
    // once everything else has finished (that is precisely why we switch there).
    SCB->SHP[1] = 0xFFu;             // SHPR3 byte 2 = PendSV priority = lowest

    // Start SysTick: MSI is 4 MHz after reset, so 40000 counts = 10 ms tick.
    // current stays -1; the first PendSV will launch task 0.
    SysTick_Config(40000);

    // main() keeps running on the MSP until the first tick fires PendSV, which
    // switches the CPU onto task 0's PSP. From then on this loop is dead code --
    // main()'s context is never scheduled again.
    while (1);
}
