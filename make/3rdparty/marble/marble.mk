################################
# Targets to build Marble
#
################################
#
# $ make all_marble
#
################################

MARBLE_NAME_PREFIX :=
MARBLE_NAME_SUFIX := -qt-$(QT_VERSION)

################################
#
# Marble
#
################################

MARBLE_VERSION   := 15.08.1
MARBLE_GIT_BRANCH := v$(MARBLE_VERSION)

MARBLE_BASE_NAME := marble-$(MARBLE_VERSION)
MARBLE_BUILD_CONF := Release

ifeq ($(UNAME), Linux)
	ifeq ($(ARCH), x86_64)
		MARBLE_NAME := $(MARBLE_BASE_NAME)-linux-x64
	else
		MARBLE_NAME := $(MARBLE_BASE_NAME)-linux-x86
	endif
	MARBLE_DATA_BASE_DIR := share/marble/data
	MARBLE_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	MARBLE_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(PATH)
else ifeq ($(UNAME), Darwin)
	MARBLE_NAME := $(MARBLE_BASE_NAME)-clang_64
	MARBLE_DATA_BASE_DIR := share/marble/data
	MARBLE_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	MARBLE_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(PATH)
else ifeq ($(UNAME), Windows)
	MARBLE_NAME := $(MARBLE_BASE_NAME)-$(QT_SDK_ARCH)
	MARBLE_DATA_BASE_DIR := data
	MARBLE_CMAKE_GENERATOR := "MinGW Makefiles"
endif

MARBLE_NAME        := $(MARBLE_NAME_PREFIX)$(MARBLE_NAME)$(MARBLE_NAME_SUFIX)
MARBLE_SRC_DIR     := $(ROOT_DIR)/3rdparty/marble
MARBLE_BUILD_DIR   := $(BUILD_DIR)/3rdparty/$(MARBLE_NAME)
MARBLE_INSTALL_DIR := $(BUILD_DIR)/3rdparty/install/$(MARBLE_NAME)
MARBLE_DATA_DIR    := $(MARBLE_INSTALL_DIR)/$(MARBLE_DATA_BASE_DIR)
MARBLE_PATCH_FILE  := $(ROOT_DIR)/make/3rdparty/marble/marble-$(MARBLE_VERSION).patch

GOOGLE_SAT_PATCH_FILE := $(ROOT_DIR)/make/3rdparty/marble/googlesat.patch

.PHONY: marble
marble:
	@$(ECHO) "Building marble $(call toprel, $(MARBLE_SRC_DIR)) into $(call toprel, $(MARBLE_BUILD_DIR))"
	$(V1) $(MKDIR) -p $(MARBLE_BUILD_DIR)
	$(V1) ( $(CD) $(MARBLE_BUILD_DIR) && \
		if [ -n "$(MARBLE_BUILD_PATH)" ]; then \
			PATH=$(MARBLE_BUILD_PATH) ; \
		fi ; \
		$(CMAKE) -Wno-dev -G $(MARBLE_CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(MARBLE_BUILD_CONF) \
			-DCMAKE_MAKE_PROGRAM=$(MAKE) \
			-DWITH_KF5=FALSE -DQT5BUILD=ON -DBUILD_WITH_DBUS=OFF -DMARBLE_NO_WEBKIT=TRUE -DWITH_DESIGNER_PLUGIN=NO \
			-DCMAKE_INSTALL_PREFIX=$(MARBLE_INSTALL_DIR) $(MARBLE_SRC_DIR) && \
		$(MAKE) && \
		$(MAKE) install ; \
	)
	@$(ECHO) "Copying restricted maps to $(call toprel, $(MARBLE_DATA_DIR))"
	@$(ECHO) "Copying Google Maps"
	$(V1) $(MKDIR) -p $(MARBLE_DATA_DIR)/maps/earth/googlemaps
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlemaps/googlemaps.dgml $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlemaps/preview.png $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlemaps/0 $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	@$(ECHO) "Copying Google Sat"
	$(V1) $(MKDIR) -p $(MARBLE_DATA_DIR)/maps/earth/googlesat
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlesat/googlesat.dgml $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlesat/preview.png $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlesat/0 $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlesat/streets $(MARBLE_DATA_DIR)/maps/earth/googlesat/

.PHONY: package_marble
package_marble:
	@$(ECHO) "Packaging $(call toprel, $(MARBLE_INSTALL_DIR)) into $(notdir $(MARBLE_INSTALL_DIR)).tar"
	#$(V1) $(CP) $(ROOT_DIR)/make/3rdparty/marble/LibrePilotReadme.txt $(MARBLE_INSTALL_DIR)/
	$(V1) ( \
		$(CD) $(MARBLE_INSTALL_DIR)/.. && \
		$(TAR) cf $(notdir $(MARBLE_INSTALL_DIR)).tar $(notdir $(MARBLE_INSTALL_DIR)) && \
		$(ZIP) -f $(notdir $(MARBLE_INSTALL_DIR)).tar && \
		$(call MD5_GEN_TEMPLATE,$(notdir $(MARBLE_INSTALL_DIR)).tar.gz) ; \
	)

.NOTPARALLEL:
.PHONY: prepare_marble
prepare_marble: clone_marble

.PHONY: clone_marble
clone_marble:
	$(V1) if [ ! -d "$(MARBLE_SRC_DIR)" ]; then \
		$(ECHO) "Cloning marble..." ; \
		$(GIT) clone --no-checkout git://anongit.kde.org/marble $(MARBLE_SRC_DIR) ; \
	fi
	@$(ECHO) "Fetching marble..."
	$(V1) ( $(CD) $(MARBLE_SRC_DIR) && $(GIT) fetch ; )
	@$(ECHO) "Checking out marble $(MARBLE_GIT_BRANCH)"
	$(V1) ( $(CD) $(MARBLE_SRC_DIR) && $(GIT) fetch --tags ; )
	$(V1) ( $(CD) $(MARBLE_SRC_DIR) && $(GIT) checkout --quiet --force $(MARBLE_GIT_BRANCH) ; )
	$(V1) if [ -e $(MARBLE_PATCH_FILE) ]; then \
		$(ECHO) "Patching marble..." ; \
		( $(CD) $(MARBLE_SRC_DIR) && $(GIT) apply $(MARBLE_PATCH_FILE) ; ) \
	fi

	$(V1) if [ ! -d "$(MARBLE_SRC_DIR)/googlemaps" ]; then \
		$(ECHO) "Cloning googlemaps to $(call toprel, $(MARBLE_SRC_DIR)/googlemaps)" ; \
		$(GIT) clone https://gitlab.com/marble-restricted-maps/googlemaps.git $(MARBLE_SRC_DIR)/googlemaps ; \
	fi
	@$(ECHO) "Fetching googlemaps..."
	$(V1) ( $(CD) $(MARBLE_SRC_DIR)/googlemaps && $(GIT) fetch ; )
	@$(ECHO) "Checking out googlemaps"
	$(V1) ( $(CD) $(MARBLE_SRC_DIR)/googlemaps && $(GIT) checkout --quiet --force master ; )

	$(V1) if [ ! -d "$(MARBLE_SRC_DIR)/googlesat" ]; then \
		$(ECHO) "Cloning googlesat to $(call toprel, $(MARBLE_SRC_DIR)/googlesat)" ; \
		$(GIT) clone https://gitlab.com/marble-restricted-maps/googlesat.git $(MARBLE_SRC_DIR)/googlesat ; \
	fi
	@$(ECHO) "Fetching googlesat..."
	$(V1) ( $(CD) $(MARBLE_SRC_DIR)/googlesat && $(GIT) fetch ; )
	@$(ECHO) "Checking out googlesat"
	$(V1) ( $(CD) $(MARBLE_SRC_DIR)/googlesat && $(GIT) checkout --quiet --force master ; )
	$(V1) if [ -e $(GOOGLE_SAT_PATCH_FILE) ]; then \
		$(ECHO) "Patching google sat..." ; \
		( $(CD) $(MARBLE_SRC_DIR)/googlesat && $(GIT) apply $(GOOGLE_SAT_PATCH_FILE) ; ) \
	fi

.PHONY: clean_marble
clean_marble:
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_BUILD_DIR))
	$(V1) [ ! -d "$(MARBLE_BUILD_DIR)" ] || $(RM) -r "$(MARBLE_BUILD_DIR)"
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_INSTALL_DIR))
	$(V1) [ ! -d "$(MARBLE_INSTALL_DIR)" ] || $(RM) -r "$(MARBLE_INSTALL_DIR)"

.PHONY: clean_all_marble
clean_all_marble: clean_marble
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_SRC_DIR))
	$(V1) [ ! -d "$(MARBLE_SRC_DIR)" ] || $(RM) -r "$(MARBLE_SRC_DIR)"

.NOTPARALLEL:
.PHONY: all_marble
all_marble: prepare_marble marble package_marble

