#include "OSGCamera.hpp"

#include "OSGNode.hpp"

#include <osg/Node>

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
        emit fieldOfViewChanged(arg);

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
        //updateFrame();
        emit rollChanged(arg);
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
        //updateFrame();
        emit pitchChanged(arg);
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
        //updateFrame();
        emit yawChanged(arg);
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
        emit latitudeChanged(arg);
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
        emit longitudeChanged(arg);
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
        emit altitudeChanged(arg);
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

} // namespace osgQtQuick
