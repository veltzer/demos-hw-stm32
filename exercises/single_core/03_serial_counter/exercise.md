# Serial Counter over UART

Print an ever-increasing counter to the serial port: `i is 4`, `i is 5`,
`i is 6`, ... one line at a time, forever.

This is the "hello world" of getting text *out* of the board. The NUCLEO-WL55JC's
on-board ST-LINK exposes a USB Virtual COM Port (`/dev/ttyACM0`), and the chip's
LPUART1 TX pin (PA2) is routed to it, so anything the firmware transmits shows up
in a serial terminal on your PC.

What you'll learn: how to bring up the LPUART1 peripheral. The bare-metal
solution does it at the register level — enable its clock, put the TX pin into
the right alternate function, set the baud-rate divisor, poll the TX-empty flag,
and (since there is no `printf`) hand-roll a tiny integer-to-decimal routine. The
HAL solution does the same job with `HAL_UART_Init` / `HAL_UART_Transmit` and
`snprintf`, so you can see how much the driver layer abstracts away.

To watch the output (9600 baud, 8N1):

    tio -b 9600 /dev/ttyACM0
    # or
    minicom -D /dev/ttyACM0 -b 9600

(See `scripts/tio_run.sh` / `scripts/minicom_run.sh`.)

## Two solutions: bare-metal and HAL

This exercise is solved two ways, both in this folder:

- `main_bare.c` — bare metal: configure and drive the peripheral registers directly.
- `main_hal.c` — the same behaviour using ST's HAL (`HAL_*` calls).

Build both images with `make 03_serial_counter`. Flash one with
`scripts/flash_exercise.sh 03_serial_counter bare` (or `hal`). Comparing the two shows what
the HAL does for you — and what it hides.
