/**
 ******************************************************************************
 *
 * @file       OSGTransformNode.cpp
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

#include "OSGTransformNode.hpp"

#include <osg/Quat>
#include <osg/PositionAttitudeTransform>
#include <osg/Math>

#include <QDebug>

namespace osgQtQuick {
// NOTE : these flags should not overlap with OSGGroup flags!!!
// TODO : find a better way...
enum DirtyFlag { Scale = 1 << 10, Position = 1 << 11, Attitude = 1 << 12 };

struct OSGTransformNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGTransformNode * const self;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform;

public:
    QVector3D scale;
    QVector3D attitude;
    QVector3D position;

    Hidden(OSGTransformNode *self) : QObject(self), self(self)
    {}

    osg::Node *createNode()
    {
        transform = new osg::PositionAttitudeTransform();
        return transform;
    }

    void updateScale()
    {
        // qDebug() << "OSGTransformNode::updateScale" << scale;
        if ((scale.x() != 0.0) || (scale.y() != 0.0) || (scale.z() != 0.0)) {
            transform->setScale(osg::Vec3d(scale.x(), scale.y(), scale.z()));
            // transform->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
            transform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);
        }
    }

    void updateAttitude()
    {
        double roll  = osg::DegreesToRadians(attitude.x());
        double pitch = osg::DegreesToRadians(attitude.y());
        double yaw   = osg::DegreesToRadians(attitude.z());
        osg::Quat q  = osg::Quat(
            roll, osg::Vec3d(0, 1, 0),
            pitch, osg::Vec3d(1, 0, 0),
            yaw, osg::Vec3d(0, 0, -1));

        transform->setAttitude(q);
    }

    void updatePosition()
    {
        transform->setPosition(osg::Vec3d(position.x(), position.y(), position.z()));
    }
};

/* class OSGTransformNode */

OSGTransformNode::OSGTransformNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGTransformNode::~OSGTransformNode()
{
    delete h;
}

QVector3D OSGTransformNode::scale() const
{
    return h->scale;
}

void OSGTransformNode::setScale(QVector3D arg)
{
    if (h->scale != arg) {
        h->scale = arg;
        setDirty(Scale);
        emit scaleChanged(scale());
    }
}

QVector3D OSGTransformNode::attitude() const
{
    return h->attitude;
}

void OSGTransformNode::setAttitude(QVector3D arg)
{
    if (h->attitude != arg) {
        h->attitude = arg;
        setDirty(Attitude);
        emit attitudeChanged(attitude());
    }
}

QVector3D OSGTransformNode::position() const
{
    return h->position;
}

void OSGTransformNode::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        setDirty(Position);
        emit positionChanged(position());
    }
}

osg::Node *OSGTransformNode::createNode()
{
    return h->createNode();
}

void OSGTransformNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(Scale)) {
        h->updateScale();
    }
    if (isDirty(Attitude)) {
        h->updateAttitude();
    }
    if (isDirty(Position)) {
        h->updatePosition();
    }
}
} // namespace osgQtQuick

#include "OSGTransformNode.moc"
