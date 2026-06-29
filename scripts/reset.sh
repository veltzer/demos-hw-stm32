#!/bin/bash -eu

# reset everything

source "$(dirname "$0")/cubeide.sh"

# mode=UR (connect under reset): the running firmware can hold the SWD lines so
# a plain connect fails with "Unable to get core ID". Connecting under reset
# fixes it. --start 0x08000000 releases the chip to actually run after the reset
# (a bare -rst under reset leaves it halted -> "Unable to run MCU").
# "${CLI}" -c port=SWD mode=UR -ob RDP=0
"${CLI}" -c port=SWD mode=UR -rst --start 0x08000000
