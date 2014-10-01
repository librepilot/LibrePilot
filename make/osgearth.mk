################################
# Targets to build osg and osgearth
#
################################
# Prerequisites:
################################
#
# Install development libraries for:
# - libtiff
# - curl
# - gdal, ...
#
# $ sudo apt-get install libcurl4-openssl-dev libgdal-dev libtiff5-dev
#
# $ curl --version
# curl 7.35.0 (i686-pc-linux-gnu) libcurl/7.35.0 OpenSSL/1.0.1f zlib/1.2.8 libidn/1.28 librtmp/2.3
# Protocols: dict file ftp ftps gopher http https imap imaps ldap ldaps pop3 pop3s rtmp rtsp smtp smtps telnet tftp
# Features: AsynchDNS GSS-Negotiate IDN IPv6 Largefile NTLM NTLM_WB SSL libz TLS-SRP
#
# $ gdal-config --version
# 1.10.1
#
################################
# Building:
################################
#
# $ make all_osg
#
# This will:
# - clone the git repositories into the ./thirdparty directory
# - build osg in the build directory, building steps are : cmake, make, make install
# - osg is installed in the OP tools directory
# - distribution files are created in the build directory
# - TODO: distribution files are uploaded to the OP wiki download page
#
################################
# Todo:
# - install osgearth in osg (a minor issue in the osgearth cmake file prevents
#   easy to fix then set INSTALL_TO_OSG_DIR=ON when running cmake on osgearth
# - don't build osg deprecated code (if we don't use it...)
# - add targets to publish distribution files to OP wiki.
# - provide complete list of dependencies for osg and osgearth (current list is most probably incomplete as I already had some stuff installed)
#
################################

################################
#
# osg
#
################################

OSG_BUILD_CONF  := Release
OSG_VERSION	 := 3.2.1
OSG_GIT_BRANCH  := OpenSceneGraph-$(OSG_VERSION)
OSG_SRC_DIR	 := $(ROOT_DIR)/thirdparty/osg-$(OSG_VERSION)
OSG_BUILD_DIR   := $(BUILD_DIR)/thirdparty/osg-$(OSG_VERSION)-$(ARCH)
OSG_INSTALL_DIR := $(BUILD_DIR)/thirdparty/install/osg-$(OSG_VERSION)-$(ARCH)
OSG_TAR_FILE	:= $(BUILD_DIR)/thirdparty/osg-$(OSG_VERSION)-$(ARCH).tar

ifeq ($(UNAME), Windows)
	OSG_CMAKE_GENERATOR := "MinGW Makefiles"
	# CMake is quite picky about its PATH and will complain if sh.exe is found in it
	OSG_BUILD_PATH := "$(TOOLS_DIR)/bin;$(QT_SDK_PREFIX)/bin;$(MINGW_DIR)/bin;$$SYSTEMROOT/System32"
else
	OSG_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	OSG_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(PATH)
endif

.PHONY: osg
osg:
	@$(ECHO) "Building osg $(call toprel, $(OSG_SRC_DIR)) into $(call toprel, $(OSG_BUILD_DIR))"
	$(V1) $(MKDIR) -p $(OSG_BUILD_DIR)
	$(V1) ( $(CD) $(OSG_BUILD_DIR) && \
		PATH=$(OSG_BUILD_PATH) && \
		$(CMAKE) -G $(OSG_CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(OSG_BUILD_CONF) \
			-DBUILD_OPENTHREADS_WITH_QT=OFF \
			-DOSG_PLUGIN_SEARCH_INSTALL_DIR_FOR_PLUGINS=OFF \
			-DCMAKE_INSTALL_PREFIX=$(OSG_INSTALL_DIR) $(OSG_SRC_DIR) && \
		$(MAKE) && \
		$(MAKE) install ; \
	)
	$(V1) $(TAR) cvf $(OSG_TAR_FILE) $(OSG_INSTALL_DIR)/*
	$(V1) $(ZIP) -f $(OSG_TAR_FILE)
	$(V1) $(call MD5_GEN_TEMPLATE,$(OSG_TAR_FILE).gz)

.NOTPARALLEL:
.PHONY: prepare_osg
prepare_osg: clone_osg

.PHONY: clone_osg
clone_osg:
	$(V1) $(MKDIR) -p $(OSG_SRC_DIR)
	@$(ECHO) "Cloning osg to $(call toprel, $(OSG_SRC_DIR))"
	$(V1) $(GIT) clone git://github.com/openscenegraph/osg.git $(OSG_SRC_DIR)
	@$(ECHO) "Checking out osg branch $(OSG_GIT_BRANCH)"
	$(V1) ( $(CD) $(OSG_SRC_DIR) && \
		$(GIT) checkout $(OSG_GIT_BRANCH) ; \
	)

.PHONY: clean_osg
clean_osg:
	@$(ECHO) " CLEAN	  $(call toprel, $(OSG_BUILD_DIR))"
	$(V1) [ ! -d "$(OSG_BUILD_DIR)" ] || $(RM) -r "$(OSG_BUILD_DIR)"
	@$(ECHO) " CLEAN	  $(call toprel, $(OSG_SDK_DIR))"
	$(V1) [ ! -d "$(OSG_SDK_DIR)" ] || $(RM) -r "$(OSG_SDK_DIR)"

.PHONY: clean_all_osg
clean_all_osg: clean_osg
	@$(ECHO) " CLEAN	  $(call toprel, $(OSG_SRC_DIR))"
	$(V1) [ ! -d "$(OSG_SRC_DIR)" ] || $(RM) -r "$(OSG_SRC_DIR)"


################################
#
# osgEarth
#
################################

OSGEARTH_BUILD_CONF  := Release
OSGEARTH_VERSION	 := f5fb2ae
OSGEARTH_GIT_BRANCH  := f5fb2ae
OSGEARTH_SRC_DIR	 := $(ROOT_DIR)/thirdparty/osgearth-$(OSGEARTH_VERSION)
OSGEARTH_BUILD_DIR   := $(BUILD_DIR)/thirdparty/osgearth-$(OSGEARTH_VERSION)-$(ARCH)
OSGEARTH_INSTALL_DIR := $(BUILD_DIR)/thirdparty/install/osgearth-$(OSGEARTH_VERSION)-$(ARCH)
OSGEARTH_TAR_FILE	:= $(BUILD_DIR)/thirdparty/osgearth-$(OSGEARTH_VERSION)-$(ARCH).tar

ifeq ($(UNAME), Windows)
	OSGEARTH_CMAKE_GENERATOR := "MinGW Makefiles"
	# CMake is quite picky about its PATH and will complain if sh.exe is found in it
	OSGEARTH_BUILD_PATH := "$(TOOLS_DIR)/bin;$(QT_SDK_PREFIX)/bin;$(MINGW_DIR)/bin;$(OSG_INSTALL_DIR)/bin;$$SYSTEMROOT/System32"
else
	OSGEARTH_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	OSGEARTH_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(OSG_INSTALL_DIR)/bin:$(PATH)
endif

.PHONY: osgearth
osgearth:
	@$(ECHO) "Building osgEarth $(call toprel, $(OSGEARTH_SRC_DIR)) into $(call toprel, $(OSGEARTH_BUILD_DIR))"
	$(V1) $(MKDIR) -p $(OSGEARTH_BUILD_DIR)
	$(V1) ( $(CD) $(OSGEARTH_BUILD_DIR) && \
		PATH=$(OSGEARTH_BUILD_PATH) && \
		$(CMAKE) -G $(OSGEARTH_CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(OSGEARTH_BUILD_CONF) \
			-DOSG_DIR=$(OSG_INSTALL_DIR) -DINSTALL_TO_OSG_DIR=OFF \
			-DCMAKE_INCLUDE_PATH=$(OSG_INSTALL_DIR)/include \
			-DCMAKE_LIBRARY_PATH=$(OSG_INSTALL_DIR)/lib \
						-DCMAKE_INSTALL_PREFIX=$(OSGEARTH_INSTALL_DIR) $(OSGEARTH_SRC_DIR) && \
		$(MAKE) && \
		$(MAKE) install ; \
	)
	$(V1) $(TAR) cvf $(OSGEARTH_TAR_FILE) $(OSGEARTH_INSTALL_DIR)/*
	$(V1) $(ZIP) -f $(OSGEARTH_TAR_FILE)
	$(call MD5_GEN_TEMPLATE,$(OSGEARTH_TAR_FILE).gz)

.NOTPARALLEL:
.PHONY: prepare_osgearth
prepare_osgearth: clone_osgearth

.PHONY: clone_osgearth
clone_osgearth:
	$(V1) $(MKDIR) -p $(OSGEARTH_SRC_DIR)
	@$(ECHO) "Cloning osgearth to $(call toprel, $(OSGEARTH_SRC_DIR))"
	$(V1) $(GIT) clone git://github.com/gwaldron/osgearth.git $(OSGEARTH_SRC_DIR)
	@$(ECHO) "Checking out osgearth branch $(OSGEARTH_GIT_BRANCH)"
	$(V1) ( $(CD) $(OSGEARTH_SRC_DIR) && \
		$(GIT) checkout $(OSGEARTH_GIT_BRANCH) ; \
	)

.PHONY: clean_osgearth
clean_osgearth:
	@$(ECHO) " CLEAN	  $(call toprel, $(OSGEARTH_BUILD_DIR))"
	$(V1) [ ! -d "$(OSGEARTH_BUILD_DIR)" ] || $(RM) -r "$(OSGEARTH_BUILD_DIR)"
	@$(ECHO) " CLEAN	  $(call toprel, $(OSGEARTH_SDK_DIR))"
	$(V1) [ ! -d "$(OSGEARTH_SDK_DIR)" ] || $(RM) -r "$(OSGEARTH_SDK_DIR)"

.PHONY: clean_all_osgearth
clean_all_osgearth: clean_osgearth
	@$(ECHO) " CLEAN	  $(call toprel, $(OSGEARTH_SRC_DIR))"
	$(V1) [ ! -d "$(OSGEARTH_SRC_DIR)" ] || $(RM) -r "$(OSGEARTH_SRC_DIR)"

################################
#
# all
#
################################

.NOTPARALLEL:
.PHONY: all_osg
all_osg: prepare_osg prepare_osgearth osg osgearth

