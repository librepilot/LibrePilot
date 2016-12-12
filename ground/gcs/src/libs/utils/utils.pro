TEMPLATE = lib
TARGET = Utils

QT += network xml svg gui widgets qml quick quickwidgets

DEFINES += QTCREATOR_UTILS_LIB

include(../../library.pri)

DEFINES += DATA_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_DATA_PATH, $$GCS_APP_PATH)\")
DEFINES += LIB_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_LIBRARY_PATH, $$GCS_APP_PATH)\")
DEFINES += PLUGIN_REL_PATH=$$shell_quote(\"$$relative_path($$GCS_PLUGIN_PATH, $$GCS_APP_PATH)\")

SOURCES += \
    reloadpromptutils.cpp \
    stringutils.cpp \
    filesearch.cpp \
    pathchooser.cpp \
    pathlisteditor.cpp \
    filewizardpage.cpp \
    filewizarddialog.cpp \
    projectintropage.cpp \
    basevalidatinglineedit.cpp \
    filenamevalidatinglineedit.cpp \
    projectnamevalidatinglineedit.cpp \
    codegeneration.cpp \
    newclasswidget.cpp \
    classnamevalidatinglineedit.cpp \
    linecolumnlabel.cpp \
    fancylineedit.cpp \
    qtcolorbutton.cpp \
    savedaction.cpp \
    submiteditorwidget.cpp \
    synchronousprocess.cpp \
    submitfieldwidget.cpp \
    consoleprocess.cpp \
    uncommentselection.cpp \
    parameteraction.cpp \
    treewidgetcolumnstretcher.cpp \
    checkablemessagebox.cpp \
    styledbar.cpp \
    stylehelper.cpp \
    welcomemodetreewidget.cpp \
    iwelcomepage.cpp \
    fancymainwindow.cpp \
    detailsbutton.cpp \
    detailswidget.cpp \
    coordinateconversions.cpp \
    pathutils.cpp \
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
    textbubbleslider.cpp


SOURCES += xmlconfig.cpp

win32 {
    SOURCES += \
        abstractprocess_win.cpp \
        consoleprocess_win.cpp \
        winutils.cpp
    HEADERS += winutils.h
}
else:SOURCES += consoleprocess_unix.cpp

HEADERS += \
    utils_global.h \
    reloadpromptutils.h \
    stringutils.h \
    filesearch.h \
    listutils.h \
    pathchooser.h \
    pathlisteditor.h \
    filewizardpage.h \
    filewizarddialog.h \
    projectintropage.h \
    basevalidatinglineedit.h \
    filenamevalidatinglineedit.h \
    projectnamevalidatinglineedit.h \
    codegeneration.h \
    newclasswidget.h \
    classnamevalidatinglineedit.h \
    linecolumnlabel.h \
    fancylineedit.h \
    qtcolorbutton.h \
    savedaction.h \
    submiteditorwidget.h \
    abstractprocess.h \
    consoleprocess.h \
    synchronousprocess.h \
    submitfieldwidget.h \
    uncommentselection.h \
    parameteraction.h \
    treewidgetcolumnstretcher.h \
    checkablemessagebox.h \
    qtcassert.h \
    styledbar.h \
    stylehelper.h \
    welcomemodetreewidget.h \
    iwelcomepage.h \
    fancymainwindow.h \
    detailsbutton.h \
    detailswidget.h \
    coordinateconversions.h \
    pathutils.h \
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
    filelogger.h

HEADERS += xmlconfig.h

FORMS += \
    filewizardpage.ui \
    projectintropage.ui \
    newclasswidget.ui \
    submiteditorwidget.ui \
    checkablemessagebox.ui

RESOURCES += utils.qrc
