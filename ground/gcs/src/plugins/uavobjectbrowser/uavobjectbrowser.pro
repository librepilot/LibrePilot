TEMPLATE = lib
TARGET = UAVObjectBrowser

QT += widgets

include(../../plugin.pri)
include(uavobjectbrowser_dependencies.pri)

HEADERS += \
    treeitem.h \
    fieldtreeitem.h \
    browseritemdelegate.h \
    uavobjecttreemodel.h \
    uavobjectbrowserconfiguration.h \
    uavobjectbrowseroptionspage.h \
    uavobjectbrowser.h \
    uavobjectbrowserwidget.h \
    uavobjectbrowserfactory.h \
    browserplugin.h

SOURCES += \
    treeitem.cpp \
    browseritemdelegate.cpp \
    uavobjecttreemodel.cpp \
    uavobjectbrowserconfiguration.cpp \
    uavobjectbrowseroptionspage.cpp \
    uavobjectbrowser.cpp \
    uavobjectbrowserfactory.cpp \
    uavobjectbrowserwidget.cpp \
    browserplugin.cpp

OTHER_FILES += UAVObjectBrowser.pluginspec

FORMS += \
    uavobjectbrowser.ui \
    uavobjectbrowseroptionspage.ui \
    viewoptions.ui

RESOURCES += \
    uavobjectbrowser.qrc
