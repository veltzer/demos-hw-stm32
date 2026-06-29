#!/bin/bash -eu

# reset everything

source "$(dirname "$0")/cubeide.sh"

# "${CLI}" -c port=SWD mode=UR -ob RDP=0
"${CLI}" -c port=SWD -rst
