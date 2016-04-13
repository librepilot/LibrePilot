/**
 ******************************************************************************
 *
 * @file       OSGImageNode.cpp
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

#include "OSGImageNode.hpp"

#include <osg/Texture2D>

#include <osgDB/ReadFile>

#include <QUrl>
#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { ImageFile = 1 << 0 };

struct OSGImageNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGImageNode * const self;

    osg::ref_ptr<osg::Texture2D> texture;

public:
    QUrl url;

    Hidden(OSGImageNode *self) : QObject(self), self(self), url()
    {}

    osg::Node *createNode()
    {
        osg::Drawable *quad = osg::createTexturedQuadGeometry(osg::Vec3(0, 0, 0), osg::Vec3(1, 0, 0), osg::Vec3(0, 1, 0));

        osg::Geode *geode   = new osg::Geode;

        geode->addDrawable(quad);

        geode->setStateSet(createState());

        return geode;
    }

    osg::StateSet *createState()
    {
        texture = new osg::Texture2D;

        // create the StateSet to store the texture data
        osg::StateSet *stateset = new osg::StateSet;

        stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

        return stateset;
    }

    void updateImageFile()
    {
        qDebug() << "OSGImageNode::updateImageFile - reading image file" << url.path();
        osg::Image *image = osgDB::readImageFile(url.path().toStdString());
        if (texture.valid()) {
            texture->setImage(image);
        }
    }
};

/* class OSGImageNode */

OSGImageNode::OSGImageNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGImageNode::~OSGImageNode()
{
    delete h;
}

const QUrl OSGImageNode::imageFile() const
{
    return h->url;
}

void OSGImageNode::setImageFile(const QUrl &url)
{
    if (h->url != url) {
        h->url = url;
        setDirty(ImageFile);
        emit imageFileChanged(url);
    }
}

osg::Node *OSGImageNode::createNode()
{
    return h->createNode();
}

void OSGImageNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(ImageFile)) {
        h->updateImageFile();
    }
}
} // namespace osgQtQuick

#include "OSGImageNode.moc"
