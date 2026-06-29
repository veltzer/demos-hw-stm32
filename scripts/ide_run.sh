#!/bin/bash -eu

source "$(dirname "$0")/cubeide.sh"

"${IDE}" 2> /dev/null > /dev/null &
