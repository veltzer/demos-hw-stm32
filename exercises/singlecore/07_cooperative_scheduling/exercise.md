# Cooperative Scheduling (Bare-metal Scheduler)

Instead of a simple loop, you'll create a basic cooperative scheduler. This involves setting up a simple task management system where different "tasks" (functions) voluntarily give up control to allow other tasks to run. For example, you could have one task blinking an LED at one rate, and another task checking for a button press, both running in a "round-robin" fashion.

What you'll learn: This is your first step into understanding how an operating system's scheduler works. You'll learn about task control blocks, context switching (in a simplified, cooperative manner), and the concept of a main loop that dispatches tasks. This teaches the fundamentals of concurrent operations without the complexity of preemption.
