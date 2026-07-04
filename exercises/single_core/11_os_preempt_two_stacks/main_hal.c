// 11_os_preempt_two_stacks -- HAL. Compare with main_bare.c.
//
// Same preemptive kernel (SysTick -> PendSV context switch, one PSP stack per
// task, R4-R11 saved by hand). The kernel core -- the PendSV assembly and the
// stack-frame bootstrap -- is pure Cortex-M and identical to the bare version;
// it is not something the HAL provides. What changes is only the setup and the
// task bodies: HAL_Init/HAL_GPIO for the LED, and HAL's own 1 ms SysTick reused
// as the preemption heartbeat (we pend PendSV from it, and still HAL_IncTick so
// HAL_Delay keeps working inside the tasks).
#include "stm32wlxx_hal.h"

#define NUM_TASKS 2

// sp MUST be field 0: the PendSV assembly dereferences tasks[i] as the sp.
typedef struct {
    uint32_t *sp;
    uint32_t  stack[256];
} tcb_t;

// External linkage so the PendSV assembly can reference them by symbol name.
tcb_t tasks[NUM_TASKS];
int   current = -1;

// Both tasks blink LD1 at the SAME rate as the cooperative exercises (09/10);
// preemption -- not a differing rate -- is what this exercise demonstrates.
static void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // LD1
        HAL_Delay(250);
    }
}

static void (*const task_entries[NUM_TASKS])(void) = {
    task_blink,
    task_blink,
};

static void task_init_stack(int i, void (*entry)(void)) {
    uint32_t *sp = &tasks[i].stack[256];
    *(--sp) = 0x01000000u;                 // xPSR (Thumb bit)
    *(--sp) = (uint32_t)entry & ~1u;       // PC
    *(--sp) = 0xFFFFFFFFu;                 // LR (tasks never return)
    *(--sp) = 0;                           // R12
    *(--sp) = 0;                           // R3
    *(--sp) = 0;                           // R2
    *(--sp) = 0;                           // R1
    *(--sp) = 0;                           // R0
    for (int r = 0; r < 8; r++) *(--sp) = 0; // R4-R11
    tasks[i].sp = sp;
}

void __attribute__((naked)) PendSV_Handler(void) {
    __asm volatile(
        "  ldr   r3, =current       \n"
        "  ldr   r2, [r3]           \n"
        "  cmp   r2, #0             \n"
        "  blt   1f                 \n"
        "  mrs   r0, psp            \n"
        "  stmdb r0!, {r4-r11}      \n"
        "  ldr   r1, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r12, r2, r12       \n"
        "  str   r0, [r1, r12]      \n"
        "1:                         \n"
        "  adds  r2, r2, #1         \n"
        "  cmp   r2, %[n]           \n"
        "  it    ge                 \n"
        "  movge r2, #0             \n"
        "  str   r2, [r3]           \n"
        "  ldr   r1, =tasks         \n"
        "  mov   r12, %[stride]     \n"
        "  mul   r12, r2, r12       \n"
        "  ldr   r0, [r1, r12]      \n"
        "  ldmia r0!, {r4-r11}      \n"
        "  msr   psp, r0            \n"
        "  ldr   r0, =0xFFFFFFFD    \n"
        "  bx    r0                 \n"
        :
        : [stride] "i" (sizeof(tcb_t)), [n] "i" (NUM_TASKS)
        : "memory");
}

// HAL's 1 ms tick doubles as the preemption heartbeat: advance HAL's tick (so
// HAL_Delay works in the tasks) and pend PendSV to switch tasks.
void SysTick_Handler(void) {
    HAL_IncTick();
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

int main(void) {
    HAL_Init();                      // starts the 1 ms SysTick

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    for (int i = 0; i < NUM_TASKS; i++)
        task_init_stack(i, task_entries[i]);

    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0); // lowest priority for PendSV

    // current stays -1; the first SysTick -> PendSV launches task 0. main()'s
    // context is abandoned once the switch happens.
    while (1);
}
