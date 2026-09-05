#!/bin/bash -eu

# shellcheck source=scripts/cubeide.sh disable=SC1091
source "$(dirname "$0")/cubeide.sh"

"${IDE}" 2> /dev/null > /dev/null &
