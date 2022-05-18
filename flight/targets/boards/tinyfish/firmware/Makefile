#
# Copyright (C) 2015-2016, The LibrePilot Project, http://www.librepilot.org
# Copyright (C) 2009-2013, The OpenPilot Team, http://www.openpilot.org
# Copyright (C) 2012, PhoenixPilot, http://github.com/PhoenixPilot
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
#####

ifndef FLIGHT_MAKEFILE
    $(error Top level Makefile must be used to build this target)
endif

include ../board-info.mk
include $(FLIGHT_ROOT_DIR)/make/firmware-defs.mk

# C++ support
USE_CXX = YES

# ARM DSP library
USE_DSP_LIB ?= NO

DEBUG = NO

# List of mandatory modules to include
MODULES += Sensors
MODULES += StateEstimation
MODULES += Stabilization
MODULES += ManualControl
MODULES += Receiver
MODULES += Actuator
MODULES += FirmwareIAP
#MODULES += PathPlanner
#MODULES += PathFollower
#MODULES += Osd/osdoutout
#MODULES += Logging
MODULES += Telemetry
MODULES += Notify
MODULES += Battery
MODULES += UAVOFrSKYSensorHubBridge

OPTMODULES += Airspeed
OPTMODULES += AutoTune
OPTMODULES += GPS
OPTMODULES += TxPID
OPTMODULES += CameraStab
OPTMODULES += CameraControl
OPTMODULES += ComUsbBridge
OPTMODULES += UAVOMSPBridge
OPTMODULES += UAVOMavlinkBridge

SRC += $(FLIGHTLIB)/notification.c

# Include all camera options
CDEFS += -DUSE_INPUT_LPF -DUSE_GIMBAL_LPF -DUSE_GIMBAL_FF

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
    CPPSRC += $(OPSYSTEM)/main.cpp
    SRC += $(OPSYSTEM)/pios_board.c
    SRC += $(FLIGHTLIB)/alarms.c
    SRC += $(OPUAVTALK)/uavtalk.c
    SRC += $(OPUAVOBJ)/uavobjectmanager.c
    SRC += $(OPUAVOBJ)/uavobjectpersistence.c
    SRC += $(OPUAVOBJ)/eventdispatcher.c
    SRC += $(PIOSCOMMON)/pios_flashfs_logfs.c
    SRC += $(PIOSCOMMON)/pios_flash_jedec.c
    
    ## Misc library functions
    SRC += $(FLIGHTLIB)/instrumentation.c
    SRC += $(FLIGHTLIB)/paths.c
	SRC += $(FLIGHTLIB)/plans.c
    SRC += $(FLIGHTLIB)/WorldMagModel.c
    SRC += $(FLIGHTLIB)/insgps13state.c
    SRC += $(FLIGHTLIB)/auxmagsupport.c
    SRC += $(FLIGHTLIB)/lednotification.c    

    ## UAVObjects
    include ./UAVObjects.inc
    SRC += $(UAVOBJSRC)
else
    ## Test Code
    SRC += $(OPTESTS)/test_common.c
    SRC += $(OPTESTS)/$(TESTAPP).c
endif

# Optional component libraries
#include $(FLIGHTLIB)/rscode/library.mk
#include $(FLIGHTLIB)/PyMite/pymite.mk

include $(FLIGHT_ROOT_DIR)/make/apps-defs.mk
include $(FLIGHT_ROOT_DIR)/make/common-defs.mk
