#!/bin/bash -eu
# install all Ubuntu dependencies needed to work with the STM32 board.
#
# Note: STM32_Programmer_CLI (used by a few scripts) is NOT an apt package - it
# ships with the proprietary STM32CubeIDE install and is not handled here.
sudo apt update
sudo apt install -y \
	usbutils \
	stlink-tools \
	minicom \
	tio \
	make \
	whiptail \
	dialog \
	pass \
	gcc-arm-none-eabi \
	binutils-arm-none-eabi \
	libnewlib-arm-none-eabi
