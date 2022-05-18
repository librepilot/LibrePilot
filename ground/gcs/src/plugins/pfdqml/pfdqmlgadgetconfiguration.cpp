/**
 ******************************************************************************
 *
 * @file       pfdqmlgadgetconfiguration.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************//*
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

#include "pfdqmlgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
PfdQmlGadgetConfiguration::PfdQmlGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    // TODO move to some conversion utility class
    m_speedMap[1.0]       = "m/s";
    m_speedMap[3.6]       = "km/h";
    m_speedMap[2.2369]    = "mph";
    m_speedMap[1.9438]    = "knots";

    m_altitudeMap[1.0]    = "m";
    m_altitudeMap[3.2808] = "ft";

    m_qmlFile             = settings.value("qmlFile", "Unknown").toString();
    m_qmlFile             = Utils::InsertDataPath(m_qmlFile);

    m_speedFactor         = settings.value("speedFactor", 1.0).toDouble();
    m_altitudeFactor      = settings.value("altitudeFactor", 1.0).toDouble();

    // terrain
    m_terrainEnabled      = settings.value("terrainEnabled", false).toBool();
    m_terrainFile         = settings.value("earthFile", "Unknown").toString();
    m_terrainFile         = Utils::InsertDataPath(m_terrainFile);
    m_cacheOnly           = settings.value("cacheOnly", false).toBool();

    m_latitude            = settings.value("latitude").toDouble();
    m_longitude           = settings.value("longitude").toDouble();
    m_altitude            = settings.value("altitude").toDouble();

    // sky
    m_timeMode            = static_cast<TimeMode::Enum>(settings.value("timeMode", TimeMode::Local).toUInt());
    m_dateTime            = settings.value("dateTime", QDateTime()).toDateTime();
    m_minAmbientLight     = settings.value("minAmbientLight").toDouble();

    // model
    m_modelEnabled        = settings.value("modelEnabled").toBool();
    m_modelSelectionMode  = static_cast<ModelSelectionMode::Enum>(settings.value("modelSelectionMode", ModelSelectionMode::Auto).toUInt());
    m_modelFile           = Utils::InsertDataPath(settings.value("modelFile", "Unknown").toString());

    // background image
    m_backgroundImageFile = Utils::InsertDataPath(settings.value("backgroundImageFile", "Unknown").toString());

    // gstreamer pipeline
    m_gstPipeline         = settings.value("gstPipeline").toString();
}

PfdQmlGadgetConfiguration::PfdQmlGadgetConfiguration(const PfdQmlGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_speedMap            = obj.m_speedMap;
    m_altitudeMap         = obj.m_altitudeMap;

    m_qmlFile             = obj.m_qmlFile;

    m_speedFactor         = obj.m_speedFactor;
    m_altitudeFactor      = obj.m_altitudeFactor;

    // terrain
    m_terrainEnabled      = obj.m_terrainEnabled;
    m_terrainFile         = obj.m_terrainFile;
    m_cacheOnly           = obj.m_cacheOnly;

    m_latitude            = obj.m_latitude;
    m_longitude           = obj.m_longitude;
    m_altitude            = obj.m_altitude;

    // sky
    m_timeMode            = obj.m_timeMode;
    m_dateTime            = obj.m_dateTime;
    m_minAmbientLight     = obj.m_minAmbientLight;

    // model
    m_modelEnabled        = obj.m_modelEnabled;
    m_modelSelectionMode  = obj.m_modelSelectionMode;
    m_modelFile           = obj.m_modelFile;

    // background image
    m_backgroundImageFile = obj.m_backgroundImageFile;

    // gstreamer pipeline
    m_gstPipeline         = obj.m_gstPipeline;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *PfdQmlGadgetConfiguration::clone() const
{
    return new PfdQmlGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void PfdQmlGadgetConfiguration::saveConfig(QSettings &settings) const
{
    QString qmlFile = Utils::RemoveDataPath(m_qmlFile);

    settings.setValue("qmlFile", qmlFile);

    settings.setValue("speedFactor", m_speedFactor);
    settings.setValue("altitudeFactor", m_altitudeFactor);

    // terrain
    settings.setValue("terrainEnabled", m_terrainEnabled);
    QString terrainFile = Utils::RemoveDataPath(m_terrainFile);
    settings.setValue("earthFile", terrainFile);
    settings.setValue("cacheOnly", m_cacheOnly);

    settings.setValue("latitude", m_latitude);
    settings.setValue("longitude", m_longitude);
    settings.setValue("altitude", m_altitude);

    // sky
    settings.setValue("timeMode", static_cast<uint>(m_timeMode));
    settings.setValue("dateTime", m_dateTime);
    settings.setValue("minAmbientLight", m_minAmbientLight);

    // model
    settings.setValue("modelEnabled", m_modelEnabled);
    settings.setValue("modelSelectionMode", static_cast<uint>(m_modelSelectionMode));
    settings.setValue("modelFile", Utils::RemoveDataPath(m_modelFile));

    // background image
    settings.setValue("backgroundImageFile", Utils::RemoveDataPath(m_backgroundImageFile));

    // gstreamer pipeline
    settings.setValue("gstPipeline", m_gstPipeline);
}
