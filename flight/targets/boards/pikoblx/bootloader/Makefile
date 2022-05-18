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
include $(FLIGHT_ROOT_DIR)/make/boot-defs.mk
include $(FLIGHT_ROOT_DIR)/make/common-defs.mk

$(info Making bootloader for PikoBLX, board revision $(BOARD_REVISION))
