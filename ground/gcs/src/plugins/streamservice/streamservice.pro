TEMPLATE = lib
TARGET = StreamServicePlugin

QT       += network widgets

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavtalk/uavtalk.pri)
include(../../plugins/uavobjects/uavobjects.pri)

SOURCES += streamserviceplugin.cpp

HEADERS += streamserviceplugin.h

OTHER_FILES +=

DISTFILES += \
    StreamServicePlugin.pluginspec
