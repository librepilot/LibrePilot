TEMPLATE = lib
TARGET = Core

DEFINES += CORE_LIBRARY

QT += widgets qml quick xml network script svg sql

include(../../plugin.pri)
include(../../libs/utils/utils.pri)
include(../../libs/version_info/version_info.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
include(coreplugin_dependencies.pri)
include(authorsdialog.pri)

INCLUDEPATH += \
    dialogs \
    uavgadgetmanager \
    actionmanager

DEPENDPATH += \
    dialogs \
    uavgadgetmanager \
    actionmanager

SOURCES += \
    mainwindow.cpp \
    generalsettings.cpp \
    uniqueidmanager.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    iuavgadget.cpp \
    uavgadgetmanager/uavgadgetmanager.cpp \
    uavgadgetmanager/uavgadgetview.cpp \
    uavgadgetmanager/splitterorview.cpp \
    actionmanager/actionmanager.cpp \
    actionmanager/command.cpp \
    actionmanager/actioncontainer.cpp \
    actionmanager/commandsfile.cpp \
    dialogs/settingsdialog.cpp \
    dialogs/shortcutsettings.cpp \
    basemode.cpp \
    baseview.cpp \
    coreplugin.cpp \
    variablemanager.cpp \
    threadmanager.cpp \
    modemanager.cpp \
    coreimpl.cpp \
    plugindialog.cpp \
    minisplitter.cpp \
    mimedatabase.cpp \
    icore.cpp \
    dialogs/ioptionspage.cpp \
    dialogs/iwizard.cpp \
    settingsdatabase.cpp \
    eventfilteringmainwindow.cpp \
    connectionmanager.cpp \
    iconnection.cpp \
    iuavgadgetconfiguration.cpp \
    uavgadgetinstancemanager.cpp \
    uavgadgetoptionspagedecorator.cpp \
    uavgadgetdecorator.cpp \
    workspacesettings.cpp \
    uavconfiginfo.cpp \
    aboutdialog.cpp \

HEADERS += \
    mainwindow.h \
    generalsettings.h \
    uniqueidmanager.h \
    messagemanager.h \
    messageoutputwindow.h \
    iuavgadget.h \
    iuavgadgetfactory.h \
    uavgadgetmanager/uavgadgetmanager.h \
    uavgadgetmanager/uavgadgetview.h \
    uavgadgetmanager/splitterorview.h \
    actionmanager/actioncontainer.h \
    actionmanager/actionmanager.h \
    actionmanager/command.h \
    actionmanager/actionmanager_p.h \
    actionmanager/command_p.h \
    actionmanager/actioncontainer_p.h \
    actionmanager/commandsfile.h \
    dialogs/settingsdialog.h \
    dialogs/shortcutsettings.h \
    dialogs/iwizard.h \
    dialogs/ioptionspage.h \
    icontext.h \
    icore.h \
    imode.h \
    ioutputpane.h \
    coreconstants.h \
    iversioncontrol.h \
    iview.h \
    icorelistener.h \
    core_global.h \
    basemode.h \
    baseview.h \
    coreplugin.h \
    variablemanager.h \
    threadmanager.h \
    modemanager.h \
    coreimpl.h \
    plugindialog.h \
    minisplitter.h \
    mimedatabase.h \
    settingsdatabase.h \
    eventfilteringmainwindow.h \
    connectionmanager.h \
    iconnection.h \
    iuavgadgetconfiguration.h \
    uavgadgetinstancemanager.h \
    uavgadgetoptionspagedecorator.h \
    uavgadgetdecorator.h \
    workspacesettings.h \
    uavconfiginfo.h \
    iconfigurableplugin.h \
    aboutdialog.h

FORMS += \
    dialogs/settingsdialog.ui \
    dialogs/shortcutsettings.ui \
    generalsettings.ui \
    uavgadgetoptionspage.ui \
    workspacesettings.ui

RESOURCES += \
    core.qrc

unix:!macx { 
    images.files = images/librepilot_logo_*.png
    images.path = /share/pixmaps
    INSTALLS += images
}

OTHER_FILES += Core.pluginspec
