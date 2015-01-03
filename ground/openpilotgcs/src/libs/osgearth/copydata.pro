#
# copy osg and osgearth libraries and data to build dir
#

equals(copydata, 1) {

    OSG_VERSION = 3.2.1

    linux {
        !exists( $(OSG_DIR)/lib64 ) { 

            # copy osg libraries
            data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg\") $$addNewline()
            data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSG_DIR/lib/.\") \
                $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg/\") $$addNewline()

            # copy osgearth libraries
            data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth\") $$addNewline()
            data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSGEARTH_DIR/lib/.\") \
                $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth/\") $$addNewline()
            message("Copy OSG and OSGEarth ***Linux32*** libraries")

        } else {

            # copy osg libraries
            data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg\") $$addNewline()
            data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSG_DIR/lib64/.\") \
                $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg/\") $$addNewline()

            # copy osgearth libraries
            data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth\") $$addNewline()
            data_copy.commands += $(COPY_DIR) $$targetPath(\"$$OSGEARTH_DIR/lib64/.\") \
                $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth/\") $$addNewline()
            message("Copy OSG and OSGEarth ***Linux64*** libraries")
        }
    }

    macx {

        isEmpty(PROVIDER) {
            PROVIDER = OpenPilot
        }

        # copy Marble library
        MARBLE_LIB = $$targetPath(\"$$MARBLE_SDK_DIR/Marble.app/Contents/MacOS/lib/libmarblewidget.20.dylib\")
        data_copy.commands += $(COPY_FILE) $$MARBLE_LIB $$targetPath(\"$$GCS_LIBRARY_PATH/\") $$addNewline()
        ASTRO_LIB = $$targetPath(\"$$MARBLE_SDK_DIR/Marble.app/Contents/MacOS/lib/libastro.1.dylib\")
        data_copy.commands += $(COPY_FILE) $$ASTRO_LIB $$targetPath(\"$$GCS_LIBRARY_PATH/\") $$addNewline()

        # copy Marble plugins
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/marble\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$MARBLE_SDK_DIR/Marble.app/Contents/MacOS/resources/plugins\") \
            $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/marble/\") $$addNewline()

        # copy Marble data
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/share/marble\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$MARBLE_SDK_DIR/Marble.app/Contents/MacOS/resources/data\") \
            $$targetPath(\"$$GCS_BUILD_TREE/share/marble/\") $$addNewline()

        # change marble libs' id names
        data_copy.commands += install_name_tool -id @executable_path/../Plugins/libmarblewidget.20.dylib \
            $$targetPath(\"$$GCS_LIBRARY_PATH/libmarblewidget.20.dylib\") $$addNewline()
        data_copy.commands += install_name_tool -id @executable_path/../Plugins/libastro.1.dylib \
            $$targetPath(\"$$GCS_LIBRARY_PATH/libastro.1.dylib\") $$addNewline()

        # change library lookup paths
        data_copy.commands += install_name_tool -change @executable_path/lib/libastro.1.dylib @executable_path/../Plugins/libastro.1.dylib \
            $$targetPath(\"$$GCS_LIBRARY_PATH/libmarblewidget.20.dylib\") $$addNewline()
    }

    win32 {
        # set debug suffix if needed
        CONFIG(debug, debug|release):DS = "d"

        # copy dependencies libraries
        data_copy.commands += $(COPY_FILE) $$targetPath(\"D:/Projects/OpenPilotTools/osgearth_dependencies/bin/\"*.dll) \
            $$targetPath(\"$$GCS_APP_PATH/\") $$addNewline()
        data_copy.commands += $(COPY_FILE) $$targetPath(\"D:/Projects/OpenPilotTools/osgearth_dependencies/lib/\"*.dll) \
            $$targetPath(\"$$GCS_APP_PATH/\") $$addNewline()

        # copy osg libraries
        data_copy.commands += $(COPY_FILE) $$targetPath(\"$$(OSG_DIR)/bin/\"*.dll) \
            $$targetPath(\"$$GCS_APP_PATH/\") $$addNewline()

        # copy osg plugins
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$(OSG_DIR)/bin/osgPlugins-$${OSG_VERSION}\") \
            $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osg/\") $$addNewline()

        # copy osgearth libraries
        data_copy.commands += $(COPY_FILE) $$targetPath(\"$$(OSGEARTH_DIR)/bin/\"*.dll) \
            $$targetPath(\"$$GCS_APP_PATH/\") $$addNewline()

        # copy osgearth plugins
        data_copy.commands += -@$(MKDIR) $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth\") $$addNewline()
        data_copy.commands += $(COPY_DIR) $$targetPath(\"$$(OSGEARTH_DIR)/bin/osgPlugins-$${OSG_VERSION}\") \
            $$targetPath(\"$$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/osgearth/\") $$addNewline()

    }

    # add make target
    POST_TARGETDEPS += copydata

    data_copy.target = copydata
    QMAKE_EXTRA_TARGETS += data_copy
}
