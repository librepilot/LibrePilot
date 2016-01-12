# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

# set debug suffix if needed
#win32:CONFIG(debug, debug|release):DS = "d"

contains(QT_ARCH, x86_64)  {
    LIB_DIR_NAME = lib64
} else {
    LIB_DIR_NAME = lib
}

osg {
    OSG_SDK_DIR = $$clean_path($$(OSG_SDK_DIR))
    message(Using osg from here: $$OSG_SDK_DIR)

    INCLUDEPATH += $$OSG_SDK_DIR/include

    linux|macx {
        LIBS += -L$$OSG_SDK_DIR/$$LIB_DIR_NAME
        LIBS +=-lOpenThreads -losg -losgUtil -losgDB -losgGA -losgFX -losgViewer -losgText -losgQt
    }

    win32 {
        LIBS += -L$$OSG_SDK_DIR/lib
        LIBS += -lOpenThreads$${DS} -losg$${DS} -losgUtil$${DS} -losgDB$${DS} -losgGA$${DS} -losgFX$${DS} -losgViewer$${DS} -losgText$${DS} -losgQt$${DS}
    }
}

osgearth {
    OSGEARTH_SDK_DIR = $$clean_path($$(OSGEARTH_SDK_DIR))
    message(Using osgearth from here: $$OSGEARTH_SDK_DIR)

    INCLUDEPATH += $$OSGEARTH_SDK_DIR/include

    linux|macx {
        LIBS += -L$$OSGEARTH_SDK_DIR/$$LIB_DIR_NAME
        LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation -losgEarthQt
    }

    win32 {
        LIBS += -L$$OSGEARTH_SDK_DIR/lib
        LIBS += -losgEarth$${DS} -losgEarthUtil$${DS} -losgEarthFeatures$${DS} -losgEarthSymbology$${DS} -losgEarthAnnotation$${DS} -losgEarthQt$${DS}
    }
}
