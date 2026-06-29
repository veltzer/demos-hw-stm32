# Serial Counter over UART

Print an ever-increasing counter to the serial port: `i is 4`, `i is 5`,
`i is 6`, ... one line at a time, forever.

This is the "hello world" of getting text *out* of the board. The NUCLEO-WL55JC's
on-board ST-LINK exposes a USB Virtual COM Port (`/dev/ttyACM0`), and the chip's
LPUART1 TX pin (PA2) is routed to it, so anything the firmware transmits shows up
in a serial terminal on your PC.

What you'll learn: how to bring up the LPUART1 peripheral at the register level —
enable its clock, put the TX pin into the right alternate function, set the baud
rate divisor, and poll the TX-FIFO-empty flag to send one byte at a time. Because
there is no `printf` on bare metal, you also write a tiny integer-to-decimal
routine by hand.

To watch the output (9600 baud, 8N1):

    tio -b 9600 /dev/ttyACM0
    # or
    minicom -D /dev/ttyACM0 -b 9600

(See `scripts/tio_run.sh` / `scripts/minicom_run.sh`.)
