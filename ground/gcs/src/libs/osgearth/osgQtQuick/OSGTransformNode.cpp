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

    struct NodeUpdateCallback : public osg::NodeCallback {
public:
        NodeUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGTransformNode *parent) : QObject(parent), self(parent), modelData(NULL), dirty(false)
    {}

    ~Hidden()
    {}

    bool acceptModelData(OSGNode *node)
    {
        qDebug() << "OSGTransformNode::acceptModelData" << node;
        if (modelData == node) {
            return false;
        }

        if (modelData) {
            disconnect(modelData);
        }

        modelData = node;

        if (modelData) {
            acceptNode(modelData->node());
            connect(modelData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGTransformNode::acceptNode" << node;
        if (!node) {
            qWarning() << "OSGTransformNode::acceptNode - node is null";
            return false;
        }

        osg::Transform *transform = getOrCreateTransform();

        transform->addChild(node);

        self->setNode(transform);

        dirty = true;

        return true;
    }

    osg::Transform *getOrCreateTransform()
    {
        if (transform.valid()) {
            return transform.get();
        }

        transform = new osg::PositionAttitudeTransform();

        transform->addUpdateCallback(new NodeUpdateCallback(this));

        return transform.get();
    }

    void updateNode()
    {
        if (!dirty || !transform.valid()) {
            return;
        }
        dirty = false;

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

    OSGTransformNode *const self;

    OSGNode *modelData;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform;

    bool dirty;

    QVector3D scale;
    QVector3D attitude;
    QVector3D position;

private slots:

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGTransformNode::onNodeChanged" << node;
        acceptNode(node);
    }
};

/* struct Hidden::NodeUpdateCallback */

void OSGTransformNode::Hidden::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    h->updateNode();
}

OSGTransformNode::OSGTransformNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGTransformNode::OSGTransformNode";
}

OSGTransformNode::~OSGTransformNode()
{
    qDebug() << "OSGTransformNode::~OSGTransformNode";
}

OSGNode *OSGTransformNode::modelData()
{
    return h->modelData;
}

void OSGTransformNode::setModelData(OSGNode *node)
{
    if (h->acceptModelData(node)) {
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
        h->dirty = true;
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
        h->dirty    = true;
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
        h->dirty    = true;
        emit positionChanged(position());
    }
}

void OSGTransformNode::attach(osgViewer::View *view)
{
    // qDebug() << "OSGTransformNode::attach " << view;
    if (h->modelData) {
        h->modelData->attach(view);
    }
    h->updateNode();
}

void OSGTransformNode::detach(osgViewer::View *view)
{
    // qDebug() << "OSGTransformNode::detach " << view;
    if (h->modelData) {
        h->modelData->detach(view);
    }
}
} // namespace osgQtQuick

#include "OSGTransformNode.moc"
