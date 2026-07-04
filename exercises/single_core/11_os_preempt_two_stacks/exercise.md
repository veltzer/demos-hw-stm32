# Preemptive scheduling (SysTick + PendSV, per-task stacks)

The real step up from cooperation. In `10_os_coop_two_stacks` a task keeps the
CPU until it politely calls `yield()`. Here a hardware timer forcibly switches
tasks **behind their backs**: the two tasks contain **no `yield()` and no
cooperation at all** -- each just loops forever -- yet both run, because every
timer tick the kernel snatches the CPU from whoever holds it and hands it to the
next task. That is what makes an OS *preemptive*.

Both tasks blink **LD1 (blue, PB15)** at different rates, so the LED shows a
compound pattern no single task produces -- visible proof that two
never-yielding loops are being time-sliced onto one CPU.

## Why `setjmp`/`longjmp` is not enough anymore

The cooperative exercises could switch with `setjmp`/`longjmp` because the
switch happened at a *known point* -- inside `yield()` -- where the compiler had
already saved anything it cared about. Preemption interrupts a task at an
**arbitrary instruction**, so the kernel must save and restore the **entire**
register file, and do it from an interrupt. `setjmp`/`longjmp` cannot; the
Cortex-M exception mechanism can, and is built for exactly this.

## How the Cortex-M does it

Three hardware features carry the design:

1. **Two stack pointers.** Each task runs in Thread mode on its own **PSP**
   (process stack pointer); the kernel and all interrupt handlers run on the
   **MSP** (main stack pointer). Switching tasks is then mostly just swapping
   PSP.
2. **Automatic stacking.** On any exception entry the CPU pushes
   `R0-R3, R12, LR, PC, xPSR` of the interrupted code onto its stack for free.
   So on a switch the kernel only has to save the *other* eight registers,
   `R4-R11`, by hand.
3. **PendSV.** A dedicated exception meant for context switching, set to the
   **lowest** priority. `SysTick` fires periodically and does almost nothing --
   it just *pends* PendSV. PendSV then runs once no higher-priority interrupt is
   active and performs the actual stack swap. Doing the switch in PendSV (not in
   SysTick) means a switch never preempts a real device ISR mid-flight.

### The context switch, in PendSV

```
save:    R4-R11 -> current task's PSP,  store PSP into tasks[current].sp
pick:    current = (current + 1) % NUM_TASKS          // round robin
restore: PSP <- tasks[current].sp,  pop R4-R11
return:  bx 0xFFFFFFFD   // EXC_RETURN: resume in Thread mode on the PSP
```

The hardware pops `R0-R3, R12, LR, PC, xPSR` on that return, so the next task
resumes at exactly the instruction it was interrupted on.

### Launching the first task

Each task's stack is pre-loaded with a **fake exception frame** (`xPSR` with the
Thumb bit, `PC` = the task entry, and zeroed `R0-R3,R12,LR` plus `R4-R11`), so
that "returning" into it starts the task cleanly. `main()` sets `current = -1`
and starts `SysTick`. The very first PendSV sees "no outgoing task" (`current
< 0`), skips the save step, and simply loads task 0 -- returning into it on the
PSP. `main()`'s own context is then abandoned and never scheduled again.

## What this still is *not*

A round-robin toy, deliberately: fixed time slices, no priorities, no blocking,
no sleeping (a task that wants to wait just burns its slice). Adding those --
priority scheduling, a real `sleep()`, mutexes -- is what turns this into an
actual RTOS. But the hard part, a true preemptive context switch on real
hardware, is done here.

## Two solutions: bare-metal and HAL

- `main_bare.c` -- register level: configure GPIO and `SysTick` directly, set
  PendSV priority via `SCB->SHP`.
- `main_hal.c` -- the same kernel, but GPIO via the HAL and HAL's own 1 ms
  `SysTick` reused as the preemption heartbeat (it pends PendSV *and*
  `HAL_IncTick`s, so `HAL_Delay` still works inside the tasks).

The kernel core -- the PendSV assembly and the stack-frame bootstrap -- is pure
Cortex-M and **identical** in both; it is not something the HAL provides. Only
the setup and task bodies differ.

Build both images with `make 11_os_preempt_two_stacks`. Flash one with
`scripts/flash_exercise.sh 11_os_preempt_two_stacks bare` (or `hal`).
