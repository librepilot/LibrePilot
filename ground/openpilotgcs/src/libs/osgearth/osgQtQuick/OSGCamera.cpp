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

#include <QDebug>

namespace osgQtQuick {

struct OSGCamera::Hidden : public QObject
{
    Hidden(OSGCamera *parent) : QObject(parent), trackNode(NULL) {
        fieldOfView = 90.0;
        roll = pitch = yaw = 0.0;
        latitude = longitude = altitude = 0.0;
    }

    ~Hidden()
    {
    }

    bool acceptTrackNode(OSGNode *node)
    {
        qDebug() << "OSGCamera - acceptTrackNode" << node;
        if (trackNode == node) {
            return false;
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

    OSGNode *trackNode;

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
}

OSGCamera::~OSGCamera()
{
    delete h;
}

qreal OSGCamera::fieldOfView() const
{
    return h->fieldOfView;
}

// ! Camera vertical field of view in degrees
void OSGCamera::setFieldOfView(qreal arg)
{
    if (!qFuzzyCompare(h->fieldOfView, arg)) {
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

OSGNode *OSGCamera::trackNode()
{
    return h->trackNode;
}

void OSGCamera::setTrackNode(OSGNode *node)
{
    if (h->acceptTrackNode(node)) {
        emit trackNodeChanged(node);
    }
}

qreal OSGCamera::roll() const
{
    return h->roll;
}

void OSGCamera::setRoll(qreal arg)
{
    qDebug() << "ROLLING" << arg;
    if (!qFuzzyCompare(h->roll, arg)) {
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
    if (!qFuzzyCompare(h->pitch, arg)) {
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
    if (!qFuzzyCompare(h->yaw, arg)) {
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
    // not sure qFuzzyCompare is accurate enough for geo coordinates
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
    if (!qFuzzyCompare(h->altitude, arg)) {
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
    if (h->trackNode) {
        // setup tracking camera
        osgGA::NodeTrackerManipulator *ntm = new osgGA::NodeTrackerManipulator();
        if (h->trackNode->node()) {
            ntm->setTrackNode(h->trackNode->node());
            //        //ntm->setAutoComputeHomePosition(true);
            //        //ntm->computeHomePosition();
            ntm->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER);
        }
        view->setCameraManipulator(ntm);
    }
    else {
        // camera is positioned using location and attitude
        // disable any installed camera manipulator
        view->setCameraManipulator(NULL);
    }
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
    //qDebug() << "updating camera";
    // Camera position
    // Altitude mode is absolute (absolute height above MSL/HAE)
    // HAE : Height above ellipsoid (HAE). This is the default.
    // MSL : Height above Mean Sea Level (MSL) if a geoid separation value is specified.
    // TODO handle the case where the terrain SRS is not "wgs84"
    // TODO check if position is not below terrain?
    // TODO compensate antenna height when source of position is GPS (i.e. subtract antenna height from altitude) ;)
    osg::Matrix cameraPosition;
    //qDebug() << "updating camera" << longitude() << latitude();
    osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
            longitude(), //osg::RadiansToDegrees(camera->longitude()),
            latitude(), //osg::RadiansToDegrees(camera->latitude()),
                                1000,
                                osgEarth::ALTMODE_ABSOLUTE);
    geoPoint.createLocalToWorld(cameraPosition);

    //model->setPosition(geoPoint);


//        osg::Quat q = osg::Quat(
//                osg::DegreesToRadians(camera->roll()), osg::Vec3d(1, 0, 0),
//                osg::DegreesToRadians(camera->pitch()), osg::Vec3d(0, 1, 0), osg::DegreesToRadians(camera->yaw()),
//                osg::Vec3d(0, 0, 1));

    //model->setLocalRotation(q);

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

