RPM_NAME             := $(PACKAGE_NAME)
UPSTREAM_VER         := $(subst -,~,$(subst RELEASE-,,$(DIST_LBL)))
RPM_REL              := 1
RPM_ARCH             := $(shell rpm --eval '%{_arch}')
RPM_PACKAGE_NAME     := $(RPM_NAME)-$(UPSTREAM_VER)-$(RPM_REL)$(shell rpm --eval '%{?dist}').$(RPM_ARCH).rpm
RPM_PACKAGE_FILE     := $(PACKAGE_DIR)/RPMS/$(RPM_ARCH)/$(RPM_PACKAGE_NAME)

SED_SCRIPT           := sed -i -e ' \
			s/<VERSION>/$(UPSTREAM_VER)/g; \
			s/<NAME>/$(RPM_NAME)/g; \
			s/<RELEASE>/$(RPM_REL)/g; \
			s/<SOURCE>/$(notdir $(DIST_TAR_GZ))/g; \
			s/<ARCHIVE_PREFIX>/$(PACKAGE_NAME)/g; \
			'

RPM_DIRS := $(addprefix $(PACKAGE_DIR)/,BUILD RPMS SOURCES SPECS SRPMS)
DIRS += $(RPM_DIRS)

SPEC_FILE    := $(PACKAGE_DIR)/SPECS/$(RPM_NAME).spec
SPEC_FILE_IN := $(ROOT_DIR)/package/linux/rpmspec.in

.PHONY: rpmspec
rpmspec: $(SPEC_FILE)

$(SPEC_FILE): $(SPEC_FILE_IN) | $(RPM_DIRS) 
	$(V1) cp -f $(SPEC_FILE_IN) $(SPEC_FILE)
	$(V1) $(SED_SCRIPT) $(SPEC_FILE)


.PHONY: package
package: $(RPM_PACKAGE_FILE)

$(RPM_PACKAGE_FILE): $(SPEC_FILE) $(DIST_TAR_GZ) | $(RPM_DIRS)
	@$(ECHO) "Building $(RPM_PACKAGE_NAME), please wait..."
	$(V1) ln -sf $(DIST_TAR_GZ) $(PACKAGE_DIR)/SOURCES
	$(V1) rpmbuild -bb --define "_topdir $(PACKAGE_DIR)" $(SPEC_FILE)

