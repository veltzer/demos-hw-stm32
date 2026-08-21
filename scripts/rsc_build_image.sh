#!/bin/bash -eu
# Build STM32WL55 firmware images for rsconstruct.
#
# Invoked by the [processor.explicit.fw_*] stanzas in rsconstruct.local.toml as:
#   rsc_build_image.sh --inputs <files...> --output-files <files...>
#
# Every --inputs entry whose basename matches main*.c is built into three
# artifacts beside it -- app_<tag>.elf, app_<tag>.map, firmware_<tag>.bin --
# where <tag> is the main file's name minus the main_ prefix and .c suffix
# (bare, hal, cm4_bare, cm0p_hal, ...). The target core (M4 vs M0+) and the
# variant (bare vs HAL) are derived from that same name, exactly as the root
# Makefile's IMAGE template classifies images. All other inputs (the common
# startup/system/linker files, the HAL libraries, this script itself) are
# rebuild triggers only; their paths are known here.
#
# Flags mirror the Makefile. Each image is one compile+link invocation of its
# three small translation units, so no intermediate objects are kept.

CUBEWL="${CUBEWL:-STM32CubeWL}"
COMMON="exercises/common"
CMSIS_INC="$CUBEWL/Drivers/CMSIS"
HAL_DRV="$CUBEWL/Drivers/STM32WLxx_HAL_Driver"
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

if [ ! -f "$HAL_DRV/Src/stm32wlxx_hal_ipcc.c" ]; then
	echo "STM32CubeWL not found at '$CUBEWL'. No exercises can build." >&2
	echo "Run: scripts/clone_cubewl.sh   (or set CUBEWL=/path/to/STM32CubeWL)" >&2
	exit 1
fi

# Benign linker noise to hide (newlib-nano unimplemented-syscall warnings and
# the RWX-segment note), copied from the Makefile's LD_NOISE. Real errors do
# not match these and still show.
LD_NOISE='is not implemented and will always fail|does not take linker garbage collection into account|libc_nano\.a|in function .(_close_r|_lseek_r|_read_r|_write_r|_sbrk_r|_fstat_r|_isatty_r|_getpid_r|_kill_r).|has a LOAD segment with RWX permissions|warn-rwx-segments'

# Collect the main*.c files from --inputs; everything else there is a trigger.
mains=()
section=""
for arg in "$@"; do
	case "$arg" in
	--inputs) section=inputs ;;
	--output-files | --output-dirs) section=outputs ;;
	*)
		if [ "$section" = inputs ]; then
			case "$(basename "$arg")" in
			main*.c) mains+=("$arg") ;;
			esac
		fi
		;;
	esac
done

if [ "${#mains[@]}" -eq 0 ]; then
	echo "$0: no main*.c files among --inputs" >&2
	exit 1
fi

for main in "${mains[@]}"; do
	dir="$(dirname "$main")"
	tag="$(basename "$main")"
	tag="${tag#main_}"
	tag="${tag%.c}"

	# Core: M0+ images carry cm0p in the name; everything else is M4.
	if [[ "$tag" == *cm0p* ]]; then
		cpu=(-mcpu=cortex-m0plus -mthumb -mfloat-abi=soft)
		defs=(-DSTM32WL55xx -DCORE_CM0PLUS)
		startup="$COMMON/startup_stm32wl55jcix_cm0plus.s"
		ld_name="STM32WL55JCIX_FLASH_CM0PLUS.ld"
		hal_lib="$COMMON/libhal_rsc_cm0.a"
	else
		cpu=(-mcpu=cortex-m4 -mthumb -mfloat-abi=soft)
		defs=(-DSTM32WL55xx -DCORE_CM4)
		startup="$COMMON/startup_stm32wl55jcix.s"
		ld_name="STM32WL55JCIX_FLASH_M4.ld"
		hal_lib="$COMMON/libhal_rsc_cm4.a"
	fi

	# Per-exercise linker script override, else the common default (the
	# Makefile's ld-m4 / ld-cm0 helpers).
	ld="$dir/$ld_name"
	if [ ! -f "$ld" ]; then
		ld="$COMMON/$ld_name"
	fi

	includes=(-I"$CMSIS_INC/Include" -I"$CMSIS_INC/Device/ST/STM32WLxx/Include")
	libs=()
	if [[ "$tag" == *hal* ]]; then
		defs+=(-DUSE_HAL_DRIVER)
		includes+=(-I"$HAL_DRV/Inc" -I"$HAL_DRV/Inc/Legacy" -I"$COMMON")
		libs=("$hal_lib")
	fi

	elf="$dir/app_$tag.elf"
	map="$dir/app_$tag.map"
	bin="$dir/firmware_$tag.bin"
	rm -f "$elf" "$map" "$bin"

	echo "  IMG   $elf"
	"$CC" "${cpu[@]}" "${defs[@]}" "${includes[@]}" \
		-Wall -Wextra -O2 -g3 -ffunction-sections -fdata-sections \
		-Wl,--gc-sections --specs=nano.specs --specs=nosys.specs \
		-T"$ld" -Wl,-Map="$map" \
		-o "$elf" "$main" "$COMMON/system_stm32wlxx.c" "$startup" \
		"${libs[@]}" 2>&1 | grep -vE "$LD_NOISE" || true
	if [ ! -f "$elf" ]; then
		echo "$0: build failed for $main" >&2
		exit 1
	fi
	"$OBJCOPY" -O binary "$elf" "$bin"
done
