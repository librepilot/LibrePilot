/**
 ******************************************************************************
 *
 * @file       OSGGeoTransformNode.cpp
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

private:
    OSGGeoTransformNode * const self;

    osg::ref_ptr<osgEarth::GeoTransform> transform;

public:
    OSGNode   *childNode;
    OSGNode   *sceneNode;

    float     offset;

    bool      clampToTerrain;
    bool      intoTerrain;

    QVector3D position;

    Hidden(OSGGeoTransformNode *node) : QObject(node), self(node), childNode(NULL), sceneNode(NULL), offset(-1.0), clampToTerrain(false), intoTerrain(false)
    {
        transform = new osgEarth::GeoTransform();
        transform->setAutoRecomputeHeights(true);
        self->setNode(transform);
    }

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

        if (sceneNode) {
            connect(sceneNode, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onSceneNodeChanged(osg::Node *)));
        }

        return true;
    }

    void updateTransformNode()
    {
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

    void updateSceneNode()
    {
        if (sceneNode && sceneNode->node()) {
            osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneNode->node());
            if (mapNode) {
                transform->setTerrain(mapNode->getTerrain());
            } else {
                qWarning() << "OSGGeoTransformNode::updateScene - scene data does not contain a map node";
            }
        }
    }

    void updatePosition()
    {
        osgEarth::MapNode *mapNode = NULL;

        if (sceneNode && sceneNode->node()) {
            mapNode = osgEarth::MapNode::findMapNode(sceneNode->node());
        }
        if (!mapNode) {
            qWarning() << "OSGGeoTransformNode::updatePosition - scene node does not contain a map node";
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

private slots:
    void onChildNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGGeoTransformNode::onChildNodeChanged" << node;
        updateTransformNode();
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGGeoTransformNode::onSceneNodeChanged" << node;
        // TODO
    }
};

/* class OSGGeoTransformNode */

enum DirtyFlag { Child = 1 << 0, Scene = 1 << 1, Position = 1 << 2, Clamp = 1 << 3 };

OSGGeoTransformNode::OSGGeoTransformNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGGeoTransformNode::OSGGeoTransformNode";
}

OSGGeoTransformNode::~OSGGeoTransformNode()
{
    // qDebug() << "OSGGeoTransformNode::~OSGGeoTransformNode";
    delete h;
}

OSGNode *OSGGeoTransformNode::childNode()
{
    return h->childNode;
}

void OSGGeoTransformNode::setChildNode(OSGNode *node)
{
    if (h->acceptChildNode(node)) {
        setDirty(Child);
        emit childNodeChanged(node);
    }
}

OSGNode *OSGGeoTransformNode::sceneNode()
{
    return h->sceneNode;
}

void OSGGeoTransformNode::setSceneNode(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        setDirty(Scene);
        emit sceneNodeChanged(node);
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
        setDirty(Clamp);
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
        setDirty(Position);
        emit positionChanged(position());
    }
}

void OSGGeoTransformNode::update()
{
    if (isDirty(Child)) {
        h->updateTransformNode();
    }
    if (isDirty(Scene)) {
        h->updateSceneNode();
    }
    if (isDirty(Clamp)) {}
    if (isDirty(Position)) {
        h->updatePosition();
    }
}

void OSGGeoTransformNode::attach(osgViewer::View *view)
{
    OSGNode::attach(h->childNode, view);

    update();
    clearDirty();
}

void OSGGeoTransformNode::detach(osgViewer::View *view)
{
    OSGNode::detach(h->childNode, view);
}
} // namespace osgQtQuick

#include "OSGGeoTransformNode.moc"
