/**
 ******************************************************************************
 *
 * @file       qtwindowingsystem.cpp
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

#include "qtwindowingsystem.h"

#include "utility.h"

#include <osg/DeleteHandler>
#include <osg/Version>
#include <osgViewer/GraphicsWindow>

#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QDebug>

#include <string>

// TODO no need to derive from GraphicsWindow (derive directly from GraphicsContext instead)
class GraphicsWindowQt : public osgViewer::GraphicsWindow {
public:
    GraphicsWindowQt(osg::GraphicsContext::Traits *traits);
    ~GraphicsWindowQt();

    bool isSameKindAs(const Object *object) const
    {
        return dynamic_cast<const GraphicsWindowQt *>(object) != 0;
    }
    const char *libraryName() const
    {
        return "osgViewerXYZ";
    }
    const char *className() const
    {
        return "GraphicsWindowQt";
    }

    bool setWindowRectangleImplementation(int x, int y, int width, int height);
    void getWindowRectangle(int & x, int & y, int & width, int & height);
    bool setWindowDecorationImplementation(bool windowDecoration);
    bool getWindowDecoration() const;
    void grabFocus();
    void grabFocusIfPointerInWindow();
    void raiseWindow();
    void setWindowName(const std::string & name);
    std::string getWindowName();
    void useCursor(bool cursorOn);
    void setCursor(MouseCursor cursor);
    bool valid() const;
    bool realizeImplementation();
    bool isRealizedImplementation() const;
    void closeImplementation();
    bool makeCurrentImplementation();
    bool releaseContextImplementation();
    void swapBuffersImplementation();
    void runOperations();

    void requestWarpPointer(float x, float y);

protected:
    void init();

    bool _initialized;
    bool _valid;
    bool _realized;

    bool _owned;

    QOpenGLContext *_glContext;
    QOffscreenSurface *_surface;
};

GraphicsWindowQt::GraphicsWindowQt(osg::GraphicsContext::Traits *traits) :
    _initialized(false),
    _valid(false),
    _realized(false),
    _owned(false),
    _glContext(NULL),
    _surface(NULL)
{
    // qDebug() << "GraphicsWindowQt::GraphicsWindowQt";
    _traits = traits;
    init();
}

GraphicsWindowQt::~GraphicsWindowQt()
{
    // qDebug() << "GraphicsWindowQt::~GraphicsWindowQt";
    close(true);
}

void GraphicsWindowQt::init()
{
    // qDebug() << "GraphicsWindowQt::init";
    if (_initialized) {
        return;
    }

    // update by WindowData
    // WindowData* windowData = _traits.get() ? dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : 0;
    // if ( !_widget )
    // _widget = windowData ? windowData->_widget : NULL;
    // if ( !parent )
    // parent = windowData ? windowData->_parent : NULL;


    setState(new osg::State);
    getState()->setGraphicsContext(this);

    if (_traits.valid() && _traits->sharedContext.valid()) {
        getState()->setContextID(_traits->sharedContext->getState()->getContextID());
        incrementContextIDUsageCount(getState()->getContextID());
    } else {
        getState()->setContextID(osg::GraphicsContext::createNewContextID());
    }

    // make sure the event queue has the correct window rectangle size and input range
#if OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#else
    getEventQueue()->syncWindowRectangleWithGraphcisContext();
#endif

    _initialized = true;

    _valid = _initialized;
}

bool GraphicsWindowQt::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    return true;
}

void GraphicsWindowQt::getWindowRectangle(int & x, int & y, int & width, int & height)
{}

bool GraphicsWindowQt::setWindowDecorationImplementation(bool windowDecoration)
{
    return false;
}

bool GraphicsWindowQt::getWindowDecoration() const
{
    return false;
}

void GraphicsWindowQt::grabFocus()
{}

void GraphicsWindowQt::grabFocusIfPointerInWindow()
{}

void GraphicsWindowQt::raiseWindow()
{}

void GraphicsWindowQt::setWindowName(const std::string & name)
{}

std::string GraphicsWindowQt::getWindowName()
{
    return "";
}

void GraphicsWindowQt::useCursor(bool cursorOn)
{}

void GraphicsWindowQt::setCursor(MouseCursor cursor)
{}

bool GraphicsWindowQt::valid() const
{
    return _valid;
}

bool GraphicsWindowQt::realizeImplementation()
{
    // qDebug() << "GraphicsWindowQt::realizeImplementation";
    // save the current context
    // note: this will save only Qt-based contexts

    if (_realized) {
        return true;
    }

    if (!_initialized) {
        init();
        if (!_initialized) {
            return false;
        }
    }

    QOpenGLContext *currentContext = QOpenGLContext::currentContext();

    if (!currentContext) {
        // qDebug() << "GraphicsWindowQt::realizeImplementation - creating owned context";
        _owned     = true;
        _glContext = new QOpenGLContext();
        _glContext->create();
        _surface   = new QOffscreenSurface();
        _surface->setFormat(_glContext->format());
        _surface->create();
#ifdef OSG_VERBOSE
        osgQtQuick::formatInfo(_surface->format());
#endif
    } else {
        // qDebug() << "GraphicsWindowQt::realizeImplementation - using current context";
        _glContext = currentContext;
    }

    //// initialize GL context for the widget
    // if ( !valid() )
    // _widget->glInit();

    // make current
    _realized = true;
    bool current = makeCurrent();

    // fail if we do not have current context
    if (!current) {
        // if ( savedContext )
        // const_cast< QGLContext* >( savedContext )->makeCurrent();
        //
        qWarning() << "GraphicsWindowQt::realizeImplementation - can not make context current.";
        _realized = false;
        return false;
    }

    // make sure the event queue has the correct window rectangle size and input range
#if OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#else
    getEventQueue()->syncWindowRectangleWithGraphcisContext();
#endif

    // make this window's context not current
    // note: this must be done as we will probably make the context current from another thread
    // and it is not allowed to have one context current in two threads
    if (!releaseContext()) {
        qWarning() << "GraphicsWindowQt::realizeImplementation - can not release context.";
    }

//// restore previous context
// if ( savedContext )
// const_cast< QGLContext* >( savedContext )->makeCurrent();

    return _realized;
}

bool GraphicsWindowQt::isRealizedImplementation() const
{
    return _realized;
}

void GraphicsWindowQt::runOperations()
{
    // While in graphics thread this is last chance to do something useful before
    // graphics thread will execute its operations.
// if (_widget->getNumDeferredEvents() > 0)
// _widget->processDeferredEvents();
//
// if (QGLContext::currentContext() != _widget->context())
// _widget->makeCurrent();
//
    // qDebug() << "GraphicsWindowQt::runOperations";
    GraphicsWindow::runOperations();
}

bool GraphicsWindowQt::makeCurrentImplementation()
{
    if (!_glContext) {
        qWarning() << "GraphicsWindowQt::makeCurrentImplementation() - no context.";
        return false;
    }
    if (!_realized) {
        qWarning() << "GraphicsWindowQt::makeCurrentImplementation() - not realized; cannot make current.";
        return false;
    }
    if (_owned) {
        if (!_glContext->makeCurrent(_surface)) {
            qWarning() << "GraphicsWindowQt::makeCurrentImplementation : failed to make context current";
            return false;
        }
    }
    if (_glContext != QOpenGLContext::currentContext()) {
        qWarning() << "GraphicsWindowQt::makeCurrentImplementation : context is not current";
        // abort();
        return false;
    }
    return true;
}

bool GraphicsWindowQt::releaseContextImplementation()
{
    if (!_glContext) {
        qWarning() << "GraphicsWindowQt::releaseContextImplementation() - no context.";
        return false;
    }
    if (_glContext != QOpenGLContext::currentContext()) {
        qWarning() << "GraphicsWindowQt::releaseContextImplementation : context is not current";
        return false;
    }
    if (_owned) {
        // qDebug() << "GraphicsWindowQt::releaseContextImplementation";
        _glContext->doneCurrent();
        if (_glContext == QOpenGLContext::currentContext()) {
            qWarning() << "GraphicsWindowQt::releaseContextImplementation : context is still current";
        }
    }
    return true;
}

void GraphicsWindowQt::closeImplementation()
{
    // qDebug() << "GraphicsWindowQt::closeImplementation";
    _initialized = false;
    _valid = false;
    _realized    = false;
    if (_owned) {
        if (_glContext) {
            // qDebug() << "GraphicsWindowQt::closeImplementation - deleting owned context";
            delete _glContext;
        }
        if (_surface) {
            _surface->destroy();
            delete _surface;
        }
    }
    _glContext = NULL;
    _surface   = NULL;
}

void GraphicsWindowQt::swapBuffersImplementation()
{
// _widget->swapBuffers();
//
//// FIXME: the processDeferredEvents should really be executed in a GUI (main) thread context but
//// I couln't find any reliable way to do this. For now, lets hope non of *GUI thread only operations* will
//// be executed in a QGLWidget::event handler. On the other hand, calling GUI only operations in the
//// QGLWidget event handler is an indication of a Qt bug.
// if (_widget->getNumDeferredEvents() > 0)
// _widget->processDeferredEvents();
//
//// We need to call makeCurrent here to restore our previously current context
//// which may be changed by the processDeferredEvents function.
// if (QGLContext::currentContext() != _widget->context())
// _widget->makeCurrent();
}

void GraphicsWindowQt::requestWarpPointer(float x, float y)
{}

QtWindowingSystem::QtWindowingSystem()
{}

QtWindowingSystem::~QtWindowingSystem()
{
    if (osg::Referenced::getDeleteHandler()) {
        osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
        osg::Referenced::getDeleteHandler()->flushAll();
    }
}

// Return the number of screens present in the system
unsigned int QtWindowingSystem::getNumScreens(const osg::GraphicsContext::ScreenIdentifier & /*si*/)
{
    qWarning() << "osgQt: getNumScreens() not implemented yet.";
    return 0;
}

// Return the resolution of specified screen
// (0,0) is returned if screen is unknown
void QtWindowingSystem::getScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/)
{
    qWarning() << "osgQt: getScreenSettings() not implemented yet.";
}

// Set the resolution for given screen
bool QtWindowingSystem::setScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/)
{
    qWarning() << "osgQt: setScreenSettings() not implemented yet.";
    return false;
}

// Enumerates available resolutions
void QtWindowingSystem::enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/)
{
    qWarning() << "osgQt: enumerateScreenSettings() not implemented yet.";
}

// Create a graphics context with given traits
osg::GraphicsContext *QtWindowingSystem::createGraphicsContext(osg::GraphicsContext::Traits *traits)
{
    // if (traits->pbuffer)
    // {
    // OSG_WARN << "osgQt: createGraphicsContext - pbuffer not implemented yet.";
    // return NULL;
    // }
    // else
    // {
    osg::ref_ptr< GraphicsWindowQt > gc = new GraphicsWindowQt(traits);

    if (gc->valid()) {
        return gc.release();
    } else {
        return NULL;
    }
    // }
}
