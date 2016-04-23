TEMPLATE = lib 
TARGET = NotifyPlugin 
 
QT += widgets multimedia

include(../../plugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(notifyplugin_dependencies.pri)

HEADERS += \
    notifyplugin.h \
    notifypluginoptionspage.h \
    notifyitemdelegate.h \
    notifytablemodel.h \
    notificationitem.h \
    notifylogging.h

SOURCES += \
    notifyplugin.cpp \
    notifypluginoptionspage.cpp \
    notifyitemdelegate.cpp \
    notifytablemodel.cpp \
    notificationitem.cpp \
    notifylogging.cpp
 
OTHER_FILES += NotifyPlugin.pluginspec

FORMS += \
    notifypluginoptionspage.ui

RESOURCES += \
    res.qrc
