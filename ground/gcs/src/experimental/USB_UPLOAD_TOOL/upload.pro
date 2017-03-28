#-------------------------------------------------
#
# Project created by QtCreator 2010-07-24T11:26:38
#
#-------------------------------------------------

TEMPLATE = app
TARGET = UploadTool

QT += core serialport
QT -= gui

CONFIG   += console
CONFIG   -= app_bundle
# don't build both debug and release
CONFIG -= debug_and_release

SOURCES += \
    main.cpp \
    dfu.cpp \
    ./SSP/qssp.cpp \
    ./SSP/port.cpp \
    ./SSP/qsspt.cpp \

HEADERS += \
    dfu.h \
    ./SSP/qssp.h \
    ./SSP/port.h \
    ./SSP/common.h \
    ./SSP/qsspt.h
