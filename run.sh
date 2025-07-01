#!/bin/bash -eu

CLI="/opt/st/stm32cubeide_1.18.1/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.100.202412061334/tools/bin/STM32_Programmer_CLI"

"${CLI}" -c port=SWD mode=UR -d "blink2/CM4/Release/blink_CM4.elf" --start
