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
#include <osg/Version>
#include <osgDB/Registry>
#include <osgQt/GraphicsWindowQt>

#include <osgEarth/Version>
#include <osgEarth/Cache>
#include <osgEarth/Capabilities>
#include <osgEarth/Registry>

#include <QDebug>

#ifdef OSG_USE_QT_PRIVATE
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#endif

#include <deque>
#include <string>
// #include <stdlib.h>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

bool OsgEarth::initialized = false;

/*

   Debugging tips

   Windows DIB issues

   The environment variable QT_QPA_VERBOSE controls the debug level.
   It takes the form {<keyword1>:<level1>,<keyword2>:<level2>},
   where keyword is one of integration,  windows, backingstore and fonts.
   Level is an integer 0..9.

   OSG:
   export OSG_NOTIFY_LEVEL=DEBUG



   StackHash failure

   Here’s how 99 percent of Vista users can fix a StackHash failure. I’ll walk you through it step by step:

   Method A:
   —————
   1. Open your Start menu and click Control Panel
   2. Browse to “System Maintenance” then “System”
   3. In the left panel, select “Advanced System Settings” from the available links
   4. You should now see the System Properties Window, which will have three sections. The top section is labeled “Performance” and has a “Settings” button. Click this button.
   5. Select the “Data Execution Prevention” tab.
   6. Select the option which reads “Turn on DEP for all programs and services except those I select”
   7. Use the “Browse” button to locate the executable file for the application you were trying to start when you received the StackHash error, and click Open to add it to your exceptions list.
   8. Click Apply or OK to commit your changes.

 */


/*
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
 */
void OsgEarth::initialize()
{
    if (initialized) {
        return;
    }
    initialized = true;

#ifdef Q_WS_X11
    // required for multi-threaded viewer on linux:
    XInitThreads();
#endif

    osg::DisplaySettings::instance()->setNumOfDatabaseThreadsHint(16);
    osg::DisplaySettings::instance()->setNumOfHttpDatabaseThreadsHint(8);

    initializePathes();

    // osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
    // this line causes crashes on Windows!!!
    // osgQt::initQtWindowingSystem();

    // force early initialization of osgEarth registry
    // this important as doing it later (when OpenGL is already in use) might thrash some GL contextes
    // TODO : this is done too early when no window is displayed which causes a windows to be briefly flashed on Linux
    osgEarth::Registry::capabilities();

    initializeCache();

    // Register Qml types
    osgQtQuick::registerTypes("osgQtQuick");

    displayInfo();
}

void OsgEarth::initializePathes()
{
    // clear and initialize data file path list
    osgDB::FilePathList &dataFilePathList = osgDB::Registry::instance()->getDataFilePathList();
    dataFilePathList.clear();
    dataFilePathList.push_back(GCSDirs::sharePath("osgearth").toStdString());
    dataFilePathList.push_back(GCSDirs::sharePath("osgearth/data").toStdString());

    // clear and initialize library file path list
    osgDB::FilePathList &libraryFilePathList = osgDB::Registry::instance()->getLibraryFilePathList();
    libraryFilePathList.clear();
    libraryFilePathList.push_back(GCSDirs::libraryPath("osg").toStdString());
}

void OsgEarth::initializeCache()
{
    // force init of osgearth registry (without caching) to work around a deadlock
    // TODO might not be necessary anymore because of the early call to getCapabilities()
    osgEarth::Registry::instance();

    // enable caching
    // TODO try to use FileSystemCacheOptions instead of using OSGEARTH_CACHE_PATH env variables
    QString cachePath = Utils::PathUtils().GetStoragePath() + "osgearth/cache";

    qputenv("OSGEARTH_CACHE_PATH", cachePath.toLatin1());

    osgEarth::CacheOptions cacheOptions;
    cacheOptions.setDriver(osgEarth::Registry::instance()->getDefaultCacheDriverName());

    osgEarth::Cache *cache = osgEarth::CacheFactory::create(cacheOptions);
    if (cache) {
        osgEarth::Registry::instance()->setCache(cache);
    } else {
        qWarning() << "Failed to initialize cache";
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

    qDebug() << "osg cache:" << qgetenv("OSGEARTH_CACHE_PATH");

    qDebug() << "osg database threads:" << osg::DisplaySettings::instance()->getNumOfDatabaseThreadsHint();
    qDebug() << "osg http database threads:" << osg::DisplaySettings::instance()->getNumOfHttpDatabaseThreadsHint();

    qDebug() << "Platform supports GLSL:" << osgEarth::Registry::capabilities().supportsGLSL();

#ifdef OSG_USE_QT_PRIVATE
    bool threadedOpenGL = QGuiApplicationPrivate::platform_integration->hasCapability(QPlatformIntegration::ThreadedOpenGL);
    qDebug() << "Platform supports threaded OpenGL:" << threadedOpenGL;
#endif

}

/*

   Why not use setRenderTarget

   export OSG_MULTIMONITOR_MULTITHREAD_WIN32_NVIDIA_WORKAROUND=On
   export OSG_NOTIFY_LEVEL=DEBUG
   http://trac.openscenegraph.org/projects/osg//wiki/Support/TipsAndTricks

   Switch to 5.4 and use this :http://doc-snapshot.qt-project.org/qt5-5.4/qquickframebufferobject.html#details
   http://doc-snapshot.qt-project.org/qt5-5.4/qtquick-visualcanvas-scenegraph.html#scene-graph-and-rendering
   http://www.kdab.com/opengl-in-qt-5-1-part-1/

   THIS MENTIONS GLUT...
   http://www.multigesture.net/articles/how-to-compile-openscenegraph-2-x-using-mingw/
   http://www.mingw.org/category/wiki/opengl

   INTERESTING : http://qt-project.org/forums/viewthread/24535

   https://groups.google.com/forum/#!msg/osg-users/HDabWUVaR2w/C6rPKKeKoYkJ

   http://trac.openscenegraph.org/projects/osg/wiki/Community/OpenGL-ES

   http://upstream.rosalinux.ru/changelogs/openscenegraph/3.2.0/changelog.html

   MESA https://groups.google.com/forum/#!topic/osg-users/_2MqkA-wH1Q

   GL version https://groups.google.com/forum/#!topic/osg-users/UiyYQ0Fw7MQ

   http://trac.openscenegraph.org/projects/osg//wiki/Support/PlatformSpecifics/Mingw

   Qt & OpenGL
   http://blog.qt.digia.com/blog/2009/12/16/qt-graphics-and-performance-an-overview/
   http://blog.qt.digia.com/blog/2010/01/06/qt-graphics-and-performance-opengl/
 */

// BUGS
// if resizing a gadget below the viewport, the viewport will stop shrinking at 64 pixel
// but it is possible to continue resizing the gadget and the gadget decorator will disapear behing the GL view
// will

// Errors/Freeze/Crashes:

// BLANK : Error: cannot draw stage due to undefined viewport.
// ABNORMAL TERMINATION : createDIB: CreateDIBSection failed.

/*
   CRASH
   osgQtQuick::OSGViewport : Update called for a item without content
   VERTEX glCompileShader "atmos_vertex_main" FAILED
   VERTEX glCompileShader "main(vert)" FAILED
   FRAGMENT glCompileShader "main(frag)" FAILED
   glLinkProgram "SimpleSky Scene Lighting" FAILED
   Program "SimpleSky Scene Lighting" infolog:
   Vertex info
   -----------
   An error occurred

   Fragment info
   -------------
   An error occurred

   [osgEarth]* [VirtualProgram] Program link failure!
   VERTEX glCompileShader "main(vert)" FAILED
   FRAGMENT glCompileShader "main(frag)" FAILED
   glLinkProgram "osgEarth.ModelNode" FAILED
   Program "osgEarth.ModelNode" infolog:
   Vertex info
   -----------
   An error occurred

   Fragment info
   -------------
   An error occurred

   [osgEarth]* [VirtualProgram] Program link failure!
 */

/*
   osgQtQuick::OSGViewport : Update called for a item without content
   VERTEX glCompileShader "atmos_vertex_main" FAILED
   VERTEX glCompileShader "main(vert)" FAILED
   FRAGMENT glCompileShader "main(frag)" FAILED
   glLinkProgram "SimpleSky Scene Lighting" FAILED
   Program "SimpleSky Scene Lighting" infolog:
   Vertex info
   -----------
   An error occurred

   Fragment info
   -------------
   An error occurred

   [osgEarth]* [VirtualProgram] Program link failure!
   Warning: detected OpenGL error 'out of memory' at After Renderer::compile
   terminate called after throwing an instance of 'std::bad_alloc'
   what():  std::bad_alloc

   This application has requested the Runtime to terminate it in an unusual way.
   Please contact the application's support team for more information.
 */
