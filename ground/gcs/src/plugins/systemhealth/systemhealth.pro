TEMPLATE = lib
TARGET = SystemHealthGadget

QT += svg

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(systemhealth_dependencies.pri)

HEADERS += \
    systemhealthplugin.h \
    systemhealthgadget.h \
    systemhealthgadgetwidget.h \
    systemhealthgadgetfactory.h \
    systemhealthgadgetconfiguration.h \
    systemhealthgadgetoptionspage.h

SOURCES += \
    systemhealthplugin.cpp \
    systemhealthgadget.cpp \
    systemhealthgadgetfactory.cpp \
    systemhealthgadgetwidget.cpp \
    systemhealthgadgetconfiguration.cpp \
    systemhealthgadgetoptionspage.cpp

OTHER_FILES += SystemHealthGadget.pluginspec

FORMS += systemhealthgadgetoptionspage.ui

RESOURCES += \
    systemhealth.qrc
