/**
 ******************************************************************************
 *
 * @file       OSGBillboardNode.cpp
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

#include "OSGBillboardNode.hpp"

#include <osg/Depth>
#include <osg/Texture2D>
#include <osg/ImageSequence>

#include <osgDB/ReadFile>

#include <QDebug>

namespace osgQtQuick {
// NOTE : these flags should not overlap with OSGGroup flags!!!
// TODO : find a better way...
enum DirtyFlag {};

struct OSGBillboardNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGBillboardNode * const self;

    osg::ref_ptr<osg::Camera> camera;

public:
    Hidden(OSGBillboardNode *self) : QObject(self), self(self)
    {}

    osg::Node *createNode()
    {
        camera = new osg::Camera;
        camera->setClearMask(0);
        camera->setCullingActive(false);
        camera->setAllowEventFocus(false);
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));

        osg::StateSet *ss = camera->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0, 1.0));

        return camera;
    }
};

/* class OSGBillboardNode */

OSGBillboardNode::OSGBillboardNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGBillboardNode::~OSGBillboardNode()
{
    delete h;
}

osg::Node *OSGBillboardNode::createNode()
{
    return h->createNode();
}

void OSGBillboardNode::updateNode()
{
    Inherited::updateNode();
}
} // namespace osgQtQuick

#include "OSGBillboardNode.moc"
