TEMPLATE = lib
TARGET = GCSOsgEarth

DEFINES += OSGEARTH_LIBRARY

QT += widgets opengl qml quick

# To make threaded gl check...
QT += core-private gui-private

include(../../openpilotgcslibrary.pri)
include(../utils/utils.pri)

# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

HEADERS += \
    osgearth_global.h \
    osgearth.h \
    osgQtQuick/Export.hpp \
    osgQtQuick/Utility.hpp \
    osgQtQuick/OSGNode.hpp \
    osgQtQuick/OSGGroup.hpp \
    osgQtQuick/OSGTransformNode.hpp \
    osgQtQuick/OSGCubeNode.hpp \
    osgQtQuick/OSGTextNode.hpp \
    osgQtQuick/OSGNodeFile.hpp \
    osgQtQuick/OSGModelNode.hpp \
    osgQtQuick/OSGSkyNode.hpp \
    osgQtQuick/OSGCamera.hpp \
    osgQtQuick/OSGViewport.hpp

SOURCES += \
    osgearth.cpp \
    osgQtQuick/Utility.cpp \ 
    osgQtQuick/OSGNode.cpp \
    osgQtQuick/OSGGroup.cpp \
    osgQtQuick/OSGTransformNode.cpp \
    osgQtQuick/OSGCubeNode.cpp \
    osgQtQuick/OSGTextNode.cpp \
    osgQtQuick/OSGNodeFile.cpp \
    osgQtQuick/OSGModelNode.cpp \
    osgQtQuick/OSGSkyNode.cpp \
    osgQtQuick/OSGCamera.cpp \
    osgQtQuick/OSGViewport.cpp

OSG_DIR = $$clean_path($$(OSG_DIR))
message(Using osg from here: $$OSG_DIR)

OSGEARTH_DIR = $$clean_path($$(OSGEARTH_DIR))
message(Using osgEarth from here: $$OSGEARTH_DIR)

INCLUDEPATH += $$OSG_DIR/include $$OSGEARTH_DIR/include  

linux {
    !exists( $(OSG_DIR)/lib64 ) {
        LIBS += -L$$OSG_DIR/lib -L$$OSGEARTH_DIR/lib
    } else {
        LIBS += -L$$OSG_DIR/lib64 -L$$OSGEARTH_DIR/lib64
    }

    LIBS +=-lOpenThreads
    LIBS += -losg -losgUtil -losgDB -losgGA -losgViewer -losgText -losgQt
    LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation -losgEarthQt
}

win32 {
    LIBS += -L$$OSG_DIR/lib -L$$OSGEARTH_DIR/lib

    CONFIG(release, debug|release) {
        LIBS += -losg -losgUtil -losgDB -losgGA -losgViewer -losgText -losgQt -lOpenThreads
        LIBS += -losgEarth -losgEarthUtil -losgEarthFeatures -losgEarthSymbology -losgEarthAnnotation -losgEarthQt
    }
    CONFIG(debug, debug|release) {
        LIBS += -losgd -losgUtild -losgDBd -losgGAd -losgViewerd -losgTextd -losgQtd -lOpenThreadsd
        LIBS += -losgEarthd -losgEarthUtild -losgEarthFeaturesd -losgEarthSymbologyd -losgEarthAnnotationd -losgEarthQtd
    }
}

include(copydata.pro)
