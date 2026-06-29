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

## Build system

`Makefile` lives at the repo root and builds the single-core exercises under
`exercises/`.

- The set of buildable exercises is **discovered from the filesystem**, not
  hardcoded: an `exercises/NN_name/` dir with a `main.c` and no
  `main_cm4.c` is a single-core exercise and is built. A dir with a
  `main_cm4.c` + `main_cm0p.c` is a dual-core exercise: it builds **two**
  images, one per core (see below). Sourceless dirs are excluded automatically.
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

    scripts/build_exercise.sh <name>   # build one exercise
    scripts/build_all.sh               # build all (make -k)
    scripts/flash_exercise.sh <name>   # build + st-flash to 0x08000000
    scripts/flash_menu.sh              # TUI: pick an exercise, build + flash

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
- The `exercises/*/main.c` are bare register-level programs. Some have genuine
  bugs that the build surfaces; the build system is a plain compiler and does
  not paper over them.
- Dual-core exercises (`08`, `09`) now **build** both cores' images (M4 + M0+)
  via the from-scratch M0+ startup/linker added under `exercises/common/`. They
  are **not flashed/run** yet: that needs the `C2BOOT`/`SBRV` option bytes set
  so the M0+ boots from flash bank 2, plus flashing both images (TODO).
- The ST-LINK exposes a USB Virtual COM Port at `/dev/ttyACM0`. Serial output
  appears only if firmware transmits on a UART that is routed to that VCP, at a
  matching baud rate.
