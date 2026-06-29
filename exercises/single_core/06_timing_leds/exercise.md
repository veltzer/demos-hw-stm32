# Basic Timer for Precise LED Blinking 

While you can create delays with simple software loops, these are imprecise. This exercise toggles an LED at a precise interval — exactly once per second — using a real hardware time base instead of a busy loop. The bare-metal solution uses the TIM16 timer; the HAL solution uses the SysTick-based `HAL_Delay` (the HAL set vendored here does not include the TIM driver, and SysTick is the idiomatic HAL time base anyway).

What you'll learn: This introduces another crucial peripheral: the hardware timer. You will learn to configure a timer to count up to a specific value (the auto-reload register, ARR) and generate an interrupt or an event. This provides a much more accurate and reliable way to manage timing than software delays, which can be affected by the compiler and other code. This is a fundamental skill for any real-time application.

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure TIM16 (prescaler/ARR) and toggle the LED
  from its update interrupt.
- `main_hal.c` — `HAL_Delay(1000)` off the 1 ms SysTick that `HAL_Init()` starts.

Build both images with `make 06_timing_leds`. Flash one with
`scripts/flash_exercise.sh 06_timing_leds bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
