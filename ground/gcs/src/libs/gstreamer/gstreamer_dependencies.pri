DEFINES += USE_GSTREAMER
opencv:DEFINES += USE_OPENCV

linux|win32 {
    CONFIG += link_pkgconfig
    PKGCONFIG += glib-2.0 gobject-2.0
    PKGCONFIG += gstreamer-1.0 gstreamer-video-1.0
    opencv:PKGCONFIG += opencv
}
