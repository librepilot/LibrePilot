/**
 ******************************************************************************
 *
 * @file       OSGBackgroundNode.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
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

#include "OSGBackgroundNode.hpp"

#include <osg/Depth>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <QUrl>
#include <QDebug>

namespace osgQtQuick {
struct OSGBackgroundNode::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGBackgroundNode *parent) : QObject(parent), self(parent), url() {}

    bool acceptImageFile(QUrl url)
    {
        // qDebug() << "OSGBackgroundNode::acceptImageFile" << url;
        if (this->url == url) {
            return false;
        }

        this->url = url;

        realize();

        return true;
    }

    void realize()
    {
        qDebug() << "OSGBackgroundNode::realize";

        // qDebug() << "OSGBackgroundNode::realize - reading image file" << h->url.path();
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        osg::ref_ptr<osg::Image> image   = osgDB::readImageFile(url.path().toStdString());
        texture->setImage(image.get());

        osg::ref_ptr<osg::Drawable> quad = osg::createTexturedQuadGeometry(
            osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 0.0f));
        quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(quad.get());

        osg::Camera *camera = new osg::Camera;
        camera->setClearMask(0);
        camera->setCullingActive(false);
        camera->setAllowEventFocus(false);
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->addChild(geode.get());

        osg::StateSet *ss = camera->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0, 1.0));

        self->setNode(camera);
    }

    OSGBackgroundNode *const self;

    QUrl url;
};

OSGBackgroundNode::OSGBackgroundNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    // qDebug() << "OSGBackgroundNode::OSGBackgroundNode";
}

OSGBackgroundNode::~OSGBackgroundNode()
{
    // qDebug() << "OSGBackgroundNode::~OSGBackgroundNode";
    delete h;
}

const QUrl OSGBackgroundNode::imageFile() const
{
    return h->url;
}

void OSGBackgroundNode::setImageFile(const QUrl &url)
{
    // qDebug() << "OSGBackgroundNode::setImageFile" << url;
    if (h->acceptImageFile(url)) {
        emit imageFileChanged(imageFile());
    }
}
} // namespace osgQtQuick

#include "OSGBackgroundNode.moc"
