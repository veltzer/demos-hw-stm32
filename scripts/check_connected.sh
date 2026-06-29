#!/bin/bash -eu
#
# Check that the STM32 board (NUCLEO-WL55JC1 / ST-LINK-V3) is connected properly.
#
# Two levels of checking:
#   Level 1 - USB enumeration: does the USB hub even see the ST-LINK probe?
#             This only proves the on-board debug probe powered up and enumerated.
#   Level 2 - target reachable: can the probe actually talk to the MCU over SWD?
#             This proves wiring/power/SWD are fine and identifies the chip.
#             (lsusb alone CANNOT tell you this - you must talk to the target.)
#
# ST USB vendor id is 0483; the ST-LINK-V3 product id is 374e.

ST_VID="0483"
STLINK_V3_PID="374e"

source "$(dirname "$0")/cubeide.sh"

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
info()  { printf '\033[36m%s\033[0m\n' "$*"; }

# ---------------------------------------------------------------------------
# Level 1: USB enumeration
# ---------------------------------------------------------------------------
info "== Level 1: USB enumeration (lsusb) =="
usb_line=$(lsusb | grep -i "${ST_VID}:" || true)
if [ -z "${usb_line}" ]; then
	red "FAIL: no STMicroelectronics (VID ${ST_VID}) device found on USB."
	red "      The board is not plugged in, the cable is power-only, or the probe is dead."
	exit 1
fi
green "OK: ST device present on USB:"
echo "    ${usb_line}"
if echo "${usb_line}" | grep -qi "${ST_VID}:${STLINK_V3_PID}"; then
	green "    (recognised as ST-LINK-V3)"
fi

# ---------------------------------------------------------------------------
# Level 2: can we actually talk to the target MCU?
# ---------------------------------------------------------------------------
echo
info "== Level 2: target MCU reachable over SWD =="

if command -v st-info >/dev/null 2>&1; then
	# stlink-tools path (apt install stlink-tools)
	probe_out=$(st-info --probe 2>&1 || true)
	echo "${probe_out}"
	# "Found 0 stlink programmers" => probe not usable; otherwise it prints chipid/flash.
	if echo "${probe_out}" | grep -q "Found 0 stlink"; then
		red "FAIL: st-info found 0 programmers."
		red "      Usually a USB permission problem: the udev rule exists but was not"
		red "      applied to an already-plugged-in board (e.g. stlink-tools was"
		red "      installed after the board was connected). Fix it with either:"
		red "        - unplug and replug the board, or"
		red "        - sudo udevadm control --reload-rules && \\"
		red "          sudo udevadm trigger --action=add --subsystem-match=usb"
		exit 1
	fi
	if echo "${probe_out}" | grep -qiE "chipid:\s*0x0000|unknown"; then
		red "FAIL: probe is up but cannot read the target MCU (check power / SWD)."
		exit 1
	fi
	green "OK: ST-LINK can read the target MCU."
elif [ -x "${CLI}" ]; then
	# Fall back to ST's official programmer CLI (already installed with CubeIDE).
	info "st-info not installed; using STM32_Programmer_CLI instead."
	if "${CLI}" -c port=SWD; then
		green "OK: STM32_Programmer_CLI connected to the target over SWD."
	else
		red "FAIL: STM32_Programmer_CLI could not connect over SWD."
		exit 1
	fi
else
	red "SKIP: deep check unavailable."
	red "      Install stlink-tools:  sudo apt install stlink-tools"
	red "      (or make sure STM32CubeIDE's STM32_Programmer_CLI exists at the path in this script)"
	exit 2
fi

echo
green "All checks passed: the board is connected properly."
