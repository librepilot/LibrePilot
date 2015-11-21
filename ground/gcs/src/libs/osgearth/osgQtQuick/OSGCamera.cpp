/**
 ******************************************************************************
 *
 * @file       OSGCamera.cpp
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

#include "OSGCamera.hpp"

#include "OSGNode.hpp"

#include "../utility.h"

#include <osg/Camera>
#include <osg/Matrix>
#include <osg/Node>
#include <osg/Vec3d>

#include <osgGA/NodeTrackerManipulator>
#include <osgGA/TrackballManipulator>

#include <osgViewer/View>

#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/LogarithmicDepthBuffer>

#include <QDebug>
#include <QThread>
#include <QApplication>

namespace osgQtQuick {
struct OSGCamera::Hidden : public QObject {
    Q_OBJECT

    struct CameraUpdateCallback : public osg::NodeCallback {
public:
        CameraUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class CameraUpdateCallback;

public:

    Hidden(OSGCamera *parent) :
        QObject(parent), sceneData(NULL), manipulatorMode(ManipulatorMode::Default), node(NULL),
        trackerMode(TrackerMode::NodeCenterAndAzim), trackNode(NULL),
        logDepthBufferEnabled(false), logDepthBuffer(NULL), clampToTerrain(false)
    {
        fieldOfView = 90.0;

        first    = true;

        dirty    = false;
        fovDirty = false;
    }

    ~Hidden()
    {
        if (logDepthBuffer) {
            delete logDepthBuffer;
            logDepthBuffer = NULL;
        }
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGCamera::acceptSceneData" << node;
        if (sceneData == node) {
            return true;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            // connect(sceneData, &OSGNode::nodeChanged, this, &Hidden::onSceneDataChanged);
        }

        return true;
    }

    bool acceptManipulatorMode(ManipulatorMode::Enum mode)
    {
        // qDebug() << "OSGCamera::acceptManipulatorMode" << mode;
        if (manipulatorMode == mode) {
            return false;
        }

        manipulatorMode = mode;

        return true;
    }

    bool acceptNode(OSGNode *node)
    {
        qDebug() << "OSGCamera::acceptNode" << node;
        if (this->node == node) {
            return false;
        }

        if (this->node) {
            disconnect(this->node);
        }

        this->node = node;

        if (this->node) {
            connect(this->node, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptTrackNode(OSGNode *node)
    {
        qDebug() << "OSGCamera::acceptTrackNode" << node;
        if (trackNode == node) {
            return false;
        }

        if (trackNode) {
            disconnect(trackNode);
        }

        trackNode = node;

        if (trackNode) {
            connect(trackNode, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onTrackNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool attach(osgViewer::View *view)
    {
        attachCamera(view->getCamera());
        attachManipulator(view);
        return true;
    }

    bool detach(osgViewer::View *view)
    {
        detachManipulator(view);
        detachCamera(view->getCamera());
        return true;
    }

    void attachCamera(osg::Camera *camera)
    {
        qDebug() << "OSGCamera::attach" << camera;

        this->camera = camera;

        cameraUpdateCallback = new CameraUpdateCallback(this);
        camera->addUpdateCallback(cameraUpdateCallback);

        // install log depth buffer if requested
        if (logDepthBufferEnabled) {
            qDebug() << "OSGCamera::attach - install logarithmic depth buffer";
            logDepthBuffer = new osgEarth::Util::LogarithmicDepthBuffer();
            logDepthBuffer->setUseFragDepth(true);
            logDepthBuffer->install(camera);
        }

        dirty    = true;
        fovDirty = true;
        updateCamera();
        updateAspectRatio();
    }

    void detachCamera(osg::Camera *camera)
    {
        qDebug() << "OSGCamera::detach" << camera;

        if (camera != this->camera) {
            qWarning() << "OSGCamera::detach - camera not attached" << camera;
            return;
        }
        this->camera = NULL;

        if (cameraUpdateCallback.valid()) {
            camera->removeUpdateCallback(cameraUpdateCallback);
            cameraUpdateCallback = NULL;
        }

        if (logDepthBuffer) {
            logDepthBuffer->uninstall(camera);
            delete logDepthBuffer;
            logDepthBuffer = NULL;
        }
    }

    void attachManipulator(osgViewer::View *view)
    {
        qDebug() << "OSGCamera::attachManipulator" << view;

        osgGA::CameraManipulator *cm = NULL;

        switch (manipulatorMode) {
        case ManipulatorMode::Default:
        {
            qDebug() << "OSGCamera::attachManipulator - use TrackballManipulator";
            osgGA::TrackballManipulator *tm = new osgGA::TrackballManipulator();
            // Set the minimum distance of the eye point from the center before the center is pushed forward.
            // tm->setMinimumDistance(1, true);
            cm = tm;
            break;
        }
        case ManipulatorMode::User:
            qDebug() << "OSGCamera::attachManipulator - no camera manipulator";
            // disable any installed camera manipulator
            cm = NULL;
            break;
        case ManipulatorMode::Earth:
        {
            qDebug() << "OSGCamera::attachManipulator - use EarthManipulator";
            osgEarth::Util::EarthManipulator *em = new osgEarth::Util::EarthManipulator();
            em->getSettings()->setThrowingEnabled(true);
            cm = em;
            break;
        }
        case ManipulatorMode::Track:
            qDebug() << "OSGCamera::attachManipulator - use NodeTrackerManipulator";
            if (trackNode && trackNode->node()) {
                // setup tracking camera
                // TODO when camera is thrown, then changing attitude has jitter (could be due to different frequency between refresh and animation)
                // TODO who takes ownership?
                osgGA::NodeTrackerManipulator *ntm = new osgGA::NodeTrackerManipulator(
                    /*osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS*/);
                switch (trackerMode) {
                case TrackerMode::NodeCenter:
                    ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER);
                    break;
                case TrackerMode::NodeCenterAndAzim:
                    ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER_AND_AZIM);
                    break;
                case TrackerMode::NodeCenterAndRotation:
                    ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER_AND_ROTATION);
                    break;
                }
                ntm->setTrackNode(trackNode->node());
                ntm->setVerticalAxisFixed(false);
                cm = ntm;
            } else {
                qWarning() << "OSGCamera::attachManipulator - no track node provided.";
                cm = NULL;
            }
            break;
        default:
            qWarning() << "OSGCamera::attachManipulator - should not reach here!";
            break;
        }

        view->setCameraManipulator(cm, false);
        if (cm && node && node->node()) {
            qDebug() << "OSGCamera::attachManipulator - camera node" << node;
            // set node used to auto compute home position
            // needs to be done after setting the manipulator on the view as the view will set its scene as the node
            cm->setNode(node->node());
        }
        if (cm) {
            view->home();
        }
    }

    void detachManipulator(osgViewer::View *view)
    {
        qDebug() << "OSGCamera::detachManipulator" << view;

        view->setCameraManipulator(NULL, false);
    }

    void updateCamera()
    {
        updateCameraFOV();
        if (manipulatorMode == ManipulatorMode::User) {
            updateCameraPosition();
        }
    }

    void updateCameraFOV()
    {
        if (!fovDirty || !camera.valid()) {
            return;
        }
        fovDirty = false;

        // qDebug() << "OSGCamera::updateCameraFOV";
        double fovy, ar, zn, zf;

        camera->getProjectionMatrixAsPerspective(fovy, ar, zn, zf);

        fovy = fieldOfView;
        camera->setProjectionMatrixAsPerspective(fovy, ar, zn, zf);
    }

    void updateAspectRatio()
    {
        double fovy, ar, zn, zf;

        camera->getProjectionMatrixAsPerspective(fovy, ar, zn, zf);

        osg::Viewport *viewport = camera->getViewport();
        ar = static_cast<double>(viewport->width()) / static_cast<double>(viewport->height());
        camera->setProjectionMatrixAsPerspective(fovy, ar, zn, zf);
    }

    void updateCameraPosition()
    {
        if (!dirty || !camera.valid()) {
            return;
        }
        dirty = false;

        // Altitude mode is absolute (absolute height above MSL/HAE)
        // HAE : Height above ellipsoid. This is the default.
        // MSL : Height above Mean Sea Level (MSL) if a geoid separation value is specified.
        // TODO handle the case where the terrain SRS is not "wgs84"
        // TODO check if position is not below terrain?
        // TODO compensate antenna height when source of position is GPS (i.e. subtract antenna height from altitude) ;)

        // Camera position
        osgEarth::GeoPoint geoPoint = osgQtQuick::toGeoPoint(position);
        if (clampToTerrain) {
            if (sceneData) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    intoTerrain = clampGeoPoint(geoPoint, 0.5f, mapNode);
                } else {
                    qWarning() << "OSGCamera::updateNode - scene data does not contain a map node";
                }
            }
        }

        osg::Matrix cameraPosition;
        geoPoint.createLocalToWorld(cameraPosition);

        // Camera orientation
        // By default the camera looks toward -Z, we must rotate it so it looks toward Y
        osg::Matrix cameraRotation;
        cameraRotation.makeRotate(osg::DegreesToRadians(90.0), osg::Vec3(1.0, 0.0, 0.0),
                                  osg::DegreesToRadians(0.0), osg::Vec3(0.0, 1.0, 0.0),
                                  osg::DegreesToRadians(0.0), osg::Vec3(0.0, 0.0, 1.0));

        // Final camera matrix
        osg::Matrix cameraMatrix = cameraRotation
                                   * osg::Matrix::rotate(osg::DegreesToRadians(attitude.x()), osg::Vec3(1.0, 0.0, 0.0))
                                   * osg::Matrix::rotate(osg::DegreesToRadians(attitude.y()), osg::Vec3(0.0, 1.0, 0.0))
                                   * osg::Matrix::rotate(osg::DegreesToRadians(attitude.z()), osg::Vec3(0.0, 0.0, 1.0)) * cameraPosition;

        // Inverse the camera's position and orientation matrix to obtain the view matrix
        cameraMatrix = osg::Matrix::inverse(cameraMatrix);
        camera->setViewMatrix(cameraMatrix);
    }

    qreal   fieldOfView;
    bool    fovDirty;

    OSGNode *sceneData;

    ManipulatorMode::Enum manipulatorMode;

    // to compute home position
    OSGNode *node;

    // for NodeTrackerManipulator
    TrackerMode::Enum trackerMode;
    OSGNode   *trackNode;

    bool      logDepthBufferEnabled;
    osgEarth::Util::LogarithmicDepthBuffer *logDepthBuffer;

    bool      first;

    // for User manipulator
    bool      dirty;

    bool      clampToTerrain;
    bool      intoTerrain;

    QVector3D attitude;
    QVector3D position;

    osg::ref_ptr<osg::Camera> camera;
    osg::ref_ptr<CameraUpdateCallback> cameraUpdateCallback;

private slots:
    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGCamera::onNodeChanged" << node;
        qWarning() << "OSGCamera::onNodeChanged - needs to be implemented";
    }

    void onTrackNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGCamera::onTrackNodeChanged" << node;
        qWarning() << "OSGCamera::onTrackNodeChanged - needs to be implemented";
    }
};

/* struct Hidden::CameraUpdateCallback */

void OSGCamera::Hidden::CameraUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    h->updateCamera();
    // traverse(node, nv);
}

/* class OSGCamera */

OSGCamera::OSGCamera(QObject *parent) : QObject(parent), h(new Hidden(this))
{
    qDebug() << "OSGCamera::OSGCamera";
}

OSGCamera::~OSGCamera()
{
    qDebug() << "OSGCamera::~OSGCamera";
}

qreal OSGCamera::fieldOfView() const
{
    return h->fieldOfView;
}

// Camera vertical field of view in degrees
void OSGCamera::setFieldOfView(qreal arg)
{
    if (h->fieldOfView != arg) {
        h->fieldOfView = arg;
        h->fovDirty    = true;
        emit fieldOfViewChanged(fieldOfView());
    }
}

OSGNode *OSGCamera::sceneData()
{
    return h->sceneData;
}

void OSGCamera::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

ManipulatorMode::Enum OSGCamera::manipulatorMode() const
{
    return h->manipulatorMode;
}

void OSGCamera::setManipulatorMode(ManipulatorMode::Enum mode)
{
    if (h->acceptManipulatorMode(mode)) {
        emit manipulatorModeChanged(manipulatorMode());
    }
}

OSGNode *OSGCamera::node() const
{
    return h->node;
}

void OSGCamera::setNode(OSGNode *node)
{
    if (h->acceptNode(node)) {
        emit nodeChanged(node);
    }
}

OSGNode *OSGCamera::trackNode() const
{
    return h->trackNode;
}

void OSGCamera::setTrackNode(OSGNode *node)
{
    if (h->acceptTrackNode(node)) {
        emit trackNodeChanged(node);
    }
}

TrackerMode::Enum OSGCamera::trackerMode() const
{
    return h->trackerMode;
}

void OSGCamera::setTrackerMode(TrackerMode::Enum mode)
{
    if (h->trackerMode != mode) {
        h->trackerMode = mode;
        emit trackerModeChanged(trackerMode());
    }
}

bool OSGCamera::clampToTerrain() const
{
    return h->clampToTerrain;
}

void OSGCamera::setClampToTerrain(bool arg)
{
    if (h->clampToTerrain != arg) {
        h->clampToTerrain = arg;
        h->dirty = true;
        emit clampToTerrainChanged(clampToTerrain());
    }
}

bool OSGCamera::intoTerrain() const
{
    return h->intoTerrain;
}

QVector3D OSGCamera::attitude() const
{
    return h->attitude;
}

void OSGCamera::setAttitude(QVector3D arg)
{
    if (h->attitude != arg) {
        h->attitude = arg;
        h->dirty    = true;
        emit attitudeChanged(attitude());
    }
}

QVector3D OSGCamera::position() const
{
    return h->position;
}

void OSGCamera::setPosition(QVector3D arg)
{
    if (h->position != arg) {
        h->position = arg;
        h->dirty    = true;
        emit positionChanged(position());
    }
}

bool OSGCamera::logarithmicDepthBuffer()
{
    return h->logDepthBufferEnabled;
}

void OSGCamera::setLogarithmicDepthBuffer(bool enabled)
{
    if (h->logDepthBufferEnabled != enabled) {
        h->logDepthBufferEnabled = enabled;
        emit logarithmicDepthBufferChanged(logarithmicDepthBuffer());
    }
}

bool OSGCamera::attach(osgViewer::View *view)
{
    return h->attach(view);
}

bool OSGCamera::detach(osgViewer::View *view)
{
    return h->detach(view);
}
} // namespace osgQtQuick

#include "OSGCamera.moc"
