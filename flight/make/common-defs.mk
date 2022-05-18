#
# Copyright (c) 2009-2013, The OpenPilot Team, http://www.openpilot.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

ifndef FLIGHT_MAKEFILE
    $(error Top level Makefile must be used to build this target)
endif

# Set to YES to compile for debugging
DEBUG                ?= NO

# Set to YES to compile C++ implemented features
USE_CXX              ?= NO

# Set to YES to use the Servo output pins for debugging via scope or logic analyser
ENABLE_DEBUG_PINS    ?= NO

# Set to YES to enable the AUX UART which is mapped on the S1 (Tx) and S2 (Rx) servo outputs
ENABLE_AUX_UART      ?= NO

# Include objects that are just nice information to show
DIAG_STACK           ?= NO
DIAG_MIXERSTATUS     ?= NO
DIAG_RATEDESIRED     ?= NO
DIAG_I2C_WDG_STATS   ?= NO
DIAG_TASKS           ?= NO
DIAG_INSTRUMENTATION ?= NO

# Or just turn on all the above diagnostics. WARNING: this consumes massive amounts of memory.
DIAG_ALL             ?= NO

# Optimization level, can be [0, 1, 2, 3, s].
# 0 = turn off optimization. s = optimize for size.
# Note: 3 is not always the best optimization level.
ifeq ($(DEBUG), YES)
    OPT = 0
else
    OPT = s
endif

# Output format (can be ihex or binary or both).
#  binary to create a load-image in raw-binary format i.e. for SAM-BA,
#  ihex to create a load-image in Intel hex format
#LOADFORMAT = ihex
#LOADFORMAT = binary
LOADFORMAT = both

# Debugging format.
DEBUGF = dwarf-2

# Place project-specific -D (define) and/or -U options for C here.
CDEFS += -DUSE_$(BOARD)

ifeq ($(ENABLE_DEBUG_PINS), YES)
    CDEFS += -DPIOS_ENABLE_DEBUG_PINS
endif

ifeq ($(ENABLE_AUX_UART), YES)
    CDEFS += -DPIOS_ENABLE_AUX_UART
endif

# The following Makefile command, ifneq (,$(filter) $(A), $(B) $(C))
#    is equivalent to the pseudocode `if (A == B || A == C)`
ifneq (,$(filter YES,$(DIAG_STACK) $(DIAG_ALL)))
    CFLAGS += -DDIAG_STACK
endif

ifneq (,$(filter YES,$(DIAG_MIXERSTATUS) $(DIAG_ALL)))
    CFLAGS += -DDIAG_MIXERSTATUS
endif

ifneq (,$(filter YES,$(DIAG_RATEDESIRED) $(DIAG_ALL)))
    CFLAGS += -DDIAG_RATEDESIRED
endif

ifneq (,$(filter YES,$(DIAG_I2C_WDG_STATS) $(DIAG_ALL)))
    CFLAGS += -DDIAG_I2C_WDG_STATS
endif

ifneq (,$(filter YES,$(DIAG_TASKS) $(DIAG_ALL)))
    CFLAGS += -DDIAG_TASKS
endif

ifneq (,$(filter YES,$(DIAG_INSTRUMENTATION) $(DIAG_ALL)))
    CFLAGS += -DPIOS_INCLUDE_INSTRUMENTATION
endif
# Place project-specific -D and/or -U options for Assembler with preprocessor here.
#ADEFS = -DUSE_IRQ_ASM_WRAPPER
ADEFS = -D__ASSEMBLY__

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Compiler flags.
#
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing

# Common architecture-specific flags from the device-specific library makefile
CFLAGS += $(ARCHFLAGS)
CFLAGS += $(CDEFS)
CFLAGS += -O$(OPT)
CFLAGS += -g$(DEBUGF)
CFLAGS += -mapcs-frame
CFLAGS += -fomit-frame-pointer
CFLAGS += -Wall -Wextra
CFLAGS += -Wfloat-equal -Wdouble-promotion
CFLAGS += -Wshadow
CFLAGS += -Werror
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -I.
CFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))

# Provides board-specific defines
BOARD_CDEFS += -DBOARD_TYPE=$(BOARD_TYPE)
BOARD_CDEFS += -DBOARD_REVISION=$(BOARD_REVISION)
BOARD_CDEFS += -DHW_TYPE=$(HW_TYPE)
BOARD_CDEFS += -DBOOTLOADER_VERSION=$(BOOTLOADER_VERSION)
BOARD_CDEFS += -DFW_BANK_BASE=$(FW_BANK_BASE)
BOARD_CDEFS += -DFW_BANK_SIZE=$(FW_BANK_SIZE)
BOARD_CDEFS += -DFW_DESC_SIZE=$(FW_DESC_SIZE)

BOARD_CDEFS += -DBL_BANK_BASE=$(BL_BANK_BASE)
BOARD_CDEFS += -DBL_BANK_SIZE=$(BL_BANK_SIZE)
BOARD_CDEFS += -DBL_DESC_SIZE=$(BL_DESC_SIZE)
ifdef EE_BANK_BASE
BOARD_CDEFS += -DEE_BANK_BASE=$(EE_BANK_BASE)
BOARD_CDEFS += -DEE_BANK_SIZE=$(EE_BANK_SIZE)
endif
ifdef USER_EE_BANK_BASE
BOARD_CDEFS += -DUSER_EE_BANK_BASE=$(USER_EE_BANK_BASE)
BOARD_CDEFS += -DUSER_EE_BANK_SIZE=$(USER_EE_BANK_SIZE)
endif
ifdef SRAM_BANK_BASE
BOARD_CDEFS += -DSRAM_BANK_BASE=$(SRAM_BANK_BASE)
BOARD_CDEFS += -DSRAM_BANK_SIZE=$(SRAM_BANK_SIZE)
endif
ifdef CCSRAM_BANK_BASE
BOARD_CDEFS += -DCCSRAM_BANK_BASE=$(CCSRAM_BANK_BASE)
BOARD_CDEFS += -DCCSRAM_BANK_SIZE=$(CCSRAM_BANK_SIZE)
endif
CDEFS += $(BOARD_CDEFS)

ifeq ($(DEBUG), YES)
    CPPFLAGS += -DDEBUG
else
    CFLAGS += -fdata-sections -ffunction-sections
endif

ifeq ($(USE_CXX), YES)
    CPPFLAGS += -DPIOS_ENABLE_CXX
endif

# Compiler flags to generate dependency files
CFLAGS += -MD -MP -MF $(OUTDIR)/dep/$(@F).d

# Flags only for C
#CONLYFLAGS += -Wnested-externs
CONLYFLAGS += $(CSTANDARD) -Wunsuffixed-float-constants

# C++ Flags
# Note C++ compilation also uses CFLAGS.  CONLYFLAGS has exclusions that 
# can not be used with C++ compiler. C++ specific compile options are
# as set in CXXFLAGS. Note CPPFLAGS is for C pre-process and has not been
# used correctly - longer term move CDEFS to CPPFLAGS.
CXXFLAGS += -fno-rtti -fno-exceptions -std=c++11 -fno-use-cxa-atexit

# Assembler flags.
#  -Wa,...:    tell GCC to pass this to the assembler.
#  -ahlns:     create listing
ASFLAGS =  -mcpu=$(MCU) -I. -x assembler-with-cpp
ASFLAGS += $(ADEFS)
ASFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
ASFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--warn-common,--fatal-warnings,--sort-common,--sort-section=alignment,--gc-sections
LDFLAGS += -Wl,-Map=$(OUTDIR)/$(TARGET).map,--cref
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))
LDFLAGS += -lc -lgcc

ifneq ($(DEBUG), YES)
    LDFLAGS += -Wl,-static
endif

# List of all source files.
ifeq ($(USE_CXX), YES)
ALLSRC     = $(CPPSRCARM) $(CPPSRC) $(ASRCARM) $(ASRC) $(SRCARM) $(SRC) 
else
ALLSRC     = $(ASRCARM) $(ASRC) $(SRCARM) $(SRC)
endif

# List of all source files without directory and file-extension.
ALLSRCBASE = $(notdir $(basename $(ALLSRC)))

# Define all object files.
ALLOBJ     = $(addprefix $(OUTDIR)/, $(addsuffix .o, $(ALLSRCBASE))) $(EXTRAOBJ)

ALLLD      = $(addprefix $(OUTDIR)/, $(addsuffix .ld, $(notdir $(basename $(LDSRC)))))

# Define all listing files (used for make clean).
LSTFILES   = $(addprefix $(OUTDIR)/, $(addsuffix .lst, $(ALLSRCBASE)))
# Define all depedency-files (used for make clean).
DEPFILES   = $(addprefix $(OUTDIR)/dep/, $(addsuffix .o.d, $(ALLSRCBASE)))

# Default target.
all: build

ifeq ($(LOADFORMAT),ihex)
build: elf hex sym
else ifeq ($(LOADFORMAT),binary)
build: elf bin sym
else ifeq ($(LOADFORMAT),both)
build: elf hex bin sym
else
    $(error "$(MSG_FORMATERROR) $(FORMAT)")
endif

# Generate code for PyMite
# $(OUTDIR)/pmlib_img.c $(OUTDIR)/pmlib_nat.c $(OUTDIR)/pmlibusr_img.c $(OUTDIR)/pmlibusr_nat.c $(OUTDIR)/pmfeatures.h: $(wildcard $(PYMITELIB)/*.py) $(wildcard $(PYMITEPLAT)/*.py) $(wildcard $(FLIGHTPLANLIB)/*.py) $(wildcard $(FLIGHTPLANS)/*.py)
#	@echo $(MSG_PYMITEINIT) $(call toprel,$@)
#	@$(PYTHON) $(PYMITETOOLS)/pmImgCreator.py -f $(PYMITEPLAT)/pmfeatures.py -c -s --memspace=flash -o $(OUTDIR)/pmlib_img.c --native-file=$(OUTDIR)/pmlib_nat.c $(PYMITELIB)/list.py $(PYMITELIB)/dict.py $(PYMITELIB)/__bi.py $(PYMITELIB)/sys.py $(PYMITELIB)/string.py $(wildcard $(FLIGHTPLANLIB)/*.py)
#	@$(PYTHON) $(PYMITETOOLS)/pmGenPmFeatures.py $(PYMITEPLAT)/pmfeatures.py > $(OUTDIR)/pmfeatures.h
#	@$(PYTHON) $(PYMITETOOLS)/pmImgCreator.py -f $(PYMITEPLAT)/pmfeatures.py -c -u -o $(OUTDIR)/pmlibusr_img.c --native-file=$(OUTDIR)/pmlibusr_nat.c $(FLIGHTPLANS)/test.py

# Link: create ELF output file from object files.
ifeq ($(USE_CXX), YES)
$(eval $(call LINK_CXX_TEMPLATE,$(OUTDIR)/$(TARGET).elf,$(ALLOBJ),$(ALLLIB),$(ALLLD)))
else
$(eval $(call LINK_TEMPLATE,$(OUTDIR)/$(TARGET).elf,$(ALLOBJ),$(ALLLIB),$(ALLLD)))
endif

# Assemble: create object files from assembler source files.
$(foreach src, $(ASRC), $(eval $(call ASSEMBLE_TEMPLATE,$(src))))

# Assemble: create object files from assembler source files. ARM-only
$(foreach src, $(ASRCARM), $(eval $(call ASSEMBLE_ARM_TEMPLATE,$(src))))

# Compile: create object files from C source files.
$(foreach src, $(SRC), $(eval $(call COMPILE_C_TEMPLATE,$(src))))

# Compile: create object files from C source files. ARM-only
$(foreach src, $(SRCARM), $(eval $(call COMPILE_C_ARM_TEMPLATE,$(src))))

ifeq ($(USE_CXX), YES)
# Compile: create object files from C++ source files.
$(foreach src, $(CPPSRC), $(eval $(call COMPILE_CXX_TEMPLATE,$(src))))

# Compile: create object files from C++ source files. ARM-only
$(foreach src, $(CPPSRCARM), $(eval $(call COMPILE_CXX_ARM_TEMPLATE,$(src))))
endif

# Compile: create assembler files from C source files. ARM/Thumb
$(eval $(call PARTIAL_COMPILE_TEMPLATE,SRC))

# Compile: create assembler files from C source files. ARM only
$(eval $(call PARTIAL_COMPILE_ARM_TEMPLATE,SRCARM))

# Preprocess: create linker scripts from .lds files.
$(foreach src, $(LDSRC), $(eval $(call PREPROCESS_LDS_TEMPLATE,$(src))))

# Add opfw target
$(eval $(call OPFW_TEMPLATE,$(OUTDIR)/$(TARGET).bin,$(BOARD_TYPE),$(BOARD_REVISION)))

$(OUTDIR)/$(TARGET).bin.o: $(OUTDIR)/$(TARGET).bin

.PHONY: elf lss sym hex bin bino opfw
elf: $(OUTDIR)/$(TARGET).elf
lss: $(OUTDIR)/$(TARGET).lss
sym: $(OUTDIR)/$(TARGET).sym
hex: $(OUTDIR)/$(TARGET).hex
bin: $(OUTDIR)/$(TARGET).bin
bino: $(OUTDIR)/$(TARGET).bin.o
opfw: $(OUTDIR)/$(TARGET).opfw

# Display sizes of sections.
$(eval $(call SIZE_TEMPLATE,$(OUTDIR)/$(TARGET).elf))

# Target: clean project
clean:
	@echo $(MSG_CLEANING)
	$(V1) rm -f $(OUTDIR)/$(TARGET).map
	$(V1) rm -f $(OUTDIR)/$(TARGET).elf
	$(V1) rm -f $(OUTDIR)/$(TARGET).hex
	$(V1) rm -f $(OUTDIR)/$(TARGET).bin
	$(V1) rm -f $(OUTDIR)/$(TARGET).sym
	$(V1) rm -f $(OUTDIR)/$(TARGET).lss
	$(V1) rm -f $(OUTDIR)/$(TARGET).bin.o
	$(V1) rm -f $(OUTDIR)/$(TARGET).opfw
	$(V1) rm -f $(wildcard $(OUTDIR)/*.c)
	$(V1) rm -f $(wildcard $(OUTDIR)/*.h)
	$(V1) rm -f $(ALLOBJ)
	$(V1) rm -f $(LSTFILES)
	$(V1) rm -f $(DEPFILES)
	$(V1) rm -f $(SRC:.c=.s)
	$(V1) rm -f $(SRCARM:.c=.s)
	$(V1) rm -f $(CPPSRC:.cpp=.s)
	$(V1) rm -f $(CPPSRCARM:.cpp=.s)

# Create output files directory
# all known MS Windows OS define the ComSpec environment variable
$(shell mkdir -p $(OUTDIR) 2>/dev/null)

# Include the dependency files.
-include $(shell mkdir -p $(OUTDIR)/dep 2>/dev/null) $(wildcard $(OUTDIR)/dep/*)

# Listing of phony targets.
.PHONY: all build clean install
