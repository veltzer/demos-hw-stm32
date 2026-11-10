# demos-hw-stm32

Bare-metal and HAL exercises for the ST NUCLEO-WL55JC board (STM32WL55JC,
a dual-core Cortex-M4 + Cortex-M0+).

Each exercise under `exercises/single_core/` comes in two versions:
`main_bare.c` (register-level) and `main_hal.c` (the same lesson through ST's
HAL). The exercises under `exercises/dual_core/` program both cores.

## Prerequisites

- The ARM toolchain and board tools: `scripts/install_deps.sh`.
- ST's STM32CubeWL firmware package, cloned into `STM32CubeWL/` (gitignored).
  The build fetches it on first use, or run `scripts/clone_cubewl.sh`.
- For flashing: `STM32_Programmer_CLI`, which ships with STM32CubeIDE.
  `scripts/download_cubeide.py` downloads the installer (run `uv sync` first).

## Build

```bash
make                     # every exercise
make 03_serial_counter   # one exercise
make list                # the exercises make found
```

`rsconstruct build` builds the same images to the same paths, plus the
repository's checks.

## Flash

```bash
scripts/flash_exercise.sh 03_serial_counter       # bare version
scripts/flash_exercise.sh 03_serial_counter hal   # HAL version
scripts/flash_menu.sh                             # pick from a menu
```

The flash scripts only flash: build first. A dual-core exercise flashes both
cores. Serial output appears on `/dev/ttyACM0` (`scripts/tio_run.sh`).

`CLAUDE.md` has the details of the build and the board.
