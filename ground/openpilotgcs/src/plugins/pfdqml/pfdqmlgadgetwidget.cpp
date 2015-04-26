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

#include "pfdqmlgadgetwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "flightbatterysettings.h"
#include "utils/svgimageprovider.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>

PfdQmlProperties::PfdQmlProperties(QObject *parent) : QObject(parent),
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

PfdQmlProperties::~PfdQmlProperties()
{}

QString PfdQmlProperties::speedUnit() const
{
    return m_speedUnit;
}

void PfdQmlProperties::setSpeedUnit(QString unit)
{
    if (m_speedUnit != unit) {
        m_speedUnit = unit;
        emit speedUnitChanged(speedUnit());
    }
}

double PfdQmlProperties::speedFactor() const
{
    return m_speedFactor;
}

void PfdQmlProperties::setSpeedFactor(double factor)
{
    if (m_speedFactor != factor) {
        m_speedFactor = factor;
        emit speedFactorChanged(speedFactor());
    }
}

QString PfdQmlProperties::altitudeUnit() const
{
    return m_altitudeUnit;
}

void PfdQmlProperties::setAltitudeUnit(QString unit)
{
    if (m_altitudeUnit != unit) {
        m_altitudeUnit = unit;
        emit altitudeUnitChanged(altitudeUnit());
    }
}

double PfdQmlProperties::altitudeFactor() const
{
    return m_altitudeFactor;
}

void PfdQmlProperties::setAltitudeFactor(double factor)
{
    if (m_altitudeFactor != factor) {
        m_altitudeFactor = factor;
        emit altitudeFactorChanged(altitudeFactor());
    }
}

bool PfdQmlProperties::terrainEnabled() const
{
    return m_terrainEnabled;
}

void PfdQmlProperties::setTerrainEnabled(bool arg)
{
    if (m_terrainEnabled != arg) {
        m_terrainEnabled = arg;
        emit terrainEnabledChanged(terrainEnabled());
    }
}

QString PfdQmlProperties::terrainFile() const
{
    return m_terrainFile;
}

void PfdQmlProperties::setTerrainFile(const QString &arg)
{
    if (m_terrainFile != arg) {
        m_terrainFile = arg;
        emit terrainFileChanged(terrainFile());
    }
}


double PfdQmlProperties::latitude() const
{
    return m_latitude;
}

void PfdQmlProperties::setLatitude(double arg)
{
    if (m_latitude != arg) {
        m_latitude = arg;
        emit latitudeChanged(latitude());
    }
}

double PfdQmlProperties::longitude() const
{
    return m_longitude;
}

void PfdQmlProperties::setLongitude(double arg)
{
    if (m_longitude != arg) {
        m_longitude = arg;
        emit longitudeChanged(longitude());
    }
}

double PfdQmlProperties::altitude() const
{
    return m_altitude;
}

void PfdQmlProperties::setAltitude(double arg)
{
    if (m_altitude != arg) {
        m_altitude = arg;
        emit altitudeChanged(altitude());
    }
}

Pfd::TimeMode PfdQmlProperties::timeMode() const
{
    return m_timeMode;
}

void PfdQmlProperties::setTimeMode(Pfd::TimeMode arg)
{
    if (m_timeMode != arg) {
        m_timeMode = arg;
        emit timeModeChanged(timeMode());
    }
}

QDateTime PfdQmlProperties::dateTime() const
{
    return m_dateTime;
}

void PfdQmlProperties::setDateTime(QDateTime arg)
{
    if (m_dateTime != arg) {
        m_dateTime = arg;
        emit dateTimeChanged(dateTime());
    }
}

double PfdQmlProperties::minimumAmbientLight() const
{
    return m_minAmbientLight;
}

void PfdQmlProperties::setMinimumAmbientLight(double arg)
{
    if (m_minAmbientLight != arg) {
        m_minAmbientLight = arg;
        emit minimumAmbientLightChanged(minimumAmbientLight());
    }
}

QString PfdQmlProperties::modelFile() const
{
    return m_modelFile;
}

void PfdQmlProperties::setModelFile(const QString &arg)
{
    if (m_modelFile != arg) {
        m_modelFile = arg;
        emit modelFileChanged(modelFile());
    }
}

QString PfdQmlProperties::backgroundImageFile() const
{
    return m_backgroundImageFile;
}

void PfdQmlProperties::setBackgroundImageFile(const QString &arg)
{
    if (m_backgroundImageFile != arg) {
        m_backgroundImageFile = arg;
        emit backgroundImageFileChanged(backgroundImageFile());
    }
}

void PfdQmlProperties::resetConsumedEnergy()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(uavoManager);

    FlightBatterySettings *batterySettings = FlightBatterySettings::GetInstance(uavoManager);

    batterySettings->setResetConsumedEnergy(true);
    batterySettings->setData(batterySettings->getData());
}

PfdQmlGadgetWidget::PfdQmlGadgetWidget(QWidget *parent) :
    QQuickWidget(parent), m_pfdQmlProperties(new PfdQmlProperties(this)), m_qmlFileName()
{
    setResizeMode(SizeRootObjectToView);

    QStringList objectsToExport;
    objectsToExport <<
        "VelocityState" <<
        "PositionState" <<
        "AttitudeState" <<
        "AccelState" <<
        "VelocityDesired" <<
        "PathDesired" <<
        "GPSPositionSensor" <<
        "HomeLocation" <<
        "GCSTelemetryStats" <<
        "SystemAlarms" <<
        "NedAccel" <<
        "FlightBatteryState" <<
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
        "FlightBatterySettings";

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    foreach(const QString &objectName, objectsToExport) {
        UAVObject *object = objManager->getObject(objectName);

        if (object) {
            engine()->rootContext()->setContextProperty(objectName, object);
        } else {
            qWarning() << "PfdQmlGadgetWidget - Failed to load object" << objectName;
        }
    }

#if 0
    qDebug() << "PfdQmlGadgetWidget - OpenGLContext persistent" << isPersistentOpenGLContext();
    qDebug() << "PfdQmlGadgetWidget - SceneGraph persistent" << isPersistentSceneGraph();
#endif

    // to expose settings values
    engine()->rootContext()->setContextProperty("qmlWidget", m_pfdQmlProperties);

    connect(this, &QQuickWidget::statusChanged, this, &PfdQmlGadgetWidget::onStatusChanged);
    connect(this, &QQuickWidget::sceneGraphError, this, &PfdQmlGadgetWidget::onSceneGraphError);
}

PfdQmlGadgetWidget::~PfdQmlGadgetWidget()
{}

void PfdQmlGadgetWidget::loadConfiguration(PfdQmlGadgetConfiguration *config)
{
    qDebug() << "PfdQmlGadgetWidget - loading configuration :" << config->name();

    // clear widget
    setQmlFile("");

    m_pfdQmlProperties->setSpeedFactor(config->speedFactor());
    m_pfdQmlProperties->setSpeedUnit(config->speedUnit());
    m_pfdQmlProperties->setAltitudeFactor(config->altitudeFactor());
    m_pfdQmlProperties->setAltitudeUnit(config->altitudeUnit());

    // terrain
    m_pfdQmlProperties->setTerrainEnabled(config->terrainEnabled());
    m_pfdQmlProperties->setTerrainFile(config->terrainFile());

    m_pfdQmlProperties->setLatitude(config->latitude());
    m_pfdQmlProperties->setLongitude(config->longitude());
    m_pfdQmlProperties->setAltitude(config->altitude());

    // sky
    m_pfdQmlProperties->setTimeMode(config->timeMode());
    m_pfdQmlProperties->setDateTime(config->dateTime());
    m_pfdQmlProperties->setMinimumAmbientLight(config->minAmbientLight());

    // model
    m_pfdQmlProperties->setModelFile(config->modelFile());

    // background image
    m_pfdQmlProperties->setBackgroundImageFile(config->backgroundImageFile());

    // go!
    setQmlFile(config->qmlFile());
}

void PfdQmlGadgetWidget::setQmlFile(QString fn)
{
    if (m_qmlFileName == fn) {
        return;
    }
    qDebug() << Q_FUNC_INFO << fn;

    m_qmlFileName = fn;

    if (fn.isEmpty()) {
        setSource(QUrl());

        engine()->removeImageProvider("svg");
        engine()->rootContext()->setContextProperty("svgRenderer", NULL);

        // calling clearComponentCache() causes crashes (see https://bugreports.qt-project.org/browse/QTBUG-41465)
        // but not doing it causes almost systematic crashes when switching PFD gadget to "Model View (Without Terrain)" configuration
        engine()->clearComponentCache();
    } else {
        SvgImageProvider *svgProvider = new SvgImageProvider(fn);
        engine()->addImageProvider("svg", svgProvider);

        // it's necessary to allow qml side to query svg element position
        engine()->rootContext()->setContextProperty("svgRenderer", svgProvider);

        QUrl url = QUrl::fromLocalFile(fn);
        engine()->setBaseUrl(url);
        setSource(url);
    }

    foreach(const QQmlError &error, errors()) {
        qDebug() << "PfdQmlGadgetWidget - " << error.description();
    }
}

void PfdQmlGadgetWidget::onStatusChanged(QQuickWidget::Status status)
{
    switch (status) {
    case Null:
        qDebug() << "PfdQmlGadgetWidget - status Null";
        break;
    case Ready:
        qDebug() << "PfdQmlGadgetWidget - status Ready";
        break;
    case Loading:
        qDebug() << "PfdQmlGadgetWidget - status Loading";
        break;
    case Error:
        qDebug() << "PfdQmlGadgetWidget - status Error";
        foreach(const QQmlError &error, errors()) {
            qWarning() << error.description();
        }
        break;
    }
}

void PfdQmlGadgetWidget::onSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
{
    qWarning() << QString("Scenegraph error %1: %2").arg(error).arg(message);
}
