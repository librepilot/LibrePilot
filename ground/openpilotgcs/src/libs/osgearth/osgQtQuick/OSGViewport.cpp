#include "OSGViewport.hpp"

#include "OSGNode.hpp"
#include "OSGCamera.hpp"
#include "Utility.hpp"

#include <osg/Node>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/Optimizer>

#include <osgEarth/MapNode>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>

#include <QDebug>

#include <QThread>
#include <QApplication>

namespace osgQtQuick {

/*
 * TODO : add OSGView to handle multiple views for a given OSGViewport
 *
 */
struct OSGViewport::Hidden : public QObject
{
    Q_OBJECT

public:

    class ViewportRenderer : public QQuickFramebufferObject::Renderer
    {
    public:
        ViewportRenderer(Hidden *h) : h(h)
        {
            qDebug() << "ViewportRenderer - <init>";
            b = false;
        }

        void synchronize(QQuickFramebufferObject * item) {
            // synchronize method is the only method allowed to access the Quick item (GUI thread is blocked)
            if (!h->realized) {
                qDebug() << "ViewportRenderer - synchronize" << item->window();
                qDebug() << "ViewportRenderer - synchronize" << QOpenGLContext::currentContext();
                item->window()->setClearBeforeRendering(false);
                h->initCompositeViewer();
                h->quickItem->realize();
                h->camera->installCamera(h->view.get());
                h->realized = true;
            }
            h->camera->setViewport(h->view->getCamera(), 0, 0, item->width(), item->height());
        }

        QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) {
            qDebug() << "ViewportRenderer - createFramebufferObject" << size << QOpenGLContext::currentContext();
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            //format.setSamples(4);
            QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(size.width(), size.height(), format);
            qDebug() << "ViewportRenderer - createFramebufferObject - done" << fbo;
            return fbo;
        }

        void render() {
            //qDebug() << "ViewportRenderer - render" << QThread::currentThread() << QApplication::instance()->thread();
            // render method is not allowed to access the Quick item (can be called on a render thread)
            if (!b) {
                b = true;
                qDebug() << "ViewportRenderer - render" << QOpenGLContext::currentContext();
            }

            h->compositeViewer->frame();
            //h->quickItem->window()->resetOpenGLState();

            if (h->updateMode == OSGViewport::Continuous) {
                // trigger next update
                update();
            }
        }

    private:
        Hidden *h;
        bool b;
    };

    friend class ViewportRenderer;

    Hidden(OSGViewport *quickItem) : QObject(quickItem),
            quickItem(quickItem),
            updateMode(Discrete),
            frameTimer(-1),
            sceneData(0),
            camera(0),
            realized(false)
    {
        initOSG();
        acceptQuickItem();
        acceptUpdateMode(updateMode);
    }

    ~Hidden() {
        if (frameTimer >= 0) {
            killTimer(frameTimer);
        }
        quickItem = NULL;
    }

    QPointF mousePoint(QMouseEvent *event) {
        //        qreal x = 0.01 * (event->x() - quickItem->width() / 2);
        //        qreal y = 0.01 * (event->y() - quickItem->height() / 2);
        qreal x = 2.0 * (event->x() - quickItem->width() / 2) / quickItem->width();
        qreal y = 2.0 * (event->y() - quickItem->height() / 2) / quickItem->height();
        return QPointF(x, y);
    }

    void setKeyboardModifiers(QInputEvent* event)
    {
        int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
        unsigned int mask = 0;
        if ( modkey & Qt::ShiftModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
        if ( modkey & Qt::ControlModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
        if ( modkey & Qt::AltModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
        view->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGViewport - acceptSceneData" << node;
        if (sceneData == node) {
            return true;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            acceptNode(sceneData->node());
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGViewport - acceptNode" << node;
        if (!node) {
            qWarning() << "OSGViewport - acceptNode - node is null";
            view->setSceneData(NULL);
            return true;
        }

        // expose option to turn optimzer on/off
        //osgUtil::Optimizer optimizer;
        //optimizer.optimize(node);

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (mapNode) {
            qDebug() << "OSGViewport - acceptNode - found map node" << mapNode;
            // TODO should not be done here
            // TODO will the AutoClipPlaneCullCallback be destroyed???
            view->getCamera()->setCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
        }

        osgEarth::Util::SkyNode *skyNode = osgQtQuick::findTopMostNodeOfType<osgEarth::Util::SkyNode>(node);
        if (skyNode) {
            qDebug() << "OSGViewport - acceptNode - found sky node" << skyNode;
            // TODO should not be done here
            skyNode->attach(view.get(), 0);
        }

        view->setSceneData(node);

        return true;
    }

    bool acceptUpdateMode(OSGViewport::UpdateMode mode) {
        qDebug() << "OSGViewport - acceptUpdateMode" << mode;
        if (updateMode == mode) {
            return true;
        }

        updateMode = mode;

        return true;
    }

    bool acceptCamera(OSGCamera *camera) {
        qDebug() << "OSGViewport - acceptCamera" << camera;
        if (this->camera == camera) {
            return true;
        }

        this->camera = camera;

        if (this->camera) {
            //this->camera->installCamera(view.get());
            //updateViewport();
        }

        return true;
    }

    OSGViewport *quickItem;

    OSGViewport::UpdateMode updateMode;
    int frameTimer;

    OSGNode *sceneData;

    OSGCamera *camera;

    osg::ref_ptr<osgViewer::View> view;
    osg::ref_ptr<osgViewer::CompositeViewer> compositeViewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow;


    bool realized;

    static QtKeyboardMap keyMap;

public slots:

    void initCompositeViewer() {
        qDebug() << "OSGViewport - initCompositeViewer";
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
        traits->width = 100;//window->width();
        traits->height = 100;//window->height();
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        //traits->sharedContext = gc;
        //traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

        graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits);

        view->getCamera()->setGraphicsContext(graphicsWindow);

        compositeViewer->addView(view.get());

//        if (window) {
//            window->setClearBeforeRendering(false);
//        }
        if (updateMode == OSGViewport::Discrete) {
            frameTimer = startTimer(10);
        }

    }

protected:

    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == frameTimer) {
            if (quickItem) {
                quickItem->update();
            }
        }
        QObject::timerEvent(event);
    }


private slots:

    void onWindowChanged(QQuickWindow *window)
    {
        qDebug() << "OSGViewport - onWindowChanged" << window;
//        if (this->window) {
//            disconnect(quickItem);
//        }
//        this->window = window;
//        if (this->window) {
//            connect(quickItem, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(onWindowChanged(QQuickWindow*)));
//        }
    }

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGViewport - onNodeChanged" << node;
        if (view.valid()) {
            acceptNode(node);
        }
    }

private:

    void initOSG() {
        qDebug() << "OSGViewport - initOSG";
        view = new osgViewer::View();
        // TODO will the StatsHandler be destroyed???
        view->addEventHandler(new osgViewer::StatsHandler());
        //viewer->addEventHandler(new osgGA::StateSetManipulator());
        //viewer->addEventHandler(new osgViewer::ThreadingHandler());
    }

    void acceptQuickItem() {
        Q_ASSERT(quickItem);

        qDebug() << "OSGViewport - acceptQuickItem" << quickItem << quickItem->window();

        //connect(quickItem, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(onWindowChanged(QQuickWindow*)));
    }

};

QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

OSGViewport::OSGViewport(QQuickItem *parent) : QQuickFramebufferObject(parent), h(new Hidden(this))
{
    qDebug() << "OSGViewport - <init>";
    //setClearBeforeRendering(false);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
    qDebug() << "OSGViewport - <destruct>";
}

QQuickFramebufferObject::Renderer *OSGViewport::createRenderer() const
{
    return new Hidden::ViewportRenderer(h);
}

void OSGViewport::realize()
{
    qDebug() << "OSGViewport - realize";
    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        OSGNode *node = qobject_cast<OSGNode *>(i.next());
        if (node) {
            node->realize();
        }
    }
}

// see https://bugreports.qt-project.org/browse/QTBUG-41073
QSGNode *OSGViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
{
    //qDebug() << "OSGViewport - updatePaintNode";
    if (!node) {
        node = QQuickFramebufferObject::updatePaintNode(node, nodeData);
        QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode *>(node);
        if (n) {
            // flip Y axis
            n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        }
        return node;
    }
    return QQuickFramebufferObject::updatePaintNode(node, nodeData);
}

OSGViewport::UpdateMode OSGViewport::updateMode() const
{
    return h->updateMode;
}

void OSGViewport::setUpdateMode(OSGViewport::UpdateMode mode)
{
    if (h->acceptUpdateMode(mode)) {
        emit updateModeChanged(updateMode());
    }
}

QColor OSGViewport::color() const
{
    const osg::Vec4 osgColor = h->view->getCamera()->getClearColor();
    return QColor::fromRgbF(osgColor.r(), osgColor.g(), osgColor.b(), osgColor.a());
}

void OSGViewport::setColor(const QColor &color)
{
    osg::Vec4 osgColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    if (h->view->getCamera()->getClearColor() != osgColor) {
        h->view->getCamera()->setClearColor(osgColor);
        emit colorChanged(color);
    }
}

OSGNode *OSGViewport::sceneData()
{
    return h->sceneData;
}

void OSGViewport::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

OSGCamera* OSGViewport::camera()
{
    return h->camera;
}

void OSGViewport::setCamera(OSGCamera *camera)
{
    if (h->acceptCamera(camera)) {
        emit cameraChanged(camera);
    }
}

void OSGViewport::mousePressEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
    case Qt::LeftButton: button = 1; break;
    case Qt::MidButton: button = 2; break;
    case Qt::RightButton: button = 3; break;
    case Qt::NoButton: button = 0; break;
    default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseMotion(pos.x(), pos.y());
}

void OSGViewport::mouseReleaseEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
    case Qt::LeftButton: button = 1; break;
    case Qt::MidButton: button = 2; break;
    case Qt::RightButton: button = 3; break;
    case Qt::NoButton: button = 0; break;
    default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    h->view->getEventQueue()->mouseScroll(
            event->orientation() == Qt::Vertical ?
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT) );

}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    h->setKeyboardModifiers( event );
    int value = h->keyMap.remapKey( event );
    h->view->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
    //    if( _forwardKeyEvents )
    //        inherited::keyPressEvent( event );
}

void OSGViewport::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->ignore();
    } else {
        h->setKeyboardModifiers(event);
        int value = h->keyMap.remapKey(event);
        h->view->getEventQueue()->keyRelease(value);
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
    //    if( _forwardKeyEvents )
    //        inherited::keyReleaseEvent( event );
}

} // namespace osgQtQuick

#include "OSGViewport.moc"
