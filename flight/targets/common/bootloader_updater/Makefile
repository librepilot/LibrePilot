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

include ../../boards/$(BOARD_NAME)/board-info.mk
include $(FLIGHT_ROOT_DIR)/make/firmware-defs.mk

# ARM DSP library
override USE_DSP_LIB := NO

# Paths
TOPDIR		= .
OPSYSTEM	= $(TOPDIR)
OPSYSTEMINC	= $(OPSYSTEM)/inc
PIOSINC		= $(PIOS)/inc
PIOSCOMMON	= $(PIOS)/common
FLIGHTLIBINC	= $(FLIGHTLIB)/inc
HWDEFSINC	= ../../boards/$(BOARD_NAME)

# List C source files here (C dependencies are automatically generated).
# Use file-extension c for "c-only"-files
SRC += $(OPSYSTEM)/main.c
SRC += $(OPSYSTEM)/pios_board.c
SRC += $(PIOSCOMMON)/pios_led.c
## PIOS Hardware
ifneq (,$(findstring STM32F10,$(CHIP)))
    include $(PIOS)/stm32f10x/library.mk

    ## The standard CMSIS startup
    SRC += $(CMSIS_DEVICEDIR)/system_stm32f10x.c

    # Set linker-script name depending on selected submodel name
    LDFLAGS += -T$(LINKER_SCRIPTS_PATH)/link_$(BOARD)_memory.ld
    LDFLAGS += -T$(LINKER_SCRIPTS_PATH)/link_$(BOARD)_sections.ld
else ifneq (,$(findstring STM32F4,$(CHIP)))
    include $(PIOS)/stm32f4xx/library.mk
    # Set linker-script name depending on selected submodel name
    # Using the _compat ld script ensure that the bootloader updated is compiled with irqstack in sram
    # This allow to be used with bootloader earlier than rel5
    LDFLAGS += $(addprefix -T,$(LINKER_SCRIPTS_APP))
else ifneq (,$(findstring STM32F30,$(CHIP)))
    include $(PIOS)/stm32f30x/library.mk
    # Set linker-script name depending on selected submodel name
    # Using the _compat ld script ensure that the bootloader updated is compiled with irqstack in sram
    # This allow to be used with bootloader earlier than rel5
#    LDFLAGS += $(addprefix -T,$(LINKER_SCRIPTS_APP))
else
    $(error Unsupported MCU for BootloaderUpdater: $(CHIP))
endif

# List any extra directories to look for include files here.
#    Each directory must be seperated by a space.
EXTRAINCDIRS += $(PIOS)
EXTRAINCDIRS += $(PIOSINC)
EXTRAINCDIRS += $(HWDEFSINC)
EXTRAINCDIRS += $(FLIGHTLIBINC)
EXTRAINCDIRS += $(PIOSCOMMON)
EXTRAINCDIRS += $(OPSYSTEMINC)

# Link: create ELF output file from object files.
EXTRAOBJ := $(FLIGHT_OUT_DIR)/bl_$(BOARD_NAME)/bl_$(BOARD_NAME).bin.o

include $(FLIGHT_ROOT_DIR)/make/common-defs.mk
