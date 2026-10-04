# TOFIX

Findings from a code scan on 2026-10-04.

## High

- `scripts/download_cubeide.py:22-39` - when `playwright` is not importable, the script re-execs itself under `~/.venv/bin/python` (documented in `CLAUDE.md:169-173`). The repo already declares `playwright` in `pyproject.toml:5-7`, so the tool must come from the repo's own `.venv`; a script that only works because of the personal `~/.venv` is a repo bug. Remove `_reexec_in_venv()` and fail with a message telling the user to `uv sync` in the repo; fix the CLAUDE.md paragraph accordingly.
- `scripts/clone_cubewl.sh:28-31` - creates `.cubewl.lock` in the repo root and claims it is gitignored, but `.gitignore` only covers `/STM32CubeWL/` (`git check-ignore .cubewl.lock` matches nothing), so the first cold build leaves an untracked file and `rsconstruct status`/`git status` go dirty. Fleet-wide shared file: add `/.cubewl.lock` to the shared `.gitignore` next to the existing `/STM32CubeWL/` entry.

## Medium

- `scripts/extra_install.sh:2-4` - downloads two `.deb` files over plain `http://` and installs them with `sudo apt install ./file.deb`, which does no signature check on local files; pinned to `6.3-2ubuntu0.1` (Ubuntu 22.04 packages). Use `https://` and verify a checksum, or drop the script if CubeIDE no longer needs libncurses5 on the current release.
- `CLAUDE.md:110` and `scripts/rsc_build_hal_lib.sh:4`, `scripts/rsc_build_image.sh:4` - refer to `rsconstruct.local.toml`, which does not exist; the stanzas are in `rsconstruct.toml:55-315`. Fix the references.
- `CLAUDE.md:57-59,64-71` and `Makefile:12-14,20,52` - describe exercises as `single_core/NN/main.c` and `dual_core/NN/main_cm4.c`/`main_cm0p.c` (and `Makefile:12-13` even says `singlecore/`/`dualcore/`), but the real files are `main_bare.c`/`main_hal.c` and `main_cm4_bare.c`/`main_cm0p_hal.c` etc. Update the docs/comments to the actual layout.
- `CLAUDE.md:76-78` and `CLAUDE.md:191-195` - say dual-core images are "not flashed/run yet (TODO)", contradicting `CLAUDE.md:147-151` and `scripts/flash_exercise.sh:57-68`, which flash both cores. Remove the stale TODO paragraphs.
- `CLAUDE.md:99`, `Makefile:5`, `scripts/flash_exercise.sh:5-6` - examples use `02_serial_counter`, which does not exist (`02_buttons_test`, `03_serial_counter`); `make 02_serial_counter` fails. Fix to `03_serial_counter`.

## Low

- `scripts/rsc_build_image.sh:118` - `... 2>&1 | grep -vE "${LD_NOISE}" || true` discards gcc's exit status; failure is only caught by the "no .elf" check below it. Use `set -o pipefail` and check `PIPESTATUS[0]` so a failed link that still leaves an output is not silently accepted.
- `scripts/run.sh:6-7` - says `flash_exercise.sh` "builds then flashes via st-flash"; it does neither (it only flashes, with STM32_Programmer_CLI, `scripts/flash_exercise.sh:2-3,47-51`). Fix the comment.
- `scripts/flash.sh:3` - a script named `flash.sh` that only runs `st-flash erase`; it duplicates `scripts/mass_erase.sh`. Rename or delete it.
- `rsconstruct.toml:27-32` - `ruff` and `mypy` list `exercises` in `src_dirs`, which has no Python files (the only `.py` is `scripts/download_cubeide.py`); use `src_dirs = ["scripts"]`.
- `pyproject.toml:12` - `pytest` is in the dev group but the repo has no tests and no `pytest` processor; drop it.
- `README.md:1-2` - "Demos for this hardware" does not name the board (NUCLEO-WL55JC) or point at build/flash instructions that currently live only in `CLAUDE.md`; move the human-facing parts (build, flash, prerequisites) into the README.
