#!/bin/bash -eu
# Build the shared STM32WL HAL driver static library for one core, for
# rsconstruct. Invoked by the [processor.explicit.hal_lib_*] stanzas in
# rsconstruct.local.toml as:
#   rsc_build_hal_lib.sh <cm4|cm0> --inputs <triggers...> --output-files <lib>
#
# Mirrors the Makefile's HAL library rules: every non-template STM32CubeWL HAL
# driver source is compiled (warnings off -- vendor code is not the lesson)
# and archived into the library named by --output-files. Objects go to
# exercises/common/.rsc_hal_obj_<core>/, separate from the Makefile's
# .hal_obj_* dirs so the two build systems never share intermediates. An
# object is reused when newer than both its source and this script (the
# compile flags live here), so re-runs only pay for what changed.

core="${1:-}"
if [ "$core" != cm4 ] && [ "$core" != cm0 ]; then
	echo "usage: $0 <cm4|cm0> --inputs <files...> --output-files <lib>" >&2
	exit 1
fi
shift

CUBEWL="${CUBEWL:-STM32CubeWL}"
COMMON="exercises/common"
CMSIS_INC="$CUBEWL/Drivers/CMSIS"
HAL_DRV="$CUBEWL/Drivers/STM32WLxx_HAL_Driver"
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar

if [ ! -f "$HAL_DRV/Src/stm32wlxx_hal_ipcc.c" ]; then
	echo "STM32CubeWL not found at '$CUBEWL'. The HAL library cannot build." >&2
	echo "Run: scripts/clone_cubewl.sh   (or set CUBEWL=/path/to/STM32CubeWL)" >&2
	exit 1
fi

# The library path is the first --output-files entry.
lib=""
section=""
for arg in "$@"; do
	case "$arg" in
	--inputs) section=inputs ;;
	--output-files | --output-dirs) section=outputs ;;
	*)
		if [ "$section" = outputs ] && [ -z "$lib" ]; then
			lib="$arg"
		fi
		;;
	esac
done
if [ -z "$lib" ]; then
	echo "$0: no --output-files given" >&2
	exit 1
fi

if [ "$core" = cm4 ]; then
	cpu=(-mcpu=cortex-m4 -mthumb -mfloat-abi=soft)
	defs=(-DSTM32WL55xx -DCORE_CM4 -DUSE_HAL_DRIVER)
else
	cpu=(-mcpu=cortex-m0plus -mthumb -mfloat-abi=soft)
	defs=(-DSTM32WL55xx -DCORE_CM0PLUS -DUSE_HAL_DRIVER)
fi
cflags=("${cpu[@]}" "${defs[@]}"
	-I"$CMSIS_INC/Include" -I"$CMSIS_INC/Device/ST/STM32WLxx/Include"
	-I"$HAL_DRV/Inc" -I"$HAL_DRV/Inc/Legacy" -I"$COMMON"
	-w -O2 -g3 -ffunction-sections -fdata-sections)

objdir="$COMMON/.rsc_hal_obj_$core"
mkdir -p "$objdir"

# Compile in parallel waves of $(nproc); waiting on each recorded pid keeps
# `bash -e` failure propagation that a bare `wait` would swallow.
maxjobs="$(nproc)"
pids=()
flush() {
	local pid
	for pid in "${pids[@]}"; do
		wait "$pid"
	done
	pids=()
}

compiled=0
for src in "$HAL_DRV/Src"/*.c; do
	case "$src" in
	*_template.c) continue ;;
	esac
	obj="$objdir/$(basename "${src%.c}").o"
	if [ "$obj" -nt "$src" ] && [ "$obj" -nt "$0" ]; then
		continue
	fi
	echo "  CC    $src [$core] (hal-lib)"
	"$CC" "${cflags[@]}" -c -o "$obj" "$src" &
	pids+=("$!")
	compiled=1
	if [ "${#pids[@]}" -ge "$maxjobs" ]; then
		flush
	fi
done
flush

if [ "$compiled" -eq 1 ] || [ ! -f "$lib" ]; then
	echo "  AR    $lib"
	rm -f "$lib"
	"$AR" rcs "$lib" "$objdir"/*.o
fi
