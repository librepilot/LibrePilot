TEMPLATE = lib
TARGET = GCSOsgEarth
DEFINES += OSGEARTH_LIBRARY

#CONFIG += mys2

osg:DEFINES += USE_OSG
osgQt:DEFINES += USE_OSG_QT

osgearth:DEFINES += USE_OSGEARTH
osgearthQt:DEFINES += USE_OSGEARTH_QT

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
    shapeutils.h \
    qtwindowingsystem.h \
    osgearth.h

SOURCES += \
    utility.cpp \
    shapeutils.cpp \
    qtwindowingsystem.cpp \
    osgearth.cpp

HEADERS += \
    osgQtQuick/Export.hpp \
    osgQtQuick/OSGNode.hpp \
    osgQtQuick/OSGGroup.hpp \
    osgQtQuick/OSGTransformNode.hpp \
    osgQtQuick/OSGShapeNode.hpp \
    osgQtQuick/OSGTextNode.hpp \
    osgQtQuick/OSGFileNode.hpp \
    osgQtQuick/OSGBackgroundNode.hpp \
    osgQtQuick/OSGCamera.hpp \
    osgQtQuick/OSGViewport.hpp

SOURCES += \
    osgQtQuick/OSGNode.cpp \
    osgQtQuick/OSGGroup.cpp \
    osgQtQuick/OSGTransformNode.cpp \
    osgQtQuick/OSGShapeNode.cpp \
    osgQtQuick/OSGTextNode.cpp \
    osgQtQuick/OSGFileNode.cpp \
    osgQtQuick/OSGBackgroundNode.cpp \
    osgQtQuick/OSGCamera.cpp \
    osgQtQuick/OSGViewport.cpp

osgearth:HEADERS += \
    osgQtQuick/OSGSkyNode.hpp \
    osgQtQuick/OSGGeoTransformNode.hpp

osgearth:SOURCES += \
    osgQtQuick/OSGSkyNode.cpp \
    osgQtQuick/OSGGeoTransformNode.cpp

copy_osg:include(copydata.pro)
