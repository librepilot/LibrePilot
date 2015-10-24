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

include(osgearth_dependencies.pri)

linux {
    QMAKE_RPATHDIR = $$shell_quote(\$$ORIGIN/$$relative_path($$GCS_LIBRARY_PATH/osg, $$GCS_LIBRARY_PATH))
    include(../../rpath.pri)
}

# disable all warnings on mac to avoid build failures
macx:CONFIG += warn_off

# osg and osgearth emit a lot of unused parameter warnings...
QMAKE_CXXFLAGS += -Wno-unused-parameter

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

include(copydata.pro)
