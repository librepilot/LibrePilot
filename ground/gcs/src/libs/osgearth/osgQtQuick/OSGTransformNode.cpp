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
struct OSGTransformNode::Hidden : public QObject {
    Q_OBJECT

public:

    Hidden(OSGTransformNode *node) : QObject(node), self(node), childNode(NULL)
    {}

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

    void updateNode()
    {
        // qDebug() << "OSGTransformNode::updateNode" << this;

        osg::PositionAttitudeTransform *transform = getOrCreateTransform();

        if (transform->getNumChildren() == 0) {
            if (childNode && childNode->node()) {
                transform->addChild(childNode->node());
            }
        } else {
            if (childNode && childNode->node()) {
                if (transform->getChild(0) != childNode->node()) {
                    transform->removeChild(0, 1);
                    transform->addChild(childNode->node());
                }
            } else {
                transform->removeChild(0, 1);
            }
        }

        // scale
        if ((scale.x() != 0.0) || (scale.y() != 0.0) || (scale.z() != 0.0)) {
            transform->setScale(osg::Vec3d(scale.x(), scale.y(), scale.z()));
            // transform->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
            transform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);
        }

        // attitude
        double roll  = osg::DegreesToRadians(attitude.x());
        double pitch = osg::DegreesToRadians(attitude.y());
        double yaw   = osg::DegreesToRadians(attitude.z());
        osg::Quat q  = osg::Quat(
            roll, osg::Vec3d(0, 1, 0),
            pitch, osg::Vec3d(1, 0, 0),
            yaw, osg::Vec3d(0, 0, -1));
        transform->setAttitude(q);

        // position
        transform->setPosition(osg::Vec3d(position.x(), position.y(), position.z()));
    }

    osg::PositionAttitudeTransform *getOrCreateTransform()
    {
        if (transform.valid()) {
            return transform.get();
        }

        transform = new osg::PositionAttitudeTransform();


        self->setNode(transform);

        return transform.get();
    }

    OSGTransformNode *const self;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform;

    OSGNode   *childNode;

    QVector3D scale;
    QVector3D attitude;
    QVector3D position;

private slots:

    void onChildNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGTransformNode::onChildNodeChanged" << node;
        self->setDirty();
    }
};

OSGTransformNode::OSGTransformNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGTransformNode::OSGTransformNode";
}

OSGTransformNode::~OSGTransformNode()
{
    qDebug() << "OSGTransformNode::~OSGTransformNode";
    delete h;
}

OSGNode *OSGTransformNode::modelData()
{
    return h->childNode;
}

void OSGTransformNode::setModelData(OSGNode *node)
{
    if (h->acceptChildNode(node)) {
        emit modelDataChanged(node);
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
        setDirty();
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
        setDirty();
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
        setDirty();
        emit positionChanged(position());
    }
}

void OSGTransformNode::update()
{
    h->updateNode();
}

void OSGTransformNode::attach(osgViewer::View *view)
{
    // qDebug() << "OSGTransformNode::attach " << view;
    if (h->childNode) {
        h->childNode->attach(view);
    }
    update();
}

void OSGTransformNode::detach(osgViewer::View *view)
{
    // qDebug() << "OSGTransformNode::detach " << view;
    if (h->childNode) {
        h->childNode->detach(view);
    }
}
} // namespace osgQtQuick

#include "OSGTransformNode.moc"
