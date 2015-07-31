#
# Linux-specific packaging script
#

ifndef TOP_LEVEL_MAKEFILE
    $(error Top level Makefile must be used to build this target)
endif

# Are we using a debian based distro?
ifneq ($(shell which dpkg 2> /dev/null),)
	include $(ROOT_DIR)/package/linux/deb.mk
endif # Debian based distro?

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

.PHONY: install
install:
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
	$(V1) $(INSTALL) -T $(ROOT_DIR)/ground/openpilotgcs/src/plugins/coreplugin/images/$(ORG_SMALL_NAME)_logo_128.png \
		$(DESTDIR)$(datadir)/pixmaps/$(ORG_SMALL_NAME).png

	$(V1) sed -i -e 's/gcs/$(GCS_SMALL_NAME)/g;s/GCS/$(GCS_BIG_NAME)/g;s/org/$(ORG_SMALL_NAME)/g;s/ORG/$(ORG_BIG_NAME)/g' \
		$(DESTDIR)$(datadir)/applications/$(ORG_SMALL_NAME).desktop

ifneq ($(enable-udev-rules), no)
	$(V1) $(MKDIR) -p $(DESTDIR)$(udevrulesdir)
	$(V1) $(INSTALL) -T $(ROOT_DIR)/package/linux/45-uav.rules $(DESTDIR)$(udevrulesdir)/45-$(ORG_SMALL_NAME).rules
endif
