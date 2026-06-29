# Inter-Core Communication (M4 to M0+) using Mailboxes

Now it's time to wake up the second core! On the NUCLEO-WL55JC1, you have a powerful Cortex-M4 core and a low-power Cortex-M0+ core. In this exercise, the M4 core will perform a calculation or react to a button press and then send a message to the M0+ core to make it blink an LED. You'll use the built-in Inter-Processor Communication Controller (IPCC) which acts like a mailbox system.

What you'll learn: This is your introduction to multi-core programming. You will learn how to initialize both cores, manage separate firmware images, and use hardware mechanisms for safe communication between them. You'll understand the producer/consumer model and the importance of synchronization primitives to avoid race conditions.

## Two solutions: bare-metal and HAL

Each core has both a bare-metal and a HAL solution, so there are four sources:
`main_cm4_bare.c` / `main_cm0p_bare.c` (register level) and
`main_cm4_hal.c` / `main_cm0p_hal.c` (using the HAL, with the HAL IPCC driver).
`make 01_multi_core_mailboxes` builds all four images. The HAL is compiled twice -- once per core
(cortex-m4 and cortex-m0plus), since the two cores have different instruction
sets.
