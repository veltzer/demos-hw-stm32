#!/bin/bash -eu
# build a single exercise and flash it to the board with st-flash
# usage: scripts/flash_exercise.sh <exercise>
#   scripts/flash_exercise.sh 04_uart
HERE="$(dirname "$0")/.."
if [ "$#" -lt 1 ]; then
	echo "usage: $0 <exercise>"
	make -C "$HERE" --no-print-directory list
	exit 1
fi
EX="$1"
# build first
"$HERE/scripts/build_exercise.sh" "$EX"
BIN="$HERE/exercises/$EX/firmware.bin"
echo "flashing $BIN to 0x08000000"
st-flash write "$BIN" 0x08000000
