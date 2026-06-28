#!/bin/bash -eu
# Return the ST-LINK status LED to red (idle / no host communication).
#
# The LED is not directly controllable - it is driven by the ST-LINK firmware
# to show the host<->probe<->target communication state (green = a session is
# established). The only way back to red from software is to drop that session
# by resetting the USB device, which re-puts the probe in its powered-but-idle
# state. This is effectively a software unplug/replug.

ST_VID="0483"
STLINK_V3_PID="374e"

if ! lsusb | grep -qi "${ST_VID}:${STLINK_V3_PID}"; then
	echo "ST-LINK (${ST_VID}:${STLINK_V3_PID}) not found on USB." >&2
	exit 1
fi

# usbreset accepts a VVVV:PPPP vendor:product id.
echo "resetting ST-LINK ${ST_VID}:${STLINK_V3_PID} ..."
sudo usbreset "${ST_VID}:${STLINK_V3_PID}"
echo "done - the LED should be red again (idle)."
