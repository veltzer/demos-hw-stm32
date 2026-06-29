# Dual-Core Blink (two cores, no communication)

The simplest possible dual-core program, and the right first step into multi-core
work: **each core blinks its own LED, at its own rate, with no communication
between them.** Seeing two LEDs blink at different rates is direct proof that
both CPUs are running their own firmware at the same time.

- **M4 (CPU1)** blinks **LD1 (blue, PB15)** slowly.
- **M0+ (CPU2)** blinks **LD2 (green, PB9)** quickly.

The only interaction between the cores is the one unavoidable bootstrap step:
after reset only the M4 is running, so it must **release the M0+**. That is a
single write -- `PWR->CR4 |= C2BOOT` (bare metal) or `HAL_PWREx_EnableBootC2()`
(HAL). After that the two cores run completely independently.

What you'll learn: how the two firmware images live in separate flash banks
(M4 in bank 1 at `0x08000000`, M0+ in bank 2 at `0x08020000`), that the M4 is
responsible for booting the M0+, and what genuine parallel execution looks like.
No IPCC, no shared state -- each core only ever touches its own LED pin (and we
use atomic `BSRR` writes so even the shared GPIO port can't race).

## Two solutions: bare-metal and HAL

Each core has both a bare-metal and a HAL solution, so there are four sources:

- `main_cm4_bare.c` / `main_cm0p_bare.c` -- register level.
- `main_cm4_hal.c` / `main_cm0p_hal.c` -- using the HAL.

`make 00_dual_blink` builds all four images
(`firmware_cm4_bare.bin`, `firmware_cm0p_bare.bin`, `firmware_cm4_hal.bin`,
`firmware_cm0p_hal.bin`). Note the HAL is compiled twice -- once for the M4
(cortex-m4) and once for the M0+ (cortex-m0plus) -- because the two cores have
different instruction sets, so the same HAL source becomes different machine
code on each.

## Running it

Both cores must be flashed (M4 -> bank 1, M0+ -> bank 2), then reset. On this
board the M0+ boot vector (`SBRV`) already points at bank 2, so no option-byte
changes are needed -- the M4's `C2BOOT` write is enough to start the M0+.
