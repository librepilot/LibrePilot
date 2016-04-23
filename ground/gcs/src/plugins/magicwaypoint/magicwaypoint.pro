TEMPLATE = lib
TARGET = MagicWaypoint 

QT += svg

include(../../plugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += \
    magicwaypointgadget.h \
    magicwaypointgadgetwidget.h \
    magicwaypointgadgetfactory.h \
    magicwaypointplugin.h \
    positionfield.h

SOURCES += \
    magicwaypointgadget.cpp \
    magicwaypointgadgetwidget.cpp \
    magicwaypointgadgetfactory.cpp \
    magicwaypointplugin.cpp \
    positionfield.cpp

OTHER_FILES += MagicWaypoint.pluginspec

FORMS += magicwaypoint.ui

RESOURCES += magicwaypoint.qrc
