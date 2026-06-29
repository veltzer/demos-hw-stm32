# Build system for the STM32WL55 exercises.
#
# Usage:
#   make                       # build every exercise (incremental)
#   make 02_serial_counter     # build just one exercise (by bare name)
#   make list                  # list the exercises that will be built
#   make clean
#   make V=1 ...               # verbose: echo the underlying commands
#   make -j                    # exercises build in parallel
#
# This Makefile lives at the repo root and operates on two exercise trees:
#   exercises/singlecore/NN_name/  - one main.c, built into one M4 image
#   exercises/dualcore/NN_name/    - main_cm4.c + main_cm0p.c, built into two
#                                    images (one per core, non-overlapping flash)
# Both link against the shared CMSIS / startup / linker support under
# exercises/common/. The directory tree IS the single-vs-dual classification.
# Builds are incremental (per-object compilation, -MMD header deps).
#
# Sourceless dirs (e.g. an exercise that is just a write-up) are ignored, since
# discovery keys off the presence of main.c / main_cm4.c.

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
# This Makefile lives at the repo root. Exercises live in two trees:
#   exercises/single_core/NN_name/ - main_bare.c (register-level) AND main_hal.c
#                                    (same lesson via the HAL); each builds its
#                                    own M4 image (firmware_bare.bin / _hal.bin).
#   exercises/dual_core/NN_name/   - main_cm4.c + main_cm0p.c, one image per core.
# Our customized startup/linker/system live in exercises/common/; the vendor
# CMSIS and HAL come from the STM32CubeWL clone (gitignored, see CUBEWL below).
DIR     := exercises
SINGLEDIR := $(DIR)/single_core
DUALDIR   := $(DIR)/dual_core
COMMON  := $(DIR)/common
# STM32CubeWL package (cloned by scripts/clone_cubewl.sh). Override to relocate.
CUBEWL  ?= STM32CubeWL
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

# CMSIS (ARM core + ST device headers) comes from the STM32CubeWL clone -- it is
# pure vendor source, identical in kind to what we used to vendor, so it is not
# duplicated in-repo. (Our customized startup/linker/system DO stay in common/.)
CMSIS_INC := $(CUBEWL)/Drivers/CMSIS
INCLUDES := \
	-I$(CMSIS_INC)/Include \
	-I$(CMSIS_INC)/Device/ST/STM32WLxx/Include

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

# ---- HAL support (for the single_core_hal exercises) -----------------------
# The full STM32WL HAL/LL/CMSIS comes from the STM32CubeWL package cloned into
# the repo root by scripts/clone_cubewl.sh (gitignored -- NOT vendored in-repo,
# so no duplication). Only our project config (common/stm32wlxx_hal_conf.h) lives
# in the repo. (Each main_hal.c defines its own SysTick_Handler.) CUBEWL is
# defined near the top with the CMSIS includes.
HAL_DRV    := $(CUBEWL)/Drivers/STM32WLxx_HAL_Driver
HAL_INCLUDES := $(INCLUDES) \
	-I$(HAL_DRV)/Inc \
	-I$(HAL_DRV)/Inc/Legacy \
	-I$(COMMON)
# HAL needs USE_HAL_DRIVER; the M4 cpu/defines are otherwise the same.
CFLAGS_HAL := $(CPU) $(DEFS) -DUSE_HAL_DRIVER $(HAL_INCLUDES) -Wall -Wextra -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
# The HAL driver sources are warning-noisy under -Wextra and are not the
# lesson, so compile them without -Wall -Wextra (keep -w quiet).
CFLAGS_HALDRV := $(CPU) $(DEFS) -DUSE_HAL_DRIVER $(HAL_INCLUDES) -w -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
# The HAL driver set is compiled ONCE into a shared static library and every HAL
# app links against it (--gc-sections then drops the unused parts per app). The
# library + objects build under common/ (NOT inside the gitignored clone).
# Exclude *_template.c: those are copy-me starting points, not buildable drivers.
HAL_DRV_SRCS := $(filter-out %_template.c,$(wildcard $(HAL_DRV)/Src/*.c))
HAL_LIBDIR   := $(COMMON)/.hal_obj_cm4
HAL_LIBOBJS  := $(patsubst $(HAL_DRV)/Src/%.c,$(HAL_LIBDIR)/%.o,$(HAL_DRV_SRCS))
HAL_LIB      := $(COMMON)/libhal_cm4.a
AR           := $(PREFIX)ar

# Same HAL, compiled for the M0+ (cortex-m0plus): the cores have different
# instruction sets, so the M4 library cannot be reused -- the dual-core HAL
# exercises link this separate library on the M0+ side.
CFLAGS_HALDRV_CM0 := $(CPU_CM0) $(DEFS_CM0) -DUSE_HAL_DRIVER $(HAL_INCLUDES) -w -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
CFLAGS_HAL_CM0    := $(CPU_CM0) $(DEFS_CM0) -DUSE_HAL_DRIVER $(HAL_INCLUDES) -Wall -Wextra -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
HAL_LIBDIR_CM0 := $(COMMON)/.hal_obj_cm0
HAL_LIBOBJS_CM0 := $(patsubst $(HAL_DRV)/Src/%.c,$(HAL_LIBDIR_CM0)/%.o,$(HAL_DRV_SRCS))
HAL_LIB_CM0    := $(COMMON)/libhal_cm0.a

# Warn early with a clear message if the STM32CubeWL clone is missing. Both the
# HAL (drivers) and the bare-metal exercises (CMSIS headers) need it now.
ifeq ($(wildcard $(HAL_DRV)/Src/stm32wlxx_hal_ipcc.c),)
$(warning STM32CubeWL not found at '$(CUBEWL)'. No exercises will build.)
$(warning Run: scripts/clone_cubewl.sh   (or set CUBEWL=/path/to/STM32CubeWL))
endif

# Linker lines that are benign on bare metal and should be hidden: the
# newlib-nano unimplemented-syscall warnings, their context/note lines, and the
# RWX-segment note. Real errors don't match these and still show.
LD_NOISE := is not implemented and will always fail|\
does not take linker garbage collection into account|\
libc_nano\.a|in function .(_close_r|_lseek_r|_read_r|_write_r|_sbrk_r|_fstat_r|_isatty_r|_getpid_r|_kill_r).|\
has a LOAD segment with RWX permissions|warn-rwx-segments

# ---- discovery -------------------------------------------------------------
# The directory tree IS the classification. A single_core/ exercise carries BOTH
# main_bare.c and main_hal.c and builds two M4 images. A dual_core/ exercise
# carries per-core sources main_cm4_bare.c / main_cm0p_bare.c (and optionally the
# _hal variants) and builds one image per core per available variant. Discovery
# keys off main_bare.c / main_cm4_bare.c, so a sourceless dir (the RTOS stub) is
# ignored automatically.
EXERCISES  := $(sort $(patsubst %/,%,$(dir $(wildcard $(SINGLEDIR)/[0-9]*_*/main_bare.c))))
dualcore   := $(sort $(patsubst %/,%,$(dir $(wildcard $(DUALDIR)/[0-9]*_*/main_cm4_bare.c))))
NAMES      := $(notdir $(EXERCISES))
DUALNAMES  := $(notdir $(dualcore))

# Each single-core exercise produces two images: bare-metal and HAL.
BINS := $(foreach e,$(EXERCISES),$(e)/firmware_bare.bin $(e)/firmware_hal.bin)
# Each dual-core exercise always produces the two _bare core images; it also
# produces the two _hal core images IF the corresponding _hal sources exist.
dual-bins = $1/firmware_cm4_bare.bin $1/firmware_cm0p_bare.bin \
	$(if $(wildcard $1/main_cm4_hal.c),$1/firmware_cm4_hal.bin) \
	$(if $(wildcard $1/main_cm0p_hal.c),$1/firmware_cm0p_hal.bin)
DUALBINS := $(foreach e,$(dualcore),$(call dual-bins,$e))

.PHONY: all list clean
all: $(BINS) $(DUALBINS)

# ---- shared HAL library ----------------------------------------------------
# Compile the HAL driver set ONCE into common/hal/libhal.a; every HAL app links
# against it (the linker's --gc-sections then keeps only what each app uses).
$(HAL_LIBDIR)/%.o: $(HAL_DRV)/Src/%.c | $(HAL_LIBDIR)
	@echo "  CC    $< (hal-lib)"
	$(Q)$(CC) $(CFLAGS_HALDRV) -c -o $@ $<

$(HAL_LIBDIR):
	$(Q)mkdir -p $@

$(HAL_LIB): $(HAL_LIBOBJS)
	@echo "  AR    $@"
	$(Q)$(AR) rcs $@ $^

-include $(HAL_LIBOBJS:.o=.d)

# Same, compiled for the M0+ (cortex-m0plus) -> libhal_cm0.a.
$(HAL_LIBDIR_CM0)/%.o: $(HAL_DRV)/Src/%.c | $(HAL_LIBDIR_CM0)
	@echo "  CC    $< (hal-lib cm0+)"
	$(Q)$(CC) $(CFLAGS_HALDRV_CM0) -c -o $@ $<

$(HAL_LIBDIR_CM0):
	$(Q)mkdir -p $@

$(HAL_LIB_CM0): $(HAL_LIBOBJS_CM0)
	@echo "  AR    $@"
	$(Q)$(AR) rcs $@ $^

-include $(HAL_LIBOBJS_CM0:.o=.d)

# ---- per-single-core-exercise rules ----------------------------------------
# $1 = exercise dir under single_core/. Builds TWO M4 images from the same dir:
#   bare: main_bare.c            -> app_bare.elf / firmware_bare.bin
#   hal : main_hal.c + libhal.a  -> app_hal.elf  / firmware_hal.bin
# Both share the startup .s and system_stm32wlxx.c. The HAL image also links the
# shared HAL library; each main_hal.c provides its own SysTick_Handler.
define SINGLECORE_rules
# --- bare-metal image ---
$1/.obj/bare.o: $1/main_bare.c | $1/.obj
	@echo "  CC    $$< (bare)"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/system_bare.o: $$(SYSTEM) | $1/.obj
	@echo "  CC    $$< (bare)"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<

$1/.obj/startup_bare.o: $$(STARTUP) | $1/.obj
	@echo "  AS    $$< (bare)"
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<

$1/app_bare.elf: $1/.obj/bare.o $1/.obj/system_bare.o $1/.obj/startup_bare.o $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app_bare.map -o $$@ $$(filter %.o,$$^) 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@

$1/firmware_bare.bin: $1/app_bare.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	$$(Q)$$(SIZE) $$<

# --- HAL image (each main_hal.c defines its own SysTick_Handler) ---
$1/.obj/hal_app.o: $1/main_hal.c | $1/.obj
	@echo "  CC    $$< (hal)"
	$$(Q)$$(CC) $$(CFLAGS_HAL) -c -o $$@ $$<

$1/.obj/system_hal.o: $$(SYSTEM) | $1/.obj
	@echo "  CC    $$< (hal)"
	$$(Q)$$(CC) $$(CFLAGS_HAL) -c -o $$@ $$<

$1/.obj/startup_hal.o: $$(STARTUP) | $1/.obj
	@echo "  AS    $$< (hal)"
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<

# Link against the shared HAL library (built once); --gc-sections drops unused.
$1/app_hal.elf: $1/.obj/hal_app.o $1/.obj/system_hal.o $1/.obj/startup_hal.o $$(HAL_LIB) $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app_hal.map -o $$@ \
		$1/.obj/hal_app.o $1/.obj/system_hal.o $1/.obj/startup_hal.o $$(HAL_LIB) 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@

$1/firmware_hal.bin: $1/app_hal.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	$$(Q)$$(SIZE) $$<
	@echo "  built $1 (bare + HAL)"

$1/.obj:
	$$(Q)mkdir -p $$@

-include $1/.obj/bare.d $1/.obj/system_bare.d $1/.obj/startup_bare.d
-include $1/.obj/hal_app.d $1/.obj/system_hal.d $1/.obj/startup_hal.d
endef
$(foreach e,$(EXERCISES),$(eval $(call SINGLECORE_rules,$e)))

# ---- per-dual-core-exercise rules ------------------------------------------
# A dual-core exercise builds one image per core per available variant. Image
# names carry both core and variant, e.g. firmware_cm4_bare.bin,
# firmware_cm0p_hal.bin. The M4 links the M4 HAL library; the M0+ links the
# cortex-m0plus HAL library (different instruction set -> different machine code).
# Objects live in $1/.obj with unique suffixes so nothing collides. (Flashing
# both banks + the M4's C2BOOT write actually runs the M0+ -- see CLAUDE.md.)

# DUALCORE_bare: $1 = exercise dir. Builds the two _bare core images.
define DUALCORE_bare
$1/.obj/app_cm4_bare.o: $1/main_cm4_bare.c | $1/.obj
	@echo "  CC    $$< (cm4 bare)"
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<
$1/.obj/system_cm4_bare.o: $$(SYSTEM) | $1/.obj
	$$(Q)$$(CC) $$(CFLAGS) -c -o $$@ $$<
$1/.obj/startup_cm4_bare.o: $$(STARTUP) | $1/.obj
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<
$1/app_cm4_bare.elf: $1/.obj/app_cm4_bare.o $1/.obj/system_cm4_bare.o $1/.obj/startup_cm4_bare.o $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app_cm4_bare.map -o $$@ $$(filter %.o,$$^) 2>&1 | grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@
$1/firmware_cm4_bare.bin: $1/app_cm4_bare.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@

$1/.obj/app_cm0p_bare.o: $1/main_cm0p_bare.c | $1/.obj
	@echo "  CC    $$< (cm0+ bare)"
	$$(Q)$$(CC) $$(CFLAGS_CM0) -c -o $$@ $$<
$1/.obj/system_cm0p_bare.o: $$(SYSTEM) | $1/.obj
	$$(Q)$$(CC) $$(CFLAGS_CM0) -c -o $$@ $$<
$1/.obj/startup_cm0p_bare.o: $$(STARTUP_CM0) | $1/.obj
	$$(Q)$$(CC) $$(ASFLAGS_CM0) -c -o $$@ $$<
$1/app_cm0p_bare.elf: $1/.obj/app_cm0p_bare.o $1/.obj/system_cm0p_bare.o $1/.obj/startup_cm0p_bare.o $$(LDSCRIPT_CM0)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS_CM0) -Wl,-Map=$1/app_cm0p_bare.map -o $$@ $$(filter %.o,$$^) 2>&1 | grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@
$1/firmware_cm0p_bare.bin: $1/app_cm0p_bare.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	@echo "  built $1 (bare, both cores)"

$1/.obj:
	$$(Q)mkdir -p $$@
-include $1/.obj/app_cm4_bare.d $1/.obj/app_cm0p_bare.d
endef

# DUALCORE_hal: $1 = exercise dir. Builds the two _hal core images, each linking
# the per-core HAL library. Only evaluated when the _hal sources exist.
define DUALCORE_hal
$1/.obj/app_cm4_hal.o: $1/main_cm4_hal.c | $1/.obj
	@echo "  CC    $$< (cm4 hal)"
	$$(Q)$$(CC) $$(CFLAGS_HAL) -c -o $$@ $$<
$1/.obj/system_cm4_hal.o: $$(SYSTEM) | $1/.obj
	$$(Q)$$(CC) $$(CFLAGS_HAL) -c -o $$@ $$<
$1/.obj/startup_cm4_hal.o: $$(STARTUP) | $1/.obj
	$$(Q)$$(CC) $$(ASFLAGS) -c -o $$@ $$<
$1/app_cm4_hal.elf: $1/.obj/app_cm4_hal.o $1/.obj/system_cm4_hal.o $1/.obj/startup_cm4_hal.o $$(HAL_LIB) $$(LDSCRIPT)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS) -Wl,-Map=$1/app_cm4_hal.map -o $$@ $1/.obj/app_cm4_hal.o $1/.obj/system_cm4_hal.o $1/.obj/startup_cm4_hal.o $$(HAL_LIB) 2>&1 | grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@
$1/firmware_cm4_hal.bin: $1/app_cm4_hal.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@

$1/.obj/app_cm0p_hal.o: $1/main_cm0p_hal.c | $1/.obj
	@echo "  CC    $$< (cm0+ hal)"
	$$(Q)$$(CC) $$(CFLAGS_HAL_CM0) -c -o $$@ $$<
$1/.obj/system_cm0p_hal.o: $$(SYSTEM) | $1/.obj
	$$(Q)$$(CC) $$(CFLAGS_HAL_CM0) -c -o $$@ $$<
$1/.obj/startup_cm0p_hal.o: $$(STARTUP_CM0) | $1/.obj
	$$(Q)$$(CC) $$(ASFLAGS_CM0) -c -o $$@ $$<
$1/app_cm0p_hal.elf: $1/.obj/app_cm0p_hal.o $1/.obj/system_cm0p_hal.o $1/.obj/startup_cm0p_hal.o $$(HAL_LIB_CM0) $$(LDSCRIPT_CM0)
	@echo "  LD    $$@"
	$$(Q)$$(CC) $$(LDFLAGS_CM0) -Wl,-Map=$1/app_cm0p_hal.map -o $$@ $1/.obj/app_cm0p_hal.o $1/.obj/system_cm0p_hal.o $1/.obj/startup_cm0p_hal.o $$(HAL_LIB_CM0) 2>&1 | grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@
$1/firmware_cm0p_hal.bin: $1/app_cm0p_hal.elf
	@echo "  BIN   $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	@echo "  built $1 (hal, both cores)"
-include $1/.obj/app_cm4_hal.d $1/.obj/app_cm0p_hal.d
endef

$(foreach e,$(dualcore),$(eval $(call DUALCORE_bare,$e)))
# HAL rules only for exercises that ship the _hal sources.
$(foreach e,$(dualcore),$(if $(wildcard $e/main_cm4_hal.c),$(eval $(call DUALCORE_hal,$e))))

# Convenience targets. A single-core bare name (e.g. `make 02_serial_counter`)
# builds BOTH that exercise's images (bare + HAL). Full-path targets also work.
$(NAMES): %: $(SINGLEDIR)/%/firmware_bare.bin $(SINGLEDIR)/%/firmware_hal.bin
$(EXERCISES): %: %/firmware_bare.bin %/firmware_hal.bin
# Dual-core convenience targets build ALL available core/variant images. Emit one
# rule per exercise so dual-bins' $(wildcard) sees the real path (a static pattern
# rule would feed it a literal '%' and silently drop the HAL images).
$(foreach e,$(dualcore),$(eval $(notdir $e): $(call dual-bins,$e)))
$(foreach e,$(dualcore),$(eval $e: $(call dual-bins,$e)))
.PHONY: $(NAMES) $(EXERCISES) $(DUALNAMES) $(dualcore)

list:
	@echo "single_core (each builds bare + HAL):"
	@for e in $(NAMES); do echo "  $$e"; done
	@echo "dual_core (M4 + M0+, bare; some also HAL):"
	@for e in $(DUALNAMES); do echo "  $$e"; done

clean:
	$(Q)find $(DIR) -name '*.bin' -o -name '*.elf' -o -name '*.map' | xargs -r rm -f
	$(Q)rm -rf $(SINGLEDIR)/*/.obj $(DUALDIR)/*/.obj $(HAL_LIBDIR) $(HAL_LIBDIR_CM0)
	$(Q)rm -f $(HAL_LIB) $(HAL_LIB_CM0)
	@echo "cleaned"
