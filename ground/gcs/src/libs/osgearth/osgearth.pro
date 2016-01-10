TEMPLATE = lib
TARGET = GCSOsgEarth
DEFINES += OSGEARTH_LIBRARY

#DEFINES += OSG_USE_QT_PRIVATE

QT += widgets opengl qml quick
contains(DEFINES, OSG_USE_QT_PRIVATE) {
    QT += core-private gui-private
}

include(../../library.pri)
include(../utils/utils.pri)

linux {
    QMAKE_RPATHDIR = $$shell_quote(\$$ORIGIN/$$relative_path($$GCS_LIBRARY_PATH/osg, $$GCS_LIBRARY_PATH))
    include(../../rpath.pri)
}

# disable all warnings on mac to avoid build failures
macx:CONFIG += warn_off

# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

OSG_SDK_DIR = $$clean_path($$(OSG_SDK_DIR))
message(Using osg from here: $$OSG_SDK_DIR)

HEADERS += \
    osgearth_global.h \
    utility.h \
    qtwindowingsystem.h \
    osgearth.h

SOURCES += \
    utility.cpp \
    qtwindowingsystem.cpp \
    osgearth.cpp

HEADERS += \
    osgQtQuick/Export.hpp \
    osgQtQuick/OSGNode.hpp \
    osgQtQuick/OSGGroup.hpp \
    osgQtQuick/OSGTransformNode.hpp \
    osgQtQuick/OSGCubeNode.hpp \
    osgQtQuick/OSGTextNode.hpp \
    osgQtQuick/OSGFileNode.hpp \
    osgQtQuick/OSGModelNode.hpp \
    osgQtQuick/OSGBackgroundNode.hpp \
    osgQtQuick/OSGSkyNode.hpp \
    osgQtQuick/OSGCamera.hpp \
    osgQtQuick/OSGViewport.hpp

SOURCES += \
    osgQtQuick/OSGNode.cpp \
    osgQtQuick/OSGGroup.cpp \
    osgQtQuick/OSGTransformNode.cpp \
    osgQtQuick/OSGCubeNode.cpp \
    osgQtQuick/OSGTextNode.cpp \
    osgQtQuick/OSGFileNode.cpp \
    osgQtQuick/OSGModelNode.cpp \
    osgQtQuick/OSGBackgroundNode.cpp \
    osgQtQuick/OSGSkyNode.cpp \
    osgQtQuick/OSGCamera.cpp \
    osgQtQuick/OSGViewport.cpp

INCLUDEPATH += $$OSG_SDK_DIR/include

linux {
    exists( $$OSG_SDK_DIR/lib64 ) {
        LIBS += -L$$OSG_SDK_DIR/lib64
    } else {
        LIBS += -L$$OSG_SDK_DIR/lib
    }

    LIBS +=-lOpenThreads
    LIBS += -losg -losgUtil -losgDB -losgGA -losgViewer -losgText
    LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation
    LIBS += -losgQt -losgEarthQt
}

macx {
    LIBS += -L$$OSG_SDK_DIR/lib

    LIBS += -lOpenThreads
    LIBS += -losg -losgUtil -losgDB -losgGA -losgViewer -losgText
    LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation
    LIBS += -losgQt -losgEarthQt
}

win32 {
    LIBS += -L$$OSG_SDK_DIR/lib

    #CONFIG(release, debug|release) {
        LIBS += -lOpenThreads
        LIBS += -losg -losgUtil -losgDB -losgGA -losgViewer -losgText
        LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation
        LIBS += -losgQt -losgEarthQt
    #}
    #CONFIG(debug, debug|release) {
    #    LIBS += -lOpenThreadsd
    #    LIBS += -losgd -losgUtild -losgDBd -losgGAd -losgViewerd -losgTextd
    #    LIBS += -losgEarthd -losgEarthUtild -losgEarthFeaturesd -losgEarthSymbologyd -losgEarthAnnotationd
    #    LIBS += -losgQtd -losgEarthQtd
    #}
}

include(copydata.pro)
