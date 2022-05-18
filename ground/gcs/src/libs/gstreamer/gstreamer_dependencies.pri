DEFINES += USE_GSTREAMER
opencv:DEFINES += USE_OPENCV

macx {
    GLIB_DIR = $$system(brew --prefix glib)
    GSTREAMER_DIR = $$system(brew --prefix gstreamer)
    GST_BASE_DIR = $$system(brew --prefix gst-plugins-base)

    message(Using glib from here: $$GLIB_DIR)
    message(Using gstreamer from here: $$GSTREAMER_DIR)
    message(Using gst base from here: $GST_BASE_DIR)

    INCLUDEPATH += $$GLIB_DIR/include/glib-2.0
    INCLUDEPATH += $$GLIB_DIR/lib/glib-2.0/include

    INCLUDEPATH += $$GST_BASE_DIR/include/gstreamer-1.0/
    INCLUDEPATH += $$GSTREAMER_DIR/include/gstreamer-1.0
    INCLUDEPATH += $$GSTREAMER_DIR/lib/gstreamer-1.0/include

    LIBS +=-L$$GLIB_DIR/lib
    LIBS += -L$$GSTREAMER_DIR/lib -L$$GST_BASE_DIR/lib

    LIBS += -lglib-2.0 -lgobject-2.0
    LIBS += -lgstreamer-1.0 -lgstapp-1.0 -lgstpbutils-1.0 -lgstvideo-1.0
}

linux|win32 {
    CONFIG += link_pkgconfig
    PKGCONFIG += glib-2.0 gobject-2.0
    PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0
    opencv:PKGCONFIG += opencv
}
