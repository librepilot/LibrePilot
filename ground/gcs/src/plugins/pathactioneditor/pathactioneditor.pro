TEMPLATE = lib
TARGET = PathActionEditor 

QT += widgets

include(../../plugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += \
    pathactioneditorgadget.h \
    pathactioneditorgadgetwidget.h \
    pathactioneditorgadgetfactory.h \
    pathactioneditorplugin.h \
    pathactioneditortreemodel.h \
    treeitem.h \
    fieldtreeitem.h \
    browseritemdelegate.h

SOURCES += \
    pathactioneditorgadget.cpp \
    pathactioneditorgadgetwidget.cpp \
    pathactioneditorgadgetfactory.cpp \
    pathactioneditorplugin.cpp \
    pathactioneditortreemodel.cpp \
    treeitem.cpp \
    fieldtreeitem.cpp \
    browseritemdelegate.cpp

OTHER_FILES += pathactioneditor.pluginspec

FORMS += pathactioneditor.ui

RESOURCES +=
