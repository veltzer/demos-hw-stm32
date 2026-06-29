#!/bin/bash -eu
# Flash an ELF (or .bin) to the board and start it, using STM32CubeIDE's
# STM32_Programmer_CLI.
# usage: scripts/run.sh <file.elf|file.bin>
#
# (For the exercises you usually want scripts/flash_exercise.sh, which builds
#  then flashes via st-flash. This is the CubeIDE-CLI equivalent.)

source "$(dirname "$0")/cubeide.sh"

if [ "$#" -lt 1 ]; then
	echo "usage: $0 <file.elf|file.bin>"
	exit 1
fi
APP="$1"

"${CLI}" -c port=SWD mode=UR -d "${APP}" --start

# another option is:
# st-flash write your_firmware.bin 0x08000000
