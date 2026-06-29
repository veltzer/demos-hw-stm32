#!/bin/bash -eu
# build a single exercise and flash it to the board with STM32_Programmer_CLI
# usage: scripts/flash_exercise.sh <exercise>
#   scripts/flash_exercise.sh 02_serial_counter
HERE="$(dirname "$0")/.."
source "$HERE/scripts/cubeide.sh"
if [ "$#" -lt 1 ]; then
	echo "usage: $0 <exercise>"
	make -C "$HERE" --no-print-directory list
	exit 1
fi
EX="$1"
# Resolve which tree the exercise lives in. Dual-core exercises build two
# images and need option bytes to actually run, which this script does not do.
if [ -d "$HERE/exercises/singlecore/$EX" ]; then
	ELF="$HERE/exercises/singlecore/$EX/app.elf"
elif [ -d "$HERE/exercises/dualcore/$EX" ]; then
	echo "error: '$EX' is a dual-core exercise." >&2
	echo "       It builds two images (M4 + M0+) and needs the C2BOOT/SBRV" >&2
	echo "       option bytes set to run the M0+ -- not supported by this script." >&2
	echo "       Build it with: make $EX" >&2
	exit 1
else
	echo "error: no exercise '$EX' under exercises/singlecore or exercises/dualcore" >&2
	make -C "$HERE" --no-print-directory list
	exit 1
fi
# build first
"$HERE/scripts/build_exercise.sh" "$EX"
echo "flashing $ELF and starting it"
# Flash with ST's STM32_Programmer_CLI, not st-flash: on the NUCLEO-WL55JC
# st-flash cannot drive NRST ("NRST is not connected"), so its
# --connect-under-reset is a no-op and the flash fails. The CLI's mode=UR
# (connect under reset) works -- needed because the running firmware can hold
# the SWD lines otherwise. The ELF carries its own load addresses; --start runs
# it after programming.
"${CLI}" -c port=SWD mode=UR -d "${ELF}" --start
