/**
 ******************************************************************************
 *
 * @file       pfdqmlcontext.cpp
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

#include "pfdqmlcontext.h"

#include "extensionsystem/pluginmanager.h"
#include "uavobject.h"
#include "uavobjectmanager.h"
#include "utils/stringutils.h"
#include "utils/pathutils.h"

#include "flightbatterysettings.h"

#include <QQmlContext>
#include <QDebug>
#include <QDirIterator>

const QString PfdQmlContext::CONTEXT_PROPERTY_NAME = "pfdContext";

PfdQmlContext::PfdQmlContext(QObject *parent) : QObject(parent),
    m_speedUnit("m/s"),
    m_speedFactor(1.0),
    m_altitudeUnit("m"),
    m_altitudeFactor(1.0),
    m_terrainEnabled(false),
    m_terrainFile(""),
    m_latitude(39.657380),
    m_longitude(19.805158),
    m_altitude(100),
    m_timeMode(TimeMode::Local),
    m_dateTime(QDateTime()),
    m_minAmbientLight(0.03),
    m_modelFile(""),
    m_modelIndex(0),
    m_backgroundImageFile("")
{
    addModelDir("helis");
    addModelDir("multi");
    addModelDir("planes");
}

PfdQmlContext::~PfdQmlContext()
{}

QString PfdQmlContext::speedUnit() const
{
    return m_speedUnit;
}

void PfdQmlContext::setSpeedUnit(QString unit)
{
    if (m_speedUnit != unit) {
        m_speedUnit = unit;
        emit speedUnitChanged(speedUnit());
    }
}

double PfdQmlContext::speedFactor() const
{
    return m_speedFactor;
}

void PfdQmlContext::setSpeedFactor(double factor)
{
    if (m_speedFactor != factor) {
        m_speedFactor = factor;
        emit speedFactorChanged(speedFactor());
    }
}

QString PfdQmlContext::altitudeUnit() const
{
    return m_altitudeUnit;
}

void PfdQmlContext::setAltitudeUnit(QString unit)
{
    if (m_altitudeUnit != unit) {
        m_altitudeUnit = unit;
        emit altitudeUnitChanged(altitudeUnit());
    }
}

double PfdQmlContext::altitudeFactor() const
{
    return m_altitudeFactor;
}

void PfdQmlContext::setAltitudeFactor(double factor)
{
    if (m_altitudeFactor != factor) {
        m_altitudeFactor = factor;
        emit altitudeFactorChanged(altitudeFactor());
    }
}

bool PfdQmlContext::terrainEnabled() const
{
    return m_terrainEnabled;
}

void PfdQmlContext::setTerrainEnabled(bool arg)
{
    if (m_terrainEnabled != arg) {
        m_terrainEnabled = arg;
        emit terrainEnabledChanged(terrainEnabled());
    }
}

QString PfdQmlContext::terrainFile() const
{
    return m_terrainFile;
}

void PfdQmlContext::setTerrainFile(const QString &arg)
{
    if (m_terrainFile != arg) {
        m_terrainFile = arg;
        emit terrainFileChanged(terrainFile());
    }
}

double PfdQmlContext::latitude() const
{
    return m_latitude;
}

void PfdQmlContext::setLatitude(double arg)
{
    if (m_latitude != arg) {
        m_latitude = arg;
        emit latitudeChanged(latitude());
    }
}

double PfdQmlContext::longitude() const
{
    return m_longitude;
}

void PfdQmlContext::setLongitude(double arg)
{
    if (m_longitude != arg) {
        m_longitude = arg;
        emit longitudeChanged(longitude());
    }
}

double PfdQmlContext::altitude() const
{
    return m_altitude;
}

void PfdQmlContext::setAltitude(double arg)
{
    if (m_altitude != arg) {
        m_altitude = arg;
        emit altitudeChanged(altitude());
    }
}

TimeMode::Enum PfdQmlContext::timeMode() const
{
    return m_timeMode;
}

void PfdQmlContext::setTimeMode(TimeMode::Enum arg)
{
    if (m_timeMode != arg) {
        m_timeMode = arg;
        emit timeModeChanged(timeMode());
    }
}

QDateTime PfdQmlContext::dateTime() const
{
    return m_dateTime;
}

void PfdQmlContext::setDateTime(QDateTime arg)
{
    if (m_dateTime != arg) {
        m_dateTime = arg;
        emit dateTimeChanged(dateTime());
    }
}

double PfdQmlContext::minimumAmbientLight() const
{
    return m_minAmbientLight;
}

void PfdQmlContext::setMinimumAmbientLight(double arg)
{
    if (m_minAmbientLight != arg) {
        m_minAmbientLight = arg;
        emit minimumAmbientLightChanged(minimumAmbientLight());
    }
}

QString PfdQmlContext::modelFile() const
{
    return m_modelFile;
}

void PfdQmlContext::setModelFile(const QString &arg)
{
    if (m_modelFile != arg) {
        m_modelFile  = arg;
        m_modelIndex = m_modelFileList.indexOf(m_modelFile);
        if (m_modelIndex == -1) {
            m_modelIndex = 0;
        }
        emit modelFileChanged(modelFile());
    }
}

void PfdQmlContext::nextModel()
{
    m_modelIndex = (m_modelIndex + 1) % m_modelFileList.length();
    setModelFile(m_modelFileList[m_modelIndex]);
}

void PfdQmlContext::previousModel()
{
    m_modelIndex = (m_modelFileList.length() + m_modelIndex - 1) % m_modelFileList.length();
    setModelFile(m_modelFileList[m_modelIndex]);
}

QStringList PfdQmlContext::modelFileList() const
{
    return m_modelFileList;
}

QString PfdQmlContext::backgroundImageFile() const
{
    return m_backgroundImageFile;
}

void PfdQmlContext::setBackgroundImageFile(const QString &arg)
{
    if (m_backgroundImageFile != arg) {
        m_backgroundImageFile = arg;
        emit backgroundImageFileChanged(backgroundImageFile());
    }
}

void PfdQmlContext::resetConsumedEnergy()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();

    Q_ASSERT(uavoManager);

    FlightBatterySettings *batterySettings = FlightBatterySettings::GetInstance(uavoManager);

    batterySettings->setResetConsumedEnergy(true);
    batterySettings->setData(batterySettings->getData());
}

QString PfdQmlContext::gstPipeline() const
{
    return m_gstPipeline;
}

void PfdQmlContext::setGstPipeline(const QString &arg)
{
    if (m_gstPipeline != arg) {
        m_gstPipeline = arg;
        emit gstPipelineChanged(gstPipeline());
    }
}

void PfdQmlContext::loadConfiguration(PfdQmlGadgetConfiguration *config)
{
    setSpeedFactor(config->speedFactor());
    setSpeedUnit(config->speedUnit());
    setAltitudeFactor(config->altitudeFactor());
    setAltitudeUnit(config->altitudeUnit());

    // terrain
    setTerrainEnabled(config->terrainEnabled());
    setTerrainFile(config->terrainFile());

    setLatitude(config->latitude());
    setLongitude(config->longitude());
    setAltitude(config->altitude());

    // sky
    setTimeMode(config->timeMode());
    setDateTime(config->dateTime());
    setMinimumAmbientLight(config->minAmbientLight());

    // model
    setModelFile(config->modelFile());

    // background image
    setBackgroundImageFile(config->backgroundImageFile());

    // gstreamer pipeline
    setGstPipeline(config->gstPipeline());
}


void PfdQmlContext::saveState(QSettings &settings) const
{
    settings.setValue("modelFile", Utils::RemoveDataPath(modelFile()));
}

void PfdQmlContext::restoreState(QSettings &settings)
{
    QString file = Utils::InsertDataPath(settings.value("modelFile").toString());

    if (!file.isEmpty()) {
        setModelFile(file);
    }
}

void PfdQmlContext::apply(QQmlContext *context)
{
    QStringList objectsToExport;

    objectsToExport <<
        "VelocityState" <<
        "PositionState" <<
        "AttitudeState" <<
        "AccelState" <<
        "VelocityDesired" <<
        "PathDesired" <<
        "GPSPositionSensor" <<
        "GPSSatellites" <<
        "HomeLocation" <<
        "GCSTelemetryStats" <<
        "SystemAlarms" <<
        "NedAccel" <<
        "ActuatorDesired" <<
        "TakeOffLocation" <<
        "PathPlan" <<
        "WaypointActive" <<
        "OPLinkStatus" <<
        "FlightStatus" <<
        "SystemStats" <<
        "StabilizationDesired" <<
        "VtolPathFollowerSettings" <<
        "HwSettings" <<
        "ManualControlCommand" <<
        "SystemSettings" <<
        "RevoSettings" <<
        "MagState" <<
        "AuxMagSettings" <<
        "FlightBatterySettings" <<
        "FlightBatteryState" <<
        "ReceiverStatus";

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    foreach(const QString &objectName, objectsToExport) {
        UAVObject *object = objManager->getObject(objectName);

        if (object) {
            // expose object with lower camel case name
            context->setContextProperty(Utils::toLowerCamelCase(objectName), object);
        } else {
            qWarning() << "PfdQmlContext::apply - failed to load object" << objectName;
        }
    }

    // expose this context to Qml
    context->setContextProperty(CONTEXT_PROPERTY_NAME, this);
}

void PfdQmlContext::addModelDir(QString dir)
{
    QDirIterator it(Utils::GetDataPath() + "models/" + dir, QStringList("*.3ds"), QDir::NoFilter, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString file = QDir::toNativeSeparators(it.next());
        // qDebug() << file;
        m_modelFileList.append(file);
    }
}
