/********************************************************************************
 * @file       osgearthviewplugin.cpp
 * @author     The OpenPilot Team Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OsgEarthview Plugin
 * @{
 * @brief Osg Earth view of UAV
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

#include "osgearthviewplugin.h"
#include "osgearthviewgadgetfactory.h"

#include "utils/gcsdirs.h"
#include "utils/pathutils.h"
#include <extensionsystem/pluginmanager.h>

//#include <osg/Version>
//#include <osgDB/Registry>
//#include <osgQt/GraphicsWindowQt>

//#include <osgEarth/Version>
//#include <osgEarth/Cache>
//#include <osgEarth/Registry>

#include <QDebug>
#include <QtPlugin>
#include <QStringList>

//#include <deque>
//#include <string>
//#include <stdlib.h>

//#ifdef Q_WS_X11
//#include <X11/Xlib.h>
//#endif

OsgEarthviewPlugin::OsgEarthviewPlugin()
{
    // Do nothing
}

OsgEarthviewPlugin::~OsgEarthviewPlugin()
{
    // Do nothing
}

bool OsgEarthviewPlugin::initialize(const QStringList & args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

#ifdef Q_WS_X11
    // required for multi-threaded viewer on linux:
    XInitThreads();
#endif

    qDebug() << "Using osg version :" << osgGetVersion();
    qDebug() << "Using osgEarth version :" << osgEarthGetVersion();

//    qDebug() << "OsgEarthviewPlugin::initialize - initializing osgDB registry";
    osgDB::FilePathList &libraryFilePathList = osgDB::Registry::instance()->getLibraryFilePathList();
    // clear to remove system wide library pathes
    libraryFilePathList.clear();
    libraryFilePathList.push_back(GCSDirs::libraryPath("osg").toStdString());
    libraryFilePathList.push_back(GCSDirs::libraryPath("osgearth").toStdString());

    osgDB::FilePathList::iterator it = libraryFilePathList.begin();
    while(it != libraryFilePathList.end()){
        qDebug() << "OsgEarthviewPlugin::initialize - library file path:" << QString::fromStdString(*it);
        it++;
    }

    osgDB::FilePathList &dataFilePathList = osgDB::Registry::instance()->getDataFilePathList();
    it = dataFilePathList.begin();
    while(it != dataFilePathList.end()){
        qDebug() << "OsgEarthviewPlugin::initialize - data file path:" << QString::fromStdString(*it);
        it++;
    }

    //osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
    osgQt::initQtWindowingSystem();

    // force init of osgearth registry (without caching) to work around a deadlock
    osgEarth::Registry::instance();

    // enable caching
    QString cachePath = Utils::PathUtils().GetStoragePath() + "osgearth/cache";
    setenv("OSGEARTH_CACHE_PATH", cachePath.toUtf8().data(), 1);
    osgEarth::CacheOptions options;
    options.setDriver(osgEarth::Registry::instance()->getDefaultCacheDriverName());

    osgEarth::Cache *cache = osgEarth::CacheFactory::create(options);
    if (cache) {
        osgEarth::Registry::instance()->setCache(cache);
    } else {
        qWarning() << "Failed to initialize cache";
    }


    mf = new OsgEarthviewGadgetFactory(this);
    addAutoReleasedObject(mf);

    return true;
}

void OsgEarthviewPlugin::extensionsInitialized()
{
    // Do nothing
}

void OsgEarthviewPlugin::shutdown()
{
    // Do nothing
}
