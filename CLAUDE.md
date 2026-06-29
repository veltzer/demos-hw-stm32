# CLAUDE.md

Guidance for working in this repo (STM32WL55 NUCLEO board demos & exercises).

## Dependencies

All apt-installable tools the repo uses are in `scripts/install_deps.sh`. The
command-to-package mapping (keep this in sync if scripts gain new tools):

| Command(s)                              | apt package              |
| --------------------------------------- | ------------------------ |
| `lsusb`, `usbreset`                     | `usbutils`               |
| `st-flash`, `st-info`                   | `stlink-tools`           |
| `minicom`                               | `minicom`                |
| `tio`                                   | `tio`                    |
| `make`                                  | `make`                   |
| `whiptail`                              | `whiptail`               |
| `dialog`                                | `dialog`                 |
| `pass`                                  | `pass`                   |
| `arm-none-eabi-gcc` / `objcopy` / `size`| `gcc-arm-none-eabi`, `binutils-arm-none-eabi`, `libnewlib-arm-none-eabi` |

`STM32_Programmer_CLI` (used by `mass_erase.sh`, `reset.sh`, `check*.sh`, `run.sh`,
`remove_protection.sh`, `show_devices.sh`) is **not** an apt package - it ships
with the proprietary STM32CubeIDE install, so it is not in `install_deps.sh`.

When adding a tool to any script or the Makefile, add its package to
`scripts/install_deps.sh`.

### STM32CubeWL package (required for ALL builds)

Both the HAL drivers and the CMSIS headers come from ST's full **STM32CubeWL**
firmware package. It is **not vendored in-repo** -- to avoid duplication it is
cloned into the repo root as `STM32CubeWL/` (gitignored):

    scripts/clone_cubewl.sh        # clone STM32CubeWL + needed submodules

So this clone is now a build prerequisite for **every** exercise (bare-metal and
HAL). If it is missing, `make` warns (pointing at `clone_cubewl.sh`) and the
compile fails on a missing `stm32wl55xx.h`. Override the location with
`make CUBEWL=/path/to/STM32CubeWL`.

What stays in the repo (`exercises/common/`) is only what is genuinely *ours*:
the customized startup (`startup_stm32wl55jcix*.s`), linker scripts (the M4 one
uses 128K bank 1 so the M0+ image fits in bank 2), `system_stm32wlxx.c`, and the
HAL config (`hal_config/stm32wlxx_hal_conf.h`). CMSIS and the HAL drivers are
taken from the clone (its versions are newer than the previously-vendored
copies). Each `main_hal.c` defines its own `SysTick_Handler` (forwarding to
`HAL_IncTick`), so HAL_Delay/HAL_GetTick work without any shared glue file.

## Build system

`Makefile` lives at the repo root and builds the exercises under `exercises/`,
which is split into two trees plus shared support:

- `exercises/singlecore/NN_name/` — one `main.c`, built into a single M4 image.
- `exercises/dualcore/NN_name/` — `main_cm4.c` + `main_cm0p.c`, built into two
  images, one per core (see below).
- `exercises/common/` — shared CMSIS / startup / linker support for both.

- The set of buildable exercises is **discovered from the filesystem**, not
  hardcoded: the directory tree IS the classification. Everything under
  `singlecore/` with a `main.c` builds one M4 image; everything under
  `dualcore/` with a `main_cm4.c` builds two. Sourceless dirs (e.g. a
  write-up-only exercise) are ignored, since discovery keys off the source
  files' presence.
- Dual-core exercises build an M4 image (`app_cm4.elf`/`firmware_cm4.bin`) from
  `main_cm4.c` and an M0+ image (`app_cm0p.elf`/`firmware_cm0p.bin`) from
  `main_cm0p.c`. The M0+ uses its own startup (`startup_stm32wl55jcix_cm0plus.s`,
  the smaller M0+ vector table) and linker script
  (`STM32WL55JCIX_FLASH_CM0PLUS.ld`, flash bank 2 @ `0x08020000`, SRAM2 @
  `0x20008000`) so the two cores' images never overlap. Compiled with
  `-mcpu=cortex-m0plus -DCORE_CM0PLUS`. **Building is done; actually running the
  M0+ still needs the `C2BOOT`/`SBRV` option bytes set and both images flashed —
  that is a separate, deliberate step and is not automated here.**
- Each exercise links its `main*.c` against the shared CMSIS / startup / linker
  support in `exercises/common/` (copied from the CubeIDE `workspace/blink`
  project; the `workspace/` tree itself is generated IDE output).
- Target MCU is the Cortex-M4 core: `-mcpu=cortex-m4 -mthumb -mfloat-abi=soft`
  (the chip has **no FPU**, `__FPU_PRESENT == 0`), with `-DSTM32WL55xx
  -DCORE_CM4`.
- Builds are incremental (per-object compilation, `-MMD` header deps). Output
  per exercise: `app.elf`, `firmware.bin`, `app.map`; intermediates in `.obj/`.
  All are gitignored.

Commands:

    make                 # build all single-core exercises (incremental)
    make 02_serial_counter   # build one exercise by name
    make list            # list discovered exercises
    make clean
    make -k              # keep going past a broken exercise
    make -j              # parallel
    make V=1 ...         # verbose (echo commands)

## Flashing

Build first with `make` (above); the flash scripts only flash, never build.

    scripts/flash_exercise.sh <name> [bare|hal]  # flash a pre-built single-core
                                                 #   exercise (default: bare)
                                                 #   via STM32_Programmer_CLI,
                                                 #   connect-under-reset
    scripts/flash_menu.sh                        # TUI: pick exercise + variant

Only single-core exercises are flashable; dual-core ones build two images but
need option bytes to run, so the flash scripts refuse them with a message.

## Downloading STM32CubeIDE

`scripts/download_cubeide.py` downloads the latest CubeIDE generic Linux
installer from st.com, **fully unattended** (no AI / no manual browser steps).

- It drives a headless browser via **Playwright** to perform st.com's
  JavaScript/SSO login and the download (plain curl/wget can't authenticate
  that flow).
- Requires Playwright with a chromium browser. If run under a python without
  playwright, the script re-execs itself under `~/.venv/bin/python` (where
  playwright + browsers are installed); otherwise install with
  `pip install playwright && playwright install chromium`. Not added to
  `install_deps.sh` because the browser binaries are not an apt package.
- Credentials: username from `$ST_USER` or `doc/user_and_password.txt`; password
  from `$ST_PASS` or `pass show sites/www.st.com` (override `ST_PASS_ENTRY`).
  The password is never printed or written to disk.
- Useful flags: `--headed` (watch the browser), `--dest DIR`, `--timeout SECS`.
  The self-extracting `.sh` installer is run by the user afterwards (needs sudo,
  installs to `/opt/st/`).
- st.com's selectors can change; if a step can't be found the script exits with
  a clear message — re-run with `--headed` to see where it stops.

## Conventions / gotchas

- Shell scripts start with `#!/bin/bash -eu`; they pass `shellcheck
  --severity=error --shell=bash`. The repo's `rsconstruct.toml` lints scripts
  (shellcheck), markdown (rumdl), and TOML (taplo).
- The `exercises/*/*/main*.c` are bare register-level programs. Some have
  genuine bugs that the build surfaces; the build system is a plain compiler and
  does not paper over them.
- Dual-core exercises (under `exercises/dualcore/`, numbered from `00`) now
  **build** both cores' images (M4 + M0+) via the from-scratch M0+
  startup/linker under `exercises/common/`. They are **not flashed/run** yet:
  that needs the `C2BOOT`/`SBRV` option bytes set so the M0+ boots from flash
  bank 2, plus flashing both images (TODO).
- The ST-LINK exposes a USB Virtual COM Port at `/dev/ttyACM0`. Serial output
  appears only if firmware transmits on a UART that is routed to that VCP, at a
  matching baud rate.
