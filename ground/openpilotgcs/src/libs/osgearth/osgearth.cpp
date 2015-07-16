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

#include "utility.h"
#include "qtwindowingsystem.h"

#include "utils/gcsdirs.h"
#include "utils/pathutils.h"

#include <osg/DisplaySettings>
#include <osg/GraphicsContext>
#include <osg/Version>
#include <osg/Notify>
#include <osgDB/Registry>

#include <osgEarth/Version>
#include <osgEarth/Cache>
#include <osgEarth/Capabilities>
#include <osgEarth/Registry>
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>

#include <QDebug>

#ifdef OSG_USE_QT_PRIVATE
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#endif

#include <deque>

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

    qDebug() << "OsgEarth::initialize - initializing osgearth...";

    // increase cache (default is 300);
    //setenv("OSG_MAX_PAGEDLOD", "500", 0);

    //setenv("OSG_ASSIGN_PBO_TO_IMAGES", "on", 0);

    // Number of threads in the DatbasePager set up, inclusive of the number of http dedicated threads.
    osg::DisplaySettings::instance()->setNumOfDatabaseThreadsHint(6);
    osg::DisplaySettings::instance()->setNumOfHttpDatabaseThreadsHint(3);

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
    QString cachePath = Utils::GetStoragePath() + "osgearth/cache";

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

void OsgEarth::initWindowingSystem()
{
    osg::GraphicsContext::setWindowingSystemInterface(QtWindowingSystem::getInterface());
}
