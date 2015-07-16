#include "OSGViewport.hpp"

#include "../osgearth.h"
#include "../utility.h"

#include "OSGNode.hpp"
#include "OSGCamera.hpp"

#include <osg/Node>
#include <osg/DeleteHandler>
#include <osg/GLObjects>
#include <osg/ApplicationUsage>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

#include <osgEarth/MapNode>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>

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
   Debugging tips
   - export OSG_NOTIFY_LEVEL=DEBUG


   Z-fighting can happen with coincident polygons, but it can also happen when the Z buffer has insufficient resolution
   to represent the data in the scene. In the case where you are close up to an object (the helicopter)
   and also viewing a far-off object (the earth) the Z buffer has to stretch to accommodate them both.
   This can result in loss of precision and Z fighting.

   Assuming you are not messing around with the near/far computations, and assuming you don't have any other objects
   in the scene that are farther off than the earth, there are a couple things you can try.

   One, adjust the near/far ratio of the camera. Look at osgearth_viewer.cpp to see how.

   Two, you can try to use the AutoClipPlaneHandler. You can install it automatically by running osgearth_viewer --autoclip.

   If none of that works, you can try parenting your helicopter with an osg::Camera in NESTED mode,
   which will separate the clip plane calculations of the helicopter from those of the earth. *

   TODO : add OSGView to handle multiple views for a given OSGViewport
 */
struct OSGViewport::Hidden : public QObject {
    Q_OBJECT

public:

    Hidden(OSGViewport *quickItem) : QObject(quickItem),
        self(quickItem),
        window(NULL),
        sceneData(NULL),
        camera(NULL),
        updateMode(Discrete),
        frameTimer(-1)
    {
        qDebug() << "OSGViewport::Hidden";

        OsgEarth::initialize();

        // workaround to avoid using GraphicsContext #0
        if (!dummy.valid()) {
            dummy = createGraphicsContext();
        }

        createViewer();

        connect(quickItem, &OSGViewport::windowChanged, this, &Hidden::onWindowChanged);
    }

    ~Hidden()
    {
        qDebug() << "OSGViewport::~Hidden";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "OSGViewport::~Hidden");

        stop();

        destroyViewer();
    }

public slots:
    void onWindowChanged(QQuickWindow *window)
    {
        qDebug() << "OSGViewport::onWindowChanged" << window;
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onWindowChanged");
        if (window) {
            // window->setClearBeforeRendering(false);
            connect(window, &QQuickWindow::sceneGraphInitialized, this, &Hidden::onSceneGraphInitialized, Qt::DirectConnection);
            connect(window, &QQuickWindow::sceneGraphAboutToStop, this, &Hidden::onSceneGraphAboutToStop, Qt::DirectConnection);
            connect(window, &QQuickWindow::sceneGraphInvalidated, this, &Hidden::onSceneGraphInvalidated, Qt::DirectConnection);
        } else {
            if (this->window) {
                disconnect(this->window);
            }
        }
        this->window = window;
    }
public:

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGViewport::acceptSceneData" << node;
        if (sceneData == node) {
            return true;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            acceptNode(sceneData->node());
            connect(sceneData, &OSGNode::nodeChanged, this, &Hidden::onNodeChanged);
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {

        return true;
    }

    bool attach(osgViewer::View *view)
    {
        if (!sceneData) {
            qWarning() << "OSGViewport::attach - invalid scene!";
            return false;
        }
        if (!attach(view, sceneData->node())) {
            qWarning() << "OSGViewport::attach - failed to attach node!";
            return false;
        }
        if (camera) {
            camera->setViewport(0, 0, self->width(), self->height());
            camera->attach(view);
        } else {
            qWarning() << "OSGViewport::attach - no camera!";
        }
        return true;
    }

    bool attach(osgViewer::View *view, osg::Node *node)
    {
        qDebug() << "OSGViewport::attach" << node;
        if (!view) {
            qWarning() << "OSGViewport::attach - view is null";
            return false;
        }
        if (!node) {
            qWarning() << "OSGViewport::attach - node is null";
            view->setSceneData(NULL);
            return true;
        }

        // TODO map handling should not be done here
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (false && mapNode) {
            qDebug() << "OSGViewport::attach - found map node" << mapNode;
            // install AutoClipPlaneCullCallback : computes near/far planes based on scene geometry
            qDebug() << "OSGViewport::attach - set AutoClipPlaneCullCallback on camera";
            // TODO will the AutoClipPlaneCullCallback be destroyed ?
            // TODO does it need to be added to the map node or to the view ?
            cullCallback = new osgEarth::Util::AutoClipPlaneCullCallback(mapNode);
            //view->getCamera()->addCullCallback(cullCallback);
            mapNode->addCullCallback(cullCallback);
        }

        //view->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);

        view->setSceneData(node);

        return true;
    }

    bool detach(osgViewer::View *view)
    {
        qDebug() << "OSGViewport::detach" << view;

        if (camera) {
            camera->detach(view);
        }

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(view->getSceneData());
        if (mapNode) {
            view->getCamera()->removeCullCallback(cullCallback);
            cullCallback = NULL;
        }

        return true;
    }

    void onSceneGraphInitialized()
    {
        qDebug() << "OSGViewport::onSceneGraphInitialized";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onSceneGraphInitialized");
    }

    void onSceneGraphAboutToStop()
    {
        qDebug() << "OSGViewport::onSceneGraphAboutToStop";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onSceneGraphAboutToStop");
    }

    void onSceneGraphInvalidated()
    {
        qDebug() << "OSGViewport::onSceneGraphInvalidated";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onSceneGraphInvalidated");
    }

    void initializeResources()
    {
        qDebug() << "OSGViewport::initializeResources";
        if (!view.valid()) {
            qDebug() << "OSGViewport::initializeResources - creating view";
            view = createView();
            self->attach(view.get());
            viewer->addView(view);
            start();
            // osgDB::writeNodeFile(*(h->self->sceneData()->node()), "saved.osg");
        }
    }

    void releaseResources()
    {
        qDebug() << "OSGViewport::releaseResources";
        if (!view.valid()) {
            qWarning() << "OSGViewport::releaseResources - view is not valid!";
            return;
        }
        osg::deleteAllGLObjects(view->getCamera()->getGraphicsContext()->getState()->getContextID());
        // view->getSceneData()->releaseGLObjects(view->getCamera()->getGraphicsContext()->getState());
        // view->getCamera()->releaseGLObjects(view->getCamera()->getGraphicsContext()->getState());
        // view->getCamera()->getGraphicsContext()->close();
        // view->getCamera()->setGraphicsContext(NULL);
    }

    bool acceptUpdateMode(OSGViewport::UpdateMode mode)
    {
        // qDebug() << "OSGViewport::acceptUpdateMode" << mode;
        if (updateMode == mode) {
            return true;
        }

        updateMode = mode;

        return true;
    }

    bool acceptCamera(OSGCamera *camera)
    {
        qDebug() << "OSGViewport::acceptCamera" << camera;
        if (this->camera == camera) {
            return true;
        }

        this->camera = camera;

        return true;
    }

    OSGViewport  *self;

    QQuickWindow *window;

    OSGNode      *sceneData;
    OSGCamera    *camera;

    OSGViewport::UpdateMode updateMode;

    int frameTimer;

    osg::ref_ptr<osgViewer::CompositeViewer> viewer;
    osg::ref_ptr<osgViewer::View> view;

    osg::ref_ptr<osg::NodeCallback> cullCallback;

    static osg::ref_ptr<osg::GraphicsContext> dummy;

    static QtKeyboardMap keyMap;

    void createViewer()
    {
        if (viewer.valid()) {
            qWarning() << "OSGViewport::createViewer - viewer is valid";
            return;
        }

        qDebug() << "OSGViewport::createViewer";

        viewer = new osgViewer::CompositeViewer();
        viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> ico = new osgUtil::IncrementalCompileOperation();
        ico->setTargetFrameRate(30.0f);
        viewer->setIncrementalCompileOperation(ico);

        // disable the default setting of viewer.done() by pressing Escape.
        viewer->setKeyEventSetsDone(0);
        // viewer->setQuitEventSetsDone(false);
    }

    void destroyViewer()
    {
        if (!viewer.valid()) {
            qWarning() << "OSGViewport::destroyViewer - viewer is not valid";
            return;
        }

        qDebug() << "OSGViewport::destroyViewer";

        viewer = NULL;
    }

    osgViewer::View *createView()
    {
        qWarning() << "OSGViewport::createView";
        osgViewer::View *view = new osgViewer::View();

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
        // view->addEventHandler(new osgViewer::RecordCameraPathHandler);

        // add the LOD Scale handler
        // view->addEventHandler(new osgViewer::LODScaleHandler);

        // add the screen capture handler
        // view->addEventHandler(new osgViewer::ScreenCaptureHandler);

        view->getCamera()->setGraphicsContext(createGraphicsContext());

        return view;
    }

    osg::GraphicsContext *createGraphicsContext()
    {
        qWarning() << "OSGViewport::createGraphicsContext";

        osg::GraphicsContext::Traits *traits = getTraits();
        // traitsInfo(*traits);

        traits->pbuffer = true;
        osg::GraphicsContext *graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
        // if (!graphicsContext) {
        //     qWarning() << "Failed to create pbuffer, failing back to normal graphics window.";
        //     traits->pbuffer = false;
        //     graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
        // }

        return graphicsContext;
    }

    osg::GraphicsContext::Traits *getTraits()
    {
        osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();
        osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits(ds);

        // traitsInfo(traits);

        // traits->readDISPLAY();
        // if (traits->displayNum < 0) {
        // traits->displayNum = 0;
        // }

        traits->windowDecoration = false;
        traits->x       = 0;
        traits->y       = 0;
        traits->width   = self->width();
        traits->height  = self->height();

        traits->alpha   = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();

        traits->doubleBuffer = false; //ds->getDoubleBuffer();
        traits->vsync   = false;
        // traits->sharedContext = gc;
        // traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(this);

        return traits;
    }

    void start()
    {
        if (updateMode == OSGViewport::Discrete && (frameTimer < 0)) {
            qDebug() << "OSGViewport::start - starting timer";
            frameTimer = startTimer(33, Qt::PreciseTimer);
        }
    }

    void stop()
    {
        if (frameTimer >= 0) {
            qDebug() << "OSGViewport::stop - killing timer";
            killTimer(frameTimer);
            frameTimer = -1;
        }
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
        qDebug() << "OSGViewport::onNodeChanged" << node;
        qWarning() << "OSGViewport::onNodeChanged - not implemented";
        // if (view.valid()) {
        // acceptNode(node);
        // }
    }
};

/* class ViewportRenderer */

class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    ViewportRenderer(OSGViewport::Hidden *h) : h(h)
    {
        qDebug() << "ViewportRenderer::ViewportRenderer";
        osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::ViewportRenderer");

        h->initializeResources();

        requestRedraw = false;
    }

    ~ViewportRenderer()
    {
        qDebug() << "ViewportRenderer::~ViewportRenderer";
        osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::~ViewportRenderer");

        h->releaseResources();
    }

    // This function is the only place when it is safe for the renderer and the item to read and write each others members.
    void synchronize(QQuickFramebufferObject *item)
    {
        // qDebug() << "ViewportRenderer::synchronize";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::synchronize");

        if (!h->view.valid()) {
            qWarning() << "ViewportRenderer::synchronize - invalid view";
            return;
        }

        // need to split frame() open and do the synchronization here (calling update callbacks, etc...)
    }

    // This function is called when the FBO should be rendered into.
    // The framebuffer is bound at this point and the glViewport has been set up to match the FBO size.
    void render()
    {
        // qDebug() << "ViewportRenderer::render";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::render");

        if (!h->viewer.valid()) {
            qWarning() << "ViewportRenderer::render - invalid viewport";
            return;
        }

        // needed to properly render models without terrain (Qt bug?)
        QOpenGLContext::currentContext()->functions()->glUseProgram(0);

        if (checkNeedToDoFrame()) {
            // TODO scene update should NOT be done here
            h->viewer->frame();
            requestRedraw = false;
        }

        //h->self->window()->resetOpenGLState();

        if (h->updateMode == OSGViewport::Continuous) {
            // trigger next update
            update();
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {
        qDebug() << "ViewportRenderer::createFramebufferObject" << size;
        if (h->camera) {
            h->camera->setViewport(0, 0, size.width(), size.height());
        }

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

osg::ref_ptr<osg::GraphicsContext> OSGViewport::Hidden::dummy;
QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

/* class OSGViewport */


OSGViewport::OSGViewport(QQuickItem *parent) : QQuickFramebufferObject(parent), h(new Hidden(this))
{
    qDebug() << "OSGViewport::OSGViewport";
    // setClearBeforeRendering(false);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
    qDebug() << "OSGViewport::~OSGViewport";
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

QQuickFramebufferObject::Renderer *OSGViewport::createRenderer() const
{
    qDebug() << "OSGViewport::createRenderer";
    osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "createRenderer");
    return new ViewportRenderer(h);
}

bool OSGViewport::attach(osgViewer::View *view)
{
    qDebug() << "OSGViewport::attach" << view;

    h->attach(view);

    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        QObject *object= i.next();
        OSGNode *node = qobject_cast<OSGNode *>(object);
        if (node) {
            qDebug() << "OSGViewport::attach - child" << node;
            node->attach(view);
        }
    }

    return true;
}

bool OSGViewport::detach(osgViewer::View *view)
{
    qDebug() << "OSGViewport::detach" << view;

    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        QObject *object= i.next();
        OSGNode *node = qobject_cast<OSGNode *>(object);
        if (node) {
            node->detach(view);
        }
    }

    h->detach(view);

    return true;
}

void OSGViewport::releaseResources()
{
    QQuickFramebufferObject::releaseResources();
}

// see https://bugreports.qt-project.org/browse/QTBUG-41073
QSGNode *OSGViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
{
    // qDebug() << "OSGViewport::updatePaintNode";
    if (!node) {
        qDebug() << "OSGViewport::updatePaintNode - set transform";
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


QPointF OSGViewport::mousePoint(QMouseEvent *event)
{
    // qreal x = 0.01 * (event->x() - self->width() / 2);
    // qreal y = 0.01 * (event->y() - self->height() / 2);
    qreal x = 2.0 * (event->x() - width() / 2) / width();
    qreal y = 2.0 * (event->y() - height() / 2) / height();

    return QPointF(x, y);
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
    setKeyboardModifiers(event);
    QPointF pos = mousePoint(event);
    if (h->view.valid()) {
        h->view.get()->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
    }
}

void OSGViewport::setKeyboardModifiers(QInputEvent *event)
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
    if (h->view.valid()) {
        h->view.get()->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
    }
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    setKeyboardModifiers(event);
    QPointF pos = mousePoint(event);
    if (h->view.valid()) {
        h->view.get()->getEventQueue()->mouseMotion(pos.x(), pos.y());
    }
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
    setKeyboardModifiers(event);
    QPointF pos = mousePoint(event);
    if (h->view.valid()) {
        h->view.get()->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
    }
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    osgGA::GUIEventAdapter::ScrollingMotion motion =
        event->orientation() == Qt::Vertical ?
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT);

    if (h->view.valid()) {
        h->view.get()->getEventQueue()->mouseScroll(motion);
    }
}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    setKeyboardModifiers(event);
    int value = h->keyMap.remapKey(event);
    if (h->view.valid()) {
        h->view.get()->getEventQueue()->keyPress(value);
    }

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
        setKeyboardModifiers(event);
        int value = h->keyMap.remapKey(event);
        if (h->view.valid()) {
            h->view.get()->getEventQueue()->keyRelease(value);
        }
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
    // if( _forwardKeyEvents )
    // inherited::keyReleaseEvent( event );
}
} // namespace osgQtQuick

#include "OSGViewport.moc"
