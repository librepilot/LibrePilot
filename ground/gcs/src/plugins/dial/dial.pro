TEMPLATE = lib
TARGET = DialGadget

QT += svg opengl

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(dial_dependencies.pri)

HEADERS += \
    dialplugin.h \
    dialgadget.h \
    dialgadgetwidget.h \
    dialgadgetfactory.h \
    dialgadgetconfiguration.h \
    dialgadgetoptionspage.h

SOURCES += \
    dialplugin.cpp \
    dialgadget.cpp \
    dialgadgetfactory.cpp \
    dialgadgetwidget.cpp \
    dialgadgetconfiguration.cpp \
    dialgadgetoptionspage.cpp

OTHER_FILES += DialGadget.pluginspec

FORMS += dialgadgetoptionspage.ui

RESOURCES += dial.qrc
