#!/bin/bash -eu
# flash a pre-built exercise to the board with STM32_Programmer_CLI.
# This script does NOT build -- run `make <exercise>` first.
# usage: scripts/flash_exercise.sh <exercise> [bare|hal]
#   scripts/flash_exercise.sh 02_serial_counter        # single-core, bare (default)
#   scripts/flash_exercise.sh 02_serial_counter hal    # single-core, HAL
#   scripts/flash_exercise.sh 00_dual_blink bare       # dual-core: flashes BOTH
#                                                      # cores (M4 bank1 + M0+ bank2)
HERE="$(dirname "$0")/.."
source "$HERE/scripts/cubeide.sh"

# List flashable exercises by scanning the trees directly (never invokes make).
list_exercises() {
	echo "single-core exercises:" >&2
	for dir in "$HERE"/exercises/single_core/[0-9]*_*/; do
		[ -f "$dir/main_bare.c" ] || continue
		echo "  $(basename "$dir")" >&2
	done
	echo "dual-core exercises:" >&2
	for dir in "$HERE"/exercises/dual_core/[0-9]*_*/; do
		[ -f "$dir/main_cm4_bare.c" ] || continue
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
# Helper: a built file must exist (this script only flashes, never builds).
need() {
	[ -f "$1" ] && return 0
	echo "error: $1 not found -- build it first:  make $EX" >&2
	exit 1
}

# We flash with ST's STM32_Programmer_CLI, not st-flash: on the NUCLEO-WL55JC
# st-flash cannot drive NRST ("NRST is not connected"), so its
# --connect-under-reset is a no-op and the flash fails. The CLI's mode=UR
# (connect under reset) works -- the running firmware can otherwise hold the SWD
# lines. ELFs carry their own load addresses, so no explicit address is needed.
if [ -d "$HERE/exercises/single_core/$EX" ]; then
	ELF="$HERE/exercises/single_core/$EX/app_${VARIANT}.elf"
	need "$ELF"
	echo "flashing single-core $EX ($VARIANT) and starting it"
	"${CLI}" -c port=SWD mode=UR -d "${ELF}" --start
elif [ -d "$HERE/exercises/dual_core/$EX" ]; then
	# Dual-core: program BOTH images. Each ELF lands in its own flash bank
	# (M4 -> bank 1 @ 0x08000000, M0+ -> bank 2 @ 0x08020000, per the linker
	# scripts). The M4 firmware sets PWR.C2BOOT to start the M0+; SBRV already
	# points at bank 2, so no option-byte change is needed. Flash both in one
	# CLI session, then reset to run.
	M4="$HERE/exercises/dual_core/$EX/app_cm4_${VARIANT}.elf"
	M0="$HERE/exercises/dual_core/$EX/app_cm0p_${VARIANT}.elf"
	need "$M4"; need "$M0"
	echo "flashing dual-core $EX ($VARIANT): M0+ (bank 2) then M4 (bank 1)"
	# Program the M0+ first, then the M4, then reset & run.
	"${CLI}" -c port=SWD mode=UR -d "${M0}" -d "${M4}" -rst
else
	echo "error: no exercise '$EX' under exercises/single_core or exercises/dual_core" >&2
	list_exercises
	exit 1
fi
