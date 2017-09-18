/**
 ******************************************************************************
 *
 * @file       gstimagestream.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             OpenSceneGraph, http://www.openscenegraph.org/
 *             Julen Garcia <jgarcia@vicomtech.org>
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
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
#include "gstimagestream.hpp"

#include "gst_util.h"

#include <osgDB/ReaderWriter>

#include <QDebug>

GSTImageStream::GSTImageStream() :
    _loop(0),
    _pipeline(0),
    _internal_buffer(0),
    _width(0),
    _height(0)
{
    setOrigin(osg::Image::TOP_LEFT);

    _loop = g_main_loop_new(NULL, FALSE);
}

GSTImageStream::GSTImageStream(const GSTImageStream & image, const osg::CopyOp & copyop) :
    osg::ImageStream(image, copyop), OpenThreads::Thread(),
    _loop(0),
    _pipeline(0),
    _internal_buffer(0),
    _width(0),
    _height(0)
{
    setOrigin(osg::Image::TOP_LEFT);

    _loop = g_main_loop_new(NULL, FALSE);
}

GSTImageStream::~GSTImageStream()
{
    gst_element_set_state(_pipeline, GST_STATE_NULL);
    gst_element_get_state(_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE); // wait until the state changed

    g_main_loop_quit(_loop);
    g_main_loop_unref(_loop);

    free(_internal_buffer);
}

// osgDB::ReaderWriter::ReadResult readImage(const std::string & filename, const osgDB::ReaderWriter::Options *options)
// {
// const std::string ext = osgDB::getLowerCaseFileExtension(filename);
//
//// if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;
//
// const std::string path = osgDB::containsServerAddress(filename) ?
// filename :
// osgDB::findDataFile(filename, options);
//
// if (path.empty()) {
// return osgDB::ReaderWriter::ReadResult::FILE_NOT_FOUND;
// }
//
// osg::ref_ptr<GSTImageStream> imageStream = new GSTImageStream();
//
// if (!imageStream->open(filename)) {
// return osgDB::ReaderWriter::ReadResult::FILE_NOT_HANDLED;
// }
//
// return imageStream.release();
// }

bool GSTImageStream::setPipeline(const std::string &pipeline)
{
    GError *error = NULL;

    // TODO not the most appropriate place to do that...
    gst::init(NULL, NULL);

    gchar *string = g_strdup_printf("%s", pipeline.c_str());

    _pipeline = gst_parse_launch(string, &error);

    // TODO make sure that there is "! videoconvert ! video/x-raw,format=RGB ! appsink name=sink emit-signals=true"
    // TOOD remove the need for a videoconvert element by adapting dynamically to format
    // TODO try to use GL buffers...

    g_free(string);

    if (error) {
        g_printerr("Error: %s\n", error->message);
        g_error_free(error);
        // TODO submit fix to osg...
        return false;
    }

    if (_pipeline == NULL) {
        return false;
    }

    // bus
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
    gst_bus_add_watch(bus, (GstBusFunc)on_message, this);
    gst_object_unref(bus);

    // sink
    GstElement *sink = gst_bin_get_by_name(GST_BIN(_pipeline), "sink");

    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
    g_signal_connect(sink, "new-preroll", G_CALLBACK(on_new_preroll), this);

    gst_object_unref(sink);

    gst_element_set_state(_pipeline, GST_STATE_PAUSED);
    gst_element_get_state(_pipeline, 0, 0, GST_CLOCK_TIME_NONE); // wait until the state changed

    if (_width == 0 || _height == 0) {
        // no valid image has been setup by a on_new_preroll() call.
        return false;
    }

    // setLoopingMode(osg::ImageStream::NO_LOOPING);

    // start the thread to run gstreamer main loop
    start();

    return true;
}

/*
   bool GSTImageStream::open(const std::string & filename)
   {
    setFileName(filename);

    GError *error = NULL;

    // get stream info

    bool has_audio_stream = false;

    gchar *uri = g_filename_to_uri(filename.c_str(), NULL, NULL);

    if (uri != 0 && gst_uri_is_valid(uri)) {
        GstDiscoverer *item     = gst_discoverer_new(1 * GST_SECOND, &error);
        GstDiscovererInfo *info = gst_discoverer_discover_uri(item, uri, &error);
        GList *audio_list = gst_discoverer_info_get_audio_streams(info);

        if (g_list_length(audio_list) > 0) {
            has_audio_stream = true;
        }

        gst_discoverer_info_unref(info);
        g_free(uri);
    }

    // build pipeline
    const gchar *audio_pipe = "";
    if (has_audio_stream) {
        audio_pipe = "deco. ! queue ! audioconvert ! autoaudiosink";
    }

    gchar *string = g_strdup_printf("filesrc location=%s ! \
        decodebin name=deco \
        deco. ! queue ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink emit-signals=true \
        %s", filename.c_str(), audio_pipe);

    _pipeline = gst_parse_launch(string, &error);

    g_free(string);

    if (error) {
        g_printerr("Error: %s\n", error->message);
        g_error_free(error);
    }

    if (_pipeline == NULL) {
        return false;
    }


    // bus
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));

    gst_bus_add_watch(bus, (GstBusFunc)on_message, this);

    gst_object_unref(bus);


    // sink

    GstElement *sink = gst_bin_get_by_name(GST_BIN(_pipeline), "sink");

    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
    g_signal_connect(sink, "new-preroll", G_CALLBACK(on_new_preroll), this);

    gst_object_unref(sink);

    gst_element_set_state(_pipeline, GST_STATE_PAUSED);
    gst_element_get_state(_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE); // wait until the state changed

    if (_width == 0 || _height == 0) {
        // no valid image has been setup by a on_new_preroll() call.
        return false;
    }

    // setLoopingMode(osg::ImageStream::NO_LOOPING);

    // start the thread to run gstreamer main loop
    start();

    return true;
   }
 */

// ** Controls **

void GSTImageStream::play()
{
    gst_element_set_state(_pipeline, GST_STATE_PLAYING);
}

void GSTImageStream::pause()
{
    gst_element_set_state(_pipeline, GST_STATE_PAUSED);
}

void GSTImageStream::rewind()
{
    gst_element_seek_simple(_pipeline, GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 0);
}

void GSTImageStream::seek(double time)
{
    gst_element_seek_simple(_pipeline, GST_FORMAT_TIME, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), time * GST_MSECOND);
}

// ** Callback implementations **

GstFlowReturn GSTImageStream::on_new_sample(GstAppSink *appsink, GSTImageStream *user_data)
{
    // get the buffer from appsink
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    if (!user_data->allocateInternalBuffer(sample)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // upload data
    GstMapInfo info;
    if (!gst_buffer_map(buffer, &info, GST_MAP_READ)) {
        qWarning() << "Failed to map buffer";
        // TODO
    }
    gsize size = gst_buffer_extract(buffer, 0, user_data->_internal_buffer, info.size);
    if (size != info.size) {
        qWarning() << "GSTImageStream::on_new_sample : extracted" << size << "/" << info.size;
        // TODO
    }

    // data has been modified so dirty the image so the texture will be updated
    user_data->dirty();

    // clean resources
    gst_buffer_unmap(buffer, &info);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

GstFlowReturn GSTImageStream::on_new_preroll(GstAppSink *appsink, GSTImageStream *user_data)
{
    qDebug() << "ON NEW PREROLL";

    // get the sample from appsink
    GstSample *sample = gst_app_sink_pull_preroll(appsink);

    if (!user_data->allocateInternalBuffer(sample)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // clean resources
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

gboolean GSTImageStream::on_message(GstBus *bus, GstMessage *message, GSTImageStream *user_data)
{
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
        if (user_data->getLoopingMode() == osg::ImageStream::LOOPING) {
            user_data->rewind();
        }
    }
    return true;
}


bool GSTImageStream::allocateInternalBuffer(GstSample *sample)
{
    // get sample info
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

/*
    GstVideoInfo info;
    if (!gst_video_info_from_caps (&info, caps)) {
       gchar *caps_str = gst_caps_to_string (caps);
       GST_ERROR ("Failed to get video info from caps %s", caps_str);
       //g_set_error (err, GST_CORE_ERROR, GST_CORE_ERROR_NEGOTIATION,
       //    "Failed to get video info from caps %s", caps_str);
       g_free (caps_str);
       return FALSE;

    GstVideoFormat format;
    format = GST_VIDEO_INFO_FORMAT (info);
 */

    int width;
    int height;

    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    if (width <= 0 || height <= 0) {
        qCritical() << "invalid video size: width=" << width << ", height=" << height;
        return false;
    }

    if (_width != width || _height != height) {
        _width  = width;
        _height = height;

        int row_width = width * 3;
        if ((row_width % 4) != 0) {
            row_width += (4 - (row_width % 4));
        }

        // qDebug() << "image width=" << width << ", height=" << height << row_width << (row_width * height);

        // if buffer previously assigned free it before allocating new buffer.
        if (_internal_buffer) {
            free(_internal_buffer);
        }

        // allocate buffer
        _internal_buffer = (unsigned char *)malloc(sizeof(unsigned char) * row_width * height);

        // assign buffer to image
        setImage(_width, _height, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, _internal_buffer, osg::Image::NO_DELETE, 4);
    }

    return true;
}

void GSTImageStream::run()
{
    g_main_loop_run(_loop);
}
