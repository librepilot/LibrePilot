/**
 ******************************************************************************
 *
 * @file       OSGViewport.hpp
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

#ifndef _H_OSGQTQUICK_OSGVIEPORT_H_
#define _H_OSGQTQUICK_OSGVIEPORT_H_

#include "Export.hpp"

#include "ga/OSGCameraManipulator.hpp"

#include <QQuickFramebufferObject>

namespace osgViewer {
class View;
}

namespace osgQtQuick {
class Renderer;
class OSGNode;
class OSGCamera;

class UpdateMode : public QObject {
    Q_OBJECT
public:
    enum Enum { Continuous, Discrete, OnDemand };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

class OSGQTQUICK_EXPORT OSGViewport : public QQuickFramebufferObject {
    Q_OBJECT Q_PROPERTY(osgQtQuick::OSGNode *sceneNode READ sceneNode WRITE setSceneNode NOTIFY sceneNodeChanged)
    Q_PROPERTY(osgQtQuick::OSGCamera * camera READ cameraNode WRITE setCameraNode NOTIFY cameraNodeChanged)
    Q_PROPERTY(osgQtQuick::OSGCameraManipulator * manipulator READ manipulator WRITE setManipulator NOTIFY manipulatorChanged)
    Q_PROPERTY(osgQtQuick::UpdateMode::Enum updateMode READ updateMode WRITE setUpdateMode NOTIFY updateModeChanged)
    Q_PROPERTY(bool incrementalCompile READ incrementalCompile WRITE setIncrementalCompile NOTIFY incrementalCompileChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

    typedef QQuickFramebufferObject Inherited;

    friend class ViewportRenderer;

public:
    explicit OSGViewport(QQuickItem *parent = 0);
    virtual ~OSGViewport();

    OSGNode *sceneNode() const;
    void setSceneNode(OSGNode *node);

    OSGCamera *cameraNode() const;
    void setCameraNode(OSGCamera *node);

    OSGCameraManipulator *manipulator() const;
    void setManipulator(OSGCameraManipulator *manipulator);

    UpdateMode::Enum updateMode() const;
    void setUpdateMode(UpdateMode::Enum mode);

    bool incrementalCompile() const;
    void setIncrementalCompile(bool busy);

    bool busy() const;
    void setBusy(bool busy);

    osgViewer::View *asView() const;

    Renderer *createRenderer() const;

signals:
    void sceneNodeChanged(OSGNode *);
    void cameraNodeChanged(OSGCamera *);
    void manipulatorChanged(OSGCameraManipulator *);
    void updateModeChanged(UpdateMode::Enum);
    void incrementalCompileChanged(bool);
    void busyChanged(bool busy);

protected:
    // QQmlParserStatus
    void classBegin();
    void componentComplete();

    // QQuickItem
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    // QQuickFramebufferObject
    QSGNode *updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData) override;
#endif

private:
    struct Hidden;
    Hidden *const h;

    void setKeyboardModifiers(QInputEvent *event);
    QPointF mousePoint(QMouseEvent *event);
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGVIEPORT_H_
