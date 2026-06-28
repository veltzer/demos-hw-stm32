#!/bin/bash -eu
# build a single exercise into app.elf and firmware.bin
# usage: scripts/build_exercise.sh <exercise>
#   scripts/build_exercise.sh 04_uart
ROOT="$(dirname "$0")/.."
if [ "$#" -lt 1 ]; then
	echo "usage: $0 <exercise>"
	echo "exercises:"
	make -C "$ROOT" --no-print-directory list
	exit 1
fi
make -C "$ROOT" "$1"
