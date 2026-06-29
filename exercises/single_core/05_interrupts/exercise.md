# Using Interrupts for Button Presses

Instead of constantly checking the button's state (polling), this exercise teaches you how to use an interrupt. The program can be doing something else (or nothing at all), and the button press will automatically trigger a specific function to control the LED.

What you'll learn: This is a significant step up in complexity and introduces a core concept in embedded systems: hardware interrupts. You'll learn how to configure the EXTI (External Interrupt) controller and the NVIC (Nested Vectored Interrupt Controller). This is a much more efficient way to handle external events compared to polling.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure and drive the peripheral registers directly.
- `main_hal.c` — the same behaviour using ST's HAL (`HAL_*` calls).

Build both images with `make 05_interrupts`. Flash one with
`scripts/flash_exercise.sh 05_interrupts bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
