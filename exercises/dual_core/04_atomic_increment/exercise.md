# Atomic increment exercise

Two cores (M4 and M0+) hammer the **same** integer in shared memory, each
incrementing it a fixed number of times. The whole point is to *see* the
difference between a non-atomic and an atomic read-modify-write on real
hardware: in non-atomic mode increments are silently lost; in atomic mode none
are. A button press switches between the two modes live.

## The race

`counter++` is **not** one indivisible operation. On Cortex-M it compiles to
three instructions:

```
LDR   r0, [counter]   ; read
ADDS  r0, r0, #1       ; modify
STR   r0, [counter]   ; write
```

When BOTH cores run that sequence against the same address, their
load/modify/store can interleave:

```
M4 : LDR  (reads 100) .................. STR (writes 101)
M0+:        LDR (reads 100) STR (writes 101)
```

Both cores read 100, both write 101 -- two increments happened, but the counter
only advanced by one. That is a **lost update**. Over millions of increments
the losses pile up and are easy to measure.

The atomic version uses the Cortex-M exclusive monitor (LDREX/STREX) via the
GCC built-in `__atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST)`. STREX fails if
the other core touched the location since our LDREX, and the built-in retries
until it succeeds -- so no increment is ever lost.

## What each round does

Both cores agree on a target `N` (e.g. 1,000,000) increments **each**, so the
counter *should* reach `2 * N` per round. A round is:

1. M4 zeroes `counter` and clears both done-flags, then starts the round.
2. Each core increments `counter` exactly `N` times (atomically or not,
   depending on `mode`), then sets ITS OWN done-flag and spins.
3. The M0+ waits until BOTH done-flags are set, then prints:
   - `mode`     : `RACY` (non-atomic) or `ATOMIC`
   - `expected` : `2 * N`
   - `actual`   : the value `counter` actually reached
   - `lost`     : `expected - actual` (how many increments vanished)
4. M4 sees both flags set, begins the next round.

Expected output (numbers illustrative):

```
mode=RACY   expected=2000000 actual=1374251 lost=625749
mode=RACY   expected=2000000 actual=1402190 lost=597810
<button press>
mode=ATOMIC expected=2000000 actual=2000000 lost=0
mode=ATOMIC expected=2000000 actual=2000000 lost=0
```

`lost` is some large, varying number in RACY mode and **exactly 0** in ATOMIC
mode, every round. Watch it with the `tio(1)` tool.

## The button

M4 watches user button B1 (PA0, active-low) and toggles the shared `mode` flag
on each press edge (`RACY` <-> `ATOMIC`). The change takes effect from the next
round. So pressing the button flips the output between "lots of lost
increments" and "zero lost increments".

## How the memory is shared

Same mechanism as `03_basic_shared_memory`: SRAM2 (base `0x20008000`) is
reachable from BOTH cores over the bus matrix. The M0+ image defines a small
`shared_t` struct in a reserved `.shared` section that its linker script
(`STM32WL55JCIX_FLASH_CM0PLUS.ld`, exercise-local) pins to the base of SRAM2, so
its address is fixed and never overlaps anything else. The M4 has no symbol for
it -- it reaches the same fields through a `volatile shared_t*` at the absolute
address `0x20008000`. The struct holds:

| field            | written by | meaning                                  |
| ---------------- | ---------- | ---------------------------------------- |
| `counter`        | both cores | the contended integer                    |
| `mode`           | M4         | `0` = RACY, `1` = ATOMIC                  |
| `m4_done`        | M4         | M4 finished its N increments this round  |
| `m0p_done`       | M0+        | M0+ finished its N increments this round |
| `round`          | M4         | round number, bumped to start each round |

Everything is `volatile` because the OTHER core mutates it. Note the handshake
is race-free *without* atomics even in RACY mode: each done-flag is written by
exactly one core and only read by the other, and `round` is written only by M4.
Only `counter` is deliberately contended.

## Why the M0+ is the printer

The M0+ owns LPUART1 (PA2 TX @ 9600, routed to the ST-LINK VCP) and does all the
printing, exactly as in `03`. The M4 owns the bootstrap (`PWR.C2BOOT` /
`LL_PWR_EnableBootC2`) that releases the M0+, owns the button, and drives the
round counter.

## Two solutions: bare-metal and HAL

Each core has both a bare-metal and a HAL solution, so there are four sources:
`main_cm4_bare.c` / `main_cm0p_bare.c` (register level) and
`main_cm4_hal.c` / `main_cm0p_hal.c` (using the HAL).
`make 04_atomic_increment` builds all four images. The HAL is compiled twice --
once per core (cortex-m4 and cortex-m0plus), since the two cores have different
instruction sets.

## A note on what "atomic" fixes here

The bug is **not** a cache-coherency / visibility bug (the STM32WL cores have no
data cache, and `volatile` already forces every access to hit SRAM -- that was
`03`'s lesson). This is a **read-modify-write atomicity** bug: even with every
load and store going straight to memory, the non-atomic `load; add; store`
sequence is interruptible by the other core between its steps. `volatile` does
NOT make `counter++` atomic; only LDREX/STREX (the atomic built-in) does.
