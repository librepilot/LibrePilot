TEMPLATE = lib
TARGET = Utils

QT += network xml svg gui widgets qml quick quickwidgets

DEFINES += QTCREATOR_UTILS_LIB

include(../../library.pri)

DEFINES += DATA_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_DATA_PATH, $$GCS_APP_PATH)\")
DEFINES += LIB_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_LIBRARY_PATH, $$GCS_APP_PATH)\")
DEFINES += PLUGIN_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_PLUGIN_PATH, $$GCS_APP_PATH)\")

SOURCES += \
    stringutils.cpp \
    pathchooser.cpp \
    basevalidatinglineedit.cpp \
    linecolumnlabel.cpp \
    qtcolorbutton.cpp \
    synchronousprocess.cpp \
    treewidgetcolumnstretcher.cpp \
    checkablemessagebox.cpp \
    styledbar.cpp \
    stylehelper.cpp \
    welcomemodetreewidget.cpp \
    iwelcomepage.cpp \
    detailsbutton.cpp \
    detailswidget.cpp \
    coordinateconversions.cpp \
    pathutils.cpp \
    settingsutils.cpp \
    worldmagmodel.cpp \
    homelocationutil.cpp \
    mytabbedstackwidget.cpp \
    mytabwidget.cpp \
    quickwidgetproxy.cpp \
    svgimageprovider.cpp \
    hostosinfo.cpp \
    logfile.cpp \
    crc.cpp \
    mustache.cpp \
    textbubbleslider.cpp \
    xmlconfig.cpp

HEADERS += \
    utils_global.h \
    stringutils.h \
    listutils.h \
    pathchooser.h \
    basevalidatinglineedit.h \
    linecolumnlabel.h \
    qtcolorbutton.h \
    synchronousprocess.h \
    treewidgetcolumnstretcher.h \
    checkablemessagebox.h \
    qtcassert.h \
    styledbar.h \
    stylehelper.h \
    welcomemodetreewidget.h \
    iwelcomepage.h \
    detailsbutton.h \
    detailswidget.h \
    coordinateconversions.h \
    pathutils.h \
    settingsutils.h \
    worldmagmodel.h \
    homelocationutil.h \
    mytabbedstackwidget.h \
    mytabwidget.h \
    quickwidgetproxy.h \
    svgimageprovider.h \
    hostosinfo.h \
    logfile.h \
    crc.h \
    mustache.h \
    textbubbleslider.h \
    filelogger.h \
    xmlconfig.h

FORMS += \
    checkablemessagebox.ui

RESOURCES += utils.qrc
