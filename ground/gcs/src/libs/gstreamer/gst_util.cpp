/**
 ******************************************************************************
 *
 * @file       gst_util.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "gst_util.h"

#include <gst/gst.h>

#ifdef USE_OPENCV
#include "plugins/cameracalibration/gstcameracalibration.h"
#include "plugins/cameracalibration/gstcameraundistort.h"
#endif

#include "utils/pathutils.h"

#include <QDebug>

static bool initialized = false;

gboolean gst_plugin_librepilot_register(GstPlugin *plugin)
{
#ifdef USE_OPENCV
    if (!gst_camera_calibration_plugin_init(plugin)) {
        return FALSE;
    }
    if (!gst_camera_undistort_plugin_init(plugin)) {
        return FALSE;
    }
#else
    Q_UNUSED(plugin)
#endif
    return TRUE;
}

void gst_plugin_librepilot_register()
{
    gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "librepilot",
                               "LibrePilot plugin", gst_plugin_librepilot_register, "1.10.0", "GPL",
                               "librepilot", "LibrePilot", "http://librepilot.org/");
}

// see http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gst-running.html
void gst::init(int *argc, char * *argv[])
{
    // TODO Not thread safe. Does it need to be?
    if (initialized) {
        return;
    }
    initialized = true;

    // qputenv("GST_DEBUG", "3");
    // qputenv("GST_DEBUG", "3,rtspsrc:6,udpsrc:6");
    // qputenv("GST_DEBUG", "3,bin:6");
    // qputenv("GST_DEBUG", "3,rtpjitterbuffer:6");
    // qputenv("GST_DEBUG_FILE", "gst.log");
    // qputenv("GST_DEBUG_DUMP_DOT_DIR", ".");

#ifdef Q_OS_WIN
    qputenv("GST_PLUGIN_PATH_1_0", (Utils::GetLibraryPath() + "gstreamer-1.0").toLatin1());
#endif

    qDebug() << "gstreamer - initializing";
    GError *error = NULL;
    if (!gst_init_check(argc, argv, &error)) {
        qCritical() << "failed to initialize gstreamer";
        return;
    }

    qDebug() << "gstreamer - version:" << gst_version_string();
    qDebug() << "gstreamer - plugin system path:" << qgetenv("GST_PLUGIN_SYSTEM_PATH_1_0");
    qDebug() << "gstreamer - plugin path:" << qgetenv("GST_PLUGIN_PATH_1_0");

    qDebug() << "gstreamer - registering plugins";
    // GST_PLUGIN_STATIC_REGISTER(librepilot);
    gst_plugin_librepilot_register();

#ifdef Q_OS_MAC
    GstRegistry *reg = gst_registry_get();

    GstPluginFeature *feature = gst_registry_lookup_feature(reg, "osxvideosink");
    if (feature) {
        // raise rank of osxvideosink so it gets selected by autovideosink
        // if not doing that then autovideosink selects the glimagesink which fails in Qt
        gst_plugin_feature_set_rank(feature, GST_RANK_PRIMARY);
        gst_object_unref(feature);
    }
#endif

#ifdef USE_OPENCV
    // see http://stackoverflow.com/questions/32477403/how-to-know-if-sse2-is-activated-in-opencv
    // see http://answers.opencv.org/question/696/how-to-enable-vectorization-in-opencv/
    if (!cv::checkHardwareSupport(CV_CPU_SSE)) {
        qWarning() << "SSE not supported";
    }
    if (!cv::checkHardwareSupport(CV_CPU_SSE2)) {
        qWarning() << "SSE2 not supported";
    }
    if (!cv::checkHardwareSupport(CV_CPU_SSE3)) {
        qWarning() << "SSE3 not supported";
    }
    qDebug() << "MMX    :" << cv::checkHardwareSupport(CV_CPU_MMX);
    qDebug() << "SSE    :" << cv::checkHardwareSupport(CV_CPU_SSE);
    qDebug() << "SSE2   :" << cv::checkHardwareSupport(CV_CPU_SSE2);
    qDebug() << "SSE3   :" << cv::checkHardwareSupport(CV_CPU_SSE3);
    qDebug() << "SSE4_1 :" << cv::checkHardwareSupport(CV_CPU_SSE4_1);
    qDebug() << "SSE4_2 :" << cv::checkHardwareSupport(CV_CPU_SSE4_2);
#endif
}

QString gst::version(void)
{
    init(NULL, NULL);
    return QString(gst_version_string());
}
