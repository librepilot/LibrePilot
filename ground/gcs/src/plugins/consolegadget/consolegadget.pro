TEMPLATE = lib
TARGET = ConsoleGadget

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += \
    consoleplugin.h \
    texteditloggerengine.h \
    consolegadget.h \
    consolegadgetwidget.h \
    consolegadgetfactory.h

SOURCES += \
    consoleplugin.cpp \
    texteditloggerengine.cpp \
    consolegadget.cpp \
    consolegadgetfactory.cpp \
    consolegadgetwidget.cpp

OTHER_FILES += ConsoleGadget.pluginspec
