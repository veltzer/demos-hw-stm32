#!/bin/bash -eu

source "$(dirname "$0")/cubeide.sh"

# mode=UR (connect under reset): running firmware can hold the SWD lines so a
# plain connect fails with "Unable to get core ID"; connecting under reset fixes it.
"${CLI}" -c port=SWD mode=UR -e all
