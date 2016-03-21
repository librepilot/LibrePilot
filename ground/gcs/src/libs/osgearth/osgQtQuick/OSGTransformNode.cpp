/**
 ******************************************************************************
 *
 * @file       OSGTransformNode.cpp
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

#include "OSGTransformNode.hpp"

#include <osg/Quat>
#include <osg/PositionAttitudeTransform>
#include <osg/Math>

#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Child = 1 << 0, Scale = 1 << 1, Position = 1 << 2, Attitude = 1 << 3 };

struct OSGTransformNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGTransformNode * const self;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform;

public:
    OSGNode   *childNode;

    QVector3D scale;
    QVector3D attitude;
    QVector3D position;

    Hidden(OSGTransformNode *self) : QObject(self), self(self), childNode(NULL)
    {
        transform = new osg::PositionAttitudeTransform();
        self->setNode(transform);
    }

    bool acceptChildNode(OSGNode *node)
    {
        qDebug() << "OSGTransformNode::acceptChildNode" << node;
        if (childNode == node) {
            return false;
        }

        if (childNode) {
            disconnect(childNode);
        }

        childNode = node;

        if (childNode) {
            connect(childNode, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onChildNodeChanged(osg::Node *)));
        }

        return true;
    }

    void updateChildNode()
    {
        qDebug() << "OSGTransformNode::updateChildNode" << childNode;
        bool updated = false;
        if (transform->getNumChildren() == 0) {
            if (childNode && childNode->node()) {
                updated |= transform->addChild(childNode->node());
            }
        } else {
            if (childNode && childNode->node()) {
                if (transform->getChild(0) != childNode->node()) {
                    updated |= transform->removeChild(0, 1);
                    updated |= transform->addChild(childNode->node());
                }
            } else {
                updated |= transform->removeChild(0, 1);
            }
        }
        // if (updated) {
        self->emitNodeChanged();
        // }
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

private slots:
    void onChildNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGTransformNode::onChildNodeChanged" << node;
        updateChildNode();
    }
};

/* class OSGTransformNode */

OSGTransformNode::OSGTransformNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGTransformNode::~OSGTransformNode()
{
    qDebug() << "OSGTransformNode::~OSGTransformNode";
    delete h;
}

OSGNode *OSGTransformNode::childNode() const
{
    return h->childNode;
}

void OSGTransformNode::setChildNode(OSGNode *node)
{
    if (h->acceptChildNode(node)) {
        setDirty(Child);
        emit childNodeChanged(node);
    }
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

void OSGTransformNode::update()
{
    Inherited::update();

    if (isDirty(Child)) {
        h->updateChildNode();
    }
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
