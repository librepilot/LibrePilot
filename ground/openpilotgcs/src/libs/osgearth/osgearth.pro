TEMPLATE = lib
TARGET = OsgEarth

DEFINES += OSGEARTH_LIBRARY

QT += widgets opengl qml quick

include(../../openpilotgcslibrary.pri)
include(osgearth_dependencies.pri)

# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

HEADERS += \
    osgearth_global.h \
    osgearth.h \
    osgQtQuick/Export.hpp \
    osgQtQuick/Utility.hpp \
    osgQtQuick/OSGNode.hpp \
    osgQtQuick/OSGGroup.hpp \
    osgQtQuick/OSGTextNode.hpp \
    osgQtQuick/OSGNodeFile.hpp \
    osgQtQuick/OSGModelNode.hpp \
    osgQtQuick/OSGSkyNode.hpp \
    osgQtQuick/OSGCamera.hpp \
    osgQtQuick/OSGViewport.hpp \
    osgQtQuick/QuickWindowViewer.hpp

SOURCES += \
    osgearth.cpp \
    osgQtQuick/Utility.cpp \ 
    osgQtQuick/OSGNode.cpp \
    osgQtQuick/OSGGroup.cpp \
    osgQtQuick/OSGTextNode.cpp \
    osgQtQuick/OSGNodeFile.cpp \
    osgQtQuick/OSGModelNode.cpp \
    osgQtQuick/OSGSkyNode.cpp \
    osgQtQuick/OSGCamera.cpp \
    osgQtQuick/OSGViewport.cpp \
    osgQtQuick/QuickWindowViewer.cpp

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

    LIBS += -losg -losgUtil -losgViewer -losgQt -losgDB -lOpenThreads -losgGA -losgQt
    LIBS += -losgEarth -losgEarthFeatures -losgEarthUtil -losgEarthQt
}

#win32 {
#    LIBS += -L$$MARBLE_SDK_DIR
#    CONFIG(release, debug|release):LIBS += -llibmarblewidget
#    CONFIG(debug, debug|release):LIBS += -llibmarblewidgetd
#}

include(copydata.pro)
