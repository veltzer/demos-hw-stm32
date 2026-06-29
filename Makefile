# Build system for the STM32WL55 single-core exercises.
#
# Usage:
#   make                       # build every single-core exercise (incremental)
#   make 04_uart               # build just one exercise
#   make list                  # list the exercises that will be built
#   make clean
#   make V=1 ...               # verbose: echo the underlying commands
#   make -j                    # exercises build in parallel
#
# This Makefile lives at the repo root and operates on exercises/.
# The set of buildable exercises is DISCOVERED from the filesystem, not listed:
# any exercises/NN_name/ that has a main.c and no main_cm4.c is a
# single-core exercise and is built. Each is linked against the shared CMSIS /
# startup / linker support under exercises/common/. Builds are incremental
# (per-object compilation with -MMD header dependency tracking).
#
# Excluded automatically: dual-core exercises (they carry a main_cm4.c and need
# a from-scratch Cortex-M0+ startup/linker/option-bytes) and sourceless ones.

# ---- verbosity -------------------------------------------------------------
# Quiet by default; `make V=1` shows the actual commands.
V ?= 0
ifeq ($(V),0)
Q := @
else
Q :=
endif

# ---- toolchain -------------------------------------------------------------
PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

# ---- board / cpu -----------------------------------------------------------
# STM32WL55 Cortex-M4 has NO FPU (__FPU_PRESENT == 0): soft float, no -mfpu.
CPU     := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
DEFS    := -DSTM32WL55xx -DCORE_CM4

# ---- paths -----------------------------------------------------------------
# This Makefile lives at the repo root; the exercises live under exercises/.
DIR     := exercises
COMMON  := $(DIR)/common
LDSCRIPT := $(COMMON)/STM32WL55JCIX_FLASH.ld
STARTUP  := $(COMMON)/startup_stm32wl55jcix.s
SYSTEM   := $(COMMON)/system_stm32wlxx.c

INCLUDES := \
	-I$(COMMON)/CMSIS/Include \
	-I$(COMMON)/CMSIS/Device/ST/STM32WLxx/Include

# ---- flags -----------------------------------------------------------------
# -MMD -MP emit per-object header-dependency files for incremental rebuilds.
CFLAGS  := $(CPU) $(DEFS) $(INCLUDES) -Wall -Wextra -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
ASFLAGS := $(CPU) -MMD -MP
LDFLAGS := $(CPU) -T$(LDSCRIPT) -Wl,--gc-sections \
	--specs=nano.specs --specs=nosys.specs

# Linker lines that are benign on bare metal and should be hidden: the
# newlib-nano unimplemented-syscall warnings, their context/note lines, and the
# RWX-segment note. Real errors don't match these and still show.
LD_NOISE := is not implemented and will always fail|\
does not take linker garbage collection into account|\
libc_nano\.a|in function .(_close_r|_lseek_r|_read_r|_write_r|_sbrk_r|_fstat_r|_isatty_r|_getpid_r|_kill_r).|\
has a LOAD segment with RWX permissions|warn-rwx-segments

# ---- discovery -------------------------------------------------------------
# Candidate exercises: every exercises/NN_name dir holding a main.c.
candidates := $(sort $(patsubst %/,%,$(dir \
	$(wildcard $(DIR)/[0-9]*_*/main.c))))
# Drop dual-core exercises (those also carry a main_cm4.c).
dualcore   := $(patsubst %/,%,$(dir $(wildcard $(DIR)/[0-9]*_*/main_cm4.c)))
# Full paths (exercises/NN_name) used by the build rules ...
EXERCISES  := $(filter-out $(dualcore),$(candidates))
# ... and their bare names (NN_name) used as convenience targets.
NAMES      := $(notdir $(EXERCISES))

# Per-exercise source.
app-src = $1/main.c

BINS := $(foreach e,$(EXERCISES),$(e)/firmware.bin)

.PHONY: all list clean
all: $(BINS)

# ---- per-exercise rules (generated, one set per discovered exercise) --------
# $1 = exercise dir. Objects live in $1/.obj; outputs are $1/{app.elf,firmware.bin}.
define EXERCISE_rules
$1/.obj/app.o: $(call app-src,$1) | $1/.obj
	@echo "  CC    $$<"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/system.o: $$(SYSTEM) | $1/.obj
	@echo "  CC    $$<"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/startup.o: $$(STARTUP) | $1/.obj
	@echo "  AS    $$<"
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<

$1/app.elf: $1/.obj/app.o $1/.obj/system.o $1/.obj/startup.o $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app.map -o $$@ $$(filter %.o,$$^) 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@

$1/firmware.bin: $1/app.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	$$(Q)$$(SIZE) $$<
	@echo "  built $1"

$1/.obj:
	$$(Q)mkdir -p $$@

-include $1/.obj/app.d $1/.obj/system.d $1/.obj/startup.d
endef
$(foreach e,$(EXERCISES),$(eval $(call EXERCISE_rules,$e)))

# Convenience targets so you can build by bare name (e.g. `make 04_uart`) or by
# full path (`make exercises/04_uart`); both map to that exercise's binary.
$(NAMES): %: $(DIR)/%/firmware.bin
$(EXERCISES): %: %/firmware.bin
.PHONY: $(NAMES) $(EXERCISES)

list:
	@for e in $(NAMES); do echo "  $$e"; done

clean:
	$(Q)rm -rf $(DIR)/*/.obj
	$(Q)rm -f $(DIR)/*/app.elf $(DIR)/*/firmware.bin $(DIR)/*/app.map
	@echo "cleaned"
