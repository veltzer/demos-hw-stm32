#!/bin/bash -eu

CLI="/opt/st/stm32cubeide_1.18.1/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.100.202412061334/tools/bin/STM32_Programmer_CLI"

APP="/home/mark/STM32CubeIDE/workspace_1.18.1/GPIO_IOToggle/STM32CubeIDE/Release/GPIO_IOToggle.elf"

"${CLI}" -c port=SWD mode=UR -d "${APPM4}" --start
"${CLI}" -c port=SWD mode=UR -d "${APPM0}" --start
# core=m4

# another option is:
# st-flash write your_firmware.bin 0x08000000
