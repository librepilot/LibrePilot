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
#include <osg/DisplaySettings>

#include <map>

#include <QThread>
#include <QApplication>

namespace osgQtQuick {

struct QuickWindowViewer::Hidden: public QObject
{
    Q_OBJECT

public:
    Hidden(QuickWindowViewer *viewer, QQuickWindow *window) :
        QObject(viewer),
        window(0),
        viewer(0),
        frameTimer(-1)
    {
        acceptViewer(viewer);
        acceptWindow(window);
        initCompositeViewer();
    }

    ~Hidden() {
    }

    QQuickWindow *window;
    QuickWindowViewer *viewer;

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

void sync() {
    if (!compositeViewer.valid()) {
        qCritical() << "QuickWindowViewer - invalid composite viewer!";
        return;
    }

    // Qt bug!?
    //QOpenGLContext::currentContext()->functions()->glUseProgram(0);

}

void frame() {
    if (!compositeViewer.valid()) {
        qCritical() << "QuickWindowViewer - invalid composite viewer!";
        return;
    }

    // Qt bug!?
    //QOpenGLContext::currentContext()->functions()->glUseProgram(0);

    // display thread name to compare with Qt ui thread

    //qDebug() << "QuickWindowViewer - frame" << QThread::currentThread() << QApplication::instance()->thread();

    // refresh all the views.
    if (compositeViewer->getRunFrameScheme() == osgViewer::ViewerBase::CONTINUOUS ||
            compositeViewer->checkNeedToDoFrame()) {
        //qDebug() << "QuickWindowViewer - frame - view count " << compositeViewer->getNumViews();
        compositeViewer->frame();

        // just in case...
        window->resetOpenGLState();
    }
    else {
        qDebug() << "QuickWindowViewer - skipped frame";
    }
    //qDebug() << "QuickWindowViewer - frame done";
}

private:
    void acceptViewer(QuickWindowViewer *viewer) {
        qDebug() << "QuickWindowViewer - accept viewer" << viewer;
        if (this->viewer == viewer) {
            return;
        }
        this->viewer = viewer;
        if (parent() != viewer) {
            setParent(viewer);
        }
    }

    void acceptWindow(QQuickWindow *window) {
        qDebug() << "QuickWindowViewer - accept window" << window;
        if (this->window == window) {
            return;
        }

        if (this->window) {
            disconnect(this->window);
        }

        this->window = window;

        if (viewer->parent() != window) {
            viewer->setParent(window);
        }

        connect(window, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
        connect(window, SIGNAL(beforeRendering()), this, SLOT(frame()), Qt::DirectConnection);

        if (frameTimer >= 0) {
            killTimer(frameTimer);
        }
        if (window) {
            window->setClearBeforeRendering(false);
            frameTimer = startTimer(20);
        }
    }

    void initCompositeViewer() {
        compositeViewer = new osgViewer::CompositeViewer();
        compositeViewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
        // disable the default setting of viewer.done() by pressing Escape.
        compositeViewer->setKeyEventSetsDone(0);
        //compositeViewer->setQuitEventSetsDone(false);

        osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(ds);

        traits->readDISPLAY();
        if (traits->displayNum < 0) {
            traits->displayNum = 0;
        }

        traits->windowDecoration = false;
        traits->x = 0;
        traits->y = 0;
        traits->width = window->width();
        traits->height = window->height();
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        //traits->sharedContext = gc;
        //traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

        graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits);
    }
};

QuickWindowViewer::Hidden::ViewersMap QuickWindowViewer::Hidden::viewers;

QuickWindowViewer::QuickWindowViewer(QQuickWindow *window) : QObject(window), h(new Hidden(this, window))
{
    qDebug() << "QuickWindowViewer - <init>";
}

QuickWindowViewer::~QuickWindowViewer()
{
    qDebug() << "QuickWindowViewer - <destruct>";
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
        // TODO if window is destroyed, the associated QuickWindowViewer should be destroyed too
        viewer = new QuickWindowViewer(window);
        Hidden::viewers.insert(std::make_pair(window, viewer));
    }
    return viewer;
}

} // namespace osgQtQuick

#include "QuickWindowViewer.moc"

/*

void ViewerWidget::reconfigure( osgViewer::View* view )
{
    if ( !_gc.valid() )
    {
        // create the Qt graphics context if necessary; it will be shared across all views.
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(ds);

        traits->readDISPLAY();
        if (traits->displayNum<0) traits->displayNum = 0;

        traits->windowName = "osgEarthViewerQt";
        traits->windowDecoration = false;
        traits->x = x();
        traits->y = y();
        traits->width = width();
        traits->height = height();
        traits->doubleBuffer = true;
        traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

        _gc = new osgQt::GraphicsWindowQt( traits.get() );
    }

    // reconfigure this view's camera to use the Qt GC if necessary.
    osg::Camera* camera = view->getCamera();
    if ( camera->getGraphicsContext() != _gc.get() )
    {
        camera->setGraphicsContext( _gc.get() );
        if ( !camera->getViewport() )
        {
            camera->setViewport(new osg::Viewport(0, 0, _gc->getTraits()->width, _gc->getTraits()->height));
        }
        camera->setProjectionMatrixAsPerspective(
            30.0f, camera->getViewport()->width()/camera->getViewport()->height(), 1.0f, 10000.0f );
    }
}


osg::GraphicsContext*
ViewWidget::createOrShareGC(osg::GraphicsContext* gc)
{
    if ( !gc )
        gc->createNewContextID();

    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(ds);

    traits->readDISPLAY();
    if (traits->displayNum<0) traits->displayNum = 0;

    traits->windowDecoration = false;
    traits->x = 0; //x();
    traits->y = 0; //y();
    traits->width = 100; // width();
    traits->height = 100; //height();
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();
    traits->sharedContext = gc;
    traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

    return new osgQt::GraphicsWindowQt(traits.get());
}
*/
