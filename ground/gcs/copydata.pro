include(gcs.pri)

TEMPLATE = aux

# Copy Qt runtime libraries into the build directory (to run or package)

linux {
    QT_LIBS = \
        libQt5Core.so.5 \
        libQt5Gui.so.5 \
        libQt5Widgets.so.5 \
        libQt5Network.so.5 \
        libQt5OpenGL.so.5 \
        libQt5Sql.so.5 \
        libQt5Svg.so.5 \
        libQt5Test.so.5 \
        libQt5Xml.so.5 \
        libQt5XmlPatterns.so.5 \
        libQt5Script.so.5 \
        libQt5Concurrent.so.5 \
        libQt5PrintSupport.so.5 \
        libQt5SerialPort.so.5 \
        libQt5Multimedia.so.5 \
        libQt5MultimediaWidgets.so.5 \
        libQt5Quick.so.5 \
        libQt5QuickWidgets.so.5 \
        libQt5Qml.so.5 \
        libQt5DBus.so.5 \
        libQt5QuickParticles.so.5 \
        libQt5XcbQpa.so.5 \
        libQt5X11Extras.so.5 \
        libicui18n.so.56 \
        libicuuc.so.56 \
        libicudata.so.56

    contains(QT_ARCH, x86_64) {
        QT_LIBS += \
            libqgsttools_p.so.1
    }

    for(lib, QT_LIBS) {
        addCopyFileTarget($${lib},$$[QT_INSTALL_LIBS],$${GCS_QT_LIBRARY_PATH})
    }

    QT_PLUGINS = \
        iconengines/libqsvgicon.so \
        imageformats/libqgif.so \
        imageformats/libqico.so \
        imageformats/libqjpeg.so \
        imageformats/libqsvg.so \
        imageformats/libqtiff.so \
        platforms/libqxcb.so \
        xcbglintegrations/libqxcb-glx-integration.so \
        sqldrivers/libqsqlite.so

    contains(QT_ARCH, x86_64) {
        QT_PLUGINS += \
            mediaservice/libgstaudiodecoder.so \
            mediaservice/libgstmediaplayer.so
    } else {
        QT_PLUGINS += \
            mediaservice/libqtmedia_audioengine.so
    }
}

win32 {
    # set debug suffix if needed
    CONFIG(debug, debug|release):DS = "d"

    # copy Qt DLLs
    QT_DLLS = \
        Qt5Core$${DS}.dll \
        Qt5Gui$${DS}.dll \
        Qt5Widgets$${DS}.dll \
        Qt5Network$${DS}.dll \
        Qt5OpenGL$${DS}.dll \
        Qt5Sql$${DS}.dll \
        Qt5Svg$${DS}.dll \
        Qt5Test$${DS}.dll \
        Qt5Xml$${DS}.dll \
        Qt5XmlPatterns$${DS}.dll \
        Qt5Script$${DS}.dll \
        Qt5Concurrent$${DS}.dll \
        Qt5PrintSupport$${DS}.dll \
        Qt5SerialPort$${DS}.dll \
        Qt5Multimedia$${DS}.dll \
        Qt5MultimediaWidgets$${DS}.dll \
        Qt5Quick$${DS}.dll \
        Qt5QuickWidgets$${DS}.dll \
        Qt5Qml$${DS}.dll

    for(dll, QT_DLLS) {
        addCopyFileTarget($${dll},$$[QT_INSTALL_BINS],$${GCS_APP_PATH})
        addCopyDependenciesTarget($${dll},$$[QT_INSTALL_BINS],$${GCS_APP_PATH})
    }

    # copy OpenSSL DLLs
    OPENSSL_DLLS = \
        ssleay32.dll \
        libeay32.dll

    for(dll, OPENSSL_DLLS) {
        addCopyFileTarget($${dll},$$[QT_INSTALL_BINS],$${GCS_APP_PATH})
    }

    QT_PLUGINS = \
        iconengines/qsvgicon$${DS}.dll \
        imageformats/qgif$${DS}.dll \
        imageformats/qico$${DS}.dll \
        imageformats/qjpeg$${DS}.dll \
        imageformats/qsvg$${DS}.dll \
        imageformats/qtiff$${DS}.dll \
        platforms/qwindows$${DS}.dll \
        mediaservice/dsengine$${DS}.dll \
        sqldrivers/qsqlite$${DS}.dll
}

for(plugin, QT_PLUGINS) {
    addCopyFileTarget($${plugin},$$[QT_INSTALL_PLUGINS],$${GCS_QT_PLUGINS_PATH})
    win32:addCopyDependenciesTarget($${plugin},$$[QT_INSTALL_PLUGINS],$${GCS_APP_PATH})
}

# Copy QtQuick2 complete directories
# Some of these directories have a lot of files
# Easier to copy everything
QT_QUICK2_DIRS = \
    QtQuick/Controls \
    QtQuick/Dialogs \
    QtQuick/Layouts \
    QtQuick/LocalStorage \
    QtQuick/Particles.2 \
    QtQuick/PrivateWidgets \
    QtQuick/Window.2 \
    QtQuick/XmlListModel \
    QtQuick.2

for(dir, QT_QUICK2_DIRS) {
    addCopyDirTarget($${dir},$$[QT_INSTALL_QML],$${GCS_QT_QML_PATH})
}
