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

class OSGQTQUICK_EXPORT OSGViewport : public QQuickFramebufferObject {
    Q_OBJECT Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(UpdateMode updateMode READ updateMode WRITE setUpdateMode NOTIFY updateModeChanged)
    Q_PROPERTY(osgQtQuick::OSGNode * sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)
    Q_PROPERTY(osgQtQuick::OSGCamera * camera READ camera WRITE setCamera NOTIFY cameraChanged)

    Q_ENUMS(UpdateMode)

public:

    friend class ViewportRenderer;

    // TODO rename to UpdateMode or something better
    enum UpdateMode {
        Continuous,
        Discrete,
        OnDemand
    };

    explicit OSGViewport(QQuickItem *parent = 0);
    virtual ~OSGViewport();

    UpdateMode updateMode() const;
    void setUpdateMode(UpdateMode mode);

    QColor color() const;
    void setColor(const QColor &color);

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    OSGCamera *camera();
    void setCamera(OSGCamera *camera);

    virtual Renderer *createRenderer() const;
    virtual void releaseResources();

    virtual bool attach(osgViewer::View *view);
    virtual bool detach(osgViewer::View *view);

signals:
    void updateModeChanged(UpdateMode mode);
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
