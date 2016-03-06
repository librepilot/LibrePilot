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

#include "OSGGeoTransformNode.hpp"

#include "../utility.h"

#include <osg/ComputeBoundsVisitor>

#include <osgearth/GeoTransform>
#include <osgEarth/MapNode>
#include <osgEarth/GeoData>

#include <QDebug>

namespace osgQtQuick {
struct OSGGeoTransformNode::Hidden : public QObject {
    Q_OBJECT

    struct NodeUpdateCallback : public osg::NodeCallback {
public:
        NodeUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGGeoTransformNode *parent) : QObject(parent), self(parent), childNode(NULL), sceneNode(NULL), offset(-1.0), clampToTerrain(false), intoTerrain(false), dirty(false)
    {}

    ~Hidden()
    {}

    bool acceptChildNode(OSGNode *node)
    {
        qDebug() << "OSGGeoTransformNode::acceptChildNode" << node;
        if (childNode == node) {
            return false;
        }

        if (childNode) {
            disconnect(childNode);
        }

        childNode = node;
        dirty     = true;

        if (childNode) {
            connect(childNode, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onChildNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptSceneNode(OSGNode *node)
    {
        qDebug() << "OSGGeoTransformNode::acceptSceneNode" << node;
        if (sceneNode == node) {
            return false;
        }

        if (sceneNode) {
            disconnect(sceneNode);
        }

        sceneNode = node;
        dirty     = true;

        if (sceneNode) {
            connect(sceneNode, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onSceneNodeChanged(osg::Node *)));
        }

        return true;
    }

    void updateNode()
    {
        if (!dirty) {
            return;
        }
        dirty = false;

        // qDebug() << "OSGGeoTransformNode::updateNode" << this;

        osgEarth::GeoTransform *transform = getOrCreateTransform();

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

        osgEarth::MapNode *mapNode = NULL;
        if (sceneNode && sceneNode->node()) {
            mapNode = osgEarth::MapNode::findMapNode(sceneNode->node());
            if (mapNode) {
                transform->setTerrain(mapNode->getTerrain());
            } else {
                qWarning() << "OSGGeoTransformNode::updateNode - scene data does not contain a map node";
            }
        }

        osgEarth::GeoPoint geoPoint;
        if (mapNode) {
            geoPoint = osgQtQuick::toGeoPoint(mapNode->getTerrain()->getSRS(), position);
        } else {
            geoPoint = osgQtQuick::toGeoPoint(position);
        }
        if (clampToTerrain && mapNode) {
            // get "size" of model
            // TODO this should be done once only...
            osg::ComputeBoundsVisitor cbv;
            childNode->node()->accept(cbv);
            const osg::BoundingBox & bbox = cbv.getBoundingBox();
            offset = bbox.radius();

            // qDebug() << "OSGGeoTransformNode::updateNode - offset" << offset;

            // clamp model to terrain if needed
            intoTerrain = clampGeoPoint(geoPoint, offset, mapNode);
        }
        transform->setPosition(geoPoint);
    }

    osgEarth::GeoTransform *getOrCreateTransform()
    {
        if (transform.valid()) {
            return transform.get();
        }

        transform = new osgEarth::GeoTransform();
        transform->setAutoRecomputeHeights(true);

        transform->addUpdateCallback(new NodeUpdateCallback(this));

        self->setNode(transform);

        return transform.get();
    }

    OSGGeoTransformNode *const self;

    OSGNode *childNode;
    OSGNode *sceneNode;

    osg::ref_ptr<osgEarth::GeoTransform> transform;

    float     offset;

    bool      clampToTerrain;
    bool      intoTerrain;

    QVector3D position;

    bool      dirty;

private slots:

    void onChildNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGGeoTransformNode::onChildNodeChanged" << node;
        dirty = true;
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGGeoTransformNode::onSceneNodeChanged" << node;
        dirty = true;
    }
};

/* struct Hidden::NodeUpdateCallback */

void OSGGeoTransformNode::Hidden::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    nv->traverse(*node);
    h->updateNode();
}

OSGGeoTransformNode::OSGGeoTransformNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGGeoTransformNode::OSGGeoTransformNode";
}

OSGGeoTransformNode::~OSGGeoTransformNode()
{
    qDebug() << "OSGGeoTransformNode::~OSGGeoTransformNode";
}

OSGNode *OSGGeoTransformNode::modelData()
{
    return h->childNode;
}

void OSGGeoTransformNode::setModelData(OSGNode *node)
{
    if (h->acceptChildNode(node)) {
        emit modelDataChanged(node);
    }
}

OSGNode *OSGGeoTransformNode::sceneData()
{
    return h->sceneNode;
}

void OSGGeoTransformNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        emit sceneDataChanged(node);
    }
}

bool OSGGeoTransformNode::clampToTerrain() const
{
    return h->clampToTerrain;
}

void OSGGeoTransformNode::setClampToTerrain(bool arg)
{
    if (h->clampToTerrain != arg) {
        h->clampToTerrain = arg;
        h->dirty = true;
        emit clampToTerrainChanged(clampToTerrain());
    }
}

bool OSGGeoTransformNode::intoTerrain() const
{
    return h->intoTerrain;
}

QVector3D OSGGeoTransformNode::position() const
{
    return h->position;
}

void OSGGeoTransformNode::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        h->dirty    = true;
        emit positionChanged(position());
    }
}

void OSGGeoTransformNode::attach(osgViewer::View *view)
{
    // qDebug() << "OSGGeoTransformNode::attach " << view;
    if (h->childNode) {
        h->childNode->attach(view);
    }
    h->updateNode();
}

void OSGGeoTransformNode::detach(osgViewer::View *view)
{
    // qDebug() << "OSGGeoTransformNode::detach " << view;
    if (h->childNode) {
        h->childNode->detach(view);
    }
}
} // namespace osgQtQuick

#include "OSGGeoTransformNode.moc"
