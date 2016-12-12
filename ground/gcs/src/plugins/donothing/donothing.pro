TEMPLATE = lib 
TARGET = DoNothing 
 
include(../../plugin.pri) 
include(../../plugins/coreplugin/coreplugin.pri) 
 
HEADERS += \
    donothingplugin.h
 
SOURCES += \
    donothingplugin.cpp

OTHER_FILES += DoNothing.pluginspec