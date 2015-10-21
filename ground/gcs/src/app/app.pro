include(../../gcs.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
TARGET = $$GCS_APP_TARGET
macx {
    # .app is 3 levels above the executable
    DESTDIR = $$GCS_APP_PATH/../../..
} else {
    DESTDIR = $$GCS_APP_PATH
}

QT += xml widgets

SOURCES += main.cpp \
    gcssplashscreen.cpp

include(../libs/utils/utils.pri)
include(../libs/version_info/version_info.pri)

LIBS *= -l$$qtLibraryName(ExtensionSystem) -l$$qtLibraryName(Aggregation)

win32 {
    RC_FILE = gcs.rc
    target.path = /bin
    INSTALLS += target
} else:macx {
    LIBS += -framework CoreFoundation
    ICON = gcs.icns
    QMAKE_INFO_PLIST = Info.plist
    FILETYPES.files = profile.icns prifile.icns
    FILETYPES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILETYPES
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Plugins/
} else {
    target.path  = /bin
    INSTALLS    += target
    QMAKE_RPATHDIR  = $$shell_quote(\$$ORIGIN/$$relative_path($$GCS_LIBRARY_PATH, $$GCS_APP_PATH))
    QMAKE_RPATHDIR += $$shell_quote(\$$ORIGIN/$$relative_path($$GCS_QT_LIBRARY_PATH, $$GCS_APP_PATH))
    include(../rpath.pri)

    equals(copyqt, 1) {
        RESOURCES += $$OUT_PWD/qtconf.qrc

        # Copy qtconf.qrc to OUT_PWD because paths are relative
        # This needs to be done at qmake time because the Makefile depends on it
        system(cp $$PWD/qtconf.qrc.in $$OUT_PWD/qtconf.qrc)
        system(printf $$shell_quote([Paths]\nPrefix = $$relative_path($$GCS_QT_BASEPATH, $$GCS_APP_PATH)\n) > $$OUT_PWD/qt.conf)
    }
}

OTHER_FILES += gcs.rc

RESOURCES += \
    appresources.qrc

HEADERS += \
    gcssplashscreen.h
