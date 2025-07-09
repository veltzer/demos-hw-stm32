# Running a Simple RTOS 

For the final exercise, you'll port a small, open-source Real-Time Operating System (RTOS) like FreeRTOS or Zephyr to the board. Your goal will be to run multiple tasks on one or both cores, using the RTOS's built-in features for scheduling, inter-task communication (queues, semaphores), and memory management. A great final project is having the M4 core handle LoRaWAN communication while the M0+ core manages sensor data acquisition and user interface LEDs.

What you'll learn: This ties everything together. While not strictly "bare-metal" in the final application, porting the RTOS itself is a deeply bare-metal process. You will learn how a professional RTOS is structured, how it abstracts the hardware, and how to use its powerful API to build complex, multi-threaded applications. This is the bridge from low-level programming to building sophisticated embedded systems.
