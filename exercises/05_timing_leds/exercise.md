# Basic Timer for Precise LED Blinking 

While you can create delays with simple software loops, these are imprecise. This exercise uses one of the built-in hardware timers (e.g., TIM1 or TIM2) to toggle an LED at a precise interval, for example, exactly once per second.

What you'll learn: This introduces another crucial peripheral: the hardware timer. You will learn to configure a timer to count up to a specific value (the auto-reload register, ARR) and generate an interrupt or an event. This provides a much more accurate and reliable way to manage timing than software delays, which can be affected by the compiler and other code. This is a fundamental skill for any real-time application.
