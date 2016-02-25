/**
 ******************************************************************************
 *
 * @file       OSGViewport.hpp
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

#ifndef _H_OSGQTQUICK_OSGVIEPORT_H_
#define _H_OSGQTQUICK_OSGVIEPORT_H_

#include "Export.hpp"

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
    Q_OBJECT Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(osgQtQuick::UpdateMode::Enum updateMode READ updateMode WRITE setUpdateMode NOTIFY updateModeChanged)
    Q_PROPERTY(osgQtQuick::OSGNode * sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)
    Q_PROPERTY(osgQtQuick::OSGCamera * camera READ camera WRITE setCamera NOTIFY cameraChanged)

public:
    friend class ViewportRenderer;

    explicit OSGViewport(QQuickItem *parent = 0);
    virtual ~OSGViewport();

    UpdateMode::Enum updateMode() const;
    void setUpdateMode(UpdateMode::Enum mode);

    QColor color() const;
    void setColor(const QColor &color);

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    OSGCamera *camera();
    void setCamera(OSGCamera *camera);

    virtual Renderer *createRenderer() const;
    virtual void releaseResources();

    virtual void attach(osgViewer::View *view);
    virtual void detach(osgViewer::View *view);

signals:
    void updateModeChanged(UpdateMode::Enum mode);
    void colorChanged(const QColor &color);
    void sceneDataChanged(OSGNode *node);
    void cameraChanged(OSGCamera *camera);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    void setKeyboardModifiers(QInputEvent *event);
    QPointF mousePoint(QMouseEvent *event);

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGVIEPORT_H_
