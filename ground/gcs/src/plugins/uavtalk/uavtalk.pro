TEMPLATE = lib
TARGET = UAVTalk

QT += widgets network

DEFINES += UAVTALK_LIBRARY

#DEFINES += VERBOSE_TELEMETRY
#DEFINES += VERBOSE_UAVTALK

include(../../plugin.pri)
include(uavtalk_dependencies.pri)

HEADERS += \
    uavtalk.h \
    uavtalkplugin.h \
    telemetrymonitor.h \
    telemetrymanager.h \
    uavtalk_global.h \
    telemetry.h

SOURCES += \
    uavtalk.cpp \
    uavtalkplugin.cpp \
    telemetrymonitor.cpp \
    telemetrymanager.cpp \
    telemetry.cpp

OTHER_FILES += UAVTalk.pluginspec
