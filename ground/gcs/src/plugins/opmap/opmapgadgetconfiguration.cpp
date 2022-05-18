/**
 ******************************************************************************
 *
 * @file       opmapgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin OpenPilot Map Plugin
 * @{
 * @brief The OpenPilot Map plugin
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

#include "opmapgadgetconfiguration.h"
#include "utils/pathutils.h"
#include <QDir>

OPMapGadgetConfiguration::OPMapGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    m_defaultWaypointAltitude = settings.value("defaultWaypointAltitude", 15).toReal();
    m_defaultWaypointVelocity = settings.value("defaultWaypointVelocity", 2).toReal();
    m_opacity           = settings.value("overlayOpacity", 1).toReal();

    m_mapProvider       = settings.value("mapProvider", "GoogleHybrid").toString();
    m_defaultZoom       = settings.value("defaultZoom", 2).toInt();
    m_defaultLatitude   = settings.value("defaultLatitude").toDouble();
    m_defaultLongitude  = settings.value("defaultLongitude").toDouble();
    m_useOpenGL         = settings.value("useOpenGL").toBool();
    m_showTileGridLines = settings.value("showTileGridLines").toBool();
    m_uavSymbol         = settings.value("uavSymbol", QString::fromUtf8(":/uavs/images/mapquad.png")).toString();
    m_safeAreaRadius    = settings.value("safeAreaRadius", 5).toInt();
    m_showSafeArea      = settings.value("showSafeArea").toBool();

    m_maxUpdateRate     = settings.value("maxUpdateRate", 2000).toInt();
    if (m_maxUpdateRate < 100 || m_maxUpdateRate > 5000) {
        m_maxUpdateRate = 2000;
    }

    m_accessMode     = settings.value("accessMode", "ServerAndCache").toString();
    m_useMemoryCache = settings.value("useMemoryCache").toBool();
    m_cacheLocation  = settings.value("cacheLocation", Utils::GetStoragePath() + "mapscache" + QDir::separator()).toString();
    m_cacheLocation  = Utils::InsertStoragePath(m_cacheLocation);
}

OPMapGadgetConfiguration::OPMapGadgetConfiguration(const OPMapGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_mapProvider       = obj.m_mapProvider;
    m_defaultZoom       = obj.m_defaultZoom;
    m_defaultLatitude   = obj.m_defaultLatitude;
    m_defaultLongitude  = obj.m_defaultLongitude;
    m_useOpenGL = obj.m_useOpenGL;
    m_showTileGridLines = obj.m_showTileGridLines;
    m_accessMode = obj.m_accessMode;
    m_useMemoryCache    = obj.m_useMemoryCache;
    m_cacheLocation     = obj.m_cacheLocation;
    m_uavSymbol = obj.m_uavSymbol;
    m_maxUpdateRate     = obj.m_maxUpdateRate;
    m_safeAreaRadius    = obj.m_safeAreaRadius;
    m_showSafeArea      = obj.m_showSafeArea;
    m_opacity = obj.m_opacity;
    m_defaultWaypointAltitude = obj.m_defaultWaypointAltitude;
    m_defaultWaypointVelocity = obj.m_defaultWaypointVelocity;
}

IUAVGadgetConfiguration *OPMapGadgetConfiguration::clone() const
{
    return new OPMapGadgetConfiguration(*this);
}

void OPMapGadgetConfiguration::save() const
{
    QSettings settings;

    saveConfig(settings);
}

void OPMapGadgetConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("mapProvider", m_mapProvider);
    settings.setValue("defaultZoom", m_defaultZoom);
    settings.setValue("defaultLatitude", m_defaultLatitude);
    settings.setValue("defaultLongitude", m_defaultLongitude);
    settings.setValue("useOpenGL", m_useOpenGL);
    settings.setValue("showTileGridLines", m_showTileGridLines);
    settings.setValue("accessMode", m_accessMode);
    settings.setValue("useMemoryCache", m_useMemoryCache);
    settings.setValue("uavSymbol", m_uavSymbol);
    settings.setValue("cacheLocation", Utils::RemoveStoragePath(m_cacheLocation));
    settings.setValue("maxUpdateRate", m_maxUpdateRate);
    settings.setValue("safeAreaRadius", m_safeAreaRadius);
    settings.setValue("showSafeArea", m_showSafeArea);
    settings.setValue("overlayOpacity", m_opacity);

    settings.setValue("defaultWaypointAltitude", m_defaultWaypointAltitude);
    settings.setValue("defaultWaypointVelocity", m_defaultWaypointVelocity);
}
void OPMapGadgetConfiguration::setCacheLocation(QString cacheLocation)
{
    m_cacheLocation = cacheLocation;
}
