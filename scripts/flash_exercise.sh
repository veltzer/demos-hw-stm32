#!/bin/bash -eu
# flash a pre-built single-core exercise to the board with STM32_Programmer_CLI.
# This script does NOT build -- run `make <exercise>` (or build_exercise.sh) first.
# usage: scripts/flash_exercise.sh <exercise> [bare|hal]
#   scripts/flash_exercise.sh 02_serial_counter        # bare-metal (default)
#   scripts/flash_exercise.sh 02_serial_counter hal    # HAL version
HERE="$(dirname "$0")/.."
source "$HERE/scripts/cubeide.sh"

# List the flashable (single-core) exercises by scanning the tree directly --
# this script never invokes make.
list_exercises() {
	echo "single-core exercises:" >&2
	for dir in "$HERE"/exercises/single_core/[0-9]*_*/; do
		[ -f "$dir/main_bare.c" ] || continue
		echo "  $(basename "$dir")" >&2
	done
}

if [ "$#" -lt 1 ]; then
	echo "usage: $0 <exercise> [bare|hal]" >&2
	list_exercises
	exit 1
fi
EX="$1"
VARIANT="${2:-bare}"
case "$VARIANT" in
	bare|hal) ;;
	*)
		echo "error: variant must be 'bare' or 'hal' (got '$VARIANT')" >&2
		exit 1
		;;
esac
# Resolve which tree the exercise lives in. Each single-core exercise builds two
# images (bare + HAL); dual-core exercises build two CORE images and need option
# bytes to actually run, which this script does not do.
if [ -d "$HERE/exercises/single_core/$EX" ]; then
	ELF="$HERE/exercises/single_core/$EX/app_${VARIANT}.elf"
elif [ -d "$HERE/exercises/dual_core/$EX" ]; then
	echo "error: '$EX' is a dual-core exercise." >&2
	echo "       It builds two images (M4 + M0+) and needs the C2BOOT/SBRV" >&2
	echo "       option bytes set to run the M0+ -- not supported by this script." >&2
	echo "       Build it with: make $EX" >&2
	exit 1
else
	echo "error: no exercise '$EX' under exercises/single_core or exercises/dual_core" >&2
	list_exercises
	exit 1
fi
# The image must already be built (this script only flashes).
if [ ! -f "$ELF" ]; then
	echo "error: $ELF not found -- build it first:" >&2
	echo "       make $EX" >&2
	exit 1
fi
echo "flashing $ELF and starting it"
# Flash with ST's STM32_Programmer_CLI, not st-flash: on the NUCLEO-WL55JC
# st-flash cannot drive NRST ("NRST is not connected"), so its
# --connect-under-reset is a no-op and the flash fails. The CLI's mode=UR
# (connect under reset) works -- needed because the running firmware can hold
# the SWD lines otherwise. The ELF carries its own load addresses; --start runs
# it after programming.
"${CLI}" -c port=SWD mode=UR -d "${ELF}" --start
