# `stm32wlxx_hal_conf.h` — the HAL configuration header

`exercises/common/stm32wlxx_hal_conf.h` is the one piece of the HAL setup that is
**ours**, not vendor code. The HAL drivers and CMSIS come from the cloned
`STM32CubeWL/` package (gitignored); this config header stays in the repo because
it is project-specific. Every HAL build is compiled with `-Iexercises/common` so
this file is the `stm32wlxx_hal_conf.h` the HAL sources pick up.

## What it does

ST ships the HAL as a big set of drivers guarded by preprocessor switches. The
HAL source files all begin with `#include "stm32wlxx_hal_conf.h"`, and each
driver's body is wrapped in `#ifdef HAL_<MODULE>_MODULE_ENABLED`. So this header
is what **turns individual HAL modules on or off** and **declares the board's
clock values**. It has three jobs:

### 1. Select which HAL modules are compiled

A block of `#define HAL_<MODULE>_MODULE_ENABLED` lines. A module that is *not*
defined here compiles to nothing (and its header is not included), which keeps
unused drivers out of the build. The modules enabled for these exercises are:

| Module    | Why it's on                                             |
| --------- | ------------------------------------------------------- |
| `GPIO`    | every exercise drives LEDs / reads buttons              |
| `RCC`     | clock + peripheral-clock enables (always needed)        |
| `CORTEX`  | `HAL_Init`, SysTick, NVIC priority helpers              |
| `PWR`     | low-power + the M0+ boot path (dual-core)               |
| `FLASH`   | a HAL dependency pulled in by the core                  |
| `DMA`     | a HAL dependency of several drivers                     |
| `UART`    | the serial-counter / buttons-test exercises             |
| `EXTI`    | the interrupt (button EXTI) exercise                    |
| `IPCC`    | inter-core mailbox (dual-core exercises)                |
| `HSEM`    | hardware semaphore (dual-core exercises)                |

`IPCC` and `HSEM` were enabled by us specifically for the dual-core work — the
CubeIDE blink project this was originally derived from left them off.

### 2. Declare the oscillator / clock values

`#define`s such as `HSE_VALUE` (32 MHz), `MSI_VALUE` (4 MHz, the reset system
clock), `HSI_VALUE`, `LSE_VALUE`, `LSI_VALUE`, plus `VDD_VALUE` and
`TICK_INT_PRIORITY`. The HAL uses these constants when it computes baud-rate
dividers, timer prescalers, `SystemCoreClock`, etc. They describe *this board's*
hardware; getting `HSE_VALUE` wrong, for instance, would make every derived
clock wrong.

### 3. Pull in the enabled module headers + assert config

The tail of the file `#include`s the header for each enabled module (again under
`#ifdef`), and provides the `assert_param` macro (a no-op unless
`USE_FULL_ASSERT` is defined).

## How it differs from the vendor file

STM32CubeWL ships only a **`stm32wlxx_hal_conf_template.h`** — a starting point
meant to be copied into a project and edited. Our `stm32wlxx_hal_conf.h` is that
template, trimmed to the modules these exercises use and with `IPCC`/`HSEM`
turned on. That is exactly why it lives in the repo rather than being taken from
the clone: it is configuration, not a driver.

## Changing it

To use a HAL peripheral not currently enabled (say `TIM`), add
`#define HAL_TIM_MODULE_ENABLED` here — the driver source is already present in
the clone and will start compiling into the shared `libhal_cm4.a`. No other build
changes are needed.
