/**
 ******************************************************************************
 *
 * @file       OSGGeoTransformManipulator.cpp
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

#include "OSGGeoTransformManipulator.hpp"

#include "../OSGNode.hpp"
#include "../../utility.h"

#include <osg/Matrix>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/Vec3d>

#include <osgGA/CameraManipulator>

#include <osgEarth/GeoData>
#include <osgEarth/MapNode>

#include <QDebug>

namespace osgQtQuick {
class MyManipulator : public osgGA::CameraManipulator {
public:
    MyManipulator()
    {}

    virtual void updateCamera(osg::Camera &camera);

    virtual const char *className() const
    {
        return "MyManipulator";
    }

    virtual void setByMatrix(const osg::Matrixd &matrix)
    {
        this->matrix = matrix;
    }

    virtual void setByInverseMatrix(const osg::Matrixd &matrix)
    {
        this->invMatrix = matrix;
    }

    virtual osg::Matrixd getMatrix() const
    {
        return matrix;
    }

    virtual osg::Matrixd getInverseMatrix() const
    {
        return invMatrix;
    }

    virtual void setNode(osg::Node *node)
    {
        this->node = node;
    }

    virtual const osg::Node *getNode() const
    {
        return node.get();
    }

    virtual osg::Node *getNode()
    {
        return node.get();
    }

private:
    osg::Matrixd matrix;
    osg::Matrixd invMatrix;
    osg::ref_ptr<osg::Node> node;
};

struct OSGGeoTransformManipulator::NodeUpdateCallback : public osg::NodeCallback {
public:
    NodeUpdateCallback(OSGGeoTransformManipulator::Hidden *h) : h(h)
    {}

    void operator()(osg::Node *node, osg::NodeVisitor *nv);

private:
    OSGGeoTransformManipulator::Hidden *const h;
};

struct OSGGeoTransformManipulator::Hidden : public QObject {
    Q_OBJECT

private:
    OSGGeoTransformManipulator * const self;

    osg::ref_ptr<osg::NodeCallback> nodeUpdateCallback;

    osg::Matrix cameraPosition;
    osg::Matrix cameraRotation;

    bool dirty;

public:
    osg::ref_ptr<MyManipulator> manipulator;

    QVector3D attitude;
    QVector3D position;

    bool clampToTerrain;
    bool intoTerrain;

    Hidden(OSGGeoTransformManipulator *self) : QObject(self), self(self), dirty(false), clampToTerrain(false), intoTerrain(false)
    {
        manipulator = new MyManipulator();
        self->setManipulator(manipulator);
    }

    ~Hidden()
    {}

    // TODO factorize up
    void setDirty()
    {
        if (dirty) {
            return;
        }
        // qDebug() << "OSGGeoTransformManipulator::setDirty";
        dirty = true;
        osg::Node *node = manipulator->getNode();
        if (node) {
            if (!nodeUpdateCallback.valid()) {
                // lazy creation
                nodeUpdateCallback = new NodeUpdateCallback(this);
            }
            node->addUpdateCallback(nodeUpdateCallback.get());
        } else {
            qWarning() << "OSGGeoTransformManipulator::setDirty - no node...";
        }
    }

    // TODO factorize up
    void clearDirty()
    {
        // qDebug() << "OSGGeoTransformManipulator::clearDirty";
        osg::Node *node = manipulator->getNode();

        if (node && nodeUpdateCallback.valid()) {
            node->removeUpdateCallback(nodeUpdateCallback.get());
        }
        dirty = false;
    }

    void update()
    {
        updatePosition();
        updateAttitude();
        updateManipulator();
    }

    void updateManipulator()
    {
        // qDebug() << "OSGGeoTransformManipulator::updateManipulator";

        osg::Matrix cameraMatrix = cameraRotation * cameraPosition;

        manipulator->setByMatrix(cameraMatrix);

        // Inverse the camera's position and orientation matrix to obtain the view matrix
        cameraMatrix = osg::Matrix::inverse(cameraMatrix);
        manipulator->setByInverseMatrix(cameraMatrix);
    }

    void updatePosition()
    {
        // qDebug() << "OSGGeoTransformManipulator::updatePosition" << position;

        // Altitude mode is absolute (absolute height above MSL/HAE)
        // HAE : Height above ellipsoid. This is the default.
        // MSL : Height above Mean Sea Level (MSL) if a geoid separation value is specified.
        // TODO handle the case where the terrain SRS is not "wgs84"
        // TODO check if position is not below terrain?
        // TODO compensate antenna height when source of position is GPS (i.e. subtract antenna height from altitude) ;)

        osgEarth::MapNode *mapNode = NULL;

        if (manipulator->getNode()) {
            mapNode = osgEarth::MapNode::findMapNode(manipulator->getNode());
            if (!mapNode) {
                qWarning() << "OSGGeoTransformManipulator::updatePosition - manipulator node does not contain a map node";
            }
        } else {
            qWarning() << "OSGGeoTransformManipulator::updatePosition - manipulator node is null";
        }

        osgEarth::GeoPoint geoPoint;
        if (mapNode) {
            geoPoint = osgQtQuick::toGeoPoint(mapNode->getTerrain()->getSRS(), position);
        } else {
            geoPoint = osgQtQuick::toGeoPoint(position);
        }
        if (clampToTerrain && mapNode) {
            // clamp model to terrain if needed
            intoTerrain = osgQtQuick::clampGeoPoint(geoPoint, 0, mapNode);
        } else if (clampToTerrain) {
            qWarning() << "OSGGeoTransformManipulator::updatePosition - cannot clamp without map node";
        }

        geoPoint.createLocalToWorld(cameraPosition);
    }

    void updateAttitude()
    {
        // qDebug() << "OSGGeoTransformManipulator::updateAttitude" << attitude;

        // By default the camera looks toward -Z, we must rotate it so it looks toward Y
        cameraRotation.makeRotate(osg::DegreesToRadians(90.0), osg::Vec3(1.0, 0.0, 0.0),
                                  osg::DegreesToRadians(0.0), osg::Vec3(0.0, 1.0, 0.0),
                                  osg::DegreesToRadians(0.0), osg::Vec3(0.0, 0.0, 1.0));

        // Final camera matrix
        double roll  = osg::DegreesToRadians(attitude.x());
        double pitch = osg::DegreesToRadians(attitude.y());
        double yaw   = osg::DegreesToRadians(attitude.z());
        cameraRotation = cameraRotation
                         * osg::Matrix::rotate(roll, osg::Vec3(0, 1, 0))
                         * osg::Matrix::rotate(pitch, osg::Vec3(1, 0, 0))
                         * osg::Matrix::rotate(yaw, osg::Vec3(0, 0, -1));
    }
};

/* class OSGGeoTransformManipulator::MyManipulator */

void MyManipulator::updateCamera(osg::Camera & camera)
{
    // qDebug() << "MyManipulator::updateCamera";
    CameraManipulator::updateCamera(camera);
}

/* struct OSGGeoTransformManipulator::NodeUpdateCallback */

void OSGGeoTransformManipulator::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    // qDebug() << "OSGGeoTransformManipulator::NodeUpdateCallback";
    nv->traverse(*node);
    h->update();
    h->clearDirty();
}

/* class OSGGeoTransformManipulator */

OSGGeoTransformManipulator::OSGGeoTransformManipulator(QObject *parent) : OSGCameraManipulator(parent), h(new Hidden(this))
{}

OSGGeoTransformManipulator::~OSGGeoTransformManipulator()
{
    qDebug() << "OSGGeoTransformManipulator::~OSGGeoTransformManipulator";
    delete h;
}

bool OSGGeoTransformManipulator::clampToTerrain() const
{
    return h->clampToTerrain;
}

void OSGGeoTransformManipulator::setClampToTerrain(bool arg)
{
    if (h->clampToTerrain != arg) {
        h->clampToTerrain = arg;
        emit clampToTerrainChanged(clampToTerrain());
    }
}

bool OSGGeoTransformManipulator::intoTerrain() const
{
    return h->intoTerrain;
}

QVector3D OSGGeoTransformManipulator::attitude() const
{
    return h->attitude;
}

void OSGGeoTransformManipulator::setAttitude(QVector3D arg)
{
    if (h->attitude != arg) {
        h->attitude = arg;
        h->setDirty();
        emit attitudeChanged(attitude());
    }
}

QVector3D OSGGeoTransformManipulator::position() const
{
    return h->position;
}

void OSGGeoTransformManipulator::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        h->setDirty();
        emit positionChanged(position());
    }
}

// TODO factorize up
void OSGGeoTransformManipulator::componentComplete()
{
    OSGCameraManipulator::componentComplete();

    qDebug() << "OSGGeoTransformManipulator::componentComplete" << this;
    h->update();
    h->clearDirty();
}
} // namespace osgQtQuick

#include "OSGGeoTransformManipulator.moc"
