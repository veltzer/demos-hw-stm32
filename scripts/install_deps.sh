#!/bin/bash -eu
# install all Ubuntu dependencies needed to work with the STM32 board.
sudo apt update
sudo apt install -y stlink-tools minicom tio
