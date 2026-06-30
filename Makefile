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
LDSCRIPT := $(COMMON)/STM32WL55JCIX_FLASH_M4.ld
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
# The linker SCRIPT is no longer baked in here: it is passed per image as the
# IMAGE template's $8 (via -T), so an exercise can override it with its own .ld.
LDFLAGS := $(CPU) -Wl,--gc-sections \
	--specs=nano.specs --specs=nosys.specs

# M0+ variants of the above (different cpu, defines, and linker script).
CFLAGS_CM0  := $(CPU_CM0) $(DEFS_CM0) $(INCLUDES) -Wall -Wextra -O2 -g3 \
	-ffunction-sections -fdata-sections -MMD -MP
ASFLAGS_CM0 := $(CPU_CM0) -MMD -MP
# As with LDFLAGS, the M0+ linker script is passed per image (IMAGE's $8), not
# baked in, so a dual-core exercise can ship its own M0+ .ld (e.g. the shared
# memory exercise reserves a .shared slot in SRAM2).
LDFLAGS_CM0 := $(CPU_CM0) -Wl,--gc-sections \
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

# Per-exercise linker scripts: an exercise may ship its OWN .ld in its directory
# to override the shared common one (e.g. 03_basic_shared_memory carries its own
# M0+ script that reserves a .shared slot in SRAM2). These helpers pick the
# exercise-local script if it exists, else fall back to the common default.
#   $1 = exercise dir
ld-m4  = $(if $(wildcard $1/STM32WL55JCIX_FLASH_M4.ld),$1/STM32WL55JCIX_FLASH_M4.ld,$(LDSCRIPT))
ld-cm0 = $(if $(wildcard $1/STM32WL55JCIX_FLASH_CM0PLUS.ld),$1/STM32WL55JCIX_FLASH_CM0PLUS.ld,$(LDSCRIPT_CM0))

# Each single-core exercise produces two images: bare-metal and HAL.
BINS := $(foreach e,$(EXERCISES),$(e)/firmware_bare.bin $(e)/firmware_hal.bin)
# Each dual-core exercise produces FOUR images: both cores, each as bare + HAL.
# (Every exercise must ship all four sources -- there is no optional variant.)
dual-bins = $1/firmware_cm4_bare.bin $1/firmware_cm0p_bare.bin \
	$1/firmware_cm4_hal.bin $1/firmware_cm0p_hal.bin
DUALBINS := $(foreach e,$(dualcore),$(call dual-bins,$e))

.PHONY: all list clean
all: $(BINS) $(DUALBINS)

# ---- shared HAL library ----------------------------------------------------
# Compile the HAL driver set ONCE (per core) into common/libhal_<cpu>.a; every
# HAL app links it (the linker's --gc-sections then keeps only what it uses).
$(HAL_LIBDIR)/%.o: $(HAL_DRV)/Src/%.c | $(HAL_LIBDIR)
	@echo "  CC    $< [M4] (hal-lib)"
	$(Q)$(CC) $(CFLAGS_HALDRV) -c -o $@ $<

$(HAL_LIBDIR):
	$(Q)mkdir -p $@

$(HAL_LIB): $(HAL_LIBOBJS)
	@echo "  AR    $@"
	$(Q)$(AR) rcs $@ $^

-include $(HAL_LIBOBJS:.o=.d)

# Same, compiled for the M0+ (cortex-m0plus) -> libhal_cm0.a.
$(HAL_LIBDIR_CM0)/%.o: $(HAL_DRV)/Src/%.c | $(HAL_LIBDIR_CM0)
	@echo "  CC    $< [M0+] (hal-lib)"
	$(Q)$(CC) $(CFLAGS_HALDRV_CM0) -c -o $@ $<

$(HAL_LIBDIR_CM0):
	$(Q)mkdir -p $@

$(HAL_LIB_CM0): $(HAL_LIBOBJS_CM0)
	@echo "  AR    $@"
	$(Q)$(AR) rcs $@ $^

-include $(HAL_LIBOBJS_CM0:.o=.d)

# ---- generic image template ------------------------------------------------
# IMAGE builds ONE firmware image (app + system + startup [+ optional HAL lib]).
# Every exercise image -- single/dual, bare/hal, M4/M0+ -- is one $(call) to it.
#   $1 dir   $2 tag (e.g. bare, hal, cm4_bare)   $3 app source (.c)
#   $4 CFLAGS   $5 ASFLAGS   $6 LDFLAGS   $7 startup .s   $8 linker .ld
#   $9 HAL lib to link (empty for bare)   $(10) core label (always "M4" or "M0+")
# The core label is ALWAYS printed in the CC/LD/BIN lines so a build log never
# leaves the target core ambiguous (single-core tags carry no core in them).
# Outputs: $1/app_$2.elf, $1/firmware_$2.bin; objects in $1/.obj/*_$2.o.
define IMAGE
$1/.obj/app_$2.o: $3 | $1/.obj
	@echo "  CC    $$< [$(10)] ($2)"
	$$(Q)$$(CC) $4 -c -o $$@ $$<
$1/.obj/system_$2.o: $$(SYSTEM) | $1/.obj
	$$(Q)$$(CC) $4 -c -o $$@ $$<
$1/.obj/startup_$2.o: $7 | $1/.obj
	$$(Q)$$(CC) $5 -c -o $$@ $$<
$1/app_$2.elf: $1/.obj/app_$2.o $1/.obj/system_$2.o $1/.obj/startup_$2.o $9 $8
	@echo "  LD    $$@ [$(10)]"
	$$(Q)$$(CC) $6 -T$8 -Wl,-Map=$1/app_$2.map -o $$@ \
		$1/.obj/app_$2.o $1/.obj/system_$2.o $1/.obj/startup_$2.o $9 2>&1 \
		| grep -vE '$$(LD_NOISE)' || true
	$$(Q)test -f $$@
$1/firmware_$2.bin: $1/app_$2.elf
	@echo "  BIN   $$@ [$(10)]"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
-include $1/.obj/app_$2.d $1/.obj/system_$2.d $1/.obj/startup_$2.d
endef

# Each exercise needs its $1/.obj dir (declared once per exercise).
define OBJDIR
$1/.obj:
	$$(Q)mkdir -p $$@
endef

# ---- single-core: one M4 image per variant (bare + HAL) --------------------
$(foreach e,$(EXERCISES),$(eval $(call OBJDIR,$e)) \
	$(eval $(call IMAGE,$e,bare,$e/main_bare.c,$(CFLAGS),$(ASFLAGS),$(LDFLAGS),$(STARTUP),$(call ld-m4,$e),,M4)) \
	$(eval $(call IMAGE,$e,hal,$e/main_hal.c,$(CFLAGS_HAL),$(ASFLAGS),$(LDFLAGS),$(STARTUP),$(call ld-m4,$e),$(HAL_LIB),M4)))

# ---- dual-core: one image per core (M4/M0+) per variant (bare/HAL) ---------
# The M0+ uses the cortex-m0plus flags/startup/linker and its own HAL library;
# image names carry core+variant (cm4_bare, cm0p_hal, ...) so nothing collides.
# (Flashing both banks + the M4's C2BOOT write runs the M0+ -- see CLAUDE.md.)
$(foreach e,$(dualcore),$(eval $(call OBJDIR,$e)) \
	$(eval $(call IMAGE,$e,cm4_bare,$e/main_cm4_bare.c,$(CFLAGS),$(ASFLAGS),$(LDFLAGS),$(STARTUP),$(call ld-m4,$e),,M4)) \
	$(eval $(call IMAGE,$e,cm0p_bare,$e/main_cm0p_bare.c,$(CFLAGS_CM0),$(ASFLAGS_CM0),$(LDFLAGS_CM0),$(STARTUP_CM0),$(call ld-cm0,$e),,M0+)) \
	$(eval $(call IMAGE,$e,cm4_hal,$e/main_cm4_hal.c,$(CFLAGS_HAL),$(ASFLAGS),$(LDFLAGS),$(STARTUP),$(call ld-m4,$e),$(HAL_LIB),M4)) \
	$(eval $(call IMAGE,$e,cm0p_hal,$e/main_cm0p_hal.c,$(CFLAGS_HAL_CM0),$(ASFLAGS_CM0),$(LDFLAGS_CM0),$(STARTUP_CM0),$(call ld-cm0,$e),$(HAL_LIB_CM0),M0+)))

# ---- convenience targets (build by bare name or full path) -----------------
$(NAMES): %: $(SINGLEDIR)/%/firmware_bare.bin $(SINGLEDIR)/%/firmware_hal.bin
$(EXERCISES): %: %/firmware_bare.bin %/firmware_hal.bin
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
