RPM_NAME             := $(PACKAGE_NAME)
UPSTREAM_VER         := $(subst -,~,$(subst RELEASE-,,$(DIST_LBL)))
RPM_REL              := 1
RPM_ARCH             := $(shell rpm --eval '%{_arch}')
RPM_PACKAGE_NAME     := $(RPM_NAME)-$(UPSTREAM_VER)-$(RPM_REL)
RPM_PACKAGE_FILE     := $(PACKAGE_DIR)/RPMS/$(RPM_ARCH)/$(RPM_PACKAGE_NAME)$(shell rpm --eval '%{?dist}').$(RPM_ARCH).rpm
RPM_PACKAGE_SRC      := $(PACKAGE_DIR)/SRPMS/$(RPM_PACKAGE_NAME).src.rpm

SED_SCRIPT           := $(SED_SCRIPT)' \
			s/<VERSION>/$(UPSTREAM_VER)/g; \
			s/<NAME>/$(RPM_NAME)/g; \
			s/<RELEASE>/$(RPM_REL)/g; \
			s/<SOURCE0>/$(notdir $(DIST_TAR_GZ))/g; \
			s/<SOURCE1>/$(notdir $(FW_DIST_TAR_GZ))/g; \
			s/<SUMMARY>/$(DESCRIPTION_SHORT)/g; \
			s/<DESCRIPTION>/$(subst ','"'"',$(subst $(NEWLINE),\n,$(DESCRIPTION_LONG)))/g; \
			'

RPM_DIRS := $(addprefix $(PACKAGE_DIR)/,BUILD RPMS SOURCES SPECS SRPMS)
DIRS += $(RPM_DIRS)

SPEC_FILE    := $(PACKAGE_DIR)/SPECS/$(RPM_NAME).spec
SPEC_FILE_IN := $(ROOT_DIR)/package/linux/rpmspec.in

.PHONY: rpmspec
rpmspec: $(SPEC_FILE)

$(SPEC_FILE): $(SPEC_FILE_IN) $(DIST_VER_INFO) | $(RPM_DIRS)
	$(V1) cp -f $(SPEC_FILE_IN) $(SPEC_FILE)
	$(V1) $(SED_SCRIPT) $(SPEC_FILE)


.PHONY: package
package: $(RPM_PACKAGE_FILE)
$(RPM_PACKAGE_FILE): RPMBUILD_OPTS := -bb

.PHONY: package_src
package_src: $(RPM_PACKAGE_SRC)
$(RPM_PACKAGE_SRC): RPMBUILD_OPTS := -bs

.PHONY: package_src_upload
package_src_upload: $(RPM_PACKAGE_SRC)
	copr-cli build --nowait $(COPR_PROJECT) $(RPM_PACKAGE_SRC)

$(RPM_PACKAGE_FILE) $(RPM_PACKAGE_SRC): $(SPEC_FILE) $(DIST_TAR_GZ) $(FW_DIST_TAR_GZ) | $(RPM_DIRS)
	@$(ECHO) "Building $(call toprel,$@), please wait..."
	$(V1) ln -sf $(DIST_TAR_GZ) $(PACKAGE_DIR)/SOURCES
	$(V1) ln -sf $(FW_DIST_TAR_GZ) $(PACKAGE_DIR)/SOURCES
	$(V1) rpmbuild $(RPMBUILD_OPTS) --define "_topdir $(PACKAGE_DIR)" $(SPEC_FILE)
