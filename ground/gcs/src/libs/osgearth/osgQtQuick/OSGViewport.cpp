/**
 ******************************************************************************
 *
 * @file       OSGViewport.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
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
#include <osg/ApplicationUsage>
#include <osg/Version>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgGA/CameraManipulator>
#include <osgUtil/IncrementalCompileOperation>

#include <QOpenGLContext>
#include <QQuickWindow>
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QOpenGLFunctions>

#include <QDebug>

#include <iterator>

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
// enum DirtyFlag { Scene = 1 << 0, Camera = 1 << 1, Manipulator = 1 << 2, UpdateMode = 1 << 3, IncrementalCompile = 1 << 4 };

class ViewportRenderer;

class MyViewer : public osgViewer::CompositeViewer {
public:
    MyViewer() : osgViewer::CompositeViewer()
    {}

    virtual bool checkNeedToDoFrame()
    {
        if (_requestRedraw) {
            return true;
        }
        if (_requestContinousUpdate) {
            return true;
        }

        for (RefViews::iterator itr = _views.begin(); itr != _views.end(); ++itr) {
            osgViewer::View *view = itr->get();
            if (view) {
                // If the database pager is going to update the scene the render flag is
                // set so that the updates show up
                if (view->getDatabasePager()->getDataToCompileListSize() > 0) {
                    return true;
                }
                if (view->getDatabasePager()->getDataToMergeListSize() > 0) {
                    return true;
                }
                // if (view->getDatabasePager()->requiresUpdateSceneGraph()) return true;
                // if (view->getDatabasePager()->getRequestsInProgress()) return true;

                // if there update callbacks then we need to do frame.
                if (view->getCamera()->getUpdateCallback()) {
                    return true;
                }
                if (view->getSceneData() && view->getSceneData()->getUpdateCallback()) {
                    return true;
                }
                if (view->getSceneData() && view->getSceneData()->getNumChildrenRequiringUpdateTraversal() > 0) {
                    return true;
                }
            }
        }

        // check if events are available and need processing
        if (checkEvents()) {
            return true;
        }

        if (_requestRedraw) {
            return true;
        }
        if (_requestContinousUpdate) {
            return true;
        }

        return false;
    }
};

struct OSGViewport::Hidden : public QObject {
    Q_OBJECT

    friend ViewportRenderer;

private:
    OSGViewport *const self;

    QQuickWindow *window;

    int frameTimer;

    int frameCount;

    osg::ref_ptr<osg::GraphicsContext> gc;

public:
    OSGNode   *sceneNode;
    OSGCamera *cameraNode;

    osg::ref_ptr<osgViewer::CompositeViewer> viewer;
    osg::ref_ptr<osgViewer::View> view;

    OSGCameraManipulator *manipulator;

    UpdateMode::Enum     updateMode;

    bool incrementalCompile;

    bool busy;

    static osg::ref_ptr<osg::GraphicsContext> dummyGC;

    static QtKeyboardMap keyMap;

    Hidden(OSGViewport *self) : QObject(self), self(self), window(NULL), frameTimer(-1), frameCount(0),
        sceneNode(NULL), cameraNode(NULL), manipulator(NULL),
        updateMode(UpdateMode::OnDemand), incrementalCompile(false), busy(false)
    {
        OsgEarth::initialize();

        // workaround to avoid using GraphicsContext #0
        // when switching tabs textures are not rebound (see https://librepilot.atlassian.net/secure/attachment/11500/lost_textures.png)
        // so we create and retain GraphicsContext #0 so it won't be used elsewhere
        if (!dummyGC.valid()) {
            dummyGC = createGraphicsContext();
        }

        createViewer();

        connect(self, &OSGViewport::windowChanged, this, &Hidden::onWindowChanged);
    }

    ~Hidden()
    {
        stopTimer();

        disconnect(self);
    }

private slots:
    void onWindowChanged(QQuickWindow *window)
    {
        // qDebug() << "OSGViewport::onWindowChanged" << window;
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "onWindowChanged");
        if (window) {
            // when hiding the QQuickWidget (happens when switching tab or re-parenting) the renderer is destroyed and sceneGraphInvalidated is signaled
            // same happens when deleting the QQuickWidget
            // problem is that there is no way to distinguish a hide and a delete so there is no good place to release gl objects, etc.
            // it can't be done from other destructors as the gl context is not active at that time
            // so we must delete the gl objects when hiding (and release it again when showing)
            // deletion of the osg viewer will happen when the QQuickWidget is deleted. the gl context will not be active but it is ok because the
            // viewer has no more gl objects to delete (we hope...)
            // bad side effect is that when showing the scene after hiding it there is delay (on heavy scenes) to realise again the gl context.
            // this is not happening on a separate thread (because of a limitation of QQuickWidget and Qt on windows related limitations)
            // a workaround would be to not invalidate the scene when hiding/showing but that is not working atm...
            // see https://bugreports.qt.io/browse/QTBUG-54133 for more details
            // window->setPersistentSceneGraph(true);

            // connect(window, &QQuickWindow::sceneGraphInitialized, this, &Hidden::onSceneGraphInitialized, Qt::DirectConnection);
            connect(window, &QQuickWindow::sceneGraphInvalidated, this, &Hidden::onSceneGraphInvalidated, Qt::DirectConnection);
            // connect(window, &QQuickWindow::afterSynchronizing, this, &Hidden::onAfterSynchronizing, Qt::DirectConnection);
            connect(window, &QQuickWindow::afterSynchronizing, this, &Hidden::onAfterSynchronizing, Qt::DirectConnection);
        }
        this->window = window;
    }

    // emitted from the scene graph rendering thread (gl context bound)
    void onSceneGraphInitialized()
    {
        // qDebug() << "OSGViewport::onSceneGraphInitialized";
        initializeResources();
    }

    // emitted from the scene graph rendering thread (gl context bound)
    void onSceneGraphInvalidated()
    {
        // qDebug() << "OSGViewport::onSceneGraphInvalidated";
        releaseResources();
    }

    // emitted from the scene graph rendering thread (gl context bound)
    void onAfterSynchronizing()
    {
        // qDebug() << "OSGViewport::onAfterSynchronizing";
    }

public:
    bool acceptSceneNode(OSGNode *node)
    {
        // qDebug() << "OSGViewport::acceptSceneNode" << node;
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
        // qDebug() << "OSGViewport::acceptCameraNode" << node;
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
        // qDebug() << "OSGViewport::acceptManipulator" << manipulator;
        if (manipulator == m) {
            return true;
        }

        manipulator = m;

        return true;
    }

private:
    void initializeResources()
    {
        // qDebug() << "OSGViewport::Hidden::initializeResources";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "OSGViewport::Hidden::initializeResources");
        if (gc.valid()) {
            // qWarning() << "OSGViewport::initializeResources - gc already created!";
            return;
        }

        // setup graphics context and camera
        gc = createGraphicsContext();

        // connect(QOpenGLContext::currentContext(), &QOpenGLContext::aboutToBeDestroyed, this, &Hidden::onAboutToBeDestroyed, Qt::DirectConnection);

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

        view->init();
        viewer->realize();

        startTimer();
    }

    void onAboutToBeDestroyed()
    {
        // qDebug() << "OSGViewport::Hidden::onAboutToBeDestroyed";
        osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "OSGViewport::Hidden::onAboutToBeDestroyed");
        // context is not current and don't know how to make it current...
    }

    // see https://github.com/openscenegraph/OpenSceneGraph/commit/161246d864ea0514543ed0493422e1bf0e99afb7#diff-91ee382a4d543072ea66aab422e5106f
    // see https://github.com/openscenegraph/OpenSceneGraph/commit/3e0435febd677f14aae5f42ef1f43e81307fec41#diff-cadcd928403543a531cf42712a3d1126
    void releaseResources()
    {
        // qDebug() << "OSGViewport::Hidden::releaseResources";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "OSGViewport::Hidden::releaseResources");
        if (!gc.valid()) {
            qWarning() << "OSGViewport::Hidden::releaseResources - gc is not valid!";
            return;
        }
        deleteAllGLObjects();
    }

    // there should be a simpler way to do that...
    // for now, we mimic what is done in GraphicsContext::close()
    // calling gc->close() and later gc->realize() does not work
    void deleteAllGLObjects()
    {
        // TODO switch off the graphics thread (see GraphicsContext::close()) ?
        // setGraphicsThread(0);

        for (osg::GraphicsContext::Cameras::iterator itr = gc->getCameras().begin();
             itr != gc->getCameras().end();
             ++itr) {
            osg::Camera *camera = (*itr);
            if (camera) {
                OSG_INFO << "Releasing GL objects for Camera=" << camera << " _state=" << gc->getState() << std::endl;
                camera->releaseGLObjects(gc->getState());
            }
        }
#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 0)
        gc->getState()->releaseGLObjects();
        osg::deleteAllGLObjects(gc->getState()->getContextID());
        osg::flushAllDeletedGLObjects(gc->getState()->getContextID());
        osg::discardAllGLObjects(gc->getState()->getContextID());
#endif
    }

    void createViewer()
    {
        if (viewer.valid()) {
            qWarning() << "OSGViewport::createViewer - viewer is valid";
            return;
        }
        // qDebug() << "OSGViewport::createViewer";

        viewer = new MyViewer();
        // viewer = new osgViewer::CompositeViewer();

        viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

        // disable the default setting of viewer.done() by pressing Escape.
        viewer->setKeyEventSetsDone(0);
        // viewer->setQuitEventSetsDone(false);

        view = createView();
        viewer->addView(view);
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

#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 3)
        // The MyQt windowing system is registered in osgearth.cpp
        traits->windowingSystemPreference = "MyQt";
#endif
        traits->windowDecoration = false;
        traits->x       = 0;
        traits->y       = 0;

        int dpr = self->window() ? self->window()->devicePixelRatio() : 1;
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
        if ((frameTimer < 0) && (updateMode != UpdateMode::Continuous)) {
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

    bool initFrame;
    bool needToDoFrame;

public:
    ViewportRenderer(OSGViewport::Hidden *h) : h(h), initFrame(true), needToDoFrame(false)
    {
        // qDebug() << "ViewportRenderer::ViewportRenderer";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::ViewportRenderer");
        h->initializeResources();
    }

    ~ViewportRenderer()
    {
        // qDebug() << "ViewportRenderer::~ViewportRenderer";
        // osgQtQuick::openGLContextInfo(QOpenGLContext::currentContext(), "ViewportRenderer::~ViewportRenderer");
        // h->releaseResources();
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

        if (initFrame) {
            // workaround for https://bugreports.qt.io/browse/QTBUG-54073
            // busy indicator starting to spin indefinitly when switching tabs
            h->self->setBusy(true);
            h->self->setBusy(false);
        }

        // NOTES:
        // - needToDoFrame is always orred so "last" value is preserved. Make sure to set it to false if needed.
        // - when switching tabs or re-parenting, the renderer is destroyed and recreated
        // some special care is taken in case a renderer is (re)created;
        // a new renderer will have initFrame set to true for the duration of the first frame

        // draw first frame of freshly initialized renderer
        needToDoFrame |= initFrame;

        // if not on-demand then do frame
        if (h->updateMode != UpdateMode::OnDemand) {
            needToDoFrame = true;
        }

        // check if viewport needs to be resized
        // a redraw will be requested if necessary
        // not really event driven...
        int dpr    = h->self->window()->devicePixelRatio();
        int width  = item->width() * dpr;
        int height = item->height() * dpr;
        osg::Viewport *viewport = h->view->getCamera()->getViewport();
        if (initFrame || (viewport->width() != width) || (viewport->height() != height)) {
            // qDebug() << "*** RESIZE" << h->frameCount << << initFrame << viewport->width() << "x" << viewport->height() << "->" << width << "x" << height;
            needToDoFrame = true;

            h->gc->resized(0, 0, width, height);
            h->view->getEventQueue()->windowResize(0, 0, width, height /*, resizeTime*/);

            // trick to force a "home" on first few frames to absorb initial spurious resizes
            if (h->frameCount <= 2) {
                h->view->home();
            }
        }

        if (!needToDoFrame) {
            // issue : UI events don't trigger a redraw
            // this issue should be fixed here...
            // event handling needs a lot of attention :
            // - sometimes the scene is redrawing continuously (after a drag for example, and single click will stop continuous redraw)
            // - some events (simple click for instance) trigger a redraw when not needed
            // - in Earth View : continuous zoom (triggered by holding right button and moving mouse up/down) sometimes stops working when holding mouse still after initiating
            needToDoFrame = !h->view->getEventQueue()->empty();
        }

        if (!needToDoFrame) {
            needToDoFrame = h->viewer->checkNeedToDoFrame();
        }
        if (needToDoFrame) {
            // qDebug() << "ViewportRenderer::synchronize - update scene" << h->frameCount;
            h->viewer->advance();
            h->viewer->eventTraversal();
            h->viewer->updateTraversal();
        }

        // refresh busy state
        // TODO state becomes busy when scene is loading or downloading tiles (should do it only for download)
        // TODO also expose request list size to Qml
        h->self->setBusy(h->view->getDatabasePager()->getRequestsInProgress());
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
            // qDebug() << "ViewportRenderer::render - render scene" << h->frameCount;

            // needed to properly render models without terrain (Qt bug?)
            QOpenGLContext::currentContext()->functions()->glUseProgram(0);

            h->viewer->renderingTraversals();

            needToDoFrame = false;
        }

        if (h->updateMode == UpdateMode::Continuous) {
            // trigger next update
            update();
        }

        ++(h->frameCount);
        initFrame = false;
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

osg::ref_ptr<osg::GraphicsContext> OSGViewport::Hidden::dummyGC;

/* class OSGViewport */

OSGViewport::OSGViewport(QQuickItem *parent) : Inherited(parent), h(new Hidden(this))
{
    // setClearBeforeRendering(false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    setMirrorVertically(true);
#endif
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
QSGNode *OSGViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
{
    if (!node) {
        node = QQuickFramebufferObject::updatePaintNode(node, nodeData);
        QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode *>(node);
        if (n) {
            n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        }
        return node;
    }
    return QQuickFramebufferObject::updatePaintNode(node, nodeData);
}
#endif

OSGViewport::~OSGViewport()
{
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

bool OSGViewport::incrementalCompile() const
{
    return h->incrementalCompile;
}

void OSGViewport::setIncrementalCompile(bool incrementalCompile)
{
    if (h->incrementalCompile != incrementalCompile) {
        h->incrementalCompile = incrementalCompile;
        // setDirty(IncrementalCompile);
        // TODO not thread safe...
        h->viewer->setIncrementalCompileOperation(incrementalCompile ? new osgUtil::IncrementalCompileOperation() : NULL);
        emit incrementalCompileChanged(incrementalCompile);
    }
}

bool OSGViewport::busy() const
{
    return h->busy;
}

void OSGViewport::setBusy(bool busy)
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
        h->view->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
    }
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    setKeyboardModifiers(event);
    QPointF pos = mousePoint(event);
    if (h->view.valid()) {
        h->view->getEventQueue()->mouseMotion(pos.x(), pos.y());
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
        h->view->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
    }
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    osgGA::GUIEventAdapter::ScrollingMotion motion =
        event->orientation() == Qt::Vertical ?
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT);

    if (h->view.valid()) {
        h->view->getEventQueue()->mouseScroll(motion);
    }
}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    setKeyboardModifiers(event);
    int value = h->keyMap.remapKey(event);
    if (h->view.valid()) {
        h->view->getEventQueue()->keyPress(value);
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
            h->view->getEventQueue()->keyRelease(value);
        }
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
    // if( _forwardKeyEvents )
    // Inherited::keyReleaseEvent(event);
}

QPointF OSGViewport::mousePoint(QMouseEvent *event)
{
    qreal x, y;

    if (h->view.valid() && h->view->getEventQueue()->getUseFixedMouseInputRange()) {
        x = 2.0 * (event->x() - width() / 2) / width();
        y = 2.0 * (event->y() - height() / 2) / height();
    } else {
        x = event->x();
        y = event->y();
    }
    return QPointF(x, y);
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
        h->view->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
    }
}
} // namespace osgQtQuick

#include "OSGViewport.moc"
