TEMPLATE = lib
TARGET = GpsDisplayGadget

QT += svg serialport

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(gpsdisplay_dependencies.pri)
include(../../libs/qwt/qwt.pri)

HEADERS += \
    flatearthwidget.h \
    gpsdisplayplugin.h \
    gpsconstellationwidget.h \
    gpsparser.h \
    telemetryparser.h \
    gpssnrwidget.h \
    buffer.h \
    nmeaparser.h \
    gpsdisplaygadget.h \
    gpsdisplaywidget.h \
    gpsdisplaygadgetfactory.h \
    gpsdisplaygadgetconfiguration.h \
    gpsdisplaygadgetoptionspage.h

SOURCES += \
    flatearthwidget.cpp \
    gpsdisplayplugin.cpp \
    gpsconstellationwidget.cpp \
    gpsparser.cpp \
    telemetryparser.cpp \
    gpssnrwidget.cpp \
    buffer.cpp \
    nmeaparser.cpp \
    gpsdisplaygadget.cpp \
    gpsdisplaygadgetfactory.cpp \
    gpsdisplaywidget.cpp \
    gpsdisplaygadgetconfiguration.cpp \
    gpsdisplaygadgetoptionspage.cpp

OTHER_FILES += GpsDisplayGadget.pluginspec

FORMS += \
    gpsdisplaygadgetoptionspage.ui \
    gpsdisplaywidget.ui

RESOURCES += widgetresources.qrc
