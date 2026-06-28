#!/bin/bash -eu
# build every discoverable single-core exercise.
# -k keeps going so one broken exercise doesn't stop the rest; make's exit code
# is non-zero if any exercise failed to build.
ROOT="$(dirname "$0")/.."
exec make -C "$ROOT" -k
