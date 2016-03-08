/**
 ******************************************************************************
 *
 * @file       OSGCamera.hpp
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

#ifndef _H_OSGQTQUICK_OSGCAMERA_H_
#define _H_OSGQTQUICK_OSGCAMERA_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QObject>
#include <QVector3D>

namespace osgViewer {
class View;
}

namespace osgQtQuick {
class ManipulatorMode : public QObject {
    Q_OBJECT
public:
    enum Enum { Default, Earth, Track, User };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

class TrackerMode : public QObject {
    Q_OBJECT
public:
    enum Enum { NodeCenter, NodeCenterAndAzim, NodeCenterAndRotation };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

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
class OSGQTQUICK_EXPORT OSGCamera : public OSGNode {
    Q_OBJECT Q_PROPERTY(qreal fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)
    Q_PROPERTY(osgQtQuick::OSGNode * sceneNode READ sceneNode WRITE setSceneNode NOTIFY sceneNodeChanged)
    Q_PROPERTY(osgQtQuick::ManipulatorMode::Enum manipulatorMode READ manipulatorMode WRITE setManipulatorMode NOTIFY manipulatorModeChanged)
    Q_PROPERTY(osgQtQuick::OSGNode * trackNode READ trackNode WRITE setTrackNode NOTIFY trackNodeChanged)
    Q_PROPERTY(osgQtQuick::TrackerMode::Enum trackerMode READ trackerMode WRITE setTrackerMode NOTIFY trackerModeChanged)
    Q_PROPERTY(bool clampToTerrain READ clampToTerrain WRITE setClampToTerrain NOTIFY clampToTerrainChanged)
    Q_PROPERTY(bool intoTerrain READ intoTerrain NOTIFY intoTerrainChanged)
    Q_PROPERTY(QVector3D attitude READ attitude WRITE setAttitude NOTIFY attitudeChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool logarithmicDepthBuffer READ logarithmicDepthBuffer WRITE setLogarithmicDepthBuffer NOTIFY logarithmicDepthBufferChanged)

    enum DirtyFlag { Position, Attitude, FieldOfView, All };


public:
    explicit OSGCamera(QObject *parent = 0);
    virtual ~OSGCamera();

    // fov depends on the scenery space (probaby distance)
    // here are some value: 75°, 60°, 45° many gamers use
    // x-plane uses 45° for 4:3 and 60° for 16:9/16:10
    // flightgear uses 55° / 70°
    qreal fieldOfView() const;
    void setFieldOfView(qreal arg);

    OSGNode *sceneNode();
    void setSceneNode(OSGNode *node);

    ManipulatorMode::Enum manipulatorMode() const;
    void setManipulatorMode(ManipulatorMode::Enum);

    OSGNode *trackNode() const;
    void setTrackNode(OSGNode *node);

    TrackerMode::Enum trackerMode() const;
    void setTrackerMode(TrackerMode::Enum);

    bool clampToTerrain() const;
    void setClampToTerrain(bool arg);

    bool intoTerrain() const;

    QVector3D attitude() const;
    void setAttitude(QVector3D arg);

    QVector3D position() const;
    void setPosition(QVector3D arg);

    bool logarithmicDepthBuffer();
    void setLogarithmicDepthBuffer(bool enabled);

    virtual void attach(osgViewer::View *view);
    virtual void detach(osgViewer::View *view);

signals:
    void fieldOfViewChanged(qreal arg);

    void sceneNodeChanged(OSGNode *node);

    void manipulatorModeChanged(ManipulatorMode::Enum);

    void trackNodeChanged(OSGNode *node);
    void trackerModeChanged(TrackerMode::Enum);

    void clampToTerrainChanged(bool arg);
    void intoTerrainChanged(bool arg);

    void attitudeChanged(QVector3D arg);
    void positionChanged(QVector3D arg);

    void logarithmicDepthBufferChanged(bool enabled);

private:
    struct Hidden;
    Hidden *h;

    void update();
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGCAMERA_H_
