#!/bin/bash -eu
# reload udev rules and re-apply them to already-connected USB devices.
# use this after installing stlink-tools while the board was already plugged in,
# instead of having to unplug and replug it.
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add --subsystem-match=usb
