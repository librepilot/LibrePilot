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
PfdQmlGadgetConfiguration::PfdQmlGadgetConfiguration(QString classId, QSettings *qSettings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent),
    m_qmlFile("Unknown"),
    m_speedFactor(1.0),
    m_altitudeFactor(1.0),
    m_terrainEnabled(false),
    m_terrainFile("Unknown"),
    m_cacheOnly(false),
    m_latitude(0),
    m_longitude(0),
    m_altitude(0),
    m_timeMode(TimeMode::Local),
    m_dateTime(QDateTime()),
    m_minAmbientLight(0),
    m_modelEnabled(false),
    m_modelFile("Unknown"),
    m_modelSelectionMode(ModelSelectionMode::Auto),
    m_backgroundImageFile("Unknown")
{
    m_speedMap[1.0]       = "m/s";
    m_speedMap[3.6]       = "km/h";
    m_speedMap[2.2369]    = "mph";
    m_speedMap[1.9438]    = "knots";

    m_altitudeMap[1.0]    = "m";
    m_altitudeMap[3.2808] = "ft";

    // if a saved configuration exists load it
    if (qSettings != 0) {
        m_qmlFile             = qSettings->value("qmlFile").toString();
        m_qmlFile             = Utils::InsertDataPath(m_qmlFile);

        m_speedFactor         = qSettings->value("speedFactor").toDouble();
        m_altitudeFactor      = qSettings->value("altitudeFactor").toDouble();

        // terrain
        m_terrainEnabled      = qSettings->value("terrainEnabled").toBool();
        m_terrainFile         = qSettings->value("earthFile").toString();
        m_terrainFile         = Utils::InsertDataPath(m_terrainFile);
        m_cacheOnly           = qSettings->value("cacheOnly").toBool();

        m_latitude            = qSettings->value("latitude").toDouble();
        m_longitude           = qSettings->value("longitude").toDouble();
        m_altitude            = qSettings->value("altitude").toDouble();

        // sky
        m_timeMode            = static_cast<TimeMode::Enum>(qSettings->value("timeMode").toUInt());
        m_dateTime            = qSettings->value("dateTime").toDateTime();
        m_minAmbientLight     = qSettings->value("minAmbientLight").toDouble();

        // model
        m_modelEnabled        = qSettings->value("modelEnabled").toBool();
        m_modelSelectionMode  = static_cast<ModelSelectionMode::Enum>(qSettings->value("modelSelectionMode").toUInt());
        m_modelFile           = qSettings->value("modelFile").toString();
        m_modelFile           = Utils::InsertDataPath(m_modelFile);

        // background image
        m_backgroundImageFile = qSettings->value("backgroundImageFile").toString();
        m_backgroundImageFile = Utils::InsertDataPath(m_backgroundImageFile);
    }
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *PfdQmlGadgetConfiguration::clone()
{
    PfdQmlGadgetConfiguration *m = new PfdQmlGadgetConfiguration(this->classId());

    m->m_qmlFile             = m_qmlFile;

    m->m_speedFactor         = m_speedFactor;
    m->m_altitudeFactor      = m_altitudeFactor;

    // terrain
    m->m_terrainEnabled      = m_terrainEnabled;
    m->m_terrainFile         = m_terrainFile;
    m->m_cacheOnly           = m_cacheOnly;

    m->m_latitude            = m_latitude;
    m->m_longitude           = m_longitude;
    m->m_altitude            = m_altitude;

    // sky
    m->m_timeMode            = m_timeMode;
    m->m_dateTime            = m_dateTime;
    m->m_minAmbientLight     = m_minAmbientLight;

    // model
    m->m_modelEnabled        = m_modelEnabled;
    m->m_modelSelectionMode  = m_modelSelectionMode;
    m->m_modelFile           = m_modelFile;

    // background image
    m->m_backgroundImageFile = m_backgroundImageFile;

    return m;
}

/**
 * Saves a configuration.
 *
 */
void PfdQmlGadgetConfiguration::saveConfig(QSettings *qSettings) const
{
    QString qmlFile = Utils::RemoveDataPath(m_qmlFile);

    qSettings->setValue("qmlFile", qmlFile);

    qSettings->setValue("speedFactor", m_speedFactor);
    qSettings->setValue("altitudeFactor", m_altitudeFactor);

    // terrain
    qSettings->setValue("terrainEnabled", m_terrainEnabled);
    QString terrainFile = Utils::RemoveDataPath(m_terrainFile);
    qSettings->setValue("earthFile", terrainFile);
    qSettings->setValue("cacheOnly", m_cacheOnly);

    qSettings->setValue("latitude", m_latitude);
    qSettings->setValue("longitude", m_longitude);
    qSettings->setValue("altitude", m_altitude);

    // sky
    qSettings->setValue("timeMode", static_cast<uint>(m_timeMode));
    qSettings->setValue("dateTime", m_dateTime);
    qSettings->setValue("minAmbientLight", m_minAmbientLight);

    // model
    qSettings->setValue("modelEnabled", m_modelEnabled);
    qSettings->setValue("modelSelectionMode", static_cast<uint>(m_modelSelectionMode));
    QString modelFile = Utils::RemoveDataPath(m_modelFile);
    qSettings->setValue("modelFile", modelFile);

    // background image
    QString backgroundImageFile = Utils::RemoveDataPath(m_backgroundImageFile);
    qSettings->setValue("backgroundImageFile", backgroundImageFile);
}
