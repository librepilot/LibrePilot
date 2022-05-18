win32:gstreamer {

    GST_BIN_DIR = $$system(pkg-config --variable=exec_prefix gstreamer-1.0)/bin
    GST_PLUGINS_DIR = $$system(pkg-config --variable=pluginsdir gstreamer-1.0)

    # gstreamer libraries
    GST_LIBS = \
        libgstreamer-1.0-0.dll

    gstreamer_utilities:GST_LIBS += \
        gst-inspect-1.0.exe \
        gst-launch-1.0.exe

    for(lib, GST_LIBS) {
        addCopyFileTarget($${lib},$${GST_BIN_DIR},$${GCS_APP_PATH})
        addCopyDependenciesTarget($${lib},$${GST_BIN_DIR},$${GCS_APP_PATH})
    }

    # gstreamer core
    GST_PLUGINS = \
        libgstcoreelements.dll

    # gst-plugins-base
    GST_PLUGINS += \
        libgstapp.dll \
        libgstaudiotestsrc.dll \
        libgstpango.dll \
        libgstplayback.dll \
        libgsttcp.dll \
        libgsttypefindfunctions.dll \
        libgstvideoconvert.dll \
        libgstvideorate.dll \
        libgstvideoscale.dll \
        libgstvideotestsrc.dll

    # gst-plugins-good
    GST_PLUGINS += \
        libgstautodetect.dll \
        libgstavi.dll \
        libgstdeinterlace.dll \
        libgstdirectsound.dll \
        libgstimagefreeze.dll \
        libgstjpeg.dll \
        libgstrawparse.dll \
        libgstrtp.dll \
        libgstrtpmanager.dll \
        libgstrtsp.dll \
        libgstudp.dll \
        libgstvideomixer.dll

    # gst-plugins-bad
    GST_PLUGINS += \
        libgstaudiovisualizers.dll \
        libgstautoconvert.dll \
        libgstcompositor.dll \
        libgstd3d.dll \
        libgstdebugutilsbad.dll \
        libgstdirectsoundsrc.dll \
        libgstopengl.dll \
        libgstinter.dll \
        libgstmpegpsdemux.dll \
        libgstmpegpsmux.dll \
        libgstmpegtsdemux.dll \
        libgstmpegtsmux.dll \
        libgstvideoparsersbad.dll \
        libgstwinks.dll \
        libgstwinscreencap.dll

    # gst-plugins-ugly
    GST_PLUGINS += \
        libgstmpeg2dec.dll \
        libgstx264.dll

    # gst-libav
    GST_PLUGINS += \
        libgstlibav.dll

    for(lib, GST_PLUGINS) {
        addCopyFileTarget($${lib},$${GST_PLUGINS_DIR},$${GCS_LIBRARY_PATH}/gstreamer-1.0)
        addCopyDependenciesTarget($${lib},$${GST_PLUGINS_DIR},$${GCS_APP_PATH})
    }

}
