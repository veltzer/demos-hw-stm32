# Proto-OS (a cooperative scheduler on each core)

A "proto operating system": the smallest thing that still deserves the name
*kernel*. Each core runs a **cooperative round-robin scheduler** that
context-switches between several tasks using `setjmp`/`longjmp` -- the exact
technique from `examples/coop1.cc`, now on real hardware and running on **both
cores at once**.

- **M4 (CPU1)** runs a scheduler over **two** tasks that blink **LD1 (blue,
  PB15)**.
- **M0+ (CPU2)** runs its **own** scheduler over **three** tasks that blink
  **LD2 (green, PB9)** faster.

Two LEDs blinking is proof that two independent schedulers are running in
genuine parallel, one per CPU.

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
its whole core. Adding preemption (a `SysTick` interrupt that saves/restores
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

Each core has both a bare-metal and a HAL solution, so there are four sources:

- `main_cm4_bare.c` / `main_cm0p_bare.c` -- register level.
- `main_cm4_hal.c` / `main_cm0p_hal.c` -- LED and timing via the HAL.

The scheduler itself is identical in both variants -- `setjmp`/`longjmp` are
plain C, not HAL calls -- so only the task bodies (register writes vs
`HAL_GPIO_TogglePin`/`HAL_Delay`) differ.

`make 05_proto_os` builds all four images (`firmware_cm4_bare.bin`,
`firmware_cm0p_bare.bin`, `firmware_cm4_hal.bin`, `firmware_cm0p_hal.bin`). The
HAL is compiled twice, once per core, because the two CPUs have different
instruction sets.

## Running it

Both cores must be flashed (M4 -> bank 1 at `0x08000000`, M0+ -> bank 2 at
`0x08020000`), then reset. `SBRV` already points at bank 2, so the M4's
`C2BOOT` write is enough to start the M0+ -- no option-byte changes needed.

    scripts/flash_exercise.sh 05_proto_os bare   # or: hal
