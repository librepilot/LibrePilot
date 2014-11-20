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

#include "pfdqmlgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
PfdQmlGadgetConfiguration::PfdQmlGadgetConfiguration(QString classId, QSettings *qSettings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent),
    m_qmlFile("Unknown"),
    m_terrainFile("Unknown"),
    m_openGLEnabled(true),
    m_terrainEnabled(false),
    m_positionMode(Pfd::Predefined),
    m_latitude(0),
    m_longitude(0),
    m_altitude(0),
    m_cacheOnly(false),
    m_speedFactor(1.0),
    m_altitudeFactor(1.0)
{
    m_speedMap[1.0]       = "m/s";
    m_speedMap[3.6]       = "km/h";
    m_speedMap[2.2369]    = "mph";
    m_speedMap[1.9438]    = "knots";

    m_altitudeMap[1.0]    = "m";
    m_altitudeMap[3.2808] = "ft";

    // if a saved configuration exists load it
    if (qSettings != 0) {
        m_qmlFile            = qSettings->value("qmlFile").toString();
        m_qmlFile            = Utils::PathUtils().InsertDataPath(m_qmlFile);

        m_speedFactor        = qSettings->value("speedFactor").toDouble();
        m_altitudeFactor     = qSettings->value("altitudeFactor").toDouble();

        m_positionMode       = static_cast<Pfd::PositionMode>(qSettings->value("positionMode").toUInt());
        m_latitude           = qSettings->value("latitude").toDouble();
        m_longitude          = qSettings->value("longitude").toDouble();
        m_altitude           = qSettings->value("altitude").toDouble();

        m_openGLEnabled      = qSettings->value("openGLEnabled", true).toBool();
        m_terrainEnabled     = qSettings->value("terrainEnabled").toBool();
        m_terrainFile          = qSettings->value("earthFile").toString();
        m_terrainFile          = Utils::PathUtils().InsertDataPath(m_terrainFile);
        m_modelFile          = qSettings->value("modelFile").toString();
        m_modelFile          = Utils::PathUtils().InsertDataPath(m_modelFile);
        m_cacheOnly          = qSettings->value("cacheOnly").toBool();
    }
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *PfdQmlGadgetConfiguration::clone()
{
    PfdQmlGadgetConfiguration *m = new PfdQmlGadgetConfiguration(this->classId());

    m->m_qmlFile            = m_qmlFile;

    m->m_speedFactor        = m_speedFactor;
    m->m_altitudeFactor     = m_altitudeFactor;

    m->m_positionMode       = m_positionMode;
    m->m_latitude           = m_latitude;
    m->m_longitude          = m_longitude;
    m->m_altitude           = m_altitude;

    m->m_openGLEnabled      = m_openGLEnabled;
    m->m_terrainEnabled     = m_terrainEnabled;
    m->m_terrainFile          = m_terrainFile;
    m->m_cacheOnly          = m_cacheOnly;

    return m;
}

/**
 * Saves a configuration.
 *
 */
void PfdQmlGadgetConfiguration::saveConfig(QSettings *qSettings) const
{
    QString qmlFile   = Utils::PathUtils().RemoveDataPath(m_qmlFile);
    qSettings->setValue("qmlFile", qmlFile);

    qSettings->setValue("speedFactor", m_speedFactor);
    qSettings->setValue("altitudeFactor", m_altitudeFactor);

    qSettings->setValue("positionMode", static_cast<uint>(m_positionMode));
    qSettings->setValue("latitude", m_latitude);
    qSettings->setValue("longitude", m_longitude);
    qSettings->setValue("altitude", m_altitude);

    qSettings->setValue("openGLEnabled", m_openGLEnabled);
    qSettings->setValue("terrainEnabled", m_terrainEnabled);
    QString terrainFile = Utils::PathUtils().RemoveDataPath(m_terrainFile);
    qSettings->setValue("earthFile", terrainFile);
    QString modelFile = Utils::PathUtils().RemoveDataPath(m_modelFile);
    qSettings->setValue("modelFile", modelFile);
    qSettings->setValue("cacheOnly", m_cacheOnly);
}
