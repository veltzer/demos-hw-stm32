#!/bin/bash -eu

source "$(dirname "$0")/cubeide.sh"

"${CLI}" -c port=SWD
