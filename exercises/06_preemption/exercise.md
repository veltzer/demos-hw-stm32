# Preemptive Scheduling with a Timer Interrupt

This exercise upgrades your scheduler from cooperative to preemptive. You'll configure a hardware timer (like the SysTick timer) to fire at a regular interval (e.g., every 10 milliseconds). The timer's interrupt service routine (ISR) will act as the scheduler, forcibly pausing the currently running task and switching to the next one.

What you'll learn: This is a major leap into real-time systems. You will implement a genuine preemptive scheduler, which is the foundation of any real-time operating system (RTOS). You'll deal with saving and restoring CPU context (registers) and understand the concept of a time slice or quantum.
