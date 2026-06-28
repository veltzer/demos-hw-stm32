#!/bin/bash -eu
# install all Ubuntu dependencies needed to work with the STM32 board.
sudo apt update
sudo apt install -y usbutils stlink-tools minicom tio \
	gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
