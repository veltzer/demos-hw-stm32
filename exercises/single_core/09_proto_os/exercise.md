# Proto-OS (a cooperative scheduler)

A "proto operating system": the smallest thing that still deserves the name
*kernel*. A **cooperative round-robin scheduler** context-switches between
several tasks using `setjmp`/`longjmp` -- the exact technique from
`examples/coop1.cc`, now on real hardware.

Two tasks blink **LD1 (blue, PB15)**, cooperating by yielding the CPU to a
scheduler that resumes the next one where it last left off.

## What a proto-OS is (and isn't)

The kernel here is tiny but real, and has the three pieces every scheduler has:

1. **Task control blocks** -- a table of tasks. For a cooperative kernel the
   whole "context" that must be saved and restored is a `jmp_buf`: `setjmp`
   snapshots the callee-saved registers, the stack pointer and the return
   address; `longjmp` puts them back.
2. **A scheduler** -- a dead-simple round robin. It picks the next task and
   either jumps to its entry point (first time) or `longjmp`s back to where the
   task last yielded.
3. **Cooperating tasks** -- each task loops forever, does a little work, then
   calls `yield()` to hand the CPU back to the scheduler.

What it is **not**: preemptive. There is no timer forcibly interrupting a task;
a task keeps the CPU until it *chooses* to `yield()`. Correctness therefore
depends on every task yielding often enough -- a task that never yields hangs
the whole system. Adding preemption (a `SysTick` interrupt that saves/restores
context and switches tasks behind their back) is the natural next exercise and a
much larger step. Compare with `single_core/07_cooperative_scheduling`, which
does the same idea with a plain task-array loop and no real context switch.

## The context switch, concretely

`yield()` is the heart of it:

```c
static void yield(void) {
    if (setjmp(tasks[current].context) == 0) {  // save where we are...
        longjmp(scheduler_context, 1);          // ...and jump to the scheduler
    }
    // later, longjmp(tasks[current].context) lands us right back here,
    // setjmp returns non-zero, and the task resumes as if yield() just returned
}
```

The scheduler resumes a previously-run task with `longjmp(tasks[i].context, 1)`,
which re-enters the matching `setjmp` inside that task's `yield()` -- so the
task continues exactly where it left off.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` -- bare metal: configure and drive the peripheral registers
  directly.
- `main_hal.c` -- the same behaviour using ST's HAL (`HAL_*` calls).

The scheduler itself is identical in both variants -- `setjmp`/`longjmp` are
plain C, not HAL calls -- so only the task bodies (register writes vs
`HAL_GPIO_TogglePin`/`HAL_Delay`) differ.

Build both images with `make 09_proto_os`. Flash one with
`scripts/flash_exercise.sh 09_proto_os bare` (or `hal`). Comparing the two shows
what the HAL does for you -- and what it hides.
