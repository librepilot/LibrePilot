TEMPLATE = lib
TARGET = UAVSettingsImportExport

QT += widgets xml

DEFINES += UAVSETTINGSIMPORTEXPORT_LIBRARY

include(../../plugin.pri)
include(uavsettingsimportexport_dependencies.pri)
include(../../libs/version_info/version_info.pri)

HEADERS += \
    uavsettingsimportexport.h \
    importsummary.h \
    uavsettingsimportexportfactory.h

SOURCES += \
    uavsettingsimportexport.cpp \
    importsummary.cpp \
    uavsettingsimportexportfactory.cpp
 
OTHER_FILES += uavsettingsimportexport.pluginspec

FORMS += \
    importsummarydialog.ui
