#
# Copyright (c) 2015, The LibrePilot Project, http://www.librepilot.org
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

include ../board-info.mk
include $(FLIGHT_ROOT_DIR)/make/firmware-defs.mk

# REVO C++ support 
USE_CXX = YES

# ARM DSP library
override USE_DSP_LIB := NO

# List of mandatory modules to include
MODULES += Osd/osdgen
MODULES += Osd/osdinput
MODULES += Osd/WavPlayer
MODULES += GPS
#MODULES += Extensions/MagBaro
MODULES += FirmwareIAP
MODULES += Telemetry

OPTMODULES =

SRC += $(FLIGHTLIB)/notification.c

# Some diagnostics
CDEFS += -DDIAG_TASKS

# Misc options
CFLAGS += -ffast-math

# List C source files here (C dependencies are automatically generated).
# Use file-extension c for "c-only"-files
ifndef TESTAPP
    ## Application Core
    SRC += ../pios_usb_board_data.c
    SRC += $(OPMODULEDIR)/System/systemmod.c
    CPPSRC += $(OPSYSTEM)/osd.cpp
    SRC += $(OPSYSTEM)/pios_board.c
    SRC += $(FLIGHTLIB)/alarms.c
    SRC += $(OPUAVTALK)/uavtalk.c
    SRC += $(OPUAVOBJ)/uavobjectmanager.c
    SRC += $(OPUAVOBJ)/uavobjectpersistence.c
    SRC += $(OPUAVOBJ)/eventdispatcher.c
    SRC += $(PIOSCOMMON)/pios_flashfs_logfs.c
    SRC += $(PIOSCOMMON)/pios_flash_jedec.c

    ## OSD fonts
    SRC += $(OPSYSTEM)/fonts.c
    SRC += $(OPSYSTEM)/font_outlined8x14.c
    SRC += $(OPSYSTEM)/font_outlined8x8.c

    ## Misc library functions
    SRC += $(FLIGHTLIB)/WorldMagModel.c
    SRC += $(FLIGHTLIB)/auxmagsupport.c

    ## UAVObjects
    SRC += $(FLIGHT_UAVOBJ_DIR)/objectpersistence.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gcstelemetrystats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flighttelemetrystats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flightstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flightmodesettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/manualcontrolsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemstats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemalarms.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/attitudestate.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/i2cstats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/watchdogstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/firmwareiapobj.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/hwsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/taskinfo.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/callbackinfo.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/mixerstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/homelocation.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpspositionsensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpssatellites.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpsvelocitysensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpstime.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpssettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/osdsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/barosensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/magsensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/auxmagsensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpsextendedstatus.c
else
    ## Test Code
    SRC += $(OPTESTS)/test_common.c
    SRC += $(OPTESTS)/$(TESTAPP).c
endif
	
# Optional component libraries
include $(PIOS)/common/libraries/dosfs/library.mk

include $(FLIGHT_ROOT_DIR)/make/apps-defs.mk
include $(FLIGHT_ROOT_DIR)/make/common-defs.mk
