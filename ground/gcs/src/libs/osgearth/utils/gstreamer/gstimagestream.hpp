/**
 ******************************************************************************
 *
 * @file       gstimagestream.hpp
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
#pragma once

#include <gst/app/gstappsink.h>

#include <osg/ImageStream>

#include <OpenThreads/Thread>

class GSTImageStream : public osg::ImageStream, public OpenThreads::Thread {
public:

    GSTImageStream();
    GSTImageStream(const GSTImageStream & image, const osg::CopyOp & copyop = osg::CopyOp::SHALLOW_COPY);
    virtual ~GSTImageStream();

    META_Object(osgGStreamer, GSTImageStream);

    bool setPipeline(const std::string &pipeline);

    virtual void play();
    virtual void pause();
    virtual void rewind();
    virtual void seek(double time);

private:
    virtual void run();

    static gboolean on_message(GstBus *bus, GstMessage *message, GSTImageStream *user_data);

    static GstFlowReturn on_new_sample(GstAppSink *appsink, GSTImageStream *user_data);
    static GstFlowReturn on_new_preroll(GstAppSink *appsink, GSTImageStream *user_data);

    bool allocateInternalBuffer(GstSample *sample);

    GMainLoop *_loop;
    GstElement *_pipeline;

    unsigned char *_internal_buffer;

    int _width;
    int _height;
};
