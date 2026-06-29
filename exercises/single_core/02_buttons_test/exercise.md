# Buttons Test (diagnostic over UART)

Monitor all of the board's user buttons at once and print, over the serial port,
every time one changes state — reporting the actual pin level. This is both a
GPIO-input exercise and a practical diagnostic: it tells you *which* MCU pin a
given physical button is wired to, and whether it reads HIGH or LOW when pressed.
That mapping is needed by the later button exercises (`04_buttons_and_leds`,
`05_interrupts`).

The NUCLEO-WL55JC user buttons (per UM2592) are:

| Button | Pin(s)                          |
| ------ | ------------------------------- |
| B1     | PA0 (default) or PC13 (alt)     |
| B2     | PA1                             |
| B3     | PC6                             |

(B4/RESET is NRST and is not a GPIO input — see `doc/board_buttons.md` for the
physical layout and which button is which.)

What you'll learn: configuring several GPIOs as inputs across two ports, reading
the input data register (IDR), edge-detecting in software (print only on change),
and using the UART as a debug console. Because the program prints the *level*
rather than assuming a polarity, it works no matter how the board's solder
bridges are configured.

## Watching the output

9600 baud, 8N1, on the ST-LINK virtual COM port:

    tio -b 9600 /dev/ttyACM0
    # or
    minicom -D /dev/ttyACM0 -b 9600

Press each button in turn; you'll see lines such as:

    B1 (PA0)  HIGH
    B1 (PA0)  LOW
    B3 (PC6)  LOW

The pin that reacts is the one that button is wired to, and HIGH-vs-LOW tells you
its active level.
