/**
 ******************************************************************************
 *
 * @file       osgearth.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief osgearth utilities
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

#include "osgearth.h"

#include "utils/gcsdirs.h"
#include "utils/pathutils.h"

#include "osgQtQuick/Utility.hpp"

#include <osg/DisplaySettings>
#include <osg/GraphicsContext>
#include <osg/Version>
#include <osg/Notify>
#include <osg/DeleteHandler>
#include <osgDB/Registry>
#include <osgViewer/GraphicsWindow>

#include <osgEarth/Version>
#include <osgEarth/Cache>
#include <osgEarth/Capabilities>
#include <osgEarth/Registry>
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>

#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QDebug>

#ifdef OSG_USE_QT_PRIVATE
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#endif

#include <deque>
#include <string>

bool OsgEarth::registered  = false;
bool OsgEarth::initialized = false;

class OSGEARTH_LIB_EXPORT QtNotifyHandler : public osg::NotifyHandler {
public:
    void notify(osg::NotifySeverity severity, const char *message);
};

void OsgEarth::registerQmlTypes()
{
    // TOOO not thread safe...
    if (registered) {
        return;
    }
    registered = true;

    // initialize();

    // Register Qml types
    osgQtQuick::registerTypes("osgQtQuick");
}

void OsgEarth::initialize()
{
    // TOOO not thread safe...
    if (initialized) {
        return;
    }
    initialized = true;

    qDebug() << "Initializing osgearth...";

    osg::DisplaySettings::instance()->setNumOfDatabaseThreadsHint(16);
    osg::DisplaySettings::instance()->setNumOfHttpDatabaseThreadsHint(8);

    initializePathes();

    initWindowingSystem();

    initializeCache();

    // force early initialization of osgEarth capabilities
    // Doing this too early (before main window is displayed) causes rendering glitches (black holes)
    // Not sure why... See OSGViewport for when it is called (late...)
    osgEarth::Registry::capabilities();

    displayInfo();
}

void OsgEarth::initializePathes()
{
    // initialize data file path list
    osgDB::FilePathList &dataFilePathList = osgDB::Registry::instance()->getDataFilePathList();

    dataFilePathList.push_front(GCSDirs::sharePath("osgearth").toStdString());
    dataFilePathList.push_front(GCSDirs::sharePath("osgearth/data").toStdString());

    // initialize library file path list
    osgDB::FilePathList &libraryFilePathList = osgDB::Registry::instance()->getLibraryFilePathList();
    libraryFilePathList.push_front(GCSDirs::libraryPath("osg").toStdString());
}

void OsgEarth::initializeCache()
{
    QString cachePath = Utils::PathUtils().GetStoragePath() + "osgearth/cache";

    osgEarth::Drivers::FileSystemCacheOptions cacheOptions;

    cacheOptions.rootPath() = cachePath.toStdString();

    osg::ref_ptr<osgEarth::Cache> cache = osgEarth::CacheFactory::create(cacheOptions);
    if (cache->isOK()) {
        // set cache
        osgEarth::Registry::instance()->setCache(cache.get());

        // set cache policy
        const osgEarth::CachePolicy cachePolicy(osgEarth::CachePolicy::USAGE_READ_WRITE);

        // The default cache policy used when no policy is set elsewhere
        osgEarth::Registry::instance()->setDefaultCachePolicy(cachePolicy);
        // The override cache policy (overrides all others if set)
        // osgEarth::Registry::instance()->setOverrideCachePolicy(cachePolicy);
    } else {
        qWarning() << "OsgEarth::initializeCache - Failed to initialize cache";
    }

// osgDB::SharedStateManager::ShareMode shareMode = osgDB::SharedStateManager::SHARE_NONE;// =osgDB::SharedStateManager::SHARE_ALL;
// shareMode = true ? static_cast<osgDB::SharedStateManager::ShareMode>(shareMode | osgDB::SharedStateManager::SHARE_STATESETS) : shareMode;
// shareMode = true ? static_cast<osgDB::SharedStateManager::ShareMode>(shareMode | osgDB::SharedStateManager::SHARE_TEXTURES) : shareMode;
// osgDB::Registry::instance()->getOrCreateSharedStateManager()->setShareMode(shareMode);

// osgDB::Options::CacheHintOptions cacheHintOptions = osgDB::Options::CACHE_NONE;
// cacheHintOptions = static_cast<osgDB::Options::CacheHintOptions>(cacheHintOptions | osgDB::Options::CACHE_IMAGES);
// cacheHintOptions = static_cast<osgDB::Options::CacheHintOptions>(cacheHintOptions | osgDB::Options::CACHE_NODES);
// if (osgDB::Registry::instance()->getOptions() == 0) {
// osgDB::Registry::instance()->setOptions(new osgDB::Options());
// }
// osgDB::Registry::instance()->getOptions()->setObjectCacheHint(cacheHintOptions);
}

void OsgEarth::displayInfo()
{
    qDebug() << "Using osg version :" << osgGetVersion();
    qDebug() << "Using osgEarth version :" << osgEarthGetVersion();

    // library file path list
    osgDB::FilePathList &libraryFilePathList = osgDB::Registry::instance()->getLibraryFilePathList();
    osgDB::FilePathList::iterator it = libraryFilePathList.begin();
    while (it != libraryFilePathList.end()) {
        qDebug() << "osg library file path:" << QString::fromStdString(*it);
        it++;
    }

    // data file path list
    osgDB::FilePathList &dataFilePathList = osgDB::Registry::instance()->getDataFilePathList();
    it = dataFilePathList.begin();
    while (it != dataFilePathList.end()) {
        qDebug() << "osg data file path:" << QString::fromStdString(*it);
        it++;
    }

    // qDebug() << "osg cache:" << qgetenv("OSGEARTH_CACHE_PATH");

    qDebug() << "osg database threads:" << osg::DisplaySettings::instance()->getNumOfDatabaseThreadsHint();
    qDebug() << "osg http database threads:" << osg::DisplaySettings::instance()->getNumOfHttpDatabaseThreadsHint();

    // qDebug() << "Platform supports GLSL:" << osgEarth::Registry::capabilities().supportsGLSL();

#ifdef OSG_USE_QT_PRIVATE
    bool threadedOpenGL = QGuiApplicationPrivate::platform_integration->hasCapability(QPlatformIntegration::ThreadedOpenGL);
    qDebug() << "Platform supports threaded OpenGL:" << threadedOpenGL;
#endif

    osgQtQuick::capabilitiesInfo(osgEarth::Registry::capabilities());
}

void QtNotifyHandler::notify(osg::NotifySeverity severity, const char *message)
{
    QString msg(message);

    // right trim message...
    int n = msg.size() - 1;

    for (; n >= 0; --n) {
        if (!msg.at(n).isSpace()) {
            break;
        }
    }
    msg = msg.left(n + 1);

    switch (severity) {
    case osg::ALWAYS:
        qDebug().noquote() << "[OSG]" << msg;
        break;
    case osg::FATAL:
        qCritical().noquote() << "[OSG FATAL]" << msg;
        break;
    case osg::WARN:
        qWarning().noquote() << "[OSG WARN]" << msg;
        break;
    case osg::NOTICE:
        qDebug().noquote() << "[OSG NOTICE]" << msg;
        break;
    case osg::INFO:
        qDebug().noquote() << "[OSG]" << msg;
        break;
    case osg::DEBUG_INFO:
        qDebug().noquote() << "[OSG DEBUG INFO]" << msg;
        break;
    case osg::DEBUG_FP:
        qDebug().noquote() << "[OSG DEBUG FP]" << msg;
        break;
    }
}

// TODO no need to derive from GraphicsWindow (derive directly from GraphicsContext instead)
class OSGEARTH_LIB_EXPORT GraphicsWindowQt : public osgViewer::GraphicsWindow {
public:
    GraphicsWindowQt(osg::GraphicsContext::Traits *traits);
    virtual ~GraphicsWindowQt();

    virtual bool isSameKindAs(const Object *object) const
    {
        return dynamic_cast<const GraphicsWindowQt *>(object) != 0;
    }
    virtual const char *libraryName() const
    {
        return "osgViewerXYZ";
    }
    virtual const char *className() const
    {
        return "GraphicsWindowQt";
    }

    virtual bool setWindowRectangleImplementation(int x, int y, int width, int height);
    virtual void getWindowRectangle(int & x, int & y, int & width, int & height);
    virtual bool setWindowDecorationImplementation(bool windowDecoration);
    virtual bool getWindowDecoration() const;
    virtual void grabFocus();
    virtual void grabFocusIfPointerInWindow();
    virtual void raiseWindow();
    virtual void setWindowName(const std::string & name);
    virtual std::string getWindowName();
    virtual void useCursor(bool cursorOn);
    virtual void setCursor(MouseCursor cursor);
    virtual bool valid() const;
    virtual bool realizeImplementation();
    virtual bool isRealizedImplementation() const;
    virtual void closeImplementation();
    virtual bool makeCurrentImplementation();
    virtual bool releaseContextImplementation();
    virtual void swapBuffersImplementation();
    virtual void runOperations();

    virtual void requestWarpPointer(float x, float y);

protected:
    void init();

    bool _initialized;
    bool _valid;
    bool _realized;
    bool _closing;

    bool _owned;

    QOpenGLContext *_glContext;
    QOffscreenSurface *_surface;
};

GraphicsWindowQt::GraphicsWindowQt(osg::GraphicsContext::Traits *traits) :
    _initialized(false),
    _valid(false),
    _realized(false),
    _closing(false),
    _owned(false),
    _glContext(NULL),
    _surface(NULL)
{
    qDebug() << "GraphicsWindowQt::GraphicsWindowQt";
    _traits = traits;

    init();

    if (valid()) {
        setState(new osg::State);
        getState()->setGraphicsContext(this);

        if (_traits.valid() && _traits->sharedContext.valid()) {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        } else {
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
        }
    }
}

GraphicsWindowQt::~GraphicsWindowQt()
{
    qDebug() << "GraphicsWindowQt::~GraphicsWindowQt";
    close();
}

void GraphicsWindowQt::init()
{
    qDebug() << "GraphicsWindowQt::init";
    if (_closing || _initialized) {
        return;
    }

    // update by WindowData
    // WindowData* windowData = _traits.get() ? dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : 0;
    // if ( !_widget )
    // _widget = windowData ? windowData->_widget : NULL;
    // if ( !parent )
    // parent = windowData ? windowData->_parent : NULL;

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
    qDebug() << "GraphicsWindowQt::realizeImplementation";
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
        _owned     = true;
        _glContext = new QOpenGLContext();
        _glContext->create();
        _surface   = new QOffscreenSurface();
        _surface->setFormat(_glContext->format());
        _surface->create();
        osgQtQuick::formatInfo(_surface->format());
    } else {
        qDebug() << "GraphicsWindowQt::realizeImplementation - using current context";
        _glContext = currentContext;
    }

    //// initialize GL context for the widget
    // if ( !valid() )
    // _widget->glInit();

    // make current
    _realized = true;
    bool result = makeCurrent();
    _realized = false;

    // fail if we do not have current context
    if (!result) {
        // if ( savedContext )
        // const_cast< QGLContext* >( savedContext )->makeCurrent();
        //
        qWarning() << "GraphicsWindowQt realize: Can make context current.";
        return false;
    }

    _realized = true;

//// make sure the event queue has the correct window rectangle size and input range
// getEventQueue()->syncWindowRectangleWithGraphcisContext();

    // make this window's context not current
    // note: this must be done as we will probably make the context current from another thread
    // and it is not allowed to have one context current in two threads
    if (!releaseContext()) {
        qWarning() << "Window realize: Can not release context.";
    }

//// restore previous context
// if ( savedContext )
// const_cast< QGLContext* >( savedContext )->makeCurrent();

    return true;
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
    if (!_realized) {
        qWarning() << "GraphicsWindowQt::makeCurrentImplementation() - not realized; cannot make current.";
        return false;
    }
    if (_owned && _glContext) {
        // qDebug() << "GraphicsWindowQt::makeCurrentImplementation : " << _surface;
        if (!_glContext->makeCurrent(_surface)) {
            qWarning() << "GraphicsWindowQt::makeCurrentImplementation : failed to make context current";
            return false;
        }
    }
    if (_glContext != QOpenGLContext::currentContext()) {
        qWarning() << "GraphicsWindowQt::makeCurrentImplementation : context is not current";
        return false;
    }
    return true;
}

bool GraphicsWindowQt::releaseContextImplementation()
{
    if (_glContext != QOpenGLContext::currentContext()) {
        qWarning() << "GraphicsWindowQt::releaseContextImplementation : context is not current";
        return false;
    }
    if (_owned && _glContext) {
        qDebug() << "GraphicsWindowQt::releaseContextImplementation";
        _glContext->doneCurrent();
    }
    return true;
}

void GraphicsWindowQt::closeImplementation()
{
    qDebug() << "GraphicsWindowQt::closeImplementation";
    _closing     = true;
    _initialized = false;
    _valid = false;
    _realized    = false;
    if (_glContext) {
        if (_owned) {
            delete _glContext;
        }
        _glContext = NULL;
    }
    if (_surface) {
        if (_owned) {
            _surface->destroy();
            delete _surface;
        }
        _surface = NULL;
    }
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

class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface {
public:

    QtWindowingSystem()
    {}

    ~QtWindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler()) {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
    }

    // Access the Qt windowing system through this singleton class.
    static QtWindowingSystem *getInterface()
    {
        static QtWindowingSystem *qtInterface = new QtWindowingSystem;

        return qtInterface;
    }

    // Return the number of screens present in the system
    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier & /*si*/)
    {
        qWarning() << "osgQt: getNumScreens() not implemented yet.";
        return 0;
    }

    // Return the resolution of specified screen
    // (0,0) is returned if screen is unknown
    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/)
    {
        qWarning() << "osgQt: getScreenSettings() not implemented yet.";
    }

    // Set the resolution for given screen
    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/)
    {
        qWarning() << "osgQt: setScreenSettings() not implemented yet.";
        return false;
    }

    // Enumerates available resolutions
    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/)
    {
        qWarning() << "osgQt: enumerateScreenSettings() not implemented yet.";
    }

    // Create a graphics context with given traits
    virtual osg::GraphicsContext *createGraphicsContext(osg::GraphicsContext::Traits *traits)
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

private:

    // No implementation for these
    QtWindowingSystem(const QtWindowingSystem &);
    QtWindowingSystem & operator=(const QtWindowingSystem &);
};

void OsgEarth::initWindowingSystem()
{
    osg::GraphicsContext::setWindowingSystemInterface(QtWindowingSystem::getInterface());
}
