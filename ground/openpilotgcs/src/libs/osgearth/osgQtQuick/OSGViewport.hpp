#ifndef _H_OSGQTQUICK_OSGVIEPORT_H_
#define _H_OSGQTQUICK_OSGVIEPORT_H_

#include "Export.hpp"

#include <QQuickItem>

namespace osgQtQuick {

class OSGNode;
class OSGCamera;

class OSGQTQUICK_EXPORT OSGViewport : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(osgQtQuick::OSGNode* sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)
    Q_PROPERTY(osgQtQuick::OSGCamera* camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(DrawingMode drawingMode READ drawingMode WRITE setDrawingMode NOTIFY drawingModeChanged)

    Q_ENUMS(DrawingMode)

public:
    enum DrawingMode {
        Native,
        Buffer
    };

    explicit OSGViewport(QQuickItem *parent = 0);
    virtual ~OSGViewport();

    DrawingMode drawingMode() const;
    void setDrawingMode(DrawingMode mode);

    OSGNode* sceneData();
    void setSceneData(OSGNode *node);

    OSGCamera* camera();
    void setCamera(OSGCamera *camera);

    QColor color() const;
    void setColor(const QColor &color);

signals:
    void drawingModeChanged(DrawingMode mode);
    void sceneDataChanged(OSGNode *node);
    void cameraChanged(OSGCamera *camera);
    void colorChanged(const QColor &color);

public slots:

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);

private:
    struct Hidden;
    friend struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGVIEPORT_H_
