#
# Top level Makefile for the LibrePilot Project build system.
# Copyright (c) 2015, The LibrePilot Project, http://www.librepilot.org
# Copyright (c) 2010-2013, The OpenPilot Team, http://www.openpilot.org
# Use 'make help' for instructions.
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

# This top level Makefile passes down some variables to sub-makes through
# the environment. They are explicitly exported using the export keyword.
# Lower level makefiles assume that these variables are defined. To ensure
# that a special magic variable is exported here. It must be checked for
# existance by each sub-make.
export TOP_LEVEL_MAKEFILE := TRUE

# The root directory that this makefile resides in
export ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

# Include some helper functions
include $(ROOT_DIR)/make/functions.mk

# This file can be used to override default options using the "override" keyword
CONFIG_FILE := config
-include $(CONFIG_FILE)

##############################
# It is possible to set DL_DIR and/or TOOLS_DIR environment
# variables to override local tools download and installation directorys. So the
# same toolchains can be used for all working copies. Particularly useful for CI
# server build agents, but also for local installations.
override DL_DIR    := $(if $(DL_DIR),$(call slashfix,$(DL_DIR)),$(ROOT_DIR)/downloads)
override TOOLS_DIR := $(if $(TOOLS_DIR),$(call slashfix,$(TOOLS_DIR)),$(ROOT_DIR)/tools)
export DL_DIR
export TOOLS_DIR

# Set up some macros for common directories within the tree
export BUILD_DIR     := $(CURDIR)/build
export PACKAGE_DIR   := $(BUILD_DIR)/package
export DIST_DIR      := $(BUILD_DIR)/dist
export OPGCSSYNTHDIR := $(BUILD_DIR)/gcs-synthetics

DIRS := $(DL_DIR) $(TOOLS_DIR) $(BUILD_DIR) $(PACKAGE_DIR) $(DIST_DIR) $(OPGCSSYNTHDIR)

# Naming for binaries and packaging etc,.
export ORG_BIG_NAME := LibrePilot
GCS_LABEL := GCS
GCS_BIG_NAME := $(ORG_BIG_NAME) $(GCS_LABEL)
# These should be lowercase with no spaces
export ORG_SMALL_NAME := $(call smallify,$(ORG_BIG_NAME))
GCS_SMALL_NAME := $(call smallify,$(GCS_BIG_NAME))

WEBSITE_URL      := http://librepilot.org
GIT_URL          := https://bitbucket.org/librepilot/librepilot.git
GITWEB_URL       := https://bitbucket.org/librepilot/librepilot
# Change this once the DNS is set to http://wiki.librepilot.org/
WIKI_URL_ROOT    := https://librepilot.atlassian.net/wiki/display/LPDOC/
USAGETRACKER_URL := https://usagetracker.librepilot.org/

PACKAGING_EMAIL_ADDRESS := packaging@librepilot.org

define DESCRIPTION_SHORT :=
A ground control station and firmware for UAV flight controllers
endef

define DESCRIPTION_LONG :=
The LibrePilot open source project was founded in July 2015.
It focuses on research and development of software and hardware to be used in a variety of applications including vehicle control and stabilization, unmanned autonomous vehicles and robotics.
One of the project's primary goals is to provide an open and collaborative environment making it the ideal home for development of innovative ideas.
endef


# Clean out undesirable variables from the environment and command-line
# to remove the chance that they will cause problems with our build
define SANITIZE_VAR
$(if $(filter-out undefined,$(origin $(1))),
    $(info $(EMPTY) NOTE        Sanitized $(2) variable '$(1)' from $(origin $(1)))
    MAKEOVERRIDES = $(filter-out $(1)=%,$(MAKEOVERRIDES))
    override $(1) :=
    unexport $(1)
)
endef

# These specific variables can influence compilation in unexpected (and undesirable) ways
# gcc flags
SANITIZE_GCC_VARS := TMPDIR GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH
# preprocessor flags
SANITIZE_GCC_VARS += CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH OBJC_INCLUDE_PATH DEPENDENCIES_OUTPUT
# make flags
SANITIZE_GCC_VARS += CFLAGS CXXFLAGS CPPFLAGS LDFLAGS LDLIBS
$(foreach var, $(SANITIZE_GCC_VARS), $(eval $(call SANITIZE_VAR,$(var),disallowed)))

# These specific variables used to be valid but now they make no sense
SANITIZE_DEPRECATED_VARS := USE_BOOTLOADER CLEAN_BUILD
$(foreach var, $(SANITIZE_DEPRECATED_VARS), $(eval $(call SANITIZE_VAR,$(var),deprecated)))

# Decide on a verbosity level based on the V= parameter
export AT := @
ifndef V
    export V0    :=
    export V1    := $(AT)
else ifeq ($(V), 0)
    export V0    := $(AT)
    export V1    := $(AT)
else ifeq ($(V), 1)
endif

ARCH := $(call get_arch)

# Include tools installers
include $(ROOT_DIR)/make/tools.mk

# Include third party builders if available
-include $(ROOT_DIR)/make/3rdparty/3rdparty.mk

# We almost need to consider autoconf/automake instead of this
ifeq ($(UNAME), Linux)
    UAVOBJGENERATOR := $(BUILD_DIR)/uavobjgenerator/uavobjgenerator
else ifeq ($(UNAME), Darwin)
    UAVOBJGENERATOR := $(BUILD_DIR)/uavobjgenerator/uavobjgenerator
else ifeq ($(UNAME), Windows)
    UAVOBJGENERATOR := $(BUILD_DIR)/uavobjgenerator/uavobjgenerator.exe
endif

export UAVOBJGENERATOR

# Set up default build configurations (debug | release)
GCS_BUILD_CONF := release

# Set extra configuration
GCS_EXTRA_CONF += osg copy_osg
ifeq ($(UNAME), Windows)
    GCS_EXTRA_CONF += osgearth
endif

##############################
#
# All targets
#
##############################

.PHONY: all
all: uavobjects all_ground all_flight

.PHONY: all_clean
all_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(BUILD_DIR))"
	$(V1) [ ! -d "$(BUILD_DIR)" ] || $(RM) -rf "$(BUILD_DIR)"

.PHONY: clean
clean: all_clean


##############################
#
# UAVObjects
#
##############################

UAVOBJGENERATOR_DIR := $(BUILD_DIR)/uavobjgenerator
DIRS += $(UAVOBJGENERATOR_DIR)

.PHONY: uavobjgenerator
uavobjgenerator: $(UAVOBJGENERATOR)

$(UAVOBJGENERATOR): | $(UAVOBJGENERATOR_DIR)
	$(V1) cd $(UAVOBJGENERATOR_DIR) && \
	    ( [ -f Makefile ] || $(QMAKE) $(ROOT_DIR)/ground/uavobjgenerator/uavobjgenerator.pro \
	    CONFIG+='$(GCS_BUILD_CONF) $(GCS_EXTRA_CONF)' ) && \
	    $(MAKE) --no-print-directory -w

UAVOBJ_TARGETS := gcs flight python matlab java wireshark

.PHONY: uavobjects
uavobjects:  $(addprefix uavobjects_, $(UAVOBJ_TARGETS))

UAVOBJ_XML_DIR := $(ROOT_DIR)/shared/uavobjectdefinition
UAVOBJ_OUT_DIR := $(BUILD_DIR)/uavobject-synthetics

uavobjects_%: $(UAVOBJGENERATOR)
	@$(MKDIR) -p $(UAVOBJ_OUT_DIR)/$*
	$(V1) ( cd $(UAVOBJ_OUT_DIR)/$* && \
	    $(UAVOBJGENERATOR) -$* $(UAVOBJ_XML_DIR) $(ROOT_DIR) ; \
	)

uavobjects_test: $(UAVOBJGENERATOR)
	$(V1) $(UAVOBJGENERATOR) -v $(UAVOBJ_XML_DIR) $(ROOT_DIR)

uavobjects_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(UAVOBJ_OUT_DIR))"
	$(V1) [ ! -d "$(UAVOBJ_OUT_DIR)" ] || $(RM) -r "$(UAVOBJ_OUT_DIR)"

##############################
#
# Flight related components
#
##############################


# When building any of the "all_*" targets, tell all sub makefiles to display
# additional details on each line of output to describe which build and target
# that each line applies to. The same applies also to all, opfw_resource,
# package targets
ifneq ($(strip $(filter all_% all opfw_resource package,$(MAKECMDGOALS))),)
    export ENABLE_MSG_EXTRA := yes
endif

# When building more than one goal in a single make invocation, also
# enable the extra context for each output line
ifneq ($(word 2,$(MAKECMDGOALS)),)
    export ENABLE_MSG_EXTRA := yes
endif

FLIGHT_OUT_DIR := $(BUILD_DIR)/firmware
DIRS += $(FLIGHT_OUT_DIR)

# Might not be here in source package
-include $(ROOT_DIR)/flight/Makefile

##############################
#
# GCS related components
#
##############################

.PHONY: all_ground
all_ground: gcs uploader

ifneq ($(V), 1)
    GCS_EXTRA_CONF += silent
endif

GCS_DIR := $(BUILD_DIR)/$(GCS_SMALL_NAME)_$(GCS_BUILD_CONF)
DIRS += $(GCS_DIR)

GCS_MAKEFILE := $(GCS_DIR)/Makefile

.PHONY: gcs_qmake
gcs_qmake $(GCS_MAKEFILE): | $(GCS_DIR)
	$(V1) cd $(GCS_DIR) && \
	    $(QMAKE) $(ROOT_DIR)/ground/gcs/gcs.pro \
	    -r CONFIG+='$(GCS_BUILD_CONF) $(GCS_EXTRA_CONF)' \
	    'GCS_BIG_NAME="$(GCS_BIG_NAME)"' GCS_SMALL_NAME=$(GCS_SMALL_NAME) \
	    'ORG_BIG_NAME="$(ORG_BIG_NAME)"' ORG_SMALL_NAME=$(ORG_SMALL_NAME) \
	    'WIKI_URL_ROOT="$(WIKI_URL_ROOT)"' \
	    'USAGETRACKER_URL="$(USAGETRACKER_URL)"' \
	    'GCS_LIBRARY_BASENAME=$(libbasename)' \
	    $(GCS_QMAKE_OPTS)

.PHONY: gcs
gcs: $(UAVOBJGENERATOR) $(GCS_MAKEFILE)
	$(V1) $(MAKE) -w -C $(GCS_DIR)/$(MAKE_DIR);

.PHONY: gcs_clean
gcs_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(GCS_DIR))"
	$(V1) [ ! -d "$(GCS_DIR)" ] || $(RM) -r "$(GCS_DIR)"



################################
#
# Serial Uploader tool
#
################################

UPLOADER_DIR := $(BUILD_DIR)/uploader_$(GCS_BUILD_CONF)
DIRS += $(UPLOADER_DIR)

UPLOADER_MAKEFILE := $(UPLOADER_DIR)/Makefile

.PHONY: uploader_qmake
uploader_qmake $(UPLOADER_MAKEFILE): | $(UPLOADER_DIR)
	$(V1) cd $(UPLOADER_DIR) && \
	    $(QMAKE) $(ROOT_DIR)/ground/gcs/src/experimental/USB_UPLOAD_TOOL/upload.pro \
	    -r CONFIG+='$(GCS_BUILD_CONF) $(GCS_EXTRA_CONF)' $(GCS_QMAKE_OPTS)

.PHONY: uploader
uploader: $(UPLOADER_MAKEFILE)
	$(V1) $(MAKE) -w -C $(UPLOADER_DIR)

.PHONY: uploader_clean
uploader_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(UPLOADER_DIR))"
	$(V1) [ ! -d "$(UPLOADER_DIR)" ] || $(RM) -r "$(UPLOADER_DIR)"



##############################
#
# Packaging components
#
##############################
# Firmware files to package
PACKAGE_FW_TARGETS := fw_coptercontrol fw_oplinkmini fw_revolution fw_osd fw_revoproto fw_gpsplatinum fw_revonano fw_sparky2

# Rules to generate GCS resources used to embed firmware binaries into the GCS.
# They are used later by the vehicle setup wizard to update board firmware.
# To open a firmware image use ":/firmware/fw_coptercontrol.opfw"
OPFW_RESOURCE := $(OPGCSSYNTHDIR)/opfw_resource.qrc

ifeq ($(WITH_PREBUILT_FW),)
FIRMWARE_DIR := $(FLIGHT_OUT_DIR)
# We need to build the FW targets
$(OPFW_RESOURCE): $(PACKAGE_FW_TARGETS)
else
FIRMWARE_DIR := $(WITH_PREBUILT_FW)
endif

OPFW_FILES := $(foreach fw_targ, $(PACKAGE_FW_TARGETS), $(FIRMWARE_DIR)/$(fw_targ)/$(fw_targ).opfw)
OPFW_CONTENTS := \
<!DOCTYPE RCC><RCC version="1.0"> \
    <qresource prefix="/firmware"> \
        $(foreach fw_file, $(OPFW_FILES), <file alias="$(notdir $(fw_file))">$(call system_path,$(fw_file))</file>) \
    </qresource> \
</RCC>

.PHONY: opfw_resource
opfw_resource: $(OPFW_RESOURCE)

$(OPFW_RESOURCE): | $(OPGCSSYNTHDIR)
	@$(ECHO) Generating OPFW resource file $(call toprel, $@)
	$(V1) $(ECHO) $(QUOTE)$(OPFW_CONTENTS)$(QUOTE) > $@

# If opfw_resource or all firmware are requested, GCS should depend on the resource
ifneq ($(strip $(filter opfw_resource all all_fw all_flight package,$(MAKECMDGOALS))),)
$(GCS_MAKEFILE): $(OPFW_RESOURCE)
endif

# Packaging targets: package
#  - builds all firmware, opfw_resource, gcs
#  - copies firmware into a package directory
#  - calls paltform-specific packaging script

# Define some variables
PACKAGE_LBL       := $(shell $(VERSION_INFO) --format=\$${LABEL})
PACKAGE_NAME      := $(subst $(SPACE),,$(ORG_BIG_NAME))
PACKAGE_SEP       := -
PACKAGE_FULL_NAME := $(PACKAGE_NAME)$(PACKAGE_SEP)$(PACKAGE_LBL)

# Source distribution is never dirty because it uses git archive
DIST_LBL       := $(subst -dirty,,$(PACKAGE_LBL))
DIST_NAME      := $(PACKAGE_NAME)$(PACKAGE_SEP)$(DIST_LBL)
DIST_TAR       := $(DIST_DIR)/$(DIST_NAME).tar
DIST_TAR_GZ    := $(DIST_TAR).gz
FW_DIST_NAME   := $(DIST_NAME)_firmware
FW_DIST_TAR    := $(DIST_DIR)/$(FW_DIST_NAME).tar
FW_DIST_TAR_GZ := $(FW_DIST_TAR).gz
DIST_VER_INFO  := $(DIST_DIR)/version-info.json

include $(ROOT_DIR)/package/$(UNAME).mk

##############################
#
# Source for distribution
#
##############################
FORCE:

$(DIST_VER_INFO): FORCE | $(DIST_DIR)
	$(V1) $(VERSION_INFO) --jsonpath="$(DIST_DIR)"

$(DIST_TAR): $(DIST_VER_INFO) | $(DIST_DIR)
	@$(ECHO) " SOURCE FOR DISTRIBUTION $(call toprel, $(DIST_TAR))"
	$(V1) git archive --prefix="$(PACKAGE_NAME)/" -o "$(DIST_TAR)" HEAD
	$(V1) tar --append --file="$(DIST_TAR)" \
		--owner=root --group=root --mtime="`git show -s --format=%ci`" \
		--transform='s,.*version-info.json,$(PACKAGE_NAME)/version-info.json,' \
		$(call toprel, "$(DIST_VER_INFO)")

$(DIST_TAR_GZ): $(DIST_TAR)
	@$(ECHO) " SOURCE FOR DISTRIBUTION $(call toprel, $(DIST_TAR_GZ))"
	$(V1) gzip -knf "$(DIST_TAR)"

.PHONY: dist_tar_gz
dist_tar_gz: $(DIST_TAR_GZ)

.PHONY: dist
dist: dist_tar_gz


$(FW_DIST_TAR): $(PACKAGE_FW_TARGETS) | $(DIST_DIR)
	@$(ECHO) " FIRMWARE FOR DISTRIBUTION $(call toprel, $(FW_DIST_TAR))"
	$(V1) tar -c --file="$(FW_DIST_TAR)" --directory=$(FLIGHT_OUT_DIR) \
		--owner=root --group=root --mtime="`git show -s --format=%ci`" \
		--transform='s,^,firmware/,' \
		$(foreach fw_targ,$(PACKAGE_FW_TARGETS),$(fw_targ)/$(fw_targ).opfw)

$(FW_DIST_TAR_GZ): $(FW_DIST_TAR)
	@$(ECHO) " FIRMWARE FOR DISTRIBUTION $(call toprel, $(FW_DIST_TAR_GZ))"
	$(V1) gzip -knf "$(FW_DIST_TAR)"

.PHONY: fw_dist_tar_gz
fw_dist_tar_gz: $(FW_DIST_TAR_GZ)

.PHONY: fw_dist
fw_dist: fw_dist_tar_gz


##############################
#
# Source code formatting
#
##############################

UNCRUSTIFY_TARGETS := flight ground

# $(1) = Uncrustify target (e.g flight or ground)
# $(2) = Target root directory
define UNCRUSTIFY_TEMPLATE

.PHONY: uncrustify_$(1)
uncrustify_$(1):
	@$(ECHO) "Auto-formatting $(1) source code"
	$(V1) UNCRUSTIFY_CONFIG="$(ROOT_DIR)/make/uncrustify/uncrustify.cfg" $(SHELL) make/scripts/uncrustify.sh $(call toprel, $(2))

.PHONY: pretty_$(1)
pretty_$(1): uncrustify_$(1)
endef

$(foreach uncrustify_targ, $(UNCRUSTIFY_TARGETS), $(eval $(call UNCRUSTIFY_TEMPLATE,$(uncrustify_targ),$(ROOT_DIR)/$(uncrustify_targ))))

.PHONY: uncrustify_all
uncrustify_all: $(addprefix uncrustify_,$(UNCRUSTIFY_TARGETS))

.PHONY: pretty
pretty: $(addprefix pretty_,$(UNCRUSTIFY_TARGETS))

##############################
#
# Doxygen documentation
#
# Each target should have own Doxyfile.$(target) with build directory build/docs/$(target),
# proper source directory (e.g. $(target)) and appropriate other doxygen options.
#
##############################

DOCS_TARGETS := flight ground uavobjects

# $(1) = Doxygen target (e.g flight or ground)
define DOXYGEN_TEMPLATE

.PHONY: docs_$(1)
docs_$(1): docs_$(1)_clean
	@$(ECHO) "Generating $(1) documentation"
	$(V1) $(MKDIR) -p $(BUILD_DIR)/docs/$(1)
	$(V1) $(DOXYGEN) $(ROOT_DIR)/make/doxygen/Doxyfile.$(1)

.PHONY: docs_$(1)_clean
docs_$(1)_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(BUILD_DIR)/docs/$(1))"
	$(V1) [ ! -d "$(BUILD_DIR)/docs/$(1)" ] || $(RM) -r "$(BUILD_DIR)/docs/$(1)"

endef

$(foreach docs_targ, $(DOCS_TARGETS), $(eval $(call DOXYGEN_TEMPLATE,$(docs_targ))))

.PHONY: docs_all
docs_all: $(addprefix docs_,$(DOCS_TARGETS))

.PHONY: docs_all_clean
docs_all_clean:
	@$(ECHO) " CLEAN      $(call toprel, $(BUILD_DIR)/docs)"
	$(V1) [ ! -d "$(BUILD_DIR)/docs" ] || $(RM) -rf "$(BUILD_DIR)/docs"

##############################
#
# Build info
#
##############################

.PHONY: build-info
build-info: | $(BUILD_DIR)
	@$(ECHO) " BUILD-INFO $(call toprel, $(BUILD_DIR)/$@.txt)"
	$(V1) $(VERSION_INFO) \
		--uavodir=$(ROOT_DIR)/shared/uavobjectdefinition \
		--template="make/templates/$@.txt" \
		--outfile="$(BUILD_DIR)/$@.txt"

##############################
#
# Config
#
##############################

CONFIG_OPTS := $(subst \$(SPACE),%SPACE_PLACEHOLDER%,$(MAKEOVERRIDES))
CONFIG_OPTS := $(addprefix override%SPACE_PLACEHOLDER%,$(CONFIG_OPTS))
CONFIG_OPTS := $(subst $(SPACE),\n,$(CONFIG_OPTS))\n
CONFIG_OPTS := $(subst %SPACE_PLACEHOLDER%,$(SPACE),$(CONFIG_OPTS))

.PHONY: config_new
config_new:
	@printf '$(CONFIG_OPTS)' > $(CONFIG_FILE)

.PHONY: config_append
config_append:
	@printf '$(CONFIG_OPTS)' >> $(CONFIG_FILE)

.PHONY: config_show
config_show:
	@cat $(CONFIG_FILE)

.PHONY: config_clean
config_clean:
	rm -f $(CONFIG_FILE)


##############################
#
# Directories
#
##############################

$(DIRS):
	$(V1) $(MKDIR) -p $@


##############################
#
# Help message, the default Makefile goal
#
##############################

.DEFAULT_GOAL := help

.PHONY: help
help:
	@$(ECHO)
	@$(ECHO) "   This Makefile is known to work on Linux and Mac in a standard shell environment."
	@$(ECHO) "   It also works on Windows by following the instructions given on this wiki page:"
	@$(ECHO) "       $(WIKI_ROOT_URL)Windows+Building+and+Packaging"
	@$(ECHO)
	@$(ECHO) "   Here is a summary of the available targets:"
	@$(ECHO)
	@$(ECHO) "   [Source tree preparation]"
	@$(ECHO) "     prepare              - Install GIT commit message template"
	@$(ECHO) "   [Tool Installers]"
	@$(ECHO) "     arm_sdk_install      - Install the GNU ARM gcc toolchain"
	@$(ECHO) "     qt_sdk_install       - Install the QT development tools"
	@$(ECHO) "     nsis_install         - Install the NSIS Unicode (Windows only)"
	@$(ECHO) "     mesawin_install      - Install the OpenGL32 DLL (Windows only)"
	@$(ECHO) "     uncrustify_install   - Install the Uncrustify source code beautifier"
	@$(ECHO) "     doxygen_install      - Install the Doxygen documentation generator"
	@$(ECHO) "     gtest_install        - Install the GoogleTest framework"
	@$(ECHO) "     ccache_install       - Install ccache"
	@$(ECHO) "   These targets are not updated yet and are probably broken:"
	@$(ECHO) "     openocd_install      - Install the OpenOCD JTAG daemon"
	@$(ECHO) "     stm32flash_install   - Install the stm32flash tool for unbricking F1-based boards"
	@$(ECHO) "     dfuutil_install      - Install the dfu-util tool for unbricking F4-based boards"
	@$(ECHO) "   Install all available tools:"
	@$(ECHO) "     all_sdk_install      - Install all of above (platform-dependent)"
	@$(ECHO) "     build_sdk_install    - Install only essential for build tools (platform-dependent)"
	@$(ECHO)
	@$(ECHO) "   Other tool options are:"
	@$(ECHO) "     <tool>_version       - Display <tool> version"
	@$(ECHO) "     <tool>_clean         - Remove installed <tool>"
	@$(ECHO) "     <tool>_distclean     - Remove downloaded <tool> distribution file(s)"
	@$(ECHO)
	@$(ECHO) "   [Big Hammer]"
	@$(ECHO) "     all                  - Generate UAVObjects, build $(ORG_BIG_NAME) firmware and gcs"
	@$(ECHO) "     all_flight           - Build all firmware, bootloaders and bootloader updaters"
	@$(ECHO) "     all_fw               - Build only firmware for all boards"
	@$(ECHO) "     all_bl               - Build only bootloaders for all boards"
	@$(ECHO) "     all_bu               - Build only bootloader updaters for all boards"
	@$(ECHO)
	@$(ECHO) "     all_clean            - Remove your build directory ($(BUILD_DIR))"
	@$(ECHO) "     all_flight_clean     - Remove all firmware, bootloaders and bootloader updaters"
	@$(ECHO) "     all_fw_clean         - Remove firmware for all boards"
	@$(ECHO) "     all_bl_clean         - Remove bootloaders for all boards"
	@$(ECHO) "     all_bu_clean         - Remove bootloader updaters for all boards"
	@$(ECHO)
	@$(ECHO) "     all_<board>          - Build all available images for <board>"
	@$(ECHO) "     all_<board>_clean    - Remove all available images for <board>"
	@$(ECHO)
	@$(ECHO) "     all_ut               - Build all unit tests"
	@$(ECHO) "     all_ut_tap           - Run all unit tests and capture all TAP output to files"
	@$(ECHO) "     all_ut_run           - Run all unit tests and dump TAP output to console"
	@$(ECHO)
	@$(ECHO) "   [Firmware]"
	@$(ECHO) "     <board>              - Build firmware for <board>"
	@$(ECHO) "                            Supported boards are ($(ALL_BOARDS))"
	@$(ECHO) "     fw_<board>           - Build firmware for <board>"
	@$(ECHO) "                            Supported boards are ($(FW_BOARDS))"
	@$(ECHO) "     fw_<board>_clean     - Remove firmware for <board>"
	@$(ECHO) "     fw_<board>_program   - Use OpenOCD + JTAG to write firmware to <board>"
	@$(ECHO)
	@$(ECHO) "   [Bootloader]"
	@$(ECHO) "     bl_<board>           - Build bootloader for <board>"
	@$(ECHO) "                            Supported boards are ($(BL_BOARDS))"
	@$(ECHO) "     bl_<board>_clean     - Remove bootloader for <board>"
	@$(ECHO) "     bl_<board>_program   - Use OpenOCD + JTAG to write bootloader to <board>"
	@$(ECHO)
	@$(ECHO) "   [Entire Flash]"
	@$(ECHO) "     ef_<board>           - Build entire flash image for <board>"
	@$(ECHO) "                            Supported boards are ($(EF_BOARDS))"
	@$(ECHO) "     ef_<board>_clean     - Remove entire flash image for <board>"
	@$(ECHO) "     ef_<board>_program   - Use OpenOCD + JTAG to write entire flash image to <board>"
	@$(ECHO)
	@$(ECHO) "   [Bootloader Updater]"
	@$(ECHO) "     bu_<board>           - Build bootloader updater for <board>"
	@$(ECHO) "                            Supported boards are ($(BU_BOARDS))"
	@$(ECHO) "     bu_<board>_clean     - Remove bootloader updater for <board>"
	@$(ECHO)
	@$(ECHO) "   [Unbrick a board]"
	@$(ECHO) "     unbrick_<board>      - Use the STM32's built in boot ROM to write a bootloader to <board>"
	@$(ECHO) "                            Supported boards are ($(BL_BOARDS))"
	@$(ECHO) "   [Unittests]"
	@$(ECHO) "     ut_<test>            - Build unit test <test>"
	@$(ECHO) "     ut_<test>_xml        - Run test and capture XML output into a file"
	@$(ECHO) "     ut_<test>_run        - Run test and dump output to console"
	@$(ECHO)
	@$(ECHO) "   [Simulation]"
	@$(ECHO) "     sim_osx              - Build $(ORG_BIG_NAME) simulation firmware for OSX"
	@$(ECHO) "     sim_osx_clean        - Delete all build output for the osx simulation"
	@$(ECHO) "     sim_win32            - Build $(ORG_BIG_NAME) simulation firmware for Windows"
	@$(ECHO) "                            using mingw and msys"
	@$(ECHO) "     sim_win32_clean      - Delete all build output for the win32 simulation"
	@$(ECHO)
	@$(ECHO) "   [GCS]"
	@$(ECHO) "     gcs                  - Build the Ground Control System (GCS) application (debug|release)"
	@$(ECHO) "                            Compile specific directory: MAKE_DIR=<dir>"
	@$(ECHO) "                            Example: make gcs MAKE_DIR=src/plugins/coreplugin"
	@$(ECHO) "     gcs_qmake            - Run qmake for the Ground Control System (GCS) application (debug|release)"
	@$(ECHO) "     gcs_clean            - Remove the Ground Control System (GCS) application (debug|release)"
	@$(ECHO) "                            Supported build configurations: GCS_BUILD_CONF=debug|release (default is $(GCS_BUILD_CONF))"
	@$(ECHO)
	@$(ECHO) "   [Uploader Tool]"
	@$(ECHO) "     uploader             - Build the serial uploader tool (debug|release)"
	@$(ECHO) "     uploader_qmake       - Run qmake for the serial uploader tool (debug|release)"
	@$(ECHO) "     uploader_clean       - Remove the serial uploader tool (debug|release)"
	@$(ECHO) "                            Supported build configurations: GCS_BUILD_CONF=debug|release (default is $(GCS_BUILD_CONF))"
	@$(ECHO)
	@$(ECHO)
	@$(ECHO) "   [UAVObjects]"
	@$(ECHO) "     uavobjects           - Generate source files from the UAVObject definition XML files"
	@$(ECHO) "     uavobjects_test      - Parse xml-files - check for valid, duplicate ObjId's, ..."
	@$(ECHO) "     uavobjects_<group>   - Generate source files from a subset of the UAVObject definition XML files"
	@$(ECHO) "                            Supported groups are ($(UAVOBJ_TARGETS))"
	@$(ECHO)
	@$(ECHO) "   [Packaging]"
	@$(ECHO) "     package              - Build and package the platform-dependent package (no clean)"
	@$(ECHO) "     opfw_resource        - Generate resources to embed firmware binaries into the GCS"
	@$(ECHO) "     dist                 - Generate source archive for distribution"
	@$(ECHO) "     fw_dist              - Generate archive of firmware"
	@$(ECHO) "     install              - Install GCS to \"DESTDIR\" with prefix \"prefix\" (Linux only)"
	@$(ECHO)
	@$(ECHO) "   [Code Formatting]"
	@$(ECHO) "     pretty_<source>      - Reformat <source> code according to the project's standards"
	@$(ECHO) "                            Supported sources are ($(UNCRUSTIFY_TARGETS))"
	@$(ECHO) "     pretty               - Reformat all source code"
	@$(ECHO)
	@$(ECHO) "   [Code Documentation]"
	@$(ECHO) "     docs_<source>        - Generate HTML documentation for <source>"
	@$(ECHO) "                            Supported sources are ($(DOCS_TARGETS))"
	@$(ECHO) "     docs_all             - Generate HTML documentation for all"
	@$(ECHO) "     docs_<source>_clean  - Delete generated documentation for <source>"
	@$(ECHO) "     docs_all_clean       - Delete all generated documentation"
	@$(ECHO)
	@$(ECHO) "   [Configuration]"
	@$(ECHO) "     config_new           - Place your make arguments in the config file"
	@$(ECHO) "     config_append        - Place your make arguments in the config file but append"
	@$(ECHO) "     config_clean         - Removes the config file"
	@$(ECHO)
	@$(ECHO) "   Hint: Add V=1 to your command line to see verbose build output."
	@$(ECHO)
	@$(ECHO) "  Notes: All tool distribution files will be downloaded into $(DL_DIR)"
	@$(ECHO) "         All tools will be installed into $(TOOLS_DIR)"
	@$(ECHO) "         All build output will be placed in $(BUILD_DIR)"
	@$(ECHO)
	@$(ECHO) "  Tool download and install directories can be changed using environment variables:"
	@$(ECHO) "         DL_DIR        full path to downloads directory [downloads if not set]"
	@$(ECHO) "         TOOLS_DIR     full path to installed tools directory [tools if not set]"
	@$(ECHO) "  More info: $(WIKI_URL_ROOT)LibrePilot+Build+System+Overview"
	@$(ECHO)
