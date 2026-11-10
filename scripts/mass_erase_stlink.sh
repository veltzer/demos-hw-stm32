#!/bin/bash -eu
# Mass-erase the board with the open-source stlink tools (st-flash), for a
# machine without STM32CubeIDE. scripts/mass_erase.sh does the same through
# CubeIDE's STM32_Programmer_CLI, connecting under reset.

st-flash erase
