#
# Copyright (c) 2015, The LibrePilot Project, http://www.librepilot.org
# Copyright (c) 2009-2013, The OpenPilot Team, http://www.openpilot.org
# Copyright (c) 2012, PhoenixPilot, http://github.com/PhoenixPilot
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

# ARM DSP library
USE_DSP_LIB ?= NO

# Set to YES to build a FW version that will erase data flash memory
ERASE_FLASH ?= NO

# Set to yes to include Gcsreceiver module
GCSRECEIVER ?= NO

# Enable Diag tasks ?
DIAG_TASKS ?= NO

# List of mandatory modules to include
MODULES += Attitude
MODULES += Stabilization
MODULES += Actuator
MODULES += Receiver
MODULES += ManualControl
MODULES += FirmwareIAP
MODULES += Telemetry

# List of optional modules to include
OPTMODULES += CameraStab
OPTMODULES += ComUsbBridge
OPTMODULES += GPS
OPTMODULES += TxPID
OPTMODULES += Osd/osdoutput
#OPTMODULES += Altitude
#OPTMODULES += Fault
OPTMODULES += UAVOMSPBridge
OPTMODULES += UAVOMavlinkBridge

SRC += $(FLIGHTLIB)/notification.c

# Include all camera options
CDEFS += -DUSE_INPUT_LPF -DUSE_GIMBAL_LPF -DUSE_GIMBAL_FF



# Erase flash firmware should be buildable from command line
ifeq ($(ERASE_FLASH), YES)
    CDEFS += -DERASE_FLASH
endif

# List C source files here (C dependencies are automatically generated).
# Use file-extension c for "c-only"-files
ifndef TESTAPP
    ## The standard CMSIS startup
    SRC += $(CMSIS_DEVICEDIR)/system_stm32f10x.c

    ## Application Core
    SRC += ../pios_usb_board_data.c
    SRC += $(OPMODULEDIR)/System/systemmod.c
    SRC += $(OPSYSTEM)/coptercontrol.c
    SRC += $(OPSYSTEM)/pios_board.c
    SRC += $(FLIGHTLIB)/alarms.c
    SRC += $(FLIGHTLIB)/instrumentation.c
    SRC += $(OPUAVTALK)/uavtalk.c
    SRC += $(OPUAVOBJ)/uavobjectmanager.c
    SRC += $(OPUAVOBJ)/uavobjectpersistence.c
    SRC += $(PIOSCOMMON)/pios_flashfs_logfs.c
    SRC += $(PIOSCOMMON)/pios_flash_jedec.c
    SRC += $(OPUAVOBJ)/eventdispatcher.c

    ## UAVObjects
    SRC += $(FLIGHT_UAVOBJ_DIR)/accessorydesired.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/objectpersistence.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gcstelemetrystats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flighttelemetrystats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/faultsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flightstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemstats.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemalarms.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/systemsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationdesired.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationsettingsbank1.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationsettingsbank2.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationsettingsbank3.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/stabilizationbank.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/actuatorcommand.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/actuatordesired.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/actuatorsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/accelstate.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/accelgyrosettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gyrostate.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/attitudestate.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/manualcontrolcommand.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/watchdogstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/manualcontrolsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/flightmodesettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/mixersettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/firmwareiapobj.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/attitudesettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/camerastabsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/cameradesired.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpspositionsensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpsvelocitysensor.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/gpssettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/hwsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/receiveractivity.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/receiverstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/mixerstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/ratedesired.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/txpidsettings.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/txpidstatus.c
    SRC += $(FLIGHT_UAVOBJ_DIR)/mpugyroaccelsettings.c
    # Command line option for Gcsreceiver module
    ifeq ($(GCSRECEIVER), YES)
        SRC += $(FLIGHT_UAVOBJ_DIR)/gcsreceiver.c
    endif
    # Enable Diag tasks and UAVOs needed
    ifeq ($(DIAG_TASKS), YES)
        CDEFS += -DDIAG_TASKS
        SRC += $(FLIGHT_UAVOBJ_DIR)/taskinfo.c
        SRC += $(FLIGHT_UAVOBJ_DIR)/callbackinfo.c
        SRC += $(FLIGHT_UAVOBJ_DIR)/perfcounter.c
        SRC += $(FLIGHT_UAVOBJ_DIR)/i2cstats.c
    endif
else
    ## Test Code
    SRC += $(OPTESTS)/test_common.c
    SRC += $(OPTESTS)/$(TESTAPP).c
endif

include $(FLIGHT_ROOT_DIR)/make/apps-defs.mk
include $(FLIGHT_ROOT_DIR)/make/common-defs.mk
