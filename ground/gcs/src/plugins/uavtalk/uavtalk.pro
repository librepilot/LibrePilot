TEMPLATE = lib
TARGET = UAVTalk

QT += widgets network

DEFINES += UAVTALK_LIBRARY

#DEFINES += VERBOSE_TELEMETRY
#DEFINES += VERBOSE_UAVTALK

include(../../plugin.pri)
include(uavtalk_dependencies.pri)

HEADERS += \
    uavtalk_global.h \
    uavtalk.h \
    telemetry.h \
    telemetrymonitor.h \
    telemetrymanager.h \
    oplinkmanager.h \
    uavtalkplugin.h

SOURCES += \
    uavtalk.cpp \
    telemetry.cpp \
    telemetrymonitor.cpp \
    telemetrymanager.cpp \
    oplinkmanager.cpp \
    uavtalkplugin.cpp

OTHER_FILES += UAVTalk.pluginspec
