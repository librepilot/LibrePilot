TEMPLATE = lib
TARGET = PfdQml

QT += svg qml quick quickwidgets

include(../../openpilotgcsplugin.pri)
include(pfdqml_dependencies.pri)

HEADERS += \
    pfdqml.h \
    pfdqmlplugin.h \
    pfdqmlgadget.h \
    pfdqmlgadgetwidget.h \
    pfdqmlgadgetfactory.h \
    pfdqmlgadgetconfiguration.h \
    pfdqmlgadgetoptionspage.h

SOURCES += \
    pfdqmlplugin.cpp \
    pfdqmlgadget.cpp \
    pfdqmlgadgetfactory.cpp \
    pfdqmlgadgetwidget.cpp \
    pfdqmlgadgetconfiguration.cpp \
    pfdqmlgadgetoptionspage.cpp

OTHER_FILES += PfdQml.pluginspec

FORMS += pfdqmlgadgetoptionspage.ui

RESOURCES += PfdResources.qrc
