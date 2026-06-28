# Inter-Core Communication (M4 to M0+) using Mailboxes

Now it's time to wake up the second core! On the NUCLEO-WL55JC1, you have a powerful Cortex-M4 core and a low-power Cortex-M0+ core. In this exercise, the M4 core will perform a calculation or react to a button press and then send a message to the M0+ core to make it blink an LED. You'll use the built-in Inter-Processor Communication Controller (IPCC) which acts like a mailbox system.

What you'll learn: This is your introduction to multi-core programming. You will learn how to initialize both cores, manage separate firmware images, and use hardware mechanisms for safe communication between them. You'll understand the producer/consumer model and the importance of synchronization primitives to avoid race conditions.
