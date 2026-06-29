# Preemptive Scheduling with a Timer Interrupt

This exercise upgrades your scheduler from cooperative to preemptive. You'll configure a hardware timer (like the SysTick timer) to fire at a regular interval (e.g., every 10 milliseconds). The timer's interrupt service routine (ISR) will act as the scheduler, forcibly pausing the currently running task and switching to the next one.

What you'll learn: This is a major leap into real-time systems. You will implement a genuine preemptive scheduler, which is the foundation of any real-time operating system (RTOS). You'll deal with saving and restoring CPU context (registers) and understand the concept of a time slice or quantum.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: drive SysTick and the SCB directly.
- `main_hal.c` — let `HAL_Init()` run SysTick and hook `HAL_SYSTICK_Callback()`.

Note: preemptive context switching is inherently low-level — there is no HAL API
for pending PendSV or saving/restoring CPU registers — so both versions still use
`SCB->ICSR` and an assembly `PendSV_Handler`. The HAL only abstracts the tick
setup. (Both are conceptual stubs: a working scheduler is the actual exercise.)

Build both images with `make 08_preemption`. Flash one with
`scripts/flash_exercise.sh 08_preemption bare` (or `hal`).
