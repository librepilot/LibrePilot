/**
 ******************************************************************************
 *
 * @file       gstimagesource.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
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

#include "gstimagesource.hpp"

#include "gstimagestream.hpp"

#include <osg/Image>

#include <QUrl>
#include <QDebug>

GstImageSource::GstImageSource() : is(NULL)
{}

GstImageSource::~GstImageSource()
{
    if (is) {
        delete is;
    }
}

osg::Image *GstImageSource::createImage(QUrl &url)
{
    // qDebug() << "GstImageSource::createImage - reading image file" << url.path();

    QString pipeline   = url.query(QUrl::FullyDecoded);

    GSTImageStream *is = new GSTImageStream();

    is->setPipeline(pipeline.toStdString());

    this->is = is;

    play();

    return this->is;
}

void GstImageSource::play()
{
    if (is) {
        is->play();
    }
}

void GstImageSource::pause()
{
    if (is) {
        is->pause();
    }
}

void GstImageSource::rewind()
{
    if (is) {
        is->rewind();
    }
}

void GstImageSource::seek(double time)
{
    if (is) {
        is->seek(time);
    }
}
