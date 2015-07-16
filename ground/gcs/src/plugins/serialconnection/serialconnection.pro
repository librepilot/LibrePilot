TEMPLATE = lib
TARGET = Serial
include(../../plugin.pri)
include(serial_dependencies.pri)
QT += serialport
HEADERS += serialplugin.h \
            serialpluginconfiguration.h \
            serialpluginoptionspage.h
SOURCES += serialplugin.cpp \
            serialpluginconfiguration.cpp \
            serialpluginoptionspage.cpp
FORMS += \ 
    serialpluginoptions.ui
RESOURCES += 
OTHER_FILES += Serial.pluginspec
