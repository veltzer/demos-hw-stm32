# Proto-OS with per-task stacks (a cooperative scheduler)

A "proto operating system": the smallest thing that still deserves the name
*kernel*. A **cooperative round-robin scheduler** context-switches between
several tasks using `setjmp`/`longjmp` -- the exact technique from
`examples/coop1.cc`, now on real hardware.

Two tasks blink **LD1 (blue, PB15)**, cooperating by yielding the CPU to a
scheduler that resumes the next one where it last left off.

This is the **robust** half of a pair. Its sibling,
`single_core/09_os_coop_single_stack`, is the naive `coop1.cc` port where the
tasks jump straight to each other on a **single shared stack**; this one adds a
real scheduler and a **separate stack per task**. Read them together to see why
the extra machinery is needed on bare metal.

## What a proto-OS is (and isn't)

The kernel here is tiny but real, and has the three pieces every scheduler has:

1. **Task control blocks** -- a table of tasks. Each holds a `jmp_buf` context
   and, crucially, its **own private stack** (see below). For a cooperative
   kernel the "context" that must be saved and restored is that `jmp_buf`:
   `setjmp` snapshots the callee-saved registers, the stack pointer and the
   return address; `longjmp` puts them back.
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

## Every task needs its own stack

This is the part that makes it a *real* context switch and not a toy, and it is
easy to get wrong. `setjmp`/`longjmp` save and restore the stack **pointer** --
they do **not** save the stack **contents**. So if two tasks that suspend
mid-execution shared a single stack, here is what happens:

1. Task A runs, pushes its local variables and its `yield()` call frame onto the
   stack, then `setjmp`s (saving an SP that points into that live frame) and
   yields.
2. Task B is dispatched and runs on the *same* stack, from the *same* SP -- so
   it overwrites the very bytes holding task A's suspended frame.
3. The scheduler later `longjmp`s back into task A. Its SP is restored, but the
   memory it points at is now task B's garbage. Task A resumes on a corrupted
   stack: undefined behaviour -- in practice the program hangs (here, the LED
   froze steady-on).

The cure is a **separate stack per task**. Each task control block carries its
own stack buffer:

```c
typedef struct {
    jmp_buf  context;
    int      started;
    uint32_t stack[256];   // 1 KiB private stack, per task
} tcb_t;
```

The first time the scheduler dispatches a task it does not just `call` the
entry function -- it must first point the CPU's stack pointer at that task's
private stack, then enter the task:

```c
// full-descending stack: start at the TOP of the buffer and grow downward
uint32_t *top = &tasks[current].stack[256];
__asm volatile("mov sp, %0\n blx %1\n" :: "r"(top), "r"(entry) : "memory");
```

From then on each task lives entirely on its own stack, so suspending one never
disturbs another, and `longjmp` always restores an SP that still points at valid
memory. This is exactly what a "real" RTOS does -- every thread gets its own
stack; a cooperative kernel is no exception. (The single shared stack works fine
in `single_core/07_cooperative_scheduling` only because those tasks run to
completion each tick and never suspend, so there is no frame to preserve.)

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` -- bare metal: configure and drive the peripheral registers
  directly.
- `main_hal.c` -- the same behaviour using ST's HAL (`HAL_*` calls).

The scheduler itself is identical in both variants -- `setjmp`/`longjmp` are
plain C, not HAL calls -- so only the task bodies (register writes vs
`HAL_GPIO_TogglePin`/`HAL_Delay`) differ.

Build both images with `make 10_os_coop_two_stacks`. Flash one with
`scripts/flash_exercise.sh 10_os_coop_two_stacks bare` (or `hal`). Comparing the two shows
what the HAL does for you -- and what it hides.
