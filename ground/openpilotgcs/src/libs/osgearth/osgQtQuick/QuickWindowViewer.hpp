#ifndef OSGQTQUICK_QUICKWINDOWVIEWER_HPP
#define OSGQTQUICK_QUICKWINDOWVIEWER_HPP

#include <osgQtQuick/Export>

#include <QObject>

QT_BEGIN_NAMESPACE
class QQuickWindow;
QT_END_NAMESPACE

namespace osg {
class GraphicsContext;
} // namespace osg

namespace osgViewer {
class CompositeViewer;
}

namespace osgQtQuick {

class OSGQTQUICK_EXPORT QuickWindowViewer : public QObject
{
    Q_OBJECT

public:
    QuickWindowViewer(QQuickWindow *window);
    ~QuickWindowViewer();

    osg::GraphicsContext* graphicsContext();
    osgViewer::CompositeViewer* compositeViewer();

    static QuickWindowViewer *instance(QQuickWindow *window);

private:
    struct Hidden;
    friend struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // OSGQTQUICK_QUICKWINDOWVIEWER_HPP
