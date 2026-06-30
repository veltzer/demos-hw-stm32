#  Basic shared memory exercise

create two pieces of software (running on M4,M0+) that share memory.
The shared memory is just one integer that starts off as 0.
M4 monitors the first button, when it is pressed it turns the integer from 0 to 1,
or 1 to 0 as the case may be.
M0+ just prints the value of the integer to the UART every 2 seconds.
Monitor the output using the tio(1) tool.
