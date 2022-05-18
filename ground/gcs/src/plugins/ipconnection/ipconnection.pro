TEMPLATE = lib
TARGET = IPconnection

QT += network widgets

DEFINES += IPCONNECTION_LIBRARY

include(../../plugin.pri)
include(ipconnection_dependencies.pri)

HEADERS += \
    ipconnectionplugin.h \
    ipconnection_global.h \
    ipconnectionconfiguration.h \
    ipconnectionoptionspage.h \
    ipconnection_internal.h

SOURCES += \
    ipconnectionplugin.cpp \
    ipconnectionconfiguration.cpp \
    ipconnectionoptionspage.cpp

FORMS += ipconnectionoptionspage.ui

RESOURCES += 

OTHER_FILES += IPconnection.pluginspec
