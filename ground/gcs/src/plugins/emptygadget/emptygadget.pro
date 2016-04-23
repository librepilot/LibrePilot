TEMPLATE = lib 
TARGET = EmptyGadget

QT += widgets

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri) 

HEADERS += \
    emptyplugin.h \
    emptygadget.h \
    emptygadgetwidget.h \
    emptygadgetfactory.h

SOURCES += \
    emptyplugin.cpp \
    emptygadget.cpp \
    emptygadgetfactory.cpp \
    emptygadgetwidget.cpp

OTHER_FILES += EmptyGadget.pluginspec
