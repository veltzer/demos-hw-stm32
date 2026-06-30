# Basic shared memory exercise

create two pieces of software (running on M4,M0+) that share memory.
The shared memory is just one integer that starts off as 0.
M4 monitors the first button, when it is pressed it turns the integer from 0 to 1,
or 1 to 0 as the case may be.
M0+ just prints the value of the integer to the UART every 2 seconds.
Monitor the output using the tio(1) tool.

## How the memory is shared

SRAM2 (base `0x20008000`) is reachable from BOTH cores over the bus matrix, so
a single integer placed there is visible to each. The M0+ image defines the
variable in a reserved `.shared` section that its linker script
(`exercises/common/STM32WL55JCIX_FLASH_CM0PLUS.ld`) pins to the base of SRAM2,
guaranteeing a stable, non-overlapping address. The M4 has no symbol for it --
it reads/writes the same absolute address `0x20008000` through a `volatile`
pointer. The flag is `volatile` on both sides because the other core mutates it.

The M4 owns the bootstrap (`PWR.C2BOOT` / `LL_PWR_EnableBootC2`) that releases
the M0+, and watches button B1 (PA0, active-low), toggling the flag on each
press edge. The M0+ prints the flag every 2 seconds.

## Two solutions: bare-metal and HAL

Each core has both a bare-metal and a HAL solution, so there are four sources:
`main_cm4_bare.c` / `main_cm0p_bare.c` (register level) and
`main_cm4_hal.c` / `main_cm0p_hal.c` (using the HAL).
`make 03_basic_shared_memory` builds all four images. The HAL is compiled twice
-- once per core (cortex-m4 and cortex-m0plus), since the two cores have
different instruction sets.
