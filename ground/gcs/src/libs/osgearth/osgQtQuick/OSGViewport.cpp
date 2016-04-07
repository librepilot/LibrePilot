/**
 ******************************************************************************
 *
 * @file       OSGViewport.cpp
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

#include "OSGViewport.hpp"

#include "osgearth.h"
#include "utils/utility.h"

#include "OSGNode.hpp"
#include "OSGCamera.hpp"

#include <osg/Node>
#include <osg/Vec4>
#include <osg/DeleteHandler>
#include <osg/GLObjects>
#include <osg/ApplicationUsage>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgGA/CameraManipulator>

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QOpenGLFunctions>

#include <QDebug>

/*
   Debugging tips
   - export OSG_NOTIFY_LEVEL=DEBUG

   Z-fighting can happen with coincident polygons, but it can also happen when the Z buffer has insufficient resolution
   to represent the data in the scene. In the case where you are close up to an object (the helicopter)
   and also viewing a far-off object (the earth) the Z buffer has to stretch to accommodate them both.
   This can result in loss of precision and Z fighting.

   Assuming you are not messing around with the near/far computations, and assuming you don't have any other objects
   in the scene that are farther off than the earth, there are a couple things you can try.

   Adjust the near/far ratio of the camera. Look at osgearth_viewer.cpp to see how.
   Use LogarythmicDepthBuffer.

   More complex : you can try parenting your helicopter with an osg::Camera in NESTED mode,
   which will separate the clip plane calculations of the helicopter from those of the earth. *

   TODO : add OSGView to handle multiple views for a given OSGViewport
 */

/*
   that's a typical error when working with high-resolution (retina)
   displays. The issue here is that on the high-resolution devices, the UI
   operates with a virtual pixel size that is smaller than the real number
   of pixels on the device. For example, you get coordinates from 0 to 2048
   while the real device resolution if 4096 pixels. This factor has to be
   taken into account when mapping from window coordinates to OpenGL, e.g.,
   when calling glViewport.

   How you can get this factor depends on the GUI library you are using. In
   Qt, you can query it with QWindow::devicePixelRatio():
   http://doc.qt.io/qt-5/qwindow.html#devicePixelRatio

   So, there should be something like
   glViewport(0, 0, window->width() * window->devicePixelRatio(),
   window->height() * window->devicePixelRatio()).

   Also keep in mind that you have to do the same e.g. for mouse coordinates.

   I think osgQt already handles this correctly, so you shouldn't have to
   worry about this if you use the classes provided by osgQt ...
 */

namespace osgQtQuick {
// enum DirtyFlag { Scene = 1 << 0, Camera = 1 << 1 };

class ViewportRenderer;

struct OSGViewport::Hidden : public QObject {
    Q_OBJECT

    friend ViewportRenderer;

private:
    OSGViewport *const self;

    QQuickWindow *window;

    int frameTimer;

    osg::ref_ptr<osg::GraphicsContext> gc;

public:
    OSGNode   *sceneNode;
    OSGCamera *cameraNode;

    osg::ref_ptr<osgViewer::CompositeViewer> viewer;
    osg::ref_ptr<osgViewer::View> view;

    OSGCameraManipulator *manipulator;

    UpdateMode::Enum     updateMode;

    bool busy;

    static QtKeyboardMap keyMap;

    Hidden(OSGViewport *self) : QObject(self), self(self), window(NULL), frameTimer(-1),
        sceneNode(NULL), cameraNode(NULL), manipulator(NULL), updateMode(UpdateMode::OnDemand), busy(false)
    {
        OsgEarth::initialize();

        createViewer();

        connect(self, &OSGViewport::windowChanged, this, &Hidden::onWindowChanged);
    }

    ~Hidden()
    {
        disconnect(self);

        stopTimer();

        destroyViewer();
    }

public slots:
    void onWindowChanged(QQuickWindow *window)
    {
        // qDebug() << "OSGViewport::onWindowChanged" << window;
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onWindowChanged");
        if (window) {
            // window->setClearBeforeRendering(false);
// connect(window, &QQuickWindow::sceneGraphInitialized, this, &Hidden::onSceneGraphInitialized, Qt::DirectConnection);
// connect(window, &QQuickWindow::sceneGraphAboutToStop, this, &Hidden::onSceneGraphAboutToStop, Qt::DirectConnection);
// connect(window, &QQuickWindow::sceneGraphInvalidated, this, &Hidden::onSceneGraphInvalidated, Qt::DirectConnection);
// connect(window, &QQuickWindow::visibleChanged, this, &Hidden::visibleChanged, Qt::DirectConnection);
// connect(window, &QQuickWindow::widthChanged, this, &Hidden::widthChanged, Qt::DirectConnection);
// connect(window, &QQuickWindow::heightChanged, this, &Hidden::heightChanged, Qt::DirectConnection);
        } else {
// if (this->window) {
// disconnect(this->window);
// }
        }
        this->window = window;
    }

public:
    bool acceptSceneNode(OSGNode *node)
    {
        qDebug() << "OSGViewport::acceptSceneNode" << node;
        if (sceneNode == node) {
            return true;
        }

        if (sceneNode) {
            disconnect(sceneNode);
        }

        sceneNode = node;

        if (sceneNode) {
            connect(sceneNode, &OSGNode::nodeChanged, this, &Hidden::onSceneNodeChanged);
        }

        return true;
    }

    bool acceptCameraNode(OSGCamera *node)
    {
        qDebug() << "OSGViewport::acceptCameraNode" << node;
        if (cameraNode == node) {
            return true;
        }

        if (cameraNode) {
            disconnect(cameraNode);
        }

        cameraNode = node;

        if (cameraNode) {
            connect(cameraNode, &OSGNode::nodeChanged, this, &Hidden::onCameraNodeChanged);
        }

        return true;
    }

    bool acceptManipulator(OSGCameraManipulator *m)
    {
        qDebug() << "OSGViewport::acceptManipulator" << manipulator;
        if (manipulator == m) {
            return true;
        }

        manipulator = m;

        return true;
    }

    void initializeResources()
    {
        if (gc.valid()) {
            // qWarning() << "OSGViewport::initializeResources - gc already created!";
            return;
        }
        // qDebug() << "OSGViewport::initializeResources";

        // setup graphics context and camera
        gc = createGraphicsContext();

        cameraNode->setGraphicsContext(gc);

        // qDebug() << "OSGViewport::initializeResources - camera" << cameraNode->asCamera();
        view->setCamera(cameraNode->asCamera());

        // qDebug() << "OSGViewport::initializeResources - scene data" << sceneNode->node();
        view->setSceneData(sceneNode->node());

        if (manipulator) {
            osgGA::CameraManipulator *m = manipulator->asCameraManipulator();
            // qDebug() << "OSGViewport::initializeResources - manipulator" << m;

            // Setting the manipulator on the camera will change the manipulator node (used to compute the camera home position)
            // to the view scene node. So we need to save and restore the manipulator node.
            osg::Node *node = m->getNode();
            view->setCameraManipulator(m, false);
            if (node) {
                m->setNode(node);
            }

            view->home();
        } else {
            view->setCameraManipulator(NULL, false);
        }

        installHanders();

        startTimer();
    }

    void releaseResources()
    {
        // qDebug() << "OSGViewport::releaseResources";
        if (!gc.valid()) {
            qWarning() << "OSGViewport::releaseResources - gc is not valid!";
            return;
        }
        osg::deleteAllGLObjects(gc->getState()->getContextID());
        // view->getSceneData()->releaseGLObjects(view->getCamera()->getGraphicsContext()->getState());
        // view->getCamera()->releaseGLObjects(view->getCamera()->getGraphicsContext()->getState());
        // view->getCamera()->getGraphicsContext()->close();
        // view->getCamera()->setGraphicsContext(NULL);
    }

private:
    void createViewer()
    {
        if (viewer.valid()) {
            qWarning() << "OSGViewport::createViewer - viewer is valid";
            return;
        }
        // qDebug() << "OSGViewport::createViewer";

        viewer = new osgViewer::CompositeViewer();
        viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);


        // disable the default setting of viewer.done() by pressing Escape.
        viewer->setKeyEventSetsDone(0);
        // viewer->setQuitEventSetsDone(false);

        view = createView();
        viewer->addView(view);
    }

    void destroyViewer()
    {
        if (!viewer.valid()) {
            qWarning() << "OSGViewport::destroyViewer - viewer is not valid";
            return;
        }
        // qDebug() << "OSGViewport::destroyViewer";

        viewer = NULL;
    }

    osgViewer::View *createView()
    {
        // qDebug() << "OSGViewport::createView";
        osgViewer::View *view = new osgViewer::View();

        // TODO expose as Qml properties
        view->setLightingMode(osgViewer::View::SKY_LIGHT);
        view->getLight()->setAmbient(osg::Vec4(0.6f, 0.6f, 0.6f, 1.0f));

        return view;
    }

    void installHanders()
    {
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
    }

    osg::GraphicsContext *createGraphicsContext()
    {
        // qDebug() << "OSGViewport::createGraphicsContext";

        osg::GraphicsContext::Traits *traits = getTraits();

        // traitsInfo(*traits);

        traits->pbuffer = true;
        osg::GraphicsContext *graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
        // if (!graphicsContext) {
        // qWarning() << "Failed to create pbuffer, failing back to normal graphics window.";
        // traits->pbuffer = false;
        // graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
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

        int dpr = self->window()->devicePixelRatio();
        traits->width   = self->width() * dpr;
        traits->height  = self->height() * dpr;

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

    void startTimer()
    {
        if ((updateMode != UpdateMode::Continuous) && (frameTimer < 0)) {
            // qDebug() << "OSGViewport::startTimer - starting timer";
            frameTimer = QObject::startTimer(33, Qt::PreciseTimer);
        }
    }

    void stopTimer()
    {
        if (frameTimer >= 0) {
            // qDebug() << "OSGViewport::stopTimer - killing timer";
            QObject::killTimer(frameTimer);
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

    void onSceneNodeChanged(osg::Node *node)
    {
        qWarning() << "OSGViewport::onSceneNodeChanged - not implemented";
    }

    void onCameraNodeChanged(osg::Node *node)
    {
        qWarning() << "OSGViewport::onCameraNodeChanged - not implemented";
    }
};

/* class ViewportRenderer */

class ViewportRenderer : public QQuickFramebufferObject::Renderer {
private:
    OSGViewport::Hidden *const h;

    int frameCount;
    bool needToDoFrame;

public:
    ViewportRenderer(OSGViewport::Hidden *h) : h(h), frameCount(0), needToDoFrame(false)
    {
        // qDebug() << "ViewportRenderer::ViewportRenderer";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::ViewportRenderer");

        h->initializeResources();
    }

    ~ViewportRenderer()
    {
        // qDebug() << "ViewportRenderer::~ViewportRenderer";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::~ViewportRenderer");
    }

    // This function is the only place when it is safe for the renderer and the item to read and write each others members.
    void synchronize(QQuickFramebufferObject *item)
    {
        // qDebug() << "ViewportRenderer::synchronize";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::synchronize");

        if (!h->viewer.valid()) {
            qWarning() << "ViewportRenderer::synchronize - invalid viewer";
            return;
        }

        if (!h->view.valid()) {
            qWarning() << "ViewportRenderer::synchronize - invalid view";
            return;
        }

        // TODO this is not correct : switching workspaces in GCS will destroy and recreate the renderer (and frameCount is thus reset to 0).
        if (frameCount == 0) {
            h->view->init();
            if (!h->viewer->isRealized()) {
                h->viewer->realize();
            }
        }

        // we always want to draw the first frame
        needToDoFrame = (frameCount == 0);

        // if not on-demand then do frame
        if (h->updateMode != UpdateMode::OnDemand) {
            needToDoFrame = true;
        }

        // check if viewport needs to be resized
        // a redraw will be requested if necessary
        osg::Viewport *viewport = h->view->getCamera()->getViewport();
        if ((viewport->width() != item->width()) || (viewport->height() != item->height())) {
            // qDebug() << "*** RESIZE" << frameCount << viewport->width() << "x" << viewport->height() << "->" << item->width() << "x" << item->height();
            needToDoFrame = true;

            // h->view->getCamera()->resize(item->width(), item->height());
            int dpr = h->self->window()->devicePixelRatio();
            h->view->getCamera()->getGraphicsContext()->resized(0, 0, item->width() * dpr, item->height() * dpr);

            // trick to force a "home" on first few frames to absorb initial spurious resizes
            if (frameCount <= 2) {
                h->view->home();
            }
        }

        // refresh busy state
        // TODO state becomes busy when scene is loading or downloading tiles (should do it only for download)
        h->self->setBusy(h->view->getDatabasePager()->getRequestsInProgress());
        // TODO also expose request list size to Qml
        if (h->view->getDatabasePager()->getFileRequestListSize() > 0) {
            // qDebug() << h->view->getDatabasePager()->getFileRequestListSize();
        }

        if (!needToDoFrame) {
            needToDoFrame = h->viewer->checkNeedToDoFrame();
        }
        if (!needToDoFrame) {
            // workarounds to osg issues
            // issue 1 : if only root node has an update callback checkNeedToDoFrame should return true but does not
            // a fix will be submitted to osg (current version is 3.5.1)
            if (h->view->getSceneData()) {
                needToDoFrame |= !(h->view->getSceneData()->getUpdateCallback() == NULL);
            }
            // issue 2 : UI events don't trigger a redraw
            // this issue should be fixed here...
            // event handling needs a lot of attention :
            // - sometimes the scene is redrawing continuously (after a drag for example, and single click will stop continuous redraw)
            // - some events (simple click for instance) trigger a redraw when not needed
            // - in Earth View : continuous zoom (triggered by holding right button and moving mouse up/down) sometimes stops working when holding mouse still after initiating
            needToDoFrame = !h->view->getEventQueue()->empty();
        }
        if (needToDoFrame) {
            // qDebug() << "ViewportRenderer::synchronize - update scene" << frameCount;
            h->viewer->advance();
            h->viewer->eventTraversal();
            h->viewer->updateTraversal();
        }
    }

    // This function is called when the FBO should be rendered into.
    // The framebuffer is bound at this point and the glViewport has been set up to match the FBO size.
    void render()
    {
        // qDebug() << "ViewportRenderer::render";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::render");

        if (!h->viewer.valid()) {
            qWarning() << "ViewportRenderer::render - invalid viewer";
            return;
        }

        if (needToDoFrame) {
            // qDebug() << "ViewportRenderer::render - render scene" << frameCount;

            // needed to properly render models without terrain (Qt bug?)
            QOpenGLContext::currentContext()->functions()->glUseProgram(0);
            h->viewer->renderingTraversals();
            needToDoFrame = false;
        }

        if (h->updateMode == UpdateMode::Continuous) {
            // trigger next update
            update();
        }

        ++frameCount;
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {
        QOpenGLFramebufferObjectFormat format;

        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // format.setSamples(4);

        QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(size.width(), size.height(), format);

        return fbo;
    }
};

QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

/* class OSGViewport */

OSGViewport::OSGViewport(QQuickItem *parent) : Inherited(parent), h(new Hidden(this))
{
    // setClearBeforeRendering(false);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
    // qDebug() << "OSGViewport::~OSGViewport";
    delete h;
}

OSGNode *OSGViewport::sceneNode() const
{
    return h->sceneNode;
}

void OSGViewport::setSceneNode(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        // setDirty(Scene);
        emit sceneNodeChanged(node);
    }
}

OSGCamera *OSGViewport::cameraNode() const
{
    return h->cameraNode;
}

void OSGViewport::setCameraNode(OSGCamera *node)
{
    if (h->acceptCameraNode(node)) {
        // setDirty(Camera);
        emit cameraNodeChanged(node);
    }
}

OSGCameraManipulator *OSGViewport::manipulator() const
{
    return h->manipulator;
}

void OSGViewport::setManipulator(OSGCameraManipulator *manipulator)
{
    if (h->acceptManipulator(manipulator)) {
        // setDirty(Manipulator);
        emit manipulatorChanged(manipulator);
    }
}

UpdateMode::Enum OSGViewport::updateMode() const
{
    return h->updateMode;
}

void OSGViewport::setUpdateMode(UpdateMode::Enum mode)
{
    if (h->updateMode != mode) {
        h->updateMode = mode;
        emit updateModeChanged(mode);
    }
}

bool OSGViewport::busy() const
{
    return h->busy;
}

void OSGViewport::setBusy(const bool busy)
{
    if (h->busy != busy) {
        h->busy = busy;
        emit busyChanged(busy);
    }
}

osgViewer::View *OSGViewport::asView() const
{
    return h->view;
}

QQuickFramebufferObject::Renderer *OSGViewport::createRenderer() const
{
    // qDebug() << "OSGViewport::createRenderer";
    // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "createRenderer");
    return new ViewportRenderer(h);
}

void OSGViewport::releaseResources()
{
    // qDebug() << "OSGViewport::releaseResources" << this;
    Inherited::releaseResources();
}

void OSGViewport::classBegin()
{
    // qDebug() << "OSGViewport::classBegin" << this;
    Inherited::classBegin();
}

void OSGViewport::componentComplete()
{
    // qDebug() << "OSGViewport::componentComplete" << this;
    Inherited::componentComplete();
}

QPointF OSGViewport::mousePoint(QMouseEvent *event)
{
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
    // Inherited::keyPressEvent(event);
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
    // Inherited::keyReleaseEvent(event);
}
} // namespace osgQtQuick

#include "OSGViewport.moc"
