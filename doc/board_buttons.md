# NUCLEO-WL55JC buttons & LEDs — physical layout

Hold the board so the **USB connector (CN1, to the ST-LINK) points away from you
(top edge)** and the component side faces up. Then:

                        ┌──────── USB (CN1, to PC) ────────┐
                        │            [ ST-LINK ]           │
       ┌────────────────┴──────────────────────────────────┴────────────────┐
       │  (B4)                                                                │
       │  RESET ●  ← top-left, next to ST-LINK                                │
       │           = NRST, resets the MCU. NOT a user button.                 │
       │                                                                      │
       │                         ● LD5 (ST-LINK status)                       │
       │                                                                      │
       │                  STM32WL55JC                                         │
       │                  (big chip, center)        ● LD1 (blue)   = PB15     │
       │                                            ● LD2 (green)  = PB9      │
       │                                            ● LD3 (red)    = PB11     │
       │                                                                      │
       │   user buttons, bottom-left edge:                                    │
       │     ● B1  (USER) = PA0   (confirmed; active-low)                    │
       │     ● B2  (USER) = PA1                                               │
       │     ● B3  (USER) = PC6                                               │
       │                                                                      │
       └──────────────────────────────────────────────────────────────────────┘
                          (Arduino / Morpho headers along the long edges)

## Buttons

| Silkscreen | Role        | GPIO                                   |
| ---------- | ----------- | -------------------------------------- |
| **B1**     | USER button | **PA0** (default) — or **PC13** \*     |
| **B2**     | USER button | PA1                                    |
| **B3**     | USER button | PC6                                    |
| **B4**     | RESET       | NRST (resets the chip; not a GPIO in)  |

\* B1 can be wired to **PA0** (factory default, SB16 ON / SB15 OFF) or **PC13**
(SB15 ON / SB16 OFF). On *this* board B1 is **PA0** — confirmed with
02_buttons_test (see below). The button exercises use PA0 / EXTI line 0.

All user buttons are **active-low**: the pin idles HIGH (internal pull-up) and
goes LOW while the button is held.

## LEDs

| Silkscreen | Color | GPIO  | Active level |
| ---------- | ----- | ----- | ------------ |
| **LD1**    | blue  | PB15  | high = on    |
| **LD2**    | green | PB9   | high = on    |
| **LD3**    | red   | PB11  | high = on    |

(LD5 near the ST-LINK is the debugger's own status LED, not user-controllable.)

## Known hardware issue on this board: button press drops the ST-LINK VCP

On this particular board, pressing **any** of the three user buttons (B1/B2/B3)
causes the ST-LINK's USB virtual COM port (`/dev/ttyACM0`) to **disconnect and
re-enumerate** (~1 s bounce), e.g. in `tio`:

    [..] Connected to /dev/ttyACM0
    [..] Disconnected        <- on each button press
    [..] Connected to /dev/ttyACM0

The board is powered from the ST-LINK USB only, and the user LEDs do **not**
visibly reset on press, so the MCU itself does not appear to fully reset — it is
the ST-LINK VCP that renumerates. This makes UART-based button debugging
unreliable (the stream drops exactly when a transition would be printed).

Likely a USB power-integrity glitch on the ST-LINK. Things to try (hardware):
a better/shorter USB cable, a powered USB hub, a different (rear) USB port, and
checking the board's power-source jumper.

A **different USB cable fixed it** — the VCP stopped dropping, and the mapping is
now confirmed (see below). So if button presses bounce the serial port, suspect
the cable/port first.

## Confirmed button mapping (via 02_buttons_test)

With a good cable, `02_buttons_test` printed, on press/release:

    B1 (PA0)  LOW / HIGH
    B2 (PA1)  LOW / HIGH
    B3 (PC6)  LOW / HIGH

So on this board:

| Button | Pin  | Active level                         |
| ------ | ---- | ------------------------------------ |
| B1     | PA0  | active-LOW (idle high, press = low)  |
| B2     | PA1  | active-LOW                           |
| B3     | PC6  | active-LOW                           |

B1 is **PA0** (the factory SB16/SB15 default), not PC13. All three are
active-low: enable an internal **pull-up**, and a press reads **LOW** / triggers
a **falling** edge. The button exercises (`04_buttons_and_leds`, `05_interrupts`) are
configured accordingly.
