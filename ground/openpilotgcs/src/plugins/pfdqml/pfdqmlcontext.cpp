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

#include "pfdqmlcontext.h"

#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "flightbatterysettings.h"

#include <QQmlContext>
#include <QDebug>

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
    m_timeMode(Pfd::Local),
    m_dateTime(QDateTime()),
    m_minAmbientLight(0.03),
    m_modelFile(""),
    m_backgroundImageFile("")
{}

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

Pfd::TimeMode PfdQmlContext::timeMode() const
{
    return m_timeMode;
}

void PfdQmlContext::setTimeMode(Pfd::TimeMode arg)
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
        m_modelFile = arg;
        emit modelFileChanged(modelFile());
    }
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
        "FlightBatterySettings" <<
        "FlightBatteryState" <<
        "ReceiverStatus";

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    foreach(const QString &objectName, objectsToExport) {
        UAVObject *object = objManager->getObject(objectName);

        if (object) {
            context->setContextProperty(objectName, object);
        } else {
            qWarning() << "PfdQmlContext::apply - failed to load object" << objectName;
        }
    }

    // to expose settings values
    context->setContextProperty("qmlWidget", this);
}
