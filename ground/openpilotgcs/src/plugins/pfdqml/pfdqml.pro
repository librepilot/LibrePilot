TEMPLATE = lib
TARGET = PfdQml

QT += svg qml quick quickwidgets

include(../../openpilotgcsplugin.pri)
include(pfdqml_dependencies.pri)

HEADERS += \
    pfdqml.h \
    pfdqmlcontext.h \
    pfdqmlplugin.h \
    pfdqmlgadget.h \
    pfdqmlgadgetwidget.h \
    pfdqmlgadgetfactory.h \
    pfdqmlgadgetconfiguration.h \
    pfdqmlgadgetoptionspage.h

SOURCES += \
    pfdqmlcontext.cpp \
    pfdqmlplugin.cpp \
    pfdqmlgadget.cpp \
    pfdqmlgadgetfactory.cpp \
    pfdqmlgadgetwidget.cpp \
    pfdqmlgadgetconfiguration.cpp \
    pfdqmlgadgetoptionspage.cpp

OTHER_FILES += PfdQml.pluginspec

FORMS += pfdqmlgadgetoptionspage.ui

RESOURCES += PfdResources.qrc
