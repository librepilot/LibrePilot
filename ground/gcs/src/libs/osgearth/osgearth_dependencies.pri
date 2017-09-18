# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

# set debug suffix if needed
win32:CONFIG(debug, debug|release):DS = "d"

contains(QT_ARCH, x86_64)  {
    macx {
        LIB_DIR_NAME = lib
    } else {
        LIB_DIR_NAME = lib64
    }
} else {
    LIB_DIR_NAME = lib
}

gstreamer {
    include(../gstreamer/gstreamer.pri)
    include(../gstreamer/gstreamer_dependencies.pri)
}

osg {
    OSG_SDK_DIR = $$clean_path($$(OSG_SDK_DIR))
    message(Using osg from here: $$OSG_SDK_DIR)

    linux|macx {
        INCLUDEPATH += $$OSG_SDK_DIR/include
        LIBS += -L$$OSG_SDK_DIR/$$LIB_DIR_NAME
        LIBS += -lOpenThreads -losg -losgUtil -losgDB -losgGA -losgFX -losgViewer -losgText
        osgQt:LIBS += -losgQt
    }

    win32 {
        LIBS += -lOpenThreads$${DS} -losg$${DS} -losgUtil$${DS} -losgDB$${DS} -losgGA$${DS} -losgFX$${DS} -losgViewer$${DS} -losgText$${DS}
        osgQt:LIBS += -losgQt$${DS}
    }
}

osgearth {
    OSGEARTH_SDK_DIR = $$clean_path($$(OSGEARTH_SDK_DIR))
    message(Using osgearth from here: $$OSGEARTH_SDK_DIR)

    linux|macx {
        INCLUDEPATH += $$OSGEARTH_SDK_DIR/include
        LIBS += -L$$OSGEARTH_SDK_DIR/$$LIB_DIR_NAME
        LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation
        osgearthQt:LIBS +=  -losgEarthQt
    }

    win32 {
        LIBS += -losgEarth$${DS} -losgEarthUtil$${DS} -losgEarthFeatures$${DS} -losgEarthSymbology$${DS} -losgEarthAnnotation$${DS}
        osgearthQt:LIBS += -losgEarthQt$${DS}
    }
}
