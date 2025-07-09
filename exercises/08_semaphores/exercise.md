# Shared Resource Management with Semaphores

Both cores will now try to access the same peripheral, for example, the UART for printing messages. If both try to write at the same time, the output will be garbled. You'll implement a semaphore or a mutex using the IPCC or shared memory to ensure that only one core can access the UART at a time.

What you'll learn: This exercise teaches a critical concept in concurrent programming: mutual exclusion and resource protection. You will learn to implement locking mechanisms to protect shared resources, preventing data corruption and ensuring system stability in a multi-core environment.
