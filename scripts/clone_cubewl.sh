#!/bin/bash -eu
# Clone the STM32CubeWL firmware package into the repo root as STM32CubeWL/.
#
# This provides the full STM32WL HAL/LL/CMSIS sources (including drivers the
# CubeIDE blink project does not generate, e.g. IPCC and HSEM) that the HAL
# exercises build against. It is gitignored -- vendor source, not committed.
#
# The HAL driver and CMSIS device are git SUBMODULES of STM32CubeWL, so a plain
# shallow clone leaves them empty; this script fetches exactly those submodules.
# Idempotent: re-running just ensures the submodules are present.

HERE="$(dirname "$0")/.."
DEST="$HERE/STM32CubeWL"
REPO="https://github.com/STMicroelectronics/STM32CubeWL.git"

# The submodules the build actually needs (HAL driver + CMSIS device headers).
SUBMODULES=(
	"Drivers/STM32WLxx_HAL_Driver"
	"Drivers/CMSIS"
)

if [ ! -d "$DEST/.git" ]; then
	echo "cloning STM32CubeWL into $DEST ..."
	git clone --depth 1 "$REPO" "$DEST"
else
	echo "STM32CubeWL already present at $DEST"
fi

echo "fetching required submodules (HAL driver, CMSIS) ..."
git -C "$DEST" submodule update --init --depth 1 "${SUBMODULES[@]}"

# Sanity check: the IPCC driver is the marker that the full set is in place.
if [ -f "$DEST/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_ipcc.c" ]; then
	echo "ok: STM32CubeWL ready ($(ls "$DEST"/Drivers/STM32WLxx_HAL_Driver/Src/*.c | wc -l) HAL sources)"
else
	echo "error: HAL driver sources missing after clone -- submodule fetch failed?" >&2
	exit 1
fi
