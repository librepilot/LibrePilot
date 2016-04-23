TEMPLATE = app
TARGET = udp_test

QT += core gui network widgets

include(../../../../../gcs.pri)

HEADERS += \
    udptestwidget.h

SOURCES += \
    udptestmain.cpp \
    udptestwidget.cpp

FORMS += \
    udptestwidget.ui
