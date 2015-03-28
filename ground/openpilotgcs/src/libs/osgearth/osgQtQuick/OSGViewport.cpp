#include "OSGViewport.hpp"

#include "../osgearth.h"

#include "OSGNode.hpp"
#include "OSGCamera.hpp"
#include "Utility.hpp"

#include <osg/Node>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

#include <osgEarth/MapNode>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>
#include <osgEarthUtil/LogarithmicDepthBuffer>

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QOpenGLFunctions>

#include <QDebug>

#include <QThread>
#include <QApplication>

namespace osgQtQuick {
/*
 * TODO : add OSGView to handle multiple views for a given OSGViewport
 *
 */
struct OSGViewport::Hidden : public QObject {
    Q_OBJECT

public:

    Hidden(OSGViewport *quickItem) : QObject(quickItem),
        self(quickItem),
        updateMode(Discrete),
        frameTimer(-1),
        sceneData(0),
        camera(0),
        logDepthBufferEnabled(false),
        realized(false)
    {
        qDebug() << "OSGViewport::Hidden - <init>" << self;
        view = new osgViewer::View();

        // TODO will the handlers be destroyed???
        // add the state manipulator
        view->addEventHandler(new osgGA::StateSetManipulator(view->getCamera()->getOrCreateStateSet()));
        // b : Toggle Backface Culling, l : Toggle Lighting, t : Toggle Texturing, w : Cycle Polygon Mode

        // add the thread model handler
        // view->addEventHandler(new osgViewer::ThreadingHandler);

        // add the window size toggle handler
        // view->addEventHandler(new osgViewer::WindowSizeHandler);

        // add the stats handler
        view->addEventHandler(new osgViewer::StatsHandler);

        // add the help handler
        // view->addEventHandler(new osgViewer::HelpHandler(arguments.getApplicationUsage()));

        // add the record camera path handler
        ///view->addEventHandler(new osgViewer::RecordCameraPathHandler);

        // add the LOD Scale handler
        // view->addEventHandler(new osgViewer::LODScaleHandler);

        // add the screen capture handler
        // view->addEventHandler(new osgViewer::ScreenCaptureHandler);

        connect(quickItem, SIGNAL(windowChanged(QQuickWindow *)), this, SLOT(onWindowChanged(QQuickWindow *)));
    }

    ~Hidden()
    {
        if (frameTimer >= 0) {
            killTimer(frameTimer);
        }
        self = NULL;
    }

public slots:
    void onWindowChanged(QQuickWindow *window)
    {
        qDebug() << "OSGViewport - onWindowChanged" << window;
        if (window) {
            window->setClearBeforeRendering(false);
            formatInfo(window->format());
        }
    }

public:

    QPointF mousePoint(QMouseEvent *event)
    {
        // qreal x = 0.01 * (event->x() - self->width() / 2);
        // qreal y = 0.01 * (event->y() - self->height() / 2);
        qreal x = 2.0 * (event->x() - self->width() / 2) / self->width();
        qreal y = 2.0 * (event->y() - self->height() / 2) / self->height();

        return QPointF(x, y);
    }

    void setKeyboardModifiers(QInputEvent *event)
    {
        int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
        unsigned int mask = 0;

        if (modkey & Qt::ShiftModifier) {
            mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
        }
        if (modkey & Qt::ControlModifier) {
            mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
        }
        if (modkey & Qt::AltModifier) {
            mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
        }
        view->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
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
            connect(sceneData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGViewport - acceptNode" << node;
        if (!node) {
            qWarning() << "OSGViewport - acceptNode - node is null";
            if (view.valid()) {
                view->setSceneData(NULL);
            }
            return true;
        }

        // TODO map handling should not be done here
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (mapNode) {
            qDebug() << "OSGViewport - acceptNode - found map node" << mapNode;
            // TODO will the AutoClipPlaneCullCallback be destroyed???
            qDebug() << "OSGViewport - acceptNode : set AutoClipPlaneCullCallback on camera";
            view->getCamera()->setCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
            // mapNode->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
            if (logDepthBufferEnabled) {
                qDebug() << "OSGViewport - acceptNode : install logarithmic depth buffer";
                // logDepthBuffer.setUseFragDepth(true);
                logDepthBuffer.install(view->getCamera());
            }

            // lodBlending = new osgEarth::Util::LODBlending();
            // mapNode->getTerrainEngine()->addEffect(lodBlending.get());

// osgEarth::Util::DetailTexture* detail = new osgEarth::Util::DetailTexture();
////detail->setImage(osgDB::readImageFile("noise3.jpg"));
// detail->setIntensity(0.5f);
////detail->setImageUnit(4);
// mapNode->getTerrainEngine()->addEffect(detail);
        }

        // TODO sky handling should not be done here
        osgEarth::Util::SkyNode *skyNode = osgQtQuick::findTopMostNodeOfType<osgEarth::Util::SkyNode>(node);
        if (skyNode) {
            qDebug() << "OSGViewport - acceptNode - found sky node" << skyNode;
            skyNode->attach(view.get(), 0);
        }

        view->setSceneData(node);

        return true;
    }

    bool acceptUpdateMode(OSGViewport::UpdateMode mode)
    {
        qDebug() << "OSGViewport - acceptUpdateMode" << mode;
        if (updateMode == mode) {
            return true;
        }

        updateMode = mode;

        return true;
    }

    bool acceptCamera(OSGCamera *camera)
    {
        qDebug() << "OSGViewport - acceptCamera" << camera;
        if (this->camera == camera) {
            return true;
        }

        this->camera = camera;

        if (this->camera) {
            // this->camera->installCamera(view.get());
        }

        return true;
    }

    void info(QString msg)
    {
// qDebug() << "-----------------------------------------------------";
// qDebug().noquote() << msg;
// qDebug() << "-----------------------------------------------------";
// qDebug() << "current thread     :" << QThread::currentThread();;
// qDebug() << "application thread :" << QApplication::instance()->thread();
// qDebug() << "current context    :" << QOpenGLContext::currentContext();
// qDebug() << "-----------------------------------------------------";
    }

    OSGViewport *self;

    OSGViewport::UpdateMode updateMode;
    int frameTimer;

    OSGNode   *sceneData;

    OSGCamera *camera;

    osg::ref_ptr<osgViewer::CompositeViewer> viewer;
    osg::ref_ptr<osgViewer::View> view;

    bool logDepthBufferEnabled;
    osgEarth::Util::LogarithmicDepthBuffer logDepthBuffer;

    // osg::ref_ptr<osgEarth::Util::LODBlending> lodBlending;

    bool realized;

    static QtKeyboardMap keyMap;


    void initViewer()
    {
        qDebug() << "OSGViewport - initViewer";
        viewer = new osgViewer::CompositeViewer();
        viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

        // disable the default setting of viewer.done() by pressing Escape.
        viewer->setKeyEventSetsDone(0);
        // viewer->setQuitEventSetsDone(false);

        osg::GraphicsContext::Traits *traits = getTraits();
        // traitsInfo(traits);

        osgViewer::GraphicsWindowEmbedded *graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits);

        view->getCamera()->setGraphicsContext(graphicsWindow);

        viewer->addView(view.get());

        if (camera) {
            camera->installCamera(view.get());
        } else {
            qWarning() << "OSGViewport - initViewer - no camera!";
        }

        if (updateMode == OSGViewport::Discrete) {
            qDebug() << "OSGViewport - initViewer - starting timer";
            frameTimer = startTimer(33, Qt::PreciseTimer);
        }
    }

    osg::GraphicsContext::Traits *getTraits()
    {
        osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();
        osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits(ds);

        traits->readDISPLAY();
        if (traits->displayNum < 0) {
            traits->displayNum = 0;
        }

        traits->windowDecoration = false;
        traits->x       = 0;
        traits->y       = 0;
        traits->width   = 100; // window->width();
        traits->height  = 100; // window->height();

        traits->alpha   = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();

        traits->doubleBuffer = false; // ds->getDoubleBuffer();
        traits->vsync   = false;
        // traits->sharedContext = gc;
        // traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

        return traits;
    }

protected:

    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == frameTimer) {
            if (self) {
                self->update();
            }
        }
        QObject::timerEvent(event);
    }


private slots:

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGViewport - onNodeChanged" << node;
        if (view.valid()) {
            acceptNode(node);
        }
    }
};

////////////////////////////////////////
// Renderer
////////////////////////////////////////

class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:

    ViewportRenderer(OSGViewport::Hidden *h) : h(h)
    {
        qDebug() << "ViewportRenderer - <init>";
        h->info("ViewportRenderer - <init>");

        OsgEarth::initialize();

        if (!h->realized) {
            h->self->realize();
            h->initViewer();
            h->realized = true;
        } else {
            // needed when PFD gadget is reparented
            // in that case a new glcontext and renderer are created...
            // h->resetViewer();
        }

        requestRedraw = true;
    }

    ~ViewportRenderer()
    {
        qDebug() << "ViewportRenderer - <destruct>";
        // this gets called before timer is killed
        // TODO need to kill timer here or handle in someother proper way
    }

    // This function is the only place when it is safe for the renderer and the item to read and write each others members.
    void synchronize(QQuickFramebufferObject *item)
    {
        h->camera->setViewport(0, 0, item->width(), item->height());
        // TODO scene update should be done here
    }

    // This function is called when the FBO should be rendered into.
    // The framebuffer is bound at this point and the glViewport has been set up to match the FBO size.
    void render()
    {
        // qDebug() << "ViewportRenderer - render";
        // h->info("ViewportRenderer - render");

        // needed to properly render models without terrain (Qt bug?)
        QOpenGLContext::currentContext()->functions()->glUseProgram(0);

        if (checkNeedToDoFrame()) {
            // TODO scene update should NOT be done here
            h->viewer->frame();
            requestRedraw = false;
        }

        if (h->updateMode == OSGViewport::Continuous) {
            // trigger next update
            update();
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {
        qDebug() << "ViewportRenderer - createFramebufferObject" << size;

        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // format.setSamples(4);
        int dpr = h->self->window()->devicePixelRatio();
        QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(size.width() / dpr, size.height() / dpr, format);
        return fbo;
    }

private:
    bool checkNeedToDoFrame()
    {
// if (requestRedraw) {
// return true;
// }
// if (getDatabasePager()->requiresUpdateSceneGraph() || getDatabasePager()->getRequestsInProgress()) {
// return true;
// }
        return true;
    }

    OSGViewport::Hidden *h;
    bool requestRedraw;
};

////////////////////////////////////////
// OSGViewport
////////////////////////////////////////

QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

OSGViewport::OSGViewport(QQuickItem *parent) : QQuickFramebufferObject(parent), h(new Hidden(this))
{
    qDebug() << "OSGViewport - <init>";
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
    qDebug() << "OSGViewport - <destruct>";
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

OSGCamera *OSGViewport::camera()
{
    return h->camera;
}

void OSGViewport::setCamera(OSGCamera *camera)
{
    if (h->acceptCamera(camera)) {
        emit cameraChanged(camera);
    }
}

bool OSGViewport::logarithmicDepthBuffer()
{
    return h->logDepthBufferEnabled;
}

void OSGViewport::setLogarithmicDepthBuffer(bool enabled)
{
    if (h->logDepthBufferEnabled != enabled) {
        h->logDepthBufferEnabled = enabled;
        emit logarithmicDepthBufferChanged(logarithmicDepthBuffer());
    }
}

QQuickFramebufferObject::Renderer *OSGViewport::createRenderer() const
{
    qDebug() << "OSGViewport - createRenderer";
    return new ViewportRenderer(h);
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
    // qDebug() << "OSGViewport - updatePaintNode";
    if (!node) {
        qDebug() << "OSGViewport - updatePaintNode - set transform";
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

void OSGViewport::mousePressEvent(QMouseEvent *event)
{
    int button = 0;

    switch (event->button()) {
    case Qt::LeftButton: button  = 1; break;
    case Qt::MidButton: button   = 2; break;
    case Qt::RightButton: button = 3; break;
    case Qt::NoButton: button    = 0; break;
    default: button = 0; break;
    }
    h->setKeyboardModifiers(event);
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    h->setKeyboardModifiers(event);
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseMotion(pos.x(), pos.y());
}

void OSGViewport::mouseReleaseEvent(QMouseEvent *event)
{
    int button = 0;

    switch (event->button()) {
    case Qt::LeftButton: button  = 1; break;
    case Qt::MidButton: button   = 2; break;
    case Qt::RightButton: button = 3; break;
    case Qt::NoButton: button    = 0; break;
    default: button = 0; break;
    }
    h->setKeyboardModifiers(event);
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    h->view->getEventQueue()->mouseScroll(
        event->orientation() == Qt::Vertical ?
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT));
}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    h->setKeyboardModifiers(event);
    int value = h->keyMap.remapKey(event);
    h->view->getEventQueue()->keyPress(value);

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
    // if( _forwardKeyEvents )
    // inherited::keyPressEvent( event );
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
    // if( _forwardKeyEvents )
    // inherited::keyReleaseEvent( event );
}
} // namespace osgQtQuick

#include "OSGViewport.moc"
