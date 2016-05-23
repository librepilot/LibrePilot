DEB_DIST             := $(shell lsb_release -c -s)
# Instead of RELEASE-15.01-RC1 debian wants 15.01~RC1
UPSTREAM_VER         := $(subst -,~,$(subst RELEASE-,,$(PACKAGE_LBL)))
ifeq ($(DEB_DIST), unstable) # This should be set manually for a submission to Debian or similar
DEB_REV              := 1
else
DEB_REV              := 0$(DEB_DIST)1
endif
DEB_NAME             := $(ORG_SMALL_NAME)
DEB_ORIG_SRC         := $(PACKAGE_DIR)/$(DEB_NAME)_$(UPSTREAM_VER).orig.tar.gz
DEB_ORIG_FW          := $(PACKAGE_DIR)/$(DEB_NAME)_$(UPSTREAM_VER).orig-firmware.tar.gz
DEB_SRC_CHANGES      := $(PACKAGE_DIR)/$(DEB_NAME)_$(UPSTREAM_VER)-$(DEB_REV)_source.changes
DEB_PACKAGE_DIR      := $(PACKAGE_DIR)/$(DEB_NAME)-$(UPSTREAM_VER)
DEB_ARCH             := $(shell dpkg --print-architecture)
DEB_PACKAGE_NAME     := $(DEB_NAME)_$(UPSTREAM_VER)-$(DEB_REV)_$(DEB_ARCH)
DEB_DIR              := package/linux/debian

SED_DATE_STRG        := $(shell date -R)
SED_SCRIPT           := $(SED_SCRIPT)' \
			s/<VERSION>/$(UPSTREAM_VER)-$(DEB_REV)/g; \
			s/<DATE>/$(SED_DATE_STRG)/g; \
			s/<DIST>/$(DEB_DIST)/g; \
			s/<NAME>/$(DEB_NAME)/g; \
			s/<DESCRIPTION>/$(DESCRIPTION_SHORT)\n $(subst ','"'"',$(subst $(NEWLINE),\n ,$(DESCRIPTION_LONG)))/g; \
			'

# Ubuntu 14.04 (Trusty Tahr) use qt in /opt PPA
TRUSTY_DEPS_SED      := s/qml-module-.*/$(OPT_QT)quickcontrols/g; \
                        s/qt5-default.*/$(OPT_QT)-meta-minimal, $(OPT_QT)svg, $(OPT_QT)script, $(OPT_QT)serialport, $(OPT_QT)multimedia, $(OPT_QT)translations, $(OPT_QT)tools/g;

# Leave off Qt and OSG dependencies if calling package target under the assumption that
# OP is providing them or the user already has them installed because OP is already built.
PACKAGE_DEPS_SED     := s/python.*/python/;s/{misc:Depends}.*/{misc:Depends}/;

.PHONY: package
package: debian
	@$(ECHO) "Building Linux package, please wait..."
	$(V1) sed -i -e "$(PACKAGE_DEPS_SED)" debian/control
	$(V1) sed -i -e 's,config_new.*, --help > /dev/null,' debian/rules
	$(V1) dpkg-buildpackage -b -us -uc -nc
	$(V1) mv $(ROOT_DIR)/../$(DEB_PACKAGE_NAME).deb $(BUILD_DIR)
	$(V1) mv $(ROOT_DIR)/../$(DEB_PACKAGE_NAME).changes $(BUILD_DIR)
	$(V1) rm -r debian

DEBIAN_DIR_FILES := changelog compat control copyright rules source/format
.PHONY: debian
debian: $(DEB_DIR)
	$(V1) rm -rf debian && mkdir debian
	$(V1) cd $(DEB_DIR) $(foreach file,$(DEBIAN_DIR_FILES), && cp --parents $(file) $(ROOT_DIR)/debian)
	$(V1) cp -T package/linux/45-uav.rules debian/$(DEB_NAME).udev
	$(V1) $(SED_SCRIPT) debian/changelog debian/control
ifeq ($(DEB_DIST), trusty)
	$(V1) sed -i -e '$(TRUSTY_DEPS_SED)' debian/control
endif

.PHONY: package_src
package_src:  $(DEB_SRC_CHANGES)

.PHONY: package_src_upload
package_src_upload:  $(DEB_SRC_CHANGES)
	$(V1) debsign $(DEB_SRC_CHANGES)
	$(V1) dput $(DPUT_URL) $(DEB_SRC_CHANGES)

# Only include the orig if it we haven't alreadly built it,
# because if we have it is likely to be uploaded already
ifeq ($(wildcard $(DEB_ORIG_SRC)),)
DPKG_INCLUDE_ORIG := -sa
else
DPKG_INCLUDE_ORIG := -sd
endif

$(DEB_SRC_CHANGES):  $(DEB_ORIG_SRC_NAME) $(DEB_PACKAGE_DIR)
	$(V1) cd $(DEB_PACKAGE_DIR) && dpkg-buildpackage -S -us -uc $(DPKG_INCLUDE_ORIG)

$(DEB_ORIG_SRC): $(DIST_TAR_GZ) | $(PACKAGE_DIR)
	$(V1) cp $(DIST_TAR_GZ) $(DEB_ORIG_SRC)

$(DEB_ORIG_FW): $(FW_DIST_TAR_GZ) | $(PACKAGE_DIR)
	$(V1) cp $(FW_DIST_TAR_GZ) $(DEB_ORIG_FW)

$(DEB_PACKAGE_DIR): $(DEB_ORIG_SRC) $(DEB_ORIG_FW) debian | $(PACKAGE_DIR)
	$(V1) tar -xf $(DEB_ORIG_SRC) -C $(PACKAGE_DIR)
	$(V1) tar -xf $(DEB_ORIG_FW) -C $(PACKAGE_DIR)/$(PACKAGE_NAME)
	$(V1) mv debian $(PACKAGE_DIR)/$(PACKAGE_NAME)
	$(V1) rm -rf $(DEB_PACKAGE_DIR) && mv $(PACKAGE_DIR)/$(PACKAGE_NAME) $(DEB_PACKAGE_DIR)

