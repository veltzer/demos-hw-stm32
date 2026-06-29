# Multiple LED Patterns

This exercise builds directly on the first by controlling all three user LEDs on the board (LD1, LD2, and LD3). Instead of just blinking one, you can create various sequences.

What you'll learn: This reinforces the basics of GPIO (General-Purpose Input/Output) pin configuration and manipulation. You'll work with multiple pins and learn to manage their states to create specific patterns, such as a "Cylon" scanner effect or having them light up in a binary count sequence. This requires more intricate control over the GPIO registers.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure and drive the peripheral registers directly.
- `main_hal.c` — the same behaviour using ST's HAL (`HAL_*` calls).

Build both images with `make 01_led_patterns`. Flash one with
`scripts/flash_exercise.sh 01_led_patterns bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
