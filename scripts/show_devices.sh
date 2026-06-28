#!/bin/bash -eu
# Show the two device files used to talk to the ST board, and their permissions:
#   1. the raw USB node  - debug/SWD: st-info, st-flash, STM32_Programmer_CLI
#   2. the serial VCP     - UART console: minicom, tio

ST_VID="0483"
STLINK_V3_PID="374e"

echo "== 1. USB device node (debug / SWD) =="
line=$(lsusb | grep -i "${ST_VID}:${STLINK_V3_PID}" || true)
if [ -z "${line}" ]; then
	echo "  ST-LINK (${ST_VID}:${STLINK_V3_PID}) not found on USB."
else
	bus=$(echo "${line}" | sed -E 's/^Bus ([0-9]+) Device ([0-9]+):.*/\1/')
	dev=$(echo "${line}" | sed -E 's/^Bus ([0-9]+) Device ([0-9]+):.*/\2/')
	ls -l "/dev/bus/usb/${bus}/${dev}"
fi

echo
echo "== 2. Serial VCP (UART console) =="
found=""
for s in /dev/serial/by-id/*STLINK*; do
	[ -e "${s}" ] || continue
	found="yes"
	# resolve the symlink to the real ttyACMx and show that node's permissions
	real=$(readlink -f "${s}")
	ls -l "${s}"
	ls -l "${real}"
done
[ -n "${found}" ] || echo "  no ST-LINK serial port found (no /dev/serial/by-id/*STLINK*)."
