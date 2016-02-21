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

#ifdef USE_OSGEARTH
#include <osgEarth/MapNode>
#endif

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QQuickFramebufferObject>
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

   Adjust the near/far ratio of the camera. Look at osgearth_viewer.cpp to see how.
   Use LogarythmicDepthBuffer.

   More complex : you can try parenting your helicopter with an osg::Camera in NESTED mode,
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
        updateMode(UpdateMode::Discrete),
        frameTimer(-1)
    {
        OsgEarth::initialize();

        createViewer();

        connect(quickItem, &OSGViewport::windowChanged, this, &Hidden::onWindowChanged);
    }

    ~Hidden()
    {
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
        // attach scene
        if (!attach(view, sceneData->node())) {
            qWarning() << "OSGViewport::attach - failed to attach node!";
            return false;
        }
        // attach camera
        if (camera) {
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

#ifdef USE_OSGEARTH
        // TODO map handling should not be done here
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (mapNode) {
            qDebug() << "OSGViewport::attach - found map node" << mapNode;

            // remove light to prevent unnecessary state changes in SceneView
            // scene will get light from sky
            view->setLightingMode(osg::View::NO_LIGHT);
        }
#endif

        qDebug() << "OSGViewport::attach - set scene" << node;
        view->setSceneData(node);

        return true;
    }

    bool detach(osgViewer::View *view)
    {
        qDebug() << "OSGViewport::detach" << view;
        if (camera) {
            camera->detach(view);
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
        if (view.valid()) {
            qWarning() << "OSGViewport::initializeResources - view already created!";
            return;
        }
        view = createView();
        viewer->addView(view);
        self->attach(view.get());
        start();
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

    bool acceptUpdateMode(UpdateMode::Enum mode)
    {
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

    OSGViewport      *self;

    QQuickWindow     *window;

    OSGNode          *sceneData;
    OSGCamera        *camera;

    UpdateMode::Enum updateMode;

    int frameTimer;

    osg::ref_ptr<osgViewer::CompositeViewer> viewer;
    osg::ref_ptr<osgViewer::View> view;

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
        qDebug() << "OSGViewport::createView";
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

        // setup graphics context and camera
        osg::GraphicsContext *gc = createGraphicsContext();

        osg::Camera *camera = view->getCamera();
        camera->setGraphicsContext(gc);
        camera->setViewport(0, 0, gc->getTraits()->width, gc->getTraits()->height);

        return view;
    }

    osg::GraphicsContext *createGraphicsContext()
    {
        qDebug() << "OSGViewport::createGraphicsContext";

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
        traits->width   = self->width();
        traits->height  = self->height();

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

    void start()
    {
        if (updateMode == UpdateMode::Discrete && (frameTimer < 0)) {
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

        firstFrame    = true;
        needToDoFrame = false;
    }

    ~ViewportRenderer()
    {
        qDebug() << "ViewportRenderer::~ViewportRenderer";
        osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::~ViewportRenderer");
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

        needToDoFrame = h->viewer->checkNeedToDoFrame();
        if (needToDoFrame) {
            if (firstFrame) {
                h->view->init();
                if (!h->viewer->isRealized()) {
                    h->viewer->realize();
                }
                firstFrame = false;
            }
            osg::Viewport *viewport = h->view->getCamera()->getViewport();
            if ((viewport->width() != item->width()) || (viewport->height() != item->height())) {
                h->view->getCamera()->getGraphicsContext()->resized(0, 0, item->width(), item->height());
            }

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
            // needed to properly render models without terrain (Qt bug?)
            QOpenGLContext::currentContext()->functions()->glUseProgram(0);
            h->viewer->renderingTraversals();
            needToDoFrame = false;
        }

        if (h->updateMode == UpdateMode::Continuous) {
            // trigger next update
            update();
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // format.setSamples(4);

        // Keeping this for reference :
        // Mac need(ed) to have devicePixelRatio (dpr) taken into account (i.e. dpr = 2).
        // Further tests on Mac have shown that although dpr is still 2 it should not be used to scale the fbo.
        // Note that getting the window to get the devicePixelRatio is not great (messing with windows is often a bad idea...)
        int dpr = 1; // h->self->window()->devicePixelRatio();
        QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(size.width() / dpr, size.height() / dpr, format);

        return fbo;
    }

private:
    OSGViewport::Hidden *h;

    bool firstFrame;
    bool needToDoFrame;
};

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

UpdateMode::Enum OSGViewport::updateMode() const
{
    return h->updateMode;
}

void OSGViewport::setUpdateMode(UpdateMode::Enum mode)
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
        QObject *object = i.next();
        OSGNode *node   = qobject_cast<OSGNode *>(object);
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
        QObject *object = i.next();
        OSGNode *node   = qobject_cast<OSGNode *>(object);
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
