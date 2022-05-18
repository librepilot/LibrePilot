TEMPLATE = lib 
TARGET = VideoGadget

QT += widgets

include(../../plugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../libs/gstreamer/gstreamer.pri)

HEADERS += \
    helpdialog.h \
    videoplugin.h \
    videogadgetconfiguration.h \
    videogadget.h \
    videogadgetwidget.h \
    videogadgetfactory.h \
    videogadgetoptionspage.h

SOURCES += \
    helpdialog.cpp \
    videoplugin.cpp \
    videogadgetconfiguration.cpp \
    videogadget.cpp \
    videogadgetfactory.cpp \
    videogadgetwidget.cpp \
    videogadgetoptionspage.cpp

OTHER_FILES += \
    VideoGadget.pluginspec

FORMS += \
    helpdialog.ui \
    video.ui \
    videooptionspage.ui

RESOURCES += \
    video.qrc
