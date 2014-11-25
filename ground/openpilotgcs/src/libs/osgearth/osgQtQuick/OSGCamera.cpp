#include "OSGCamera.hpp"

#include "OSGNode.hpp"

#include <osg/Camera>
#include <osg/Matrix>
#include <osg/Node>
#include <osg/Vec3d>

#include <osgGA/NodeTrackerManipulator>
//#include <osgGA/KeySwitchMatrixManipulator>
//#include <osgGA/TrackballManipulator>
//#include <osgGA/SphericalManipulator>
//#include <osgGA/FlightManipulator>
//#include <osgGA/DriveManipulator>
//#include <osgGA/OrbitManipulator>
//#include <osgGA/TerrainManipulator>

#include <osgViewer/View>

#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <osgEarthUtil/EarthManipulator>

#include <QDebug>

namespace osgQtQuick {

struct OSGCamera::Hidden : public QObject
{
    Q_OBJECT

public:

    Hidden(OSGCamera *parent) : QObject(parent), manipulatorMode(None), trackNode(NULL), trackerMode(NodeCenter) {
        fieldOfView = 90.0;
        roll = pitch = yaw = 0.0;
        latitude = longitude = altitude = 0.0;
    }

    ~Hidden()
    {
    }

    bool acceptManipulatorMode(ManipulatorMode mode)
    {
        if (manipulatorMode == mode) {
            return true;
        }

        manipulatorMode = mode;

        return true;

    }

    bool acceptTrackNode(OSGNode *node)
    {
        qDebug() << "OSGCamera - acceptTrackNode" << node;
        if (trackNode == node) {
            return true;
        }

        if (trackNode) {
            disconnect(trackNode);
        }

        trackNode = node;

        if (trackNode) {
            connect(trackNode, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onTrackNodeChanged(osg::Node*)));
        }

        return true;

    }

    qreal fieldOfView;

    ManipulatorMode manipulatorMode;

    OSGNode *trackNode;
    TrackerMode trackerMode;

    qreal roll;
    qreal pitch;
    qreal yaw;

    double latitude;
    double longitude;
    double altitude;

    //osg::ref_ptr<osg::Node> node;

private slots:
    void onTrackNodeChanged(osg::Node *node)
    {
    }

};

OSGCamera::OSGCamera(QObject *parent) : QObject(parent), h(new Hidden(this))
{    
    qDebug() << "OSGCamera - <init>";
}

OSGCamera::~OSGCamera()
{
    qDebug() << "OSGCamera - <destruct>";
}

qreal OSGCamera::fieldOfView() const
{
    return h->fieldOfView;
}

// ! Camera vertical field of view in degrees
void OSGCamera::setFieldOfView(qreal arg)
{
    if (h->fieldOfView!= arg) {
        h->fieldOfView = arg;
        emit fieldOfViewChanged(fieldOfView());

        // it should be a queued call to OSGCameraRenderer instead
        /*if (h->viewer.get()) {
            h->viewer->getCamera()->setProjectionMatrixAsPerspective(
                        h->fieldOfView,
                        qreal(h->currentSize.width())/h->currentSize.height(),
                        1.0f, 10000.0f);
           }*/

        //updateFrame();
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

qreal OSGCamera::roll() const
{
    return h->roll;
}

void OSGCamera::setRoll(qreal arg)
{
    if (h->roll!= arg) {
        h->roll = arg;
        emit rollChanged(roll());
        emit attitudeChanged(h->roll, h->pitch, h->yaw);
    }
}

qreal OSGCamera::pitch() const
{
    return h->pitch;
}

void OSGCamera::setPitch(qreal arg)
{
    if (h->pitch!= arg) {
        h->pitch = arg;
        emit pitchChanged(pitch());
        emit attitudeChanged(h->roll, h->pitch, h->yaw);
    }
}

qreal OSGCamera::yaw() const
{
    return h->yaw;
}

void OSGCamera::setYaw(qreal arg)
{
    if (h->yaw!= arg) {
        h->yaw = arg;
        emit yawChanged(yaw());
        emit attitudeChanged(h->roll, h->pitch, h->yaw);
    }
}

double OSGCamera::latitude() const
{
    return h->latitude;
}

void OSGCamera::setLatitude(double arg)
{
    if (h->latitude != arg) {
        h->latitude = arg;
        emit latitudeChanged(latitude());
        emit positionChanged(h->latitude, h->longitude, h->altitude);
    }
}

double OSGCamera::longitude() const
{
    return h->longitude;
}

void OSGCamera::setLongitude(double arg)
{
    if (h->longitude != arg) {
        h->longitude = arg;
        emit longitudeChanged(longitude());
        emit positionChanged(h->latitude, h->longitude, h->altitude);
    }
}

double OSGCamera::altitude() const
{
    return h->altitude;
}

void OSGCamera::setAltitude(double arg)
{
    if (h->altitude!= arg) {
        h->altitude = arg;
        emit altitudeChanged(altitude());
        emit positionChanged(h->latitude, h->longitude, h->altitude);
    }
}

//osg::Node *OSGCamera::node()
//{
//    return h->node.get();
//}

//void OSGCamera::setNode(osg::Node *node)
//{
//    if (h->node.get() != node) {
//        h->node = node;
//        emit nodeChanged(node);
//    }
//}

void OSGCamera::installCamera(osgViewer::View *view)
{
    qDebug() << "OSGCamera - installCamera" << view;
    switch(h->manipulatorMode) {
    case OSGCamera::None:
        break;
    case OSGCamera::User:
        // disable any installed camera manipulator
        view->setCameraManipulator(NULL);
        break;
    case OSGCamera::Earth:
        view->setCameraManipulator(new osgEarth::Util::EarthManipulator());
        break;
    case OSGCamera::Track:
        if (h->trackNode && h->trackNode->node()) {
            // setup tracking camera
            // TODO when camera is thrown, then changing attitude has jitter (could be due to different frequency between refresh and animation)
            osgGA::NodeTrackerManipulator *ntm = new osgGA::NodeTrackerManipulator(
                    osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS);
            ntm->setTrackNode(h->trackNode->node());
            switch(h->trackerMode) {
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
            //ntm->setRotationMode(h->trackRotationMode)
            //ntm->setMinimumDistance(2, false);
            //ntm->setAutoComputeHomePosition(true);
            //ntm->setDistance(10);
            view->setCameraManipulator(ntm);
        }
        else {
            qWarning() << "no trackNode provided, cannot enter tracking mode!";
        }
        break;
    default:
        break;
    }
}

void OSGCamera::setViewport(osg::Camera *camera, int x, int y, int width, int height)
{
    qDebug() << "OSGCamera - setViewport" << camera;
    if (width <= 0 || height <= 0) {
        qWarning() << "OSGCamera - setViewport - invalid size " << width << "x" << height;
        return;
    }
    camera->setViewport(x, y, width, height);
    camera->setProjectionMatrixAsPerspective(
            h->fieldOfView, static_cast<double>(width)/static_cast<double>(height), 1.0f, 10000.0f );
}

// From wikipedia : Latitude and longitude values can be based on different geodetic systems or datums, the most common being WGS 84, a global datum used by all GPS equipments
// http://en.wikipedia.org/wiki/Geographic_coordinate_system
// Revo uses WGS84 for homelocation, Gps altitude + Geoid separation
// here my altitude is 120m ASL, homelocation stored in Revo is 170m, my Geoid is 50m
// On map if you set Home, message tell user is WGS84
// user enter 120m and is wrong :)

// See http://webhelp.esri.com/arcpad/8.0/referenceguide/index.htm#gps_rangefinder/concept_gpsheight.htm
void OSGCamera::updateCamera(osg::Camera *camera)
{
    if (h->manipulatorMode != User) {
        return;
    }

    // Altitude mode is absolute (absolute height above MSL/HAE)
    // HAE : Height above ellipsoid (HAE). This is the default.
    // MSL : Height above Mean Sea Level (MSL) if a geoid separation value is specified.
    // TODO handle the case where the terrain SRS is not "wgs84"
    // TODO check if position is not below terrain?
    // TODO compensate antenna height when source of position is GPS (i.e. subtract antenna height from altitude) ;)

    // Camera position
    osg::Matrix cameraPosition;

    osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
            longitude(), latitude(), altitude(), osgEarth::ALTMODE_ABSOLUTE);
    geoPoint.createLocalToWorld(cameraPosition);

     // Camera orientation
    // By default the camera looks toward -Z, we must rotate it so it looks toward Y
    osg::Matrix cameraRotation;
    cameraRotation.makeRotate(osg::DegreesToRadians(90.0), osg::Vec3(1.0, 0.0, 0.0),
                              osg::DegreesToRadians(0.0),  osg::Vec3(0.0, 1.0, 0.0),
                              osg::DegreesToRadians(0.0),  osg::Vec3(0.0, 0.0, 1.0));

    // Final camera matrix
    osg::Matrix cameraMatrix = cameraRotation
                             * osg::Matrix::rotate(osg::DegreesToRadians(roll()),   osg::Vec3(0.0, 1.0, 0.0))
                             * osg::Matrix::rotate(osg::DegreesToRadians(pitch()), osg::Vec3(1.0, 0.0, 0.0))
                             * osg::Matrix::rotate(osg::DegreesToRadians(yaw()),   osg::Vec3(0.0, 0.0, -1.0))
                             * cameraPosition;

    // Inverse the camera's position and orientation matrix to obtain the view matrix
    cameraMatrix = osg::Matrix::inverse(cameraMatrix);
    camera->setViewMatrix(cameraMatrix);
}

//    osg::Camera *createCamera(int x, int y, int w, int h, const std::string & name = "", bool windowDecoration = false)
//    {
//        osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();
//        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
//
//        traits->windowName = name;
//        traits->windowDecoration = windowDecoration;
//        traits->x       = x;
//        traits->y       = y;
//        traits->width   = w;
//        traits->height  = h;
//        traits->doubleBuffer = true;
//        traits->alpha   = ds->getMinimumNumAlphaBits();
//        traits->stencil = ds->getMinimumNumStencilBits();
//        traits->sampleBuffers = ds->getMultiSamples();
//        traits->samples = ds->getNumMultiSamples();
//
//        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
//        camera->setGraphicsContext(new osgQt::GraphicsWindowQt(traits.get()));
//
//        camera->setClearColor(osg::Vec4(0.2, 0.2, 0.6, 1.0));
//        camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
//        camera->setProjectionMatrixAsPerspective(
//            30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f);
//        return camera.release();
//    }

} // namespace osgQtQuick

#include "OSGCamera.moc"
