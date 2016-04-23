TEMPLATE = lib 
TARGET = DebugGadget

QT += widgets

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += \
    debugplugin.h \
    debugengine.h \
    debuggadget.h \
    debuggadgetwidget.h \
    debuggadgetfactory.h

SOURCES += \
    debugplugin.cpp \
    debugengine.cpp \
    debuggadget.cpp \
    debuggadgetfactory.cpp \
    debuggadgetwidget.cpp

OTHER_FILES += DebugGadget.pluginspec

FORMS += \
    debug.ui
