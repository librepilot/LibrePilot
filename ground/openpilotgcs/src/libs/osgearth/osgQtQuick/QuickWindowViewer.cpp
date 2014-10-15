#include "QuickWindowViewer.hpp"

#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QQuickItem>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <osgViewer/CompositeViewer>
#include <osg/State>
#include <osg/StateSet>

#include <map>

namespace osgQtQuick {

struct QuickWindowViewer::Hidden: public QObject
{
    Q_OBJECT

public:
    Hidden(QuickWindowViewer *viewer, QQuickWindow *window) :
        QObject(viewer),
        viewer(0),
        window(0),
        frameTimer(-1)
    {
        acceptViewer(viewer);
        acceptWindow(window);
        initCompositeViewer();
    }

    ~Hidden() {
    }

    QuickWindowViewer *viewer;
    QQuickWindow *window;
    osg::ref_ptr<osgViewer::CompositeViewer> compositeViewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow;

    int frameTimer;

    typedef std::map<QQuickWindow*, QuickWindowViewer*> ViewersMap;
    static ViewersMap viewers;

protected:
    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == frameTimer) {
            if (window) window->update();
        }

        QObject::timerEvent(event);
    }


private slots:
    void frame() {
        if (!compositeViewer.valid()) return;

        // Qt bug!?
        QOpenGLContext::currentContext()->functions()->glUseProgram(0);

        compositeViewer->frame();
    }

private:
    void acceptViewer(QuickWindowViewer *viewer) {
        if (this->viewer == viewer) return;
        this->viewer = viewer;
        if (parent() != viewer) setParent(viewer);
    }

    void acceptWindow(QQuickWindow *window) {
        if (this->window == window) return;
        if (this->window) disconnect(this->window);
        this->window = window;
        if (viewer->parent() != window) viewer->setParent(window);
        connect(window, SIGNAL(beforeRendering()),
                this, SLOT(frame()));
        // Запускаем постоянное обновление
        if (frameTimer >= 0) killTimer(frameTimer);
        if (window) {
            window->setClearBeforeRendering(false);
            frameTimer = startTimer(10);
        }
    }

    void initCompositeViewer() {
        compositeViewer = new osgViewer::CompositeViewer();
        compositeViewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
        // disable the default setting of viewer.done() by pressing Escape.
        compositeViewer->setKeyEventSetsDone(0);
        graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0,
                                                               window->width(),
                                                               window->height());
    }
};

QuickWindowViewer::Hidden::ViewersMap QuickWindowViewer::Hidden::viewers;

QuickWindowViewer::QuickWindowViewer(QQuickWindow *window) :
    QObject(window),
    h(new Hidden(this, window))
{
}

QuickWindowViewer::~QuickWindowViewer()
{
    Hidden::viewers.erase(h->window);
}

osg::GraphicsContext *QuickWindowViewer::graphicsContext()
{
    return h->graphicsWindow.get();
}

osgViewer::CompositeViewer *QuickWindowViewer::compositeViewer()
{
    return h->compositeViewer.get();
}

QuickWindowViewer *QuickWindowViewer::instance(QQuickWindow *window)
{
    if (!window) return 0;
    QuickWindowViewer *viewer = 0;
    Hidden::ViewersMap::iterator it = Hidden::viewers.find(window);
    if (it != Hidden::viewers.end()) {
        viewer = it->second;
    } else {
        viewer = new QuickWindowViewer(window);
        Hidden::viewers.insert(std::make_pair(window, viewer));
    }
    return viewer;
}

} // namespace osgQtQuick

#include "QuickWindowViewer.moc"
