#
# Qmake project for UAVObjGenerator.
# Copyright (c) 2017, The LibrePilot Project, https://www.librepilot.org
# Copyright (c) 2010-2013, The OpenPilot Team, http://www.openpilot.org
#

QT += xml
QT -= gui
macx {
    CONFIG += warn_on
    !warn_off {
        QMAKE_CXXFLAGS_WARN_ON += -Werror
        QMAKE_CFLAGS_WARN_ON   += -Werror
        QMAKE_CXXFLAGS  += -fpermissive
    }
}
# use ccache when available
QMAKE_CC = $$(CCACHE) $$QMAKE_CC
QMAKE_CXX = $$(CCACHE) $$QMAKE_CXX

TARGET = uavobjgenerator
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
DESTDIR = $$OUT_PWD # Set a consistent output dir on windows
SOURCES += main.cpp \
    uavobjectparser.cpp \
    generators/generator_io.cpp \
    generators/java/uavobjectgeneratorjava.cpp \
    generators/flight/uavobjectgeneratorflight.cpp \
    generators/arduino/uavobjectgeneratorarduino.cpp \
    generators/gcs/uavobjectgeneratorgcs.cpp \
    generators/matlab/uavobjectgeneratormatlab.cpp \
    generators/python/uavobjectgeneratorpython.cpp \
    generators/wireshark/uavobjectgeneratorwireshark.cpp \
    generators/generator_common.cpp
HEADERS += uavobjectparser.h \
    generators/generator_io.h \
    generators/java/uavobjectgeneratorjava.h \
    generators/gcs/uavobjectgeneratorgcs.h \
    generators/arduino/uavobjectgeneratorarduino.h \
    generators/matlab/uavobjectgeneratormatlab.h \
    generators/python/uavobjectgeneratorpython.h \
    generators/wireshark/uavobjectgeneratorwireshark.h \
    generators/generator_common.h
