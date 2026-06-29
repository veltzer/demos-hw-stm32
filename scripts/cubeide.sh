#!/bin/bash
# Single source of truth for the STM32CubeIDE install location.
#
# Source this from other scripts:   source "$(dirname "$0")/cubeide.sh"
# It exports:
#   CUBEIDE_DIR  - the install dir  (/opt/st/stm32cubeide_<version>)
#   CLI          - full path to STM32_Programmer_CLI
#   IDE          - full path to the stm32cubeide launcher
#
# THE VERSION IS DEFINED IN EXACTLY ONE PLACE: the CUBEIDE_VERSION below
# (override at call time with the CUBEIDE_VERSION env var). The cubeprogrammer
# plugin has its own independent version, so we DISCOVER that dir by glob rather
# than hardcode it.

# The one knob. Bump this (or export CUBEIDE_VERSION) when you upgrade CubeIDE.
: "${CUBEIDE_VERSION:=2.1.1}"

CUBEIDE_DIR="/opt/st/stm32cubeide_${CUBEIDE_VERSION}"

# Discover the cubeprogrammer plugin dir (its version is independent of the IDE).
_cubeprog_glob=("${CUBEIDE_DIR}"/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_*/tools/bin/STM32_Programmer_CLI)
CLI="${_cubeprog_glob[0]}"

# The IDE launcher (wayland build; falls back to the plain launcher if present).
if [ -x "${CUBEIDE_DIR}/stm32cubeide_wayland" ]; then
	IDE="${CUBEIDE_DIR}/stm32cubeide_wayland"
else
	IDE="${CUBEIDE_DIR}/stm32cubeide"
fi

export CUBEIDE_DIR CLI IDE
