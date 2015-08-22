#
# Windows-specific packaging script
#

ifndef TOP_LEVEL_MAKEFILE
    $(error Top level Makefile must be used to build this target)
endif

NSIS_OPTS     := -V3
NSIS_WINX86   := $(ROOT_DIR)/package/winx86
NSIS_SCRIPT   := $(NSIS_WINX86)/gcs.nsi

.PHONY: package
package: gcs uavobjects_matlab | $(PACKAGE_DIR)
ifneq ($(GCS_BUILD_CONF),release)
	# We can only package release builds
	$(error Packaging is currently supported for release builds only)
endif
	$(V1) echo "Building Windows installer, please wait..."
	$(V1) echo "If you have a script error in line 1 - use Unicode NSIS 2.46+"
	$(V1) echo "  http://www.scratchpaper.com"
	$(NSIS) $(NSIS_OPTS) \
	-DORG_BIG_NAME='$(ORG_BIG_NAME)' \
	-DGCS_BIG_NAME='$(GCS_BIG_NAME)' \
	-DGCS_SMALL_NAME='$(GCS_SMALL_NAME)' \
	-DPACKAGE_LBL='$(PACKAGE_LBL)' \
	-DVERSION_FOUR_NUM='$(shell $(VERSION_INFO) --format=\$${VERSION_FOUR_NUM})' \
	-DOUT_FILE='$(call system_path,$(PACKAGE_EXE))' \
	-DPROJECT_ROOT='$(call system_path,$(ROOT_DIR))' \
	-DGCS_BUILD_TREE='$(call system_path,$(GCS_DIR))' \
	-DUAVO_SYNTH_TREE='$(call system_path,$(UAVOBJ_OUT_DIR))' \
	$(NSIS_SCRIPT)
