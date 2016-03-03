TEMPLATE = lib
TARGET = LineardialGadget

QT += widgets svg opengl

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(lineardial_dependencies.pri)

HEADERS += \
    lineardialplugin.h \
    lineardialgadget.h \
    lineardialgadgetwidget.h \
    lineardialgadgetfactory.h \
    lineardialgadgetconfiguration.h \
    lineardialgadgetoptionspage.h

SOURCES += \
    lineardialplugin.cpp \
    lineardialgadget.cpp \
    lineardialgadgetfactory.cpp \
    lineardialgadgetwidget.cpp \
    lineardialgadgetconfiguration.cpp \
    lineardialgadgetoptionspage.cpp

OTHER_FILES += LineardialGadget.pluginspec

FORMS += lineardialgadgetoptionspage.ui

RESOURCES += lineardial.qrc
