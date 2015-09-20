#
# Linux-specific packaging script
#

ifndef TOP_LEVEL_MAKEFILE
    $(error Top level Makefile must be used to build this target)
endif

# Are we using a debian based distro?
ifneq ($(wildcard /etc/apt/sources.list),)
	include $(ROOT_DIR)/package/linux/deb.mk
# Are we using a rpm based distro?
else ifneq ($(wildcard /etc/yum.repos.d/*),)
	include $(ROOT_DIR)/package/linux/rpm.mk
# Are we using an Arch based distro?
else ifneq ($(wildcard /etc/pacman.conf),)
    $(info TODO: built in arch package)
endif

##############################
#
# Install Linux Target
#
##############################
enable-udev-rules := no

prefix       := /usr/local
bindir       := $(prefix)/bin
libbasename  := lib
libdir       := $(prefix)/$(libbasename)
datadir      := $(prefix)/share
udevrulesdir := /etc/udev/rules.d

INSTALL = cp -a --no-preserve=ownership
LN = ln
LN_S = ln -s
RM_RF = rm -rf
RM_F = rm -f
.PHONY: install
install: uninstall
	@$(ECHO) " INSTALLING GCS TO $(DESTDIR)/)"
	$(V1) $(MKDIR) -p $(DESTDIR)$(bindir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(libdir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)/applications
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)/pixmaps
	$(V1) $(INSTALL) $(BUILD_DIR)/$(GCS_SMALL_NAME)_$(GCS_BUILD_CONF)/bin/$(GCS_SMALL_NAME) $(DESTDIR)$(bindir)
	$(V1) $(INSTALL) $(BUILD_DIR)/$(GCS_SMALL_NAME)_$(GCS_BUILD_CONF)/$(libbasename)/$(GCS_SMALL_NAME) $(DESTDIR)$(libdir)
	$(V1) $(INSTALL) $(BUILD_DIR)/$(GCS_SMALL_NAME)_$(GCS_BUILD_CONF)/share/$(GCS_SMALL_NAME) $(DESTDIR)$(datadir)
	$(V1) $(INSTALL) -T $(ROOT_DIR)/package/linux/gcs.desktop $(DESTDIR)$(datadir)/applications/$(ORG_SMALL_NAME).desktop
	$(V1) $(INSTALL) -T $(ROOT_DIR)/ground/gcs/src/plugins/coreplugin/images/$(ORG_SMALL_NAME)_logo_128.png \
		$(DESTDIR)$(datadir)/pixmaps/$(ORG_SMALL_NAME).png

	$(V1) sed -i -e 's/gcs/$(GCS_SMALL_NAME)/g;s/GCS/$(GCS_BIG_NAME)/g;s/org/$(ORG_SMALL_NAME)/g;s/ORG/$(ORG_BIG_NAME)/g' \
		$(DESTDIR)$(datadir)/applications/$(ORG_SMALL_NAME).desktop

ifneq ($(enable-udev-rules), no)
	$(V1) $(MKDIR) -p $(DESTDIR)$(udevrulesdir)
	$(V1) $(INSTALL) -T $(ROOT_DIR)/package/linux/45-uav.rules $(DESTDIR)$(udevrulesdir)/45-$(ORG_SMALL_NAME).rules
endif

# uninstall target to ensure no side effects from previous installations
.PHONY: uninstall
uninstall:
	@$(ECHO) " UNINSTALLING GCS FROM $(DESTDIR)/)"
# Protect against inadvertant 'rm -rf /'
ifeq ($(GCS_SMALL_NAME),)
	@$(ECHO) "Error in build configuration - GCS_SMALL_NAME not defined"
	exit 1
endif
ifeq ($(ORG_SMALL_NAME),)
	@$(ECHO) "Error in build configuration - ORG_SMALL_NAME not defined"
	exit 1
endif
# ...safe to Proceed
	$(V1) $(RM_RF) $(DESTDIR)$(bindir)/$(GCS_SMALL_NAME)  # Remove application
	$(V1) $(RM_RF) $(DESTDIR)$(libdir)/$(GCS_SMALL_NAME)  # Remove libraries
	$(V1) $(RM_RF) $(DESTDIR)$(datadir)/$(GCS_SMALL_NAME) # Remove other data
	$(V1) $(RM_F) $(DESTDIR)$(datadir)/applications/$(ORG_SMALL_NAME).desktop
	$(V1) $(RM_F) $(DESTDIR)$(datadir)/pixmaps/$(ORG_SMALL_NAME).png
	$(V1) $(RM_F) $(DESTDIR)$(udevrulesdir)/45-$(ORG_SMALL_NAME).rules

