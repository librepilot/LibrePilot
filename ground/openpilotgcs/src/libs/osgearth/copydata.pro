#
# copy osg and osgearth libraries and data to build dir
#

equals(copydata, 1) {

    OSG_VERSION = 3.2.1

    linux {
        !exists($(OSG_DIR)/lib64) {
			OSG_LIB_DIR = $(OSG_DIR)/lib
        } else {
			OSG_LIB_DIR = $(OSG_DIR)/lib64
        }

        # copy osg libraries
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSG_LIB_DIR/.\") \
            $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg/\") $$addNewline()
    }

    macx {

        isEmpty(PROVIDER) {
            PROVIDER = OpenPilot
        }

        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSG_DIR/lib/\"*) $$targetPath(\"$$GCS_LIBRARY_PATH\") $$addNewline()
    }

    win32 {
        # set debug suffix if needed
        CONFIG(debug, debug|release):DS = "d"

        # copy osg libraries
        data_copy.commands += $(COPY_FILE) $$targetPath(\"$$(OSG_DIR)/bin/\"*.dll) \
            $$targetPath(\"$$GCS_APP_PATH/\") $$addNewline()

        # copy osg plugins
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$(OSG_DIR)/bin/osgPlugins-$${OSG_VERSION}\") \
            $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg/\") $$addNewline()
    }

    # add make target
    POST_TARGETDEPS += copydata

    data_copy.target = copydata
    QMAKE_EXTRA_TARGETS += data_copy
}
