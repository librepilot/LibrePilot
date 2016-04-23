TEMPLATE = lib
TARGET = AntennaTrack

QT += serialport

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(antennatrack_dependencies.pri)
include(../../libs/qwt/qwt.pri)

HEADERS += \
    antennatrackplugin.h \
    gpsparser.h \
    telemetryparser.h \
    antennatrackgadget.h \
    antennatrackwidget.h \
    antennatrackgadgetfactory.h \
    antennatrackgadgetconfiguration.h \
    antennatrackgadgetoptionspage.h

SOURCES += \
    antennatrackplugin.cpp \
    gpsparser.cpp \
    telemetryparser.cpp \
    antennatrackgadget.cpp \
    antennatrackgadgetfactory.cpp \
    antennatrackwidget.cpp \
    antennatrackgadgetconfiguration.cpp \
    antennatrackgadgetoptionspage.cpp

OTHER_FILES += AntennaTrack.pluginspec

FORMS += \
    antennatrackgadgetoptionspage.ui \
    antennatrackwidget.ui
