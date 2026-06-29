# Simple Blinking LED

This is the "hello world" of embedded development: make a single user LED (LD1, the blue LED on PB15 of the NUCLEO-WL55JC) blink on and off forever.

What you'll learn: the absolute basics of GPIO (General-Purpose Input/Output). You'll enable the clock for a GPIO port, configure one pin as an output, and toggle its state in an infinite loop, using a simple busy-wait delay to make the blinking visible to the eye. Every later exercise builds on this foundation.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure and drive the peripheral registers directly.
- `main_hal.c` — the same behaviour using ST's HAL (`HAL_*` calls).

Build both images with `make 00_simple_led`. Flash one with
`scripts/flash_exercise.sh 00_simple_led bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
