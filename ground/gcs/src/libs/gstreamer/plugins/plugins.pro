DEFINES += GST_PLUGIN_BUILD_STATIC

#CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-base-1.0

do_not_compile {
    HEADERS += \
        plugins/cameracalibration/gstopencvutils.h \
        plugins/cameracalibration/gstopencvvideofilter.hpp

    SOURCES += \
        plugins/cameracalibration/gstopencvutils.cpp \
        plugins/cameracalibration/gstopencvvideofilter.cpp \
}

opencv {
    # there is no package for gst opencv yet...
    GSTREAMER_SDK_DIR = $$system(pkg-config --variable=exec_prefix gstreamer-1.0)
    LIBS += -L$(GSTREAMER_SDK_DIR)/lib/gstreamer-1.0/opencv
    LIBS += -lgstopencv-1.0

    HEADERS += \
        plugins/cameracalibration/camerautils.hpp \
        plugins/cameracalibration/cameraevent.hpp \
        plugins/cameracalibration/gstcameracalibration.h \
        plugins/cameracalibration/gstcameraundistort.h

    SOURCES += \
        plugins/cameracalibration/camerautils.cpp \
        plugins/cameracalibration/cameraevent.cpp \
        plugins/cameracalibration/gstcameracalibration.cpp \
        plugins/cameracalibration/gstcameraundistort.cpp
}
