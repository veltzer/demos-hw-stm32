# Build system for the STM32WL55 single-core exercises.
#
# Usage:
#   make                       # build every single-core exercise (incremental)
#   make 02_serial_counter     # build just one exercise
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

# ---- M0+ (CPU2) support, for the dual-core exercises ----------------------
# The M0+ has no FPU either; it links into flash bank 2 / SRAM2 (see the script)
# and uses the M0+ startup with the smaller M0+ vector table.
CPU_CM0   := -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft
DEFS_CM0  := -DSTM32WL55xx -DCORE_CM0PLUS
LDSCRIPT_CM0 := $(COMMON)/STM32WL55JCIX_FLASH_CM0PLUS.ld
STARTUP_CM0  := $(COMMON)/startup_stm32wl55jcix_cm0plus.s

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

# M0+ variants of the above (different cpu, defines, and linker script).
CFLAGS_CM0  := $(CPU_CM0) $(DEFS_CM0) $(INCLUDES) -Wall -Wextra -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
ASFLAGS_CM0 := $(CPU_CM0) -MMD -MP
LDFLAGS_CM0 := $(CPU_CM0) -T$(LDSCRIPT_CM0) -Wl,--gc-sections \
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
# Dual-core exercises carry a main_cm4.c (and a main_cm0p.c). They are NOT in
# the single-core list (which builds one main.c against the M4 support); they
# get their own rules that build TWO images, one per core.
dualcore   := $(sort $(patsubst %/,%,$(dir $(wildcard $(DIR)/[0-9]*_*/main_cm4.c))))
# Single-core full paths (exercises/NN_name) used by the build rules ...
EXERCISES  := $(filter-out $(dualcore),$(candidates))
# ... and their bare names (NN_name) used as convenience targets.
NAMES      := $(notdir $(EXERCISES))
DUALNAMES  := $(notdir $(dualcore))

# Per-exercise source.
app-src = $1/main.c

BINS := $(foreach e,$(EXERCISES),$(e)/firmware.bin)
# Each dual-core exercise produces two images: one for the M4, one for the M0+.
DUALBINS := $(foreach e,$(dualcore),$(e)/firmware_cm4.bin $(e)/firmware_cm0p.bin)

.PHONY: all list clean
all: $(BINS) $(DUALBINS)

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

# ---- per-dual-core-exercise rules ------------------------------------------
# $1 = exercise dir. Builds two independent images that DON'T overlap in flash:
#   M4  image: main_cm4.c  + M4 startup/linker  -> app_cm4.elf  / firmware_cm4.bin
#   M0+ image: main_cm0p.c + M0+ startup/linker -> app_cm0p.elf / firmware_cm0p.bin
# Objects live in $1/.obj with _cm4 / _cm0p suffixes so the two cores' build
# artifacts never collide. (Flashing both + setting C2BOOT option bytes so the
# M0+ actually runs is a separate, deliberate step -- see CLAUDE.md.)
define DUALCORE_rules
# --- M4 (CPU1) ---
$1/.obj/app_cm4.o: $1/main_cm4.c | $1/.obj
	@echo "  CC    $$< (cm4)"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/system_cm4.o: $$(SYSTEM) | $1/.obj
	@echo "  CC    $$< (cm4)"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/startup_cm4.o: $$(STARTUP) | $1/.obj
	@echo "  AS    $$< (cm4)"
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<

$1/app_cm4.elf: $1/.obj/app_cm4.o $1/.obj/system_cm4.o $1/.obj/startup_cm4.o $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app_cm4.map -o $$@ $$(filter %.o,$$^) 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@

$1/firmware_cm4.bin: $1/app_cm4.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	$$(Q)$$(SIZE) $$<

# --- M0+ (CPU2) ---
$1/.obj/app_cm0p.o: $1/main_cm0p.c | $1/.obj
	@echo "  CC    $$< (cm0+)"
	$$(Q)$$(CC) $$(CFLAGS_CM0) -c -o $$@ $$<

$1/.obj/system_cm0p.o: $$(SYSTEM) | $1/.obj
	@echo "  CC    $$< (cm0+)"
	$$(Q)$$(CC) $$(CFLAGS_CM0) -c -o $$@ $$<

$1/.obj/startup_cm0p.o: $$(STARTUP_CM0) | $1/.obj
	@echo "  AS    $$< (cm0+)"
	$$(Q)$$(CC) $$(ASFLAGS_CM0) -c -o $$@ $$<

$1/app_cm0p.elf: $1/.obj/app_cm0p.o $1/.obj/system_cm0p.o $1/.obj/startup_cm0p.o $$(LDSCRIPT_CM0)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS_CM0) -Wl,-Map=$1/app_cm0p.map -o $$@ $$(filter %.o,$$^) 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@

$1/firmware_cm0p.bin: $1/app_cm0p.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	$$(Q)$$(SIZE) $$<
	@echo "  built $1 (both cores)"

$1/.obj:
	$$(Q)mkdir -p $$@

-include $1/.obj/app_cm4.d $1/.obj/system_cm4.d $1/.obj/startup_cm4.d
-include $1/.obj/app_cm0p.d $1/.obj/system_cm0p.d $1/.obj/startup_cm0p.d
endef
$(foreach e,$(dualcore),$(eval $(call DUALCORE_rules,$e)))

# Convenience targets so you can build by bare name (e.g. `make 02_serial_counter`)
# or by full path (`make exercises/02_serial_counter`); both map to its binary.
$(NAMES): %: $(DIR)/%/firmware.bin
$(EXERCISES): %: %/firmware.bin
# Dual-core bare-name / full-path targets build BOTH images.
$(DUALNAMES): %: $(DIR)/%/firmware_cm4.bin $(DIR)/%/firmware_cm0p.bin
$(dualcore): %: %/firmware_cm4.bin %/firmware_cm0p.bin
.PHONY: $(NAMES) $(EXERCISES) $(DUALNAMES) $(dualcore)

list:
	@for e in $(NAMES); do echo "  $$e"; done
	@for e in $(DUALNAMES); do echo "  $$e (dual-core: M4 + M0+)"; done

clean:
	$(Q)rm -rf $(DIR)/*/.obj
	$(Q)rm -f $(DIR)/*/app.elf $(DIR)/*/firmware.bin $(DIR)/*/app.map
	$(Q)rm -f $(DIR)/*/app_cm4.elf $(DIR)/*/firmware_cm4.bin $(DIR)/*/app_cm4.map
	$(Q)rm -f $(DIR)/*/app_cm0p.elf $(DIR)/*/firmware_cm0p.bin $(DIR)/*/app_cm0p.map
	@echo "cleaned"
