TEMPLATE = lib
TARGET = GCSGStreamer
DEFINES += GST_LIB_LIBRARY

QT += widgets

include(../../library.pri)
include(../utils/utils.pri)

include(gstreamer_dependencies.pri)

include(plugins/plugins.pro)

HEADERS += \
    gst_global.h \
    gst_util.h \
    devicemonitor.h \
    pipeline.h \
    pipelineevent.h \
    overlay.h \
    videowidget.h

SOURCES += \
    gst_util.cpp \
    devicemonitor.cpp \
    pipeline.cpp \
    videowidget.cpp

copy_gstreamer:include(copydata.pro)
