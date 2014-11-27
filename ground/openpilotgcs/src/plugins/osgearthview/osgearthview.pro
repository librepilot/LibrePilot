TEMPLATE = lib
TARGET = OsgEarthviewGadget

QT += widgets opengl
#QT += declarative

include(../../openpilotgcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(osgearthview_dependencies.pri)
include(../../osgearth.pri)

HEADERS += \
    osgearthviewplugin.h \
    osgviewerwidget.h \
    osgearthviewgadget.h \
    osgearthviewwidget.h \
    osgearthviewgadgetfactory.h \
    osgearthviewgadgetconfiguration.h \
    osgearthviewgadgetoptionspage.h \

SOURCES += \
    osgearthviewplugin.cpp \
    osgviewerwidget.cpp \
    osgearthviewgadget.cpp \
    osgearthviewwidget.cpp \
    osgearthviewgadgetfactory.cpp \
    osgearthviewgadgetconfiguration.cpp \
    osgearthviewgadgetoptionspage.cpp \

FORMS += \
    osgearthviewgadgetoptionspage.ui \
    osgearthview.ui

OTHER_FILES += OsgEarthviewGadget.pluginspec

RESOURCES += osgearthview.qrc

include(copydata.pro)

