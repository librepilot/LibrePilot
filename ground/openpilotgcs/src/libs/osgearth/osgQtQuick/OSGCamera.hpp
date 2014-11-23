#ifndef _H_OSGQTQUICK_OSGCAMERA_H_
#define _H_OSGQTQUICK_OSGCAMERA_H_

#include "Export.hpp"

#include <QObject>

namespace osg {
class Camera;
} // namespace osg

namespace osgViewer {
class View;
} // namespace osgViewer

namespace osgQtQuick {

class OSGNode;

// This class does too much:
// - tracking a geo point and attitude
// - tracking another node
// camera should be simpler and provide only tracking
// - tracking of a modelnode (for ModelView)
// - tracking of a virtual node  (for PFD with Terrain)
//
// TODO
// - expose track mode
// - provide good default distance and attitude for tracker camera
class OSGQTQUICK_EXPORT OSGCamera : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)

    Q_PROPERTY(ManipulatorMode manipulatorMode READ manipulatorMode WRITE setManipulatorMode NOTIFY manipulatorModeChanged)

    Q_PROPERTY(osgQtQuick::OSGNode* trackNode READ trackNode WRITE setTrackNode NOTIFY trackNodeChanged)
    Q_PROPERTY(TrackerMode trackerMode READ trackerMode WRITE setTrackerMode NOTIFY trackerModeChanged)

    Q_PROPERTY(qreal roll READ roll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(qreal yaw READ yaw WRITE setYaw NOTIFY yawChanged)

    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double altitude READ altitude WRITE setAltitude NOTIFY altitudeChanged)

    Q_ENUMS(ManipulatorMode)
    Q_ENUMS(TrackerMode)

public:
    enum ManipulatorMode { None, User, Earth, Track };

    enum TrackerMode { NodeCenter, NodeCenterAndAzim, NodeCenterAndRotation };

    explicit OSGCamera(QObject *parent = 0);
    ~OSGCamera();
    
    qreal fieldOfView() const;
    void setFieldOfView(qreal arg);

    ManipulatorMode manipulatorMode() const;
    void setManipulatorMode(ManipulatorMode);

    OSGNode* trackNode() const;
    void setTrackNode(OSGNode *node);

    TrackerMode trackerMode() const;
    void setTrackerMode(TrackerMode);

    qreal roll() const;
    void setRoll(qreal arg);

    qreal pitch() const;
    void setPitch(qreal arg);

    qreal yaw() const;
    void setYaw(qreal arg);

    double latitude() const;
    void setLatitude(double arg);

    double longitude() const;
    void setLongitude(double arg);

    double altitude() const;
    void setAltitude(double arg);

    void installCamera(osgViewer::View *view);
    void setViewport(osg::Camera *camera, int x, int y, int width, int height);
    void updateCamera(osg::Camera *camera);

signals:
    void fieldOfViewChanged(qreal arg);

    void manipulatorModeChanged(ManipulatorMode);

    void trackNodeChanged(OSGNode *node);
    void trackerModeChanged(TrackerMode);

    void rollChanged(qreal arg);
    void pitchChanged(qreal arg);
    void yawChanged(qreal arg);

    void latitudeChanged(double arg);
    void longitudeChanged(double arg);
    void altitudeChanged(double arg);

    void attitudeChanged(qreal roll, qreal pitch, qreal yaw);
    void positionChanged(double latitude, double longitude, double altitude);

//    osg::Node* node();
//    void setNode(osg::Node *node);
//    void nodeChanged(osg::Node *node);
    
private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGCAMERA_H_
