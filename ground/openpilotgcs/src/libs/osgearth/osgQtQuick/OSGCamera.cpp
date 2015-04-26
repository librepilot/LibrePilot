#include "OSGCamera.hpp"

#include "OSGNode.hpp"

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
            QObject(parent), manipulatorMode(Default), node(NULL),
            trackerMode(NodeCenterAndAzim), trackNode(NULL),
            logDepthBufferEnabled(false), logDepthBuffer(NULL)

    {
        fieldOfView = 90.0;

        dirty     = false;

        sizeDirty = false;
        x = 0;
        y = 0;
        width     = 0;
        height    = 0;

        cameraUpdateCallback = new CameraUpdateCallback(this);
    }

    ~Hidden()
    {
        if (logDepthBuffer) {
            delete logDepthBuffer;
            logDepthBuffer = NULL;
        }
    }

    bool acceptManipulatorMode(ManipulatorMode mode)
    {
        //qDebug() << "OSGCamera::acceptManipulatorMode" << mode;
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

    void installCamera(osg::Camera *camera)
    {
        this->camera = camera;

        // install camera update callback
        // TODO will the CameraUpdateCallback be destroyed???
        this->camera->addUpdateCallback(new CameraUpdateCallback(this));

        camera->setProjectionMatrixAsPerspective(fieldOfView, 1.0f, 1.0f, 10000.0f);

        // TODO needed?
        dirty = true;
    }

    void updateCameraSize()
    {
        if (!sizeDirty || !camera.valid()) {
            return;
        }
        sizeDirty = false;
        //qDebug() << "OSGCamera::updateCamera size" << x << y << width << height << fieldOfView;
        camera->getGraphicsContext()->resized(x, y, width, height);
        camera->setViewport(x, y, width, height);
    }

    void updateCameraFOV()
    {
        //qDebug() << "OSGCamera::updateCameraFOV";
        // camera->setProjectionMatrixAsPerspective(
        // fieldOfView, static_cast<double>(width) / static_cast<double>(height), 1.0f, 10000.0f);
    }

    void updateCameraPosition()
    {
        if (manipulatorMode != User) {
            return;
        }
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
        osg::Matrix cameraPosition;

        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
                                    position.x(), position.y(), position.z(), osgEarth::ALTMODE_ABSOLUTE);

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

    qreal fieldOfView;

    ManipulatorMode manipulatorMode;

    // to compute home position
    OSGNode     *node;

    // for NodeTrackerManipulator
    TrackerMode trackerMode;
    OSGNode     *trackNode;

    // for User manipulator
    bool dirty;

    QVector3D attitude;
    QVector3D position;

    bool sizeDirty;
    int  x;
    int  y;
    int  width;
    int  height;

    osg::observer_ptr<osg::Camera> camera;

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
    h->updateCameraSize();
    h->updateCameraPosition();

    traverse(node, nv);
}

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

// ! Camera vertical field of view in degrees
void OSGCamera::setFieldOfView(qreal arg)
{
    if (h->fieldOfView != arg) {
        h->fieldOfView = arg;
        h->sizeDirty   = true;
        emit fieldOfViewChanged(fieldOfView());

        // it should be a queued call to OSGCameraRenderer instead
        /*if (h->viewer.get()) {
            h->viewer->getCamera()->setProjectionMatrixAsPerspective(
                        h->fieldOfView,
                        qreal(h->currentSize.width())/h->currentSize.height(),
                        1.0f, 10000.0f);
           }*/

        // updateFrame();
    }
}

OSGCamera::ManipulatorMode OSGCamera::manipulatorMode() const
{
    return h->manipulatorMode;
}

void OSGCamera::setManipulatorMode(ManipulatorMode mode)
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

OSGCamera::TrackerMode OSGCamera::trackerMode() const
{
    return h->trackerMode;
}

void OSGCamera::setTrackerMode(TrackerMode mode)
{
    if (h->trackerMode != mode) {
        h->trackerMode = mode;
        emit trackerModeChanged(trackerMode());
    }
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

void OSGCamera::installCamera(osgViewer::View *view)
{
    qDebug() << "OSGCamera::installCamera" << view << view->getCamera();

    h->installCamera(view->getCamera());

    osgGA::CameraManipulator *cm = NULL;

    switch (h->manipulatorMode) {
    case OSGCamera::Default:
    {
        //qDebug() << "OSGCamera::installCamera: use TrackballManipulator";
        osgGA::TrackballManipulator *tm = new osgGA::TrackballManipulator();
        // Set the minimum distance of the eye point from the center before the center is pushed forward.
        // tm->setMinimumDistance(1, true);
        cm = tm;
        break;
    }
    case OSGCamera::User:
        //qDebug() << "OSGCamera::installCamera: no camera manipulator";
        // disable any installed camera manipulator
        cm = NULL;
        break;
    case OSGCamera::Earth:
        //qDebug() << "OSGCamera::installCamera: use EarthManipulator";
        cm = new osgEarth::Util::EarthManipulator();
        break;
    case OSGCamera::Track:
        //qDebug() << "OSGCamera::installCamera: use NodeTrackerManipulator";
        if (h->trackNode && h->trackNode->node()) {
            // setup tracking camera
            // TODO when camera is thrown, then changing attitude has jitter (could be due to different frequency between refresh and animation)
            // TODO who takes ownership?
            osgGA::NodeTrackerManipulator *ntm = new osgGA::NodeTrackerManipulator(
                osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS);
            switch (h->trackerMode) {
            case NodeCenter:
                ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER);
                break;
            case NodeCenterAndAzim:
                ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER_AND_AZIM);
                break;
            case NodeCenterAndRotation:
                ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER_AND_ROTATION);
                break;
            }
            ntm->setTrackNode(h->trackNode->node());
            // ntm->setRotationMode(h->trackRotationMode)
            // ntm->setMinimumDistance(2, false);
            // ntm->setVerticalAxisFixed(false);
            // ntm->setAutoComputeHomePosition(true);
            // ntm->setDistance(10);
            cm = ntm;
        } else {
            qWarning() << "OSGCamera::installCamera: no track node provided.";
            cm = NULL;
        }
        break;
    default:
        qWarning() << "OSGCamera::installCamera: should not reach here!";
        break;
    }

    view->setCameraManipulator(cm, false);
    if (cm && h->node && h->node->node()) {
        // set node used to auto compute home position
        // needs to be done after setting the manipulator on the view as the view will set its scene as the node
        cm->setNode(h->node->node());
    }
    view->home();
}

void OSGCamera::setViewport(int x, int y, int width, int height)
{
    // qDebug() << "OSGCamera::setViewport" << x << y << width << "x" << heigth;
    if (width <= 0 || height <= 0) {
        qWarning() << "OSGCamera::setViewport - invalid size " << width << "x" << height;
        return;
    }
    if (h->x != x || h->y != y || h->width != width || h->height != height) {
        h->x         = x;
        h->y         = y;
        h->width     = width;
        h->height    = height;
        h->sizeDirty = true;
    }
}
} // namespace osgQtQuick

#include "OSGCamera.moc"
