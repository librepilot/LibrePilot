/**
 ******************************************************************************
 *
 * @file       OSGGeoTransformManipulator.cpp
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

#include "OSGGeoTransformManipulator.hpp"

#include "../OSGNode.hpp"
#include "utils/utility.h"

#include <osg/Matrix>
#include <osg/Node>
#include <osg/Vec3d>

#include <osgGA/CameraManipulator>

#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgEarth/Terrain>

#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Position = 1 << 10, Attitude = 1 << 11, Clamp = 1 << 12 };

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

struct OSGGeoTransformManipulator::Hidden : public QObject {
    Q_OBJECT

private:
    OSGGeoTransformManipulator * const self;

    osg::Matrix cameraPosition;
    osg::Matrix cameraRotation;

public:
    osg::ref_ptr<MyManipulator> manipulator;

    QVector3D attitude;
    QVector3D position;

    bool clampToTerrain;
    bool intoTerrain;

    Hidden(OSGGeoTransformManipulator *self) : QObject(self), self(self), clampToTerrain(false), intoTerrain(false)
    {
        manipulator = new MyManipulator();
        self->setManipulator(manipulator);
    }

    ~Hidden()
    {}

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

        OSGNode *sceneNode = self->sceneNode();

        if (sceneNode && sceneNode->node()) {
            mapNode = osgEarth::MapNode::findMapNode(sceneNode->node());
        } else {
            qWarning() << "OSGGeoTransformManipulator::updatePosition - scene node is null";
        }
        if (mapNode) {
            osgEarth::GeoPoint geoPoint = osgQtQuick::createGeoPoint(position, mapNode);
            if (clampToTerrain) {
                // clamp model to terrain if needed
                intoTerrain = osgQtQuick::clampGeoPoint(geoPoint, 0, mapNode);
            } else if (clampToTerrain) {
                qWarning() << "OSGGeoTransformManipulator::updatePosition - cannot clamp without map node";
            }
            geoPoint.createLocalToWorld(cameraPosition);
        } else {
            qWarning() << "OSGGeoTransformManipulator::updatePosition - scene node does not contain a map node";
        }
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

/* class OSGGeoTransformManipulator */

OSGGeoTransformManipulator::OSGGeoTransformManipulator(QObject *parent) : Inherited(parent), h(new Hidden(this))
{
    setDirty(Position | Attitude | Clamp);
}

OSGGeoTransformManipulator::~OSGGeoTransformManipulator()
{
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
        setDirty(Clamp);
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
        setDirty(Attitude);
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
        setDirty(Position);
        emit positionChanged(position());
    }
}

void OSGGeoTransformManipulator::update()
{
    Inherited::update();

    bool b = false;

    if (isDirty(Clamp | Position)) {
        h->updatePosition();
        b = true;
    }
    if (isDirty(Attitude)) {
        h->updateAttitude();
        b = true;
    }
    if (b) {
        h->updateManipulator();
    }
}
} // namespace osgQtQuick

#include "OSGGeoTransformManipulator.moc"
