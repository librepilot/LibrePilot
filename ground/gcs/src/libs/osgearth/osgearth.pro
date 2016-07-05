TEMPLATE = lib
TARGET = GCSOsgEarth
DEFINES += OSGEARTH_LIBRARY

#DEFINES += OSG_VERBOSE

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
    osgearth.h \
    utils/qtwindowingsystem.h \
    utils/utility.h \
    utils/shapeutils.h \
    utils/imagesource.hpp

SOURCES += \
    osgearth.cpp \
    utils/qtwindowingsystem.cpp \
    utils/utility.cpp \
    utils/shapeutils.cpp \
    utils/imagesource.cpp

HEADERS += \
    osgQtQuick/Export.hpp \
    osgQtQuick/DirtySupport.hpp \
    osgQtQuick/OSGNode.hpp \
    osgQtQuick/OSGGroup.hpp \
    osgQtQuick/OSGTransformNode.hpp \
    osgQtQuick/OSGShapeNode.hpp \
    osgQtQuick/OSGImageNode.hpp \
    osgQtQuick/OSGTextNode.hpp \
    osgQtQuick/OSGFileNode.hpp \
    osgQtQuick/OSGBillboardNode.hpp \
    osgQtQuick/OSGCamera.hpp \
    osgQtQuick/OSGViewport.hpp

SOURCES += \
    osgQtQuick/DirtySupport.cpp \
    osgQtQuick/OSGNode.cpp \
    osgQtQuick/OSGGroup.cpp \
    osgQtQuick/OSGTransformNode.cpp \
    osgQtQuick/OSGShapeNode.cpp \
    osgQtQuick/OSGImageNode.cpp \
    osgQtQuick/OSGTextNode.cpp \
    osgQtQuick/OSGFileNode.cpp \
    osgQtQuick/OSGBillboardNode.cpp \
    osgQtQuick/OSGCamera.cpp \
    osgQtQuick/OSGViewport.cpp

HEADERS += \
    osgQtQuick/ga/OSGCameraManipulator.hpp \
    osgQtQuick/ga/OSGNodeTrackerManipulator.hpp \
    osgQtQuick/ga/OSGTrackballManipulator.hpp

SOURCES += \
    osgQtQuick/ga/OSGCameraManipulator.cpp \
    osgQtQuick/ga/OSGNodeTrackerManipulator.cpp \
    osgQtQuick/ga/OSGTrackballManipulator.cpp

gstreamer:HEADERS += \
    utils/gstreamer/gstimagestream.hpp \
    utils/gstreamer/gstimagesource.hpp

gstreamer:SOURCES += \
    utils/gstreamer/gstimagestream.cpp \
    utils/gstreamer/gstimagesource.cpp

osgearth:HEADERS += \
    osgQtQuick/OSGSkyNode.hpp \
    osgQtQuick/OSGGeoTransformNode.hpp

osgearth:SOURCES += \
    osgQtQuick/OSGSkyNode.cpp \
    osgQtQuick/OSGGeoTransformNode.cpp

osgearth:HEADERS += \
    osgQtQuick/ga/OSGEarthManipulator.hpp \
    osgQtQuick/ga/OSGGeoTransformManipulator.hpp

osgearth:SOURCES += \
    osgQtQuick/ga/OSGEarthManipulator.cpp \
    osgQtQuick/ga/OSGGeoTransformManipulator.cpp

copy_osg:include(copydata.pro)
