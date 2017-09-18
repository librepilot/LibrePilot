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
 * SECTION:element-cameracalibration
 *
 * Performs fcamera calibration.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstcameracalibration.h"

#if (CV_MAJOR_VERSION >= 3)
#include <opencv2/imgproc.hpp>
#endif
#include <opencv2/calib3d.hpp>

#include <gst/opencv/gstopencvutils.h>

#include "camerautils.hpp"
#include "cameraevent.hpp"

#include <vector>

#include <QDebug>

GST_DEBUG_CATEGORY_STATIC (gst_camera_calibration_debug);
#define GST_CAT_DEFAULT gst_camera_calibration_debug

#define DEFAULT_CALIBRATON_PATTERN GST_CAMERACALIBRATION_PATTERN_CHESSBOARD
#define DEFAULT_BOARD_WIDTH 9
#define DEFAULT_BOARD_HEIGHT 6
#define DEFAULT_SQUARE_SIZE 50
#define DEFAULT_ASPECT_RATIO 1.0
#define DEFAULT_CORNER_SUB_PIXEL true
#define DEFAULT_ZERO_TANGENT_DISTORTION false
#define DEFAULT_CENTER_PRINCIPAL_POINT false
#define DEFAULT_USE_FISHEYE false
#define DEFAULT_FRAME_COUNT 25
#define DEFAULT_DELAY 350
#define DEFAULT_SHOW_CORNERS true

///* Filter signals and args */
//enum
//{
//  /* FILL ME */
//  LAST_SIGNAL
//};

enum
{
  PROP_0,
  PROP_CALIBRATON_PATTERN,
  PROP_BOARD_WIDTH,
  PROP_BOARD_HEIGHT,
  PROP_SQUARE_SIZE,
  PROP_ASPECT_RATIO,
  PROP_CORNER_SUB_PIXEL,
  PROP_ZERO_TANGENT_DISTORTION,
  PROP_CENTER_PRINCIPAL_POINT,
  PROP_USE_FISHEYE,
  PROP_FRAME_COUNT,
  PROP_DELAY,
  PROP_SHOW_CORNERS
};

enum {
 DETECTION = 0,
 CAPTURING = 1,
 CALIBRATED = 2
};

#define GST_TYPE_CAMERA_CALIBRATION_PATTERN (cameracalibration_pattern_get_type ())

static GType
cameracalibration_pattern_get_type (void)
{
  static GType cameracalibration_pattern_type = 0;
  static const GEnumValue cameracalibration_pattern[] = {
    {GST_CAMERACALIBRATION_PATTERN_CHESSBOARD, "Chessboard", "chessboard"},
    {GST_CAMERACALIBRATION_PATTERN_CIRCLES_GRID, "Circle Grids", "circle_grids"},
    {GST_CAMERACALIBRATION_PATTERN_ASYMMETRIC_CIRCLES_GRID, "Asymmetric Circle Grids", "asymmetric_circle_grids"},
    {0, NULL, NULL},
  };

  if (!cameracalibration_pattern_type) {
    cameracalibration_pattern_type =
        g_enum_register_static ("GstCameraCalibrationPattern", cameracalibration_pattern);
  }
  return cameracalibration_pattern_type;
}

G_DEFINE_TYPE (GstCameraCalibration, gst_camera_calibration, GST_TYPE_OPENCV_VIDEO_FILTER);

static void gst_camera_calibration_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_camera_calibration_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

//static gboolean gst_camera_calibration_set_caps (GstOpencvVideoFilter * transform,
//    gint in_width, gint in_height, gint in_depth, gint in_channels,
//    gint out_width, gint out_height, gint out_depth, gint out_channels);
static GstFlowReturn gst_camera_calibration_transform_frame_ip (
    GstOpencvVideoFilter * cvfilter, GstBuffer * frame, IplImage * img);

/* Clean up */
static void
gst_camera_calibration_finalize (GObject * obj)
{
  G_OBJECT_CLASS (gst_camera_calibration_parent_class)->finalize (obj);
}

/* initialize the cameracalibration's class */
static void
gst_camera_calibration_class_init (GstCameraCalibrationClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstOpencvVideoFilterClass *opencvfilter_class = GST_OPENCV_VIDEO_FILTER_CLASS (klass);
  GstCaps *caps;
  GstPadTemplate *templ;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_camera_calibration_finalize);
  gobject_class->set_property = gst_camera_calibration_set_property;
  gobject_class->get_property = gst_camera_calibration_get_property;

  opencvfilter_class->cv_trans_ip_func =
      gst_camera_calibration_transform_frame_ip;

  g_object_class_install_property (gobject_class, PROP_CALIBRATON_PATTERN,
      g_param_spec_enum ("pattern", "Calibration Pattern",
          "One of the chessboard, circles, or asymmetric circle pattern",
          GST_TYPE_CAMERA_CALIBRATION_PATTERN, DEFAULT_CALIBRATON_PATTERN,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_BOARD_WIDTH,
      g_param_spec_int ("board-width", "Board Width",
          "The board width in number of items",
          1, G_MAXINT, DEFAULT_BOARD_WIDTH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_BOARD_HEIGHT,
      g_param_spec_int ("board-height", "Board Height",
          "The board height in number of items",
          1, G_MAXINT, DEFAULT_BOARD_WIDTH,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SQUARE_SIZE,
      g_param_spec_float ("square-size", "Square Size",
          "The size of a square in your defined unit (point, millimeter, etc.)",
          0.0, G_MAXFLOAT, DEFAULT_SQUARE_SIZE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ASPECT_RATIO,
      g_param_spec_float ("aspect-ratio", "Aspect Ratio",
          "The aspect ratio",
          0.0, G_MAXFLOAT, DEFAULT_ASPECT_RATIO,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CORNER_SUB_PIXEL,
      g_param_spec_boolean ("corner-sub-pixel", "Corner Sub Pixel",
          "Improve corner detection accuracy for chessboard",
          DEFAULT_CORNER_SUB_PIXEL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ZERO_TANGENT_DISTORTION,
      g_param_spec_boolean ("zero-tangent-distorsion", "Zero Tangent Distorsion",
          "Assume zero tangential distortion",
          DEFAULT_ZERO_TANGENT_DISTORTION, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CENTER_PRINCIPAL_POINT,
      g_param_spec_boolean ("center-principal-point", "Center Principal Point",
          "Fix the principal point at the center",
          DEFAULT_CENTER_PRINCIPAL_POINT, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_USE_FISHEYE,
      g_param_spec_boolean ("use-fisheye", "Use Fisheye",
          "Use fisheye camera model for calibration",
          DEFAULT_USE_FISHEYE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DELAY,
      g_param_spec_int ("delay", "Delay",
          "Sampling periodicity in ms", 0, G_MAXINT,
          DEFAULT_DELAY,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_FRAME_COUNT,
      g_param_spec_int ("frame-count", "Frame Count",
          "The number of frames to use from the input for calibration", 1, G_MAXINT,
          DEFAULT_FRAME_COUNT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SHOW_CORNERS,
      g_param_spec_boolean ("show-corners", "Show Corners",
          "Show corners",
          DEFAULT_SHOW_CORNERS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));


  gst_element_class_set_static_metadata (element_class,
      "cameracalibration",
      "Filter/Effect/Video",
      "Performs camera calibration",
      "Philippe Renon <philippe_renon@yahoo.fr>");

  /* add sink and source pad templates */
  caps = gst_opencv_caps_from_cv_image_type (CV_8UC4);
  gst_caps_append (caps, gst_opencv_caps_from_cv_image_type (CV_8UC3));
  gst_caps_append (caps, gst_opencv_caps_from_cv_image_type (CV_8UC1));
  templ = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
      gst_caps_ref (caps));
  gst_element_class_add_pad_template (element_class, templ);
  templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps);
  gst_element_class_add_pad_template (element_class, templ);

//  gst_element_class_add_static_pad_template (element_class, &src_factory);
//  gst_element_class_add_static_pad_template (element_class, &sink_factory);
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_camera_calibration_init (GstCameraCalibration * calib)
{
  calib->calibrationPattern = DEFAULT_CALIBRATON_PATTERN;
  calib->boardSize.width = DEFAULT_BOARD_WIDTH;
  calib->boardSize.height = DEFAULT_BOARD_HEIGHT;
  calib->squareSize = DEFAULT_SQUARE_SIZE;
  calib->aspectRatio  = DEFAULT_ASPECT_RATIO;
  calib->cornerSubPix = DEFAULT_CORNER_SUB_PIXEL;
  calib->calibZeroTangentDist = DEFAULT_ZERO_TANGENT_DISTORTION;
  calib->calibFixPrincipalPoint = DEFAULT_CENTER_PRINCIPAL_POINT;
  calib->useFisheye = DEFAULT_USE_FISHEYE;
  calib->nrFrames = DEFAULT_FRAME_COUNT;
  calib->delay = DEFAULT_DELAY;
  calib->showCorners = DEFAULT_SHOW_CORNERS;

  calib->flags = cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5;
  if (calib->calibFixPrincipalPoint) calib->flags |= cv::CALIB_FIX_PRINCIPAL_POINT;
  if (calib->calibZeroTangentDist)   calib->flags |= cv::CALIB_ZERO_TANGENT_DIST;
  if (calib->aspectRatio)            calib->flags |= cv::CALIB_FIX_ASPECT_RATIO;

  if (calib->useFisheye) {
     // the fisheye model has its own enum, so overwrite the flags
     calib->flags = cv::fisheye::CALIB_FIX_SKEW | cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC |
                    // cv::fisheye::CALIB_FIX_K1 |
             cv::fisheye::CALIB_FIX_K2 | cv::fisheye::CALIB_FIX_K3 | cv::fisheye::CALIB_FIX_K4;
  }

  calib->mode = CAPTURING; //DETECTION;
  calib->prevTimestamp = 0;

  calib->imagePoints.clear();
  calib->cameraMatrix = 0;
  calib->distCoeffs = 0;

  gst_opencv_video_filter_set_in_place (
      GST_OPENCV_VIDEO_FILTER_CAST (calib), TRUE);
}

static void
gst_camera_calibration_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCameraCalibration *calib = GST_CAMERA_CALIBRATION (object);

  switch (prop_id) {
    case PROP_CALIBRATON_PATTERN:
      calib->calibrationPattern = g_value_get_enum (value);
      break;
    case PROP_BOARD_WIDTH:
      calib->boardSize.width = g_value_get_int (value);
      break;
    case PROP_BOARD_HEIGHT:
      calib->boardSize.height = g_value_get_int (value);
      break;
    case PROP_SQUARE_SIZE:
      calib->squareSize = g_value_get_float (value);
      break;
    case PROP_ASPECT_RATIO:
      calib->aspectRatio = g_value_get_float (value);
      break;
    case PROP_CORNER_SUB_PIXEL:
      calib->cornerSubPix = g_value_get_boolean (value);
      break;
    case PROP_ZERO_TANGENT_DISTORTION:
      calib->calibZeroTangentDist = g_value_get_boolean (value);
      break;
    case PROP_CENTER_PRINCIPAL_POINT:
      calib->calibFixPrincipalPoint = g_value_get_boolean (value);
      break;
    case PROP_USE_FISHEYE:
      calib->useFisheye = g_value_get_boolean (value);
      break;
    case PROP_FRAME_COUNT:
      calib->nrFrames = g_value_get_int (value);
      break;
    case PROP_DELAY:
      calib->delay = g_value_get_int (value);
      break;
    case PROP_SHOW_CORNERS:
      calib->showCorners = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_camera_calibration_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCameraCalibration *calib = GST_CAMERA_CALIBRATION (object);

  switch (prop_id) {
    case PROP_CALIBRATON_PATTERN:
      g_value_set_enum (value, calib->calibrationPattern);
      break;
    case PROP_BOARD_WIDTH:
      g_value_set_int (value, calib->boardSize.width);
      break;
    case PROP_BOARD_HEIGHT:
      g_value_set_int (value, calib->boardSize.height);
      break;
    case PROP_SQUARE_SIZE:
      g_value_set_float (value, calib->squareSize);
      break;
    case PROP_ASPECT_RATIO:
      g_value_set_float (value, calib->aspectRatio);
      break;
    case PROP_CORNER_SUB_PIXEL:
      g_value_set_boolean (value, calib->cornerSubPix);
      break;
    case PROP_ZERO_TANGENT_DISTORTION:
      g_value_set_boolean (value, calib->calibZeroTangentDist);
      break;
    case PROP_CENTER_PRINCIPAL_POINT:
      g_value_set_boolean (value, calib->calibFixPrincipalPoint);
      break;
    case PROP_USE_FISHEYE:
      g_value_set_boolean (value, calib->useFisheye);
      break;
    case PROP_FRAME_COUNT:
      g_value_set_int (value, calib->nrFrames);
      break;
    case PROP_DELAY:
      g_value_set_int (value, calib->delay);
      break;
    case PROP_SHOW_CORNERS:
      g_value_set_boolean (value, calib->showCorners);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
//static gboolean
//gst_camera_calibration_set_caps (GstOpencvVideoFilter * transform, gint in_width,
//    gint in_height, gint in_depth, gint in_channels,
//    gint out_width, gint out_height, gint out_depth, gint out_channels)
//{
//  GstCameraCalibration *calib;
//
//  calib = GST_CAMERA_CALIBRATION (transform);
//
//  if (calib->cvGray)
//    cvReleaseImage (&calib->cvGray);
//
//  calib->cvGray = cvCreateImage (cvSize (in_width, in_height), IPL_DEPTH_8U,
//      1);
//
//  return TRUE;
//}

//static GstMessage *
//gst_camera_calibration_message_new (GstCameraCalibration * calib, GstBuffer * buf)
//{
//  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (calib);
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
//  return gst_message_new_element (GST_OBJECT (calib), s);
//}

void camera_calibration_run(GstCameraCalibration *calib, IplImage *img);

/*
 * Performs the camera calibration
 */
static GstFlowReturn
gst_camera_calibration_transform_frame_ip (GstOpencvVideoFilter * cvfilter,
    G_GNUC_UNUSED GstBuffer * frame, IplImage * img)
{
  GstCameraCalibration *calib = GST_CAMERA_CALIBRATION (cvfilter);

  camera_calibration_run(calib, img);

  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
gboolean
gst_camera_calibration_plugin_init (GstPlugin * plugin)
{
  /* debug category for filtering log messages */
  GST_DEBUG_CATEGORY_INIT (gst_camera_calibration_debug, "cameracalibration",
      0,
      "Performs camera calibration");

  return gst_element_register (plugin, "cameracalibration", GST_RANK_NONE,
      GST_TYPE_CAMERA_CALIBRATION);
}

//    void validate()
//    {
//        goodInput = true;
//        if (boardSize.width <= 0 || boardSize.height <= 0)
//        {
//            cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
//            goodInput = false;
//        }
//        if (squareSize <= 10e-6)
//        {
//            cerr << "Invalid square size " << squareSize << endl;
//            goodInput = false;
//        }
//        if (nrFrames <= 0)
//        {
//            cerr << "Invalid number of frames " << nrFrames << endl;
//            goodInput = false;
//        }
//
//        if (input.empty())      // Check for valid input
//                inputType = INVALID;
//        else
//        {
//            if (input[0] >= '0' && input[0] <= '9')
//            {
//                stringstream ss(input);
//                ss >> cameraID;
//                inputType = CAMERA;
//            }
//            else
//            {
//                if (readStringList(input, imageList))
//                {
//                    inputType = IMAGE_LIST;
//                    nrFrames = (nrFrames < (int)imageList.size()) ? nrFrames : (int)imageList.size();
//                }
//                else
//                    inputType = VIDEO_FILE;
//            }
//            if (inputType == CAMERA)
//                inputCapture.open(cameraID);
//            if (inputType == VIDEO_FILE)
//                inputCapture.open(input);
//            if (inputType != IMAGE_LIST && !inputCapture.isOpened())
//                    inputType = INVALID;
//        }
//        if (inputType == INVALID)
//        {
//            cerr << " Input does not exist: " << input;
//            goodInput = false;
//        }
//
//        flag = CALIB_FIX_K4 | CALIB_FIX_K5;
//        if(calibFixPrincipalPoint) flag |= CALIB_FIX_PRINCIPAL_POINT;
//        if(calibZeroTangentDist)   flag |= CALIB_ZERO_TANGENT_DIST;
//        if(aspectRatio)            flag |= CALIB_FIX_ASPECT_RATIO;
//
//        if (useFisheye) {
//            // the fisheye model has its own enum, so overwrite the flags
//            flag = fisheye::CALIB_FIX_SKEW | fisheye::CALIB_RECOMPUTE_EXTRINSIC |
//                   // fisheye::CALIB_FIX_K1 |
//                   fisheye::CALIB_FIX_K2 | fisheye::CALIB_FIX_K3 | fisheye::CALIB_FIX_K4;
//        }
//
//        calibrationPattern = NOT_EXISTING;
//        if (!patternToUse.compare("CHESSBOARD")) calibrationPattern = CHESSBOARD;
//        if (!patternToUse.compare("CIRCLES_GRID")) calibrationPattern = CIRCLES_GRID;
//        if (!patternToUse.compare("ASYMMETRIC_CIRCLES_GRID")) calibrationPattern = ASYMMETRIC_CIRCLES_GRID;
//        if (calibrationPattern == NOT_EXISTING)
//        {
//            cerr << " Camera calibration mode does not exist: " << patternToUse << endl;
//            goodInput = false;
//        }
//        atImageList = 0;
//
//    }

bool runCalibration(GstCameraCalibration *calib, cv::Size imageSize, cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
                           std::vector<std::vector<cv::Point2f> > imagePoints );

void doCalibration(GstElement *element, gpointer user_data);

void camera_calibration_run(GstCameraCalibration *calib, IplImage *img)
{
    cv::Mat view = cv::cvarrToMat(img);

    // For camera only take new samples after delay time
    if (calib->mode == CAPTURING) {

        // get_input
        cv::Size imageSize = view.size();

        // find_pattern
        // FIXME find ways to reduce CPU usage
        // don't do it on all frames ? will it help ? corner display will be affected.
        // in a separate frame?
        // in a separate element that gets composited back into the main stream (video is tee-d into it and can then be decimated, scaled, etc..)

        std::vector<cv::Point2f> pointBuf;
        bool found;
        int chessBoardFlags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

        if (!calib->useFisheye) {
            // fast check erroneously fails with high distortions like fisheye
            chessBoardFlags |= cv::CALIB_CB_FAST_CHECK;
        }

        // Find feature points on the input format
        switch(calib->calibrationPattern) {
        case GST_CAMERACALIBRATION_PATTERN_CHESSBOARD:
            found = cv::findChessboardCorners(view, calib->boardSize, pointBuf, chessBoardFlags);
            break;
        case GST_CAMERACALIBRATION_PATTERN_CIRCLES_GRID:
            found = cv::findCirclesGrid(view, calib->boardSize, pointBuf);
            break;
        case GST_CAMERACALIBRATION_PATTERN_ASYMMETRIC_CIRCLES_GRID:
            found = cv::findCirclesGrid(view, calib->boardSize, pointBuf, cv::CALIB_CB_ASYMMETRIC_GRID );
            break;
        default:
            found = false;
            break;
        }

        bool blinkOutput = false;
        if (found) {
            // improve the found corners' coordinate accuracy for chessboard
            if (calib->calibrationPattern == GST_CAMERACALIBRATION_PATTERN_CHESSBOARD && calib->cornerSubPix) {
                // FIXME findChessboardCorners and alike do a cv::COLOR_BGR2GRAY (and a histogram balance)
                // the color convert should be done once (if needed) and shared
                // FIXME keep viewGray around to avoid reallocating it each time...
                cv::Mat viewGray;
                cv::cvtColor(view, viewGray, cv::COLOR_BGR2GRAY);
                cv::cornerSubPix(viewGray, pointBuf, cv::Size(11, 11),
                        cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));
            }

            // For camera only take new samples after delay time
            if ((calib->mode == CAPTURING) && ((clock() - calib->prevTimestamp) > calib->delay * 1e-3 * CLOCKS_PER_SEC)) {
                calib->imagePoints.push_back(pointBuf);
                calib->prevTimestamp = clock();
                blinkOutput = true;
            }

            // Draw the cornerfilter
            if (calib->showCorners) {
                cv::drawChessboardCorners(view, calib->boardSize, cv::Mat(pointBuf), found);
            }
        }

        // If got enough frames then stop calibration and show result
        if (calib->mode == CAPTURING && calib->imagePoints.size() >= (size_t)calib->nrFrames) {

            //GstElementCallAsyncFunc func;
            //gst_element_call_async (GST_ELEMENT (calib), /*GstElementCallAsyncFunc*/ doCalibration, NULL, NULL);

            if (runCalibration(calib, imageSize, calib->cameraMatrix, calib->distCoeffs, calib->imagePoints)) {
                calib->mode = CALIBRATED;

                GstPad *sinkPad = GST_BASE_TRANSFORM_SINK_PAD (calib);
                //GstPad *srcPad = GST_BASE_TRANSFORM_SRC_PAD (calib);
                GstEvent *event;
                //gboolean result;

                // create calibrated event and send upstream and downstream
                // FIXME should keep settings around for answering queries
                gchar *settings = camera_serialize_undistort_settings(calib->cameraMatrix, calib->distCoeffs);
                event = gst_camera_event_new_calibrated(settings);
                g_free (settings);

                //gst_event_ref(event);
                GST_LOG_OBJECT (sinkPad, "Sending upstream event %s.", GST_EVENT_TYPE_NAME (event));
                if (!gst_pad_push_event (sinkPad, event)) {
                  GST_WARNING_OBJECT (sinkPad, "Sending upstream event %p (%s) failed.",
                      event, GST_EVENT_TYPE_NAME (event));
                }
//                GST_LOG_OBJECT (srcPad, "Sending downstream event %s.", GST_EVENT_TYPE_NAME (event));
//                if (!gst_pad_push_event (srcPad, event)) {
//                  GST_WARNING_OBJECT (srcPad, "Sending downstream event %p (%s) failed.",
//                      event, GST_EVENT_TYPE_NAME (event));
//                }
            } else {
                calib->mode = DETECTION;
            }
        }

        if (calib->mode == CAPTURING && blinkOutput) {
            bitwise_not(view, view);
        }

    }

    // Output Text
    // FIXME all additional rendering (text, corners, ...) should be done with cairo or another gst framework.
    // this will relax the conditions on the input format (RBG only at the moment).
    // the calibration itself accepts more formats...

    std::string msg = (calib->mode == CAPTURING) ? "100/100" :
            (calib->mode == CALIBRATED) ? "Calibrated" : "Press 'g' to start";
    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(msg, 1, 1, 1, &baseLine);
    cv::Point textOrigin(view.cols - 2 * textSize.width - 10, view.rows - 2 * baseLine - 10);

    if (calib->mode == CAPTURING) {
      msg = cv::format( "%d/%d", (int)calib->imagePoints.size(), calib->nrFrames );
    }

    const cv::Scalar RED(0,0,255);
    const cv::Scalar GREEN(0,255,0);

    cv::putText(view, msg, textOrigin, 1, 1, calib->mode == CALIBRATED ?  GREEN : RED);
}

void doCalibration(__attribute__((unused)) GstElement *element, __attribute__((unused)) gpointer user_data)
{
    // GstCameraCalibration *calib = GST_CAMERA_CALIBRATION (element);
}

static double computeReprojectionErrors( const std::vector<std::vector<cv::Point3f> >& objectPoints,
                                         const std::vector<std::vector<cv::Point2f> >& imagePoints,
                                         const std::vector<cv::Mat>& rvecs, const std::vector<cv::Mat>& tvecs,
                                         const cv::Mat& cameraMatrix , const cv::Mat& distCoeffs,
                                         std::vector<float>& perViewErrors, bool fisheye)
{
    std::vector<cv::Point2f> imagePoints2;
    size_t totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for(size_t i = 0; i < objectPoints.size(); ++i)
    {
        if (fisheye)
        {
            cv::fisheye::projectPoints(objectPoints[i], imagePoints2, rvecs[i], tvecs[i], cameraMatrix,
                                   distCoeffs);
        }
        else
        {
            cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
        }
        err = cv::norm(imagePoints[i], imagePoints2, cv::NORM_L2);

        size_t n = objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}

static void calcBoardCornerPositions(cv::Size boardSize, float squareSize, std::vector<cv::Point3f>& corners,
        gint patternType /*= CHESSBOARD*/)
{
    corners.clear();

    switch(patternType)
    {
    case GST_CAMERACALIBRATION_PATTERN_CHESSBOARD:
    case GST_CAMERACALIBRATION_PATTERN_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; ++i)
            for( int j = 0; j < boardSize.width; ++j)
                corners.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
        break;

    case GST_CAMERACALIBRATION_PATTERN_ASYMMETRIC_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++)
            for( int j = 0; j < boardSize.width; j++)
                corners.push_back(cv::Point3f((2 * j + i % 2) * squareSize, i * squareSize, 0));
        break;
    default:
        break;
    }
}

static bool runCalibration(GstCameraCalibration *calib, cv::Size& imageSize, cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
                            std::vector<std::vector<cv::Point2f> > imagePoints, std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs,
                            std::vector<float>& reprojErrs, double& totalAvgErr)
{
    //! [fixed_aspect]
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    if (calib->flags & cv::CALIB_FIX_ASPECT_RATIO) {
        cameraMatrix.at<double>(0,0) = calib->aspectRatio;
    }
    //! [fixed_aspect]
    if (calib->useFisheye) {
        distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
    } else {
        distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    }

    std::vector<std::vector<cv::Point3f> > objectPoints(1);
    calcBoardCornerPositions(calib->boardSize, calib->squareSize, objectPoints[0], calib->calibrationPattern);

    objectPoints.resize(imagePoints.size(), objectPoints[0]);

    // Find intrinsic and extrinsic camera parameters
    double rms;

    if (calib->useFisheye) {
        cv::Mat _rvecs, _tvecs;
        rms = cv::fisheye::calibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, _rvecs,
                                 _tvecs, calib->flags);

        rvecs.reserve(_rvecs.rows);
        tvecs.reserve(_tvecs.rows);
        for(int i = 0; i < int(objectPoints.size()); i++){
            rvecs.push_back(_rvecs.row(i));
            tvecs.push_back(_tvecs.row(i));
        }
    } else {
        rms = cv::calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs,
                              calib->flags);
    }

    GST_LOG_OBJECT (calib,
       "Re-projection error reported by calibrateCamera: %f", rms);
    qDebug() << "Re-projection error reported by calibrateCamera:" <<  rms;


    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix,
                                            distCoeffs, reprojErrs, calib->useFisheye);

    return ok;
}

// Print camera parameters to the output file
//static void saveCameraParams( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
//                              const vector<Mat>& rvecs, const vector<Mat>& tvecs,
//                              const vector<float>& reprojErrs, const vector<vector<Point2f> >& imagePoints,
//                              double totalAvgErr)
//{
//    FileStorage fs( s.outputFileName, FileStorage::WRITE);
//
//    time_t tm;
//    time( &tm);
//    struct tm *t2 = localtime( &tm);
//    char buf[1024];
//    strftime( buf, sizeof(buf), "%c", t2);
//
//    fs << "calibration_time" << buf;
//
//    if (!rvecs.empty() || !reprojErrs.empty())
//        fs << "nr_of_frames" << (int)std::max(rvecs.size(), reprojErrs.size());
//    fs << "image_width" << imageSize.width;
//    fs << "image_height" << imageSize.height;
//    fs << "board_width" << s.boardSize.width;
//    fs << "board_height" << s.boardSize.height;
//    fs << "square_size" << s.squareSize;
//
//    if (s.flag & CALIB_FIX_ASPECT_RATIO)
//        fs << "fix_aspect_ratio" << s.aspectRatio;
//
//    if (s.flag)
//    {
//        if (s.useFisheye)
//        {
//            sprintf(buf, "flags:%s%s%s%s%s%s",
//                     s.flag & fisheye::CALIB_FIX_SKEW ? " +fix_skew" : "",
//                     s.flag & fisheye::CALIB_FIX_K1 ? " +fix_k1" : "",
//                     s.flag & fisheye::CALIB_FIX_K2 ? " +fix_k2" : "",
//                     s.flag & fisheye::CALIB_FIX_K3 ? " +fix_k3" : "",
//                     s.flag & fisheye::CALIB_FIX_K4 ? " +fix_k4" : "",
//                     s.flag & fisheye::CALIB_RECOMPUTE_EXTRINSIC ? " +recompute_extrinsic" : "");
//        }
//        else
//        {
//            sprintf(buf, "flags:%s%s%s%s",
//                     s.flag & CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "",
//                     s.flag & CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "",
//                     s.flag & CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "",
//                     s.flag & CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "");
//        }
//        cvWriteComment(*fs, buf, 0);
//    }
//
//    fs << "flags" << s.flag;
//
//    fs << "fisheye_model" << s.useFisheye;
//
//    fs << "camera_matrix" << cameraMatrix;
//    fs << "distortion_coefficients" << distCoeffs;
//
//    fs << "avg_reprojection_error" << totalAvgErr;
//    if (s.writeExtrinsics && !reprojErrs.empty())
//        fs << "per_view_reprojection_errors" << Mat(reprojErrs);
//
//    if(s.writeExtrinsics && !rvecs.empty() && !tvecs.empty())
//    {
//        CV_Assert(rvecs[0].type() == tvecs[0].type());
//        Mat bigmat((int)rvecs.size(), 6, rvecs[0].type());
//        for( size_t i = 0; i < rvecs.size(); i++)
//        {
//            Mat r = bigmat(Range(int(i), int(i+1)), Range(0,3));
//            Mat t = bigmat(Range(int(i), int(i+1)), Range(3,6));
//
//            CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
//            CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
//            //*.t() is MatExpr (not Mat) so we can use assignment operator
//            r = rvecs[i].t();
//            t = tvecs[i].t();
//        }
//        //cvWriteComment( *fs, "a set of 6-tuples (rotation vector + translation vector) for each view", 0);
//        fs << "extrinsic_parameters" << bigmat;
//    }
//
//    if(s.writePoints && !imagePoints.empty())
//    {
//        Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
//        for( size_t i = 0; i < imagePoints.size(); i++)
//        {
//            Mat r = imagePtMat.row(int(i)).reshape(2, imagePtMat.cols);
//            Mat imgpti(imagePoints[i]);
//            imgpti.copyTo(r);
//        }
//        fs << "image_points" << imagePtMat;
//    }
//}

//! [run_and_save]
//bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, Mat& distCoeffs,
//                           vector<vector<Point2f> > imagePoints)
//{
//    vector<Mat> rvecs, tvecs;
//    vector<float> reprojErrs;
//    double totalAvgErr = 0;
//
//    bool ok = runCalibration(s, imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs, reprojErrs,
//                             totalAvgErr);
//    cout << (ok ? "Calibration succeeded" : "Calibration failed")
//         << ". avg re projection error = " << totalAvgErr << endl;
//
////    if (ok)
////        saveCameraParams(s, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, reprojErrs, imagePoints,
////                         totalAvgErr);
//    return ok;
//}
//! [run_and_save]

bool runCalibration(GstCameraCalibration *calib, cv::Size imageSize, cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
        std::vector<std::vector<cv::Point2f> > imagePoints)
{
    std::vector<cv::Mat> rvecs, tvecs;
    std::vector<float> reprojErrs;
    double totalAvgErr = 0;

    bool ok = runCalibration(calib, imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs, reprojErrs,
                             totalAvgErr);
    GST_LOG_OBJECT (calib,
       (ok ? "Calibration succeeded" : "Calibration failed"));// + ". avg re projection error = " + totalAvgErr);

    return ok;
}
