# Cooperative multitasking on a single shared stack (the coop1.cc port)

This is a faithful port of `examples/coop1.cc` to the STM32: two tasks
cooperatively multitask with `setjmp`/`longjmp`, **jumping directly to each
other** with no scheduler in between, and running on the **one stack** the
startup code set up. The two tasks together blink **LD1 (blue, PB15)** -- one
turns it on, the other off.

It is the **naive** half of a pair. Its sibling,
`single_core/10_os_coop_two_stacks`, wraps the same `setjmp`/`longjmp` idea in a
real round-robin scheduler and gives **each task its own stack**. The point of
having both is to see, on real hardware, exactly what the extra machinery buys
you -- and when you can get away without it.

## How it works

The structure is straight out of `coop1.cc`:

- Each task has a `jmp_buf` (`task1_context`, `task2_context`); `main` has its
  own (`main_context`).
- `main` bootstraps the tasks one at a time. It calls `task_one()`, which
  immediately `setjmp`s its context and `longjmp`s back to `main`; then `main`
  does the same for `task_two()`. After this, both tasks' resume points are
  recorded but neither is looping yet.
- `main` kicks things off with `longjmp(task1_context, 1)`. From then on the
  tasks ping-pong: each does a little work, `setjmp`s to save where it is, and
  `longjmp`s **straight into the other task** -- there is no scheduler to return
  to.

```c
// inside task_one's forever loop:
if (setjmp(task1_context) == 0) {   // save my resume point...
    longjmp(task2_context, 1);      // ...and hand off directly to task_two
}
// task_two later longjmps back here and we continue
```

## Why one stack is enough here (and when it isn't)

`setjmp`/`longjmp` save and restore the stack **pointer**, not the stack
**contents**. So sharing one stack between suspending tasks is only safe if no
task leaves a live frame on the stack that another task then overwrites.

Here it *is* safe, for two reasons:

1. **Bootstrapping unwinds each frame.** `main` calls each task, the task
   `setjmp`s and `longjmp`s back, so its call frame is gone before the next task
   is set up. The tasks are not nested inside each other.
2. **The loop frames are tiny and equal-depth.** Once running, each task's
   forever-loop uses only a couple of local words, at the same stack depth, and
   never grows that frame across a hand-off. So when control ping-pongs, the
   bytes one task saved are still intact when it resumes.

Break either property -- give a task a large local buffer, or a deeper call
chain that is still live when it yields -- and the tasks start clobbering each
other's saved frames: undefined behaviour, and in practice the program hangs.
That is precisely the failure mode `10_os_coop_two_stacks` avoids by handing
every task its own private stack, exactly as a real RTOS does.

So this exercise is not "the wrong way" -- it is the *minimal* way, correct for
simple tasks. The sibling shows the *general* way, correct for any task.

## Two solutions: bare-metal and HAL

- `main_bare.c` -- bare metal: drive the LED and delay with registers.
- `main_hal.c` -- the same behaviour via ST's HAL (`HAL_GPIO_TogglePin` /
  `HAL_Delay`).

The `setjmp`/`longjmp` skeleton is identical in both; only the task bodies
differ.

Build both images with `make 09_os_coop_single_stack`. Flash one with
`scripts/flash_exercise.sh 09_os_coop_single_stack bare` (or `hal`).
