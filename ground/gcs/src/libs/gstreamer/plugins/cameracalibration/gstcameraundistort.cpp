/*
 * GStreamer
 * Copyright (C) <2017> Philippe Renon <philippe_renon@yahoo.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-cameraundistort
 *
 * Performs camera distortion correction.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>

#include "camerautils.hpp"
#include "cameraevent.hpp"

#include <QDebug>
#include <QElapsedTimer>

#include "gstcameraundistort.h"

#if (CV_MAJOR_VERSION >= 3)
#include <opencv2/imgproc.hpp>
#endif
#include <opencv2/calib3d.hpp>

#include <gst/opencv/gstopencvutils.h>

GST_DEBUG_CATEGORY_STATIC (gst_camera_undistort_debug);
#define GST_CAT_DEFAULT gst_camera_undistort_debug

#define DEFAULT_SHOW_UNDISTORTED true
#define DEFAULT_ALPHA 1.0
#define DEFAULT_CROP true

enum
{
  PROP_0,
  PROP_SHOW_UNDISTORTED,
  PROP_ALPHA,
  PROP_CROP,
  PROP_SETTINGS
};

/*#define GST_CAMERA_UNDISTORT_GET_LOCK(playsink) (&((GstCameraUndistort *)undist)->lock)
#define GST_CAMERA_UNDISTORT_LOCK(undist)     G_STMT_START { \
  GST_LOG_OBJECT (playsink, "locking from thread %p", g_thread_self ()); \
  g_rec_mutex_lock (GST_CAMERA_UNDISTORT_GET_LOCK (undist)); \
  GST_LOG_OBJECT (playsink, "locked from thread %p", g_thread_self ()); \
} G_STMT_END
#define GST_CAMERA_UNDISTORT_UNLOCK(undist)   G_STMT_START { \
  GST_LOG_OBJECT (playsink, "unlocking from thread %p", g_thread_self ()); \
  g_rec_mutex_unlock (GST_CAMERA_UNDISTORT_GET_LOCK (undist)); \
} G_STMT_END*/

G_DEFINE_TYPE (GstCameraUndistort, gst_camera_undistort, GST_TYPE_OPENCV_VIDEO_FILTER);

static void gst_camera_undistort_dispose (GObject * object);
static void gst_camera_undistort_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_camera_undistort_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_camera_undistort_set_info (GstOpencvVideoFilter * cvfilter,
    gint in_width, gint in_height, gint in_depth, gint in_channels,
    gint out_width, gint out_height, gint out_depth, gint out_channels);
static GstFlowReturn gst_camera_undistort_transform_frame (
    GstOpencvVideoFilter * cvfilter,
    GstBuffer * frame, IplImage * img,
    GstBuffer * outframe, IplImage * outimg);

static gboolean gst_camera_undistort_sink_event (GstBaseTransform *trans, GstEvent *event);
static gboolean gst_camera_undistort_src_event (GstBaseTransform *trans, GstEvent *event);

static void camera_undistort_run(GstCameraUndistort *undist, IplImage *img, IplImage *outimg);
static gboolean camera_undistort_init_undistort_rectify_map(GstCameraUndistort *undist);

/* initialize the cameraundistort's class */
static void
gst_camera_undistort_class_init (GstCameraUndistortClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstOpencvVideoFilterClass *opencvfilter_class = GST_OPENCV_VIDEO_FILTER_CLASS (klass);

  GstCaps *caps;
  GstPadTemplate *templ;

  gobject_class->dispose = gst_camera_undistort_dispose;
  gobject_class->set_property = gst_camera_undistort_set_property;
  gobject_class->get_property = gst_camera_undistort_get_property;

  trans_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_camera_undistort_sink_event);
  trans_class->src_event =
      GST_DEBUG_FUNCPTR (gst_camera_undistort_src_event);

  opencvfilter_class->cv_set_caps = gst_camera_undistort_set_info;
  opencvfilter_class->cv_trans_func =
      gst_camera_undistort_transform_frame;

  g_object_class_install_property (gobject_class, PROP_SHOW_UNDISTORTED,
      g_param_spec_boolean ("show-undistorted", "Show Undistorted",
          "Show undistorted images",
          DEFAULT_SHOW_UNDISTORTED, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ALPHA,
      g_param_spec_float ("alpha", "Pixels",
          "Show all pixels (1), only valid ones (0) or something in between",
          0.0, 1.0, DEFAULT_ALPHA,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SETTINGS,
      g_param_spec_string ("settings", "Settings",
          "Undistort settings (OpenCV serialized opaque string)",
          NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (element_class,
      "cameraundistort",
      "Filter/Effect/Video",
      "Performs camera undistort",
      "Philippe Renon <philippe_renon@yahoo.fr>");

  /* add sink and source pad templates */
  caps = gst_opencv_caps_from_cv_image_type (CV_16UC1);
  gst_caps_append (caps, gst_opencv_caps_from_cv_image_type (CV_8UC4));
  gst_caps_append (caps, gst_opencv_caps_from_cv_image_type (CV_8UC3));
  gst_caps_append (caps, gst_opencv_caps_from_cv_image_type (CV_8UC1));
  templ = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
      gst_caps_ref (caps));
  gst_element_class_add_pad_template (element_class, templ);
  templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps);
  gst_element_class_add_pad_template (element_class, templ);
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_camera_undistort_init (GstCameraUndistort * undist)
{
  undist->showUndistorted = DEFAULT_SHOW_UNDISTORTED;
  undist->alpha = DEFAULT_ALPHA;
  undist->crop = DEFAULT_CROP;

  undist->doUndistort = false;
  undist->settingsChanged = false;

  undist->cameraMatrix = 0;
  undist->distCoeffs = 0;
  undist->map1 = 0;
  undist->map2 = 0;
  //undist->validPixROI = 0;

  undist->settings = NULL;
}

static void
gst_camera_undistort_dispose (GObject * object)
{
  GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (object);

  g_free (undist->settings);
  undist->settings = NULL;

  G_OBJECT_CLASS (gst_camera_undistort_parent_class)->dispose (object);
}


static void
gst_camera_undistort_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (object);
  const char *str;

  switch (prop_id) {
    case PROP_SHOW_UNDISTORTED:
      undist->showUndistorted = g_value_get_boolean (value);
      undist->settingsChanged = true;
      break;
    case PROP_ALPHA:
      undist->alpha = g_value_get_float (value);
      undist->settingsChanged = true;
      break;
    case PROP_CROP:
      undist->crop = g_value_get_boolean (value);
      break;
    case PROP_SETTINGS:
      if (undist->settings) {
        g_free (undist->settings);
        undist->settings = NULL;
      }
      str = g_value_get_string (value);
      if (str)
          undist->settings = g_strdup (str);
      undist->settingsChanged = true;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_camera_undistort_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (object);

  switch (prop_id) {
    case PROP_SHOW_UNDISTORTED:
      g_value_set_boolean (value, undist->showUndistorted);
      break;
    case PROP_ALPHA:
      g_value_set_float (value, undist->alpha);
      break;
    case PROP_CROP:
      g_value_set_boolean (value, undist->crop);
      break;
    case PROP_SETTINGS:
      g_value_set_string (value, undist->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_camera_undistort_set_info (GstOpencvVideoFilter * cvfilter,
    gint in_width, gint in_height,
    __attribute__((unused)) gint in_depth, __attribute__((unused)) gint in_channels,
    __attribute__((unused)) gint out_width, __attribute__((unused)) gint out_height,
    __attribute__((unused)) gint out_depth, __attribute__((unused)) gint out_channels)
{
  GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (cvfilter);

  undist->imageSize = cv::Size(in_width, in_height);

  return TRUE;
}

//static GstMessage *
//gst_camera_undistort_message_new (GstCameraUndistort * undist, GstBuffer * buf)
//{
//  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (undist);
//  GstStructure *s;
//  GstClockTime running_time, stream_time;
//
//  running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
//      GST_BUFFER_TIMESTAMP (buf));
//  stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
//      GST_BUFFER_TIMESTAMP (buf));
//
//  s = gst_structure_new ("cameracalibration",
//      "timestamp", G_TYPE_UINT64, GST_BUFFER_TIMESTAMP (buf),
//      "stream-time", G_TYPE_UINT64, stream_time,
//      "running-time", G_TYPE_UINT64, running_time,
//      "duration", G_TYPE_UINT64, GST_BUFFER_DURATION (buf), NULL);
//
//  return gst_message_new_element (GST_OBJECT (undist), s);
//}

/*
 * Performs the camera calibration
 */
static GstFlowReturn
gst_camera_undistort_transform_frame (GstOpencvVideoFilter * cvfilter,
    G_GNUC_UNUSED GstBuffer * frame, IplImage * img,
    G_GNUC_UNUSED GstBuffer * outframe, IplImage * outimg)
{
  GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (cvfilter);

  camera_undistort_run(undist, img, outimg);

  return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
gboolean
gst_camera_undistort_plugin_init (GstPlugin * plugin)
{
  /* debug category for filtering log messages */
  GST_DEBUG_CATEGORY_INIT (gst_camera_undistort_debug, "cameraundistort",
      0,
      "Performs camera undistortion");

  return gst_element_register (plugin, "cameraundistort", GST_RANK_NONE,
      GST_TYPE_CAMERA_UNDISTORT);
}

static void
camera_undistort_run(GstCameraUndistort *undist, IplImage *img, IplImage *outimg)
{
    const cv::Mat view = cv::cvarrToMat(img);
    cv::Mat outview = cv::cvarrToMat(outimg);

    if (undist->settingsChanged) {
        undist->doUndistort = false;
        if (undist->showUndistorted && undist->settings) {
            //qDebug() << undist->settings;
            if (camera_deserialize_undistort_settings(
                    undist->settings, undist->cameraMatrix, undist->distCoeffs)) {
                undist->doUndistort = camera_undistort_init_undistort_rectify_map(undist);
            }
        }
        undist->settingsChanged = false;
    }

    if (undist->showUndistorted && undist->doUndistort) {

        QElapsedTimer timer;
        timer.start();

        cv::remap(view, outview, undist->map1, undist->map2, cv::INTER_LINEAR);

        qDebug() << "remap took" << timer.elapsed() << "ms";

        if (undist->crop) {
          const cv::Scalar CROP_COLOR(0, 255, 0);
          cv::rectangle(outview, undist->validPixROI, CROP_COLOR);
        }
    }
    else {
      // FIXME should use passthrough to avoid this copy...
      view.copyTo(outview);
    }
}


//    {
//        Mat view, rview, map1, map2;
//
//        if (undist->useFisheye)
//        {
//            Mat newCamMat;
//            fisheye::estimateNewCameraMatrixForUndistortRectify(cameraMatrix, distCoeffs, imageSize,
//                                                                Matx33d::eye(), newCamMat, 1);
//            fisheye::initUndistortRectifyMap(cameraMatrix, distCoeffs, Matx33d::eye(), newCamMat, imageSize,
//                                             CV_16SC2, map1, map2);
//        }
//        else
//        {
//            initUndistortRectifyMap(
//                cameraMatrix, distCoeffs, Mat(),
//                getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0), imageSize,
//                CV_16SC2, map1, map2);
//        }
//    }


static gboolean
camera_undistort_init_undistort_rectify_map(GstCameraUndistort *undist)
{
  QElapsedTimer timer;
  timer.start();

  cv::Size newImageSize;
  cv::Rect validPixROI;
  cv::Mat newCameraMatrix = cv::getOptimalNewCameraMatrix(
        undist->cameraMatrix, undist->distCoeffs, undist->imageSize,
        undist->alpha, newImageSize, &validPixROI);
  undist->validPixROI = validPixROI;

  cv::initUndistortRectifyMap(undist->cameraMatrix, undist->distCoeffs, cv::Mat(),
        newCameraMatrix, undist->imageSize, CV_16SC2, undist->map1, undist->map2);

  qDebug() << "init rectify took" << timer.elapsed() << "ms";

  return TRUE;
}

/*
qDebug() << "imageSize" << imageSize.width << imageSize.height;
qDebug() << "newImageSize" << imageSize.width << imageSize.height;
qDebug() << "alpha" << undist->alpha;
qDebug() << "roi" << undist->validPixROI.x << undist->validPixROI.y << undist->validPixROI.width << undist->validPixROI.height;

cv::FileStorage fs1(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);
fs1 << "cameraMatrix" << undist->cameraMatrix;
const std::string buf1 = fs1.releaseAndGetString();
qDebug() << "cameraMatrix" << QString::fromStdString(buf1);

cv::FileStorage fs2(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);
fs2 << "newCameraMatrix" << newCameraMatrix;
const std::string buf2 = fs2.releaseAndGetString();
qDebug() << "newCameraMatrix" << QString::fromStdString(buf2);

cv::FileStorage fs3(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);
fs3 << "distCoeffs" << undist->distCoeffs;
const std::string buf3 = fs3.releaseAndGetString();
qDebug() << "distCoeffs" << QString::fromStdString(buf3);
*/

static gboolean camera_undistort_calibration_event(GstCameraUndistort *undist, GstEvent *event)
{
  g_free (undist->settings);

  if (!gst_camera_event_parse_calibrated(event, &(undist->settings))) {
      qDebug() << "Failed to parse";
      return FALSE;
  }

  undist->settingsChanged = true;

  return TRUE;
}

static gboolean
gst_camera_undistort_sink_event (GstBaseTransform *trans, GstEvent *event)
{
    GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (trans);
    const GstStructure *structure = gst_event_get_structure (event);

    if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_BOTH && structure) {

      if (strcmp (gst_structure_get_name (structure), GST_CAMERA_EVENT_CALIBRATED_NAME) == 0) {
          qDebug() << "GOT CALIBRATION EVENT FROM UPSTREAM";
          return camera_undistort_calibration_event(undist, event);
      }
    }

    return GST_BASE_TRANSFORM_CLASS (gst_camera_undistort_parent_class)->sink_event (trans, event);
  }


static gboolean
gst_camera_undistort_src_event (GstBaseTransform *trans, GstEvent *event)
{
    GstCameraUndistort *undist = GST_CAMERA_UNDISTORT (trans);
    const GstStructure *structure = gst_event_get_structure (event);

    if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_BOTH && structure) {

      if (strcmp (gst_structure_get_name (structure), GST_CAMERA_EVENT_CALIBRATED_NAME) == 0) {
          qDebug() << "GOT CALIBRATION EVENT FROM DOWNSTREAM";
          return camera_undistort_calibration_event(undist, event);
      }
    }

    return GST_BASE_TRANSFORM_CLASS (gst_camera_undistort_parent_class)->src_event (trans, event);
}

