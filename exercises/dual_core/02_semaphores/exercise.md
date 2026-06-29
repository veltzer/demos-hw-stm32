# Shared Resource Management with Semaphores

Both cores will now try to access the same peripheral, for example, the UART for printing messages. If both try to write at the same time, the output will be garbled. You'll implement a semaphore or a mutex using the IPCC or shared memory to ensure that only one core can access the UART at a time.

What you'll learn: This exercise teaches a critical concept in concurrent programming: mutual exclusion and resource protection. You will learn to implement locking mechanisms to protect shared resources, preventing data corruption and ensuring system stability in a multi-core environment.

## Two solutions: bare-metal and HAL

Each core has both a bare-metal and a HAL solution, so there are four sources:
`main_cm4_bare.c` / `main_cm0p_bare.c` (register level) and
`main_cm4_hal.c` / `main_cm0p_hal.c` (using the HAL, with the HAL IPCC driver).
`make 02_semaphores` builds all four images. The HAL is compiled twice -- once per core
(cortex-m4 and cortex-m0plus), since the two cores have different instruction
sets.
