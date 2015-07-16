TEMPLATE = lib 
TARGET = FlightLog

QT += qml quick

include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += flightlogplugin.h \
    flightlogmanager.h
SOURCES += flightlogplugin.cpp \
    flightlogmanager.cpp

OTHER_FILES += Flightlog.pluginspec \
    FlightLogDialog.qml \
    functions.js

FORMS +=

RESOURCES += \
    flightLog.qrc
