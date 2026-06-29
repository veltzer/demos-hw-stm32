# Buttons and LEDs

This exercise introduces the concept of input. The goal is to make an LED turn on or change its state only when the user push-button (B1 on PA0) is pressed.

What you'll learn: You'll learn how to configure a GPIO pin as an input and how to read its state. This introduces the fundamental concept of polling, where you continuously check the button's status in a loop. It's a foundational step for creating interactive devices.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure and drive the peripheral registers directly.
- `main_hal.c` — the same behaviour using ST's HAL (`HAL_*` calls).

Build both images with `make 04_buttons_and_leds`. Flash one with
`scripts/flash_exercise.sh 04_buttons_and_leds bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
