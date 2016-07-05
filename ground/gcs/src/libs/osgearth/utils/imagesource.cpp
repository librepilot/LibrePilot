/**
 ******************************************************************************
 *
 * @file       imagesource.cpp
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

#include "imagesource.hpp"

#include <osg/Image>
#include <osgDB/ReadFile>

#include <QDebug>

osg::Image *ImageSource::createImage(QUrl &url)
{
    qDebug() << "ImageSource::createImage - reading image file" << url.path();
    osg::Image *image = osgDB::readImageFile(url.path().toStdString());
    return image;
}
