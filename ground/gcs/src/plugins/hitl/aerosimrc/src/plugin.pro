TEMPLATE = lib
TARGET = plugin_AeroSIMRC

QT += network
QT -= gui

!win32 {
    error("AeroSimRC plugin is only available for win32 platform")
}

include(../../../../../gcs.pri)

RES_DIR    = $${PWD}/resources
SIM_DIR    = $$GCS_BUILD_TREE/misc/AeroSIM-RC
PLUGIN_DIR = $$SIM_DIR/Plugin/CopterControl
DLLDESTDIR = $$PLUGIN_DIR

HEADERS = \
    aerosimrcdatastruct.h \
    enums.h \
    plugin.h \
    qdebughandler.h \
    udpconnect.h \
    settings.h

SOURCES = \
    qdebughandler.cpp \
    plugin.cpp \
    udpconnect.cpp \
    settings.cpp

# Resemble the AeroSimRC directory structure and copy plugin files and resources
equals(copydata, 1):include(copydata.pro)
