/**
 ******************************************************************************
 *
 * @file       OSGModelNode.cpp
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

#include "OSGModelNode.hpp"

#include "../utility.h"

#include <osg/Quat>
#include <osg/ComputeBoundsVisitor>
#include <osgViewer/View>

#include <osgEarth/MapNode>
#include <osgEarth/GeoData>

#include <osgEarthSymbology/Style>
#include <osgEarthSymbology/ModelSymbol>

#include <osgEarthAnnotation/ModelNode>

#include <QDebug>

namespace osgQtQuick {
struct OSGModelNode::Hidden : public QObject {
    Q_OBJECT

    struct NodeUpdateCallback : public osg::NodeCallback {
public:
        NodeUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGModelNode *parent) : QObject(parent), self(parent), modelData(NULL), sceneData(NULL), offset(-1.0), clampToTerrain(false), intoTerrain(false), dirty(false)
    {}

    ~Hidden()
    {}

    bool acceptModelData(OSGNode *node)
    {
        qDebug() << "OSGModelNode::acceptModelData" << node;
        if (modelData == node) {
            return false;
        }

        if (modelData) {
            disconnect(modelData);
        }

        modelData = node;

        if (modelData) {
            acceptModelNode(modelData->node());
            connect(modelData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onModelNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptModelNode(osg::Node *node)
    {
        qDebug() << "OSGModelNode::acceptModelNode" << node;
        if (!node) {
            qWarning() << "OSGModelNode::acceptModelNode - node is null";
            return false;
        }

        if (!sceneData || !sceneData->node()) {
            qWarning() << "OSGModelNode::acceptModelNode - no scene data";
            return false;
        }

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
        if (!mapNode) {
            qWarning() << "OSGModelNode::acceptModelNode - scene data does not contain a map node";
            return false;
        }

        // establish the coordinate system we wish to use:
        // const osgEarth::SpatialReference* latLong = osgEarth::SpatialReference::get("wgs84");

        osgEarth::Symbology::Style style;

        // construct the symbology
        osgEarth::Symbology::ModelSymbol *modelSymbol = style.getOrCreate<osgEarth::Symbology::ModelSymbol>();
        modelSymbol->setModel(node);

        // create ModelNode
        modelNode = new osgEarth::Annotation::ModelNode(mapNode, style);

        // add update callback
        if (!nodeUpdateCallback.valid()) {
            nodeUpdateCallback = new NodeUpdateCallback(this);
        }
        modelNode->addUpdateCallback(nodeUpdateCallback.get());

        // get "size" of model
        osg::ComputeBoundsVisitor cbv;
        modelNode->accept(cbv);
        const osg::BoundingBox & bbox = cbv.getBoundingBox();
        offset = bbox.radius();

        self->setNode(modelNode);

        dirty = true;

        return true;
    }

    bool attach(osgViewer::View *view)
    {
        return true;
    }

    bool detach(osgViewer::View *view)
    {
        qWarning() << "OSGModelNode::detach - not implemented";
        return true;
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGModelNode::acceptSceneData" << node;
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            // TODO find a better way
            if (modelData && modelData->node()) {
                acceptModelNode(modelData->node());
            }
            connect(sceneData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onSceneNodeChanged(osg::Node *)));
        }

        return true;
    }

    // TODO model update gets jitter if camera is thrown (i.e animated)
    void updateNode()
    {
        if (!dirty || !modelNode.valid()) {
            return;
        }
        dirty = false;

        osgEarth::GeoPoint geoPoint = osgQtQuick::toGeoPoint(position);
        if (clampToTerrain) {
            osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
            if (mapNode) {
                intoTerrain = clampGeoPoint(geoPoint, offset, mapNode);
            } else {
                qWarning() << "OSGModelNode::updateNode - scene data does not contain a map node";
            }
        }
        modelNode->setPosition(geoPoint);
        modelNode->setLocalRotation(localRotation());
    }

    osg::Quat localRotation()
    {
        osg::Quat q = osg::Quat(
            osg::DegreesToRadians(attitude.x()), osg::Vec3d(1, 0, 0),
            osg::DegreesToRadians(attitude.y()), osg::Vec3d(0, 1, 0),
            osg::DegreesToRadians(attitude.z()), osg::Vec3d(0, 0, 1));

        return q;
    }

    OSGModelNode *const self;

    OSGNode *modelData;
    OSGNode *sceneData;

    osg::ref_ptr<osgEarth::Annotation::ModelNode> modelNode;

    float     offset;

    bool      clampToTerrain;
    bool      intoTerrain;

    QVector3D attitude;
    QVector3D position;

    // handle attitude/position/etc independently
    bool      dirty;

    osg::observer_ptr<NodeUpdateCallback> nodeUpdateCallback;

private slots:

    void onModelNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode::onModelNodeChanged" << node;
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCallback.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCallback.get());
                }
            }
            if (node) {
                acceptModelNode(node);
            }
        }
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode::onSceneNodeChanged" << node;
        // TODO needs to be improved...
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCallback.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCallback.get());
                }
            }
            if (modelData->node()) {
                acceptModelNode(modelData->node());
            }
        }
    }
};

/* struct Hidden::NodeUpdateCallback */

void OSGModelNode::Hidden::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    h->updateNode();
    traverse(node, nv);
}

OSGModelNode::OSGModelNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGModelNode::OSGModelNode";
}

OSGModelNode::~OSGModelNode()
{
    qDebug() << "OSGModelNode::~OSGModelNode";
}

OSGNode *OSGModelNode::modelData()
{
    return h->modelData;
}

void OSGModelNode::setModelData(OSGNode *node)
{
    if (h->acceptModelData(node)) {
        emit modelDataChanged(node);
    }
}

OSGNode *OSGModelNode::sceneData()
{
    return h->sceneData;
}

void OSGModelNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

bool OSGModelNode::clampToTerrain() const
{
    return h->clampToTerrain;
}

void OSGModelNode::setClampToTerrain(bool arg)
{
    if (h->clampToTerrain != arg) {
        h->clampToTerrain = arg;
        h->dirty = true;
        emit clampToTerrainChanged(clampToTerrain());
    }
}

bool OSGModelNode::intoTerrain() const
{
    return h->intoTerrain;
}

QVector3D OSGModelNode::attitude() const
{
    return h->attitude;
}

void OSGModelNode::setAttitude(QVector3D arg)
{
    if (h->attitude != arg) {
        h->attitude = arg;
        h->dirty    = true;
        emit attitudeChanged(attitude());
    }
}

QVector3D OSGModelNode::position() const
{
    return h->position;
}

void OSGModelNode::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        h->dirty    = true;
        emit positionChanged(position());
    }
}

bool OSGModelNode::attach(osgViewer::View *view)
{
    return h->attach(view);
}

bool OSGModelNode::detach(osgViewer::View *view)
{
    return h->detach(view);
}
} // namespace osgQtQuick

#include "OSGModelNode.moc"
