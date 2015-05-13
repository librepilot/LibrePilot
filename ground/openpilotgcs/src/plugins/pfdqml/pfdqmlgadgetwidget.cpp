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

#include <QLayout>
#include <QStackedLayout>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>

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

/*
 * Note: QQuickWidget is an alternative to using QQuickView and QWidget::createWindowContainer().
 * The restrictions on stacking order do not apply, making QQuickWidget the more flexible alternative,
 * behaving more like an ordinary widget. This comes at the expense of performance.
 * Unlike QQuickWindow and QQuickView, QQuickWidget involves rendering into OpenGL framebuffer objects.
 * This will naturally carry a minor performance hit.
 *
 * Note: Using QQuickWidget disables the threaded render loop on all platforms.
 * This means that some of the benefits of threaded rendering, for example Animator classes
 * and vsync driven animations, will not be available.
 *
 * Note: Avoid calling winId() on a QQuickWidget. This function triggers the creation of a native window,
 * resulting in reduced performance and possibly rendering glitches.
 * The entire purpose of QQuickWidget is to render Quick scenes without a separate native window,
 * hence making it a native widget should always be avoided.
 */
QuickWidgetProxy::QuickWidgetProxy(PfdQmlGadgetWidget *parent) : QObject(parent)
{
    m_widget = true;

    m_quickWidget = NULL;

    m_container   = NULL;
    m_quickView   = NULL;

    if (m_widget) {
        m_quickWidget = new QQuickWidget(parent);
        m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

        void (PfdQmlGadgetWidget::*f)(QQuickWidget::Status) = &PfdQmlGadgetWidget::onStatusChanged;
        connect(m_quickWidget, &QQuickWidget::statusChanged, parent, f);
        connect(m_quickWidget, &QQuickWidget::sceneGraphError, parent, &PfdQmlGadgetWidget::onSceneGraphError);
    } else {
        m_quickView = new QQuickView();
        m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
        m_container = QWidget::createWindowContainer(m_quickView, parent);
        m_container->setMinimumSize(64, 64);
        m_container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        void (PfdQmlGadgetWidget::*f)(QQuickView::Status) = &PfdQmlGadgetWidget::onStatusChanged;
        connect(m_quickView, &QQuickView::statusChanged, parent, f);
        connect(m_quickView, &QQuickView::sceneGraphError, parent, &PfdQmlGadgetWidget::onSceneGraphError);
    }
}

QuickWidgetProxy::~QuickWidgetProxy()
{
    if (m_quickWidget) {
        delete m_quickWidget;
    }
    if (m_quickView) {
        delete m_quickView;
        delete m_container;
    }
}

QWidget *QuickWidgetProxy::widget()
{
    if (m_widget) {
        return m_quickWidget;
    } else {
        return m_container;
    }
}

QQmlEngine *QuickWidgetProxy::engine() const
{
    if (m_widget) {
        return m_quickWidget->engine();
    } else {
        return m_quickView->engine();
    }
}

void QuickWidgetProxy::setSource(const QUrl &url)
{
    if (m_widget) {
        m_quickWidget->setSource(url);
    } else {
        m_quickView->setSource(url);
    }
}

QList<QQmlError> QuickWidgetProxy::errors() const
{
    if (m_widget) {
        return m_quickWidget->errors();
    } else {
        return m_quickView->errors();
    }
}

PfdQmlGadgetWidget::PfdQmlGadgetWidget(QWidget *parent) :
    QWidget(parent), m_quickWidgetProxy(NULL), m_pfdQmlProperties(NULL), m_qmlFileName()
{
    setLayout(new QStackedLayout());
}

PfdQmlGadgetWidget::~PfdQmlGadgetWidget()
{
    if (m_pfdQmlProperties) {
        delete m_pfdQmlProperties;
    }
}

void PfdQmlGadgetWidget::init()
{
    m_quickWidgetProxy = new QuickWidgetProxy(this);

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
    qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent OpenGL context" << isPersistentOpenGLContext();
    qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent scene graph" << isPersistentSceneGraph();
#endif

    // to expose settings values
    m_pfdQmlProperties = new PfdQmlProperties(this);
    engine()->rootContext()->setContextProperty("qmlWidget", m_pfdQmlProperties);

// connect(this, &QQuickWidget::statusChanged, this, &PfdQmlGadgetWidget::onStatusChanged);
// connect(this, &QQuickWidget::sceneGraphError, this, &PfdQmlGadgetWidget::onSceneGraphError);
}

void PfdQmlGadgetWidget::loadConfiguration(PfdQmlGadgetConfiguration *config)
{
    qDebug() << "PfdQmlGadgetWidget::loadConfiguration" << config->name();

    QuickWidgetProxy *oldQuickWidgetProxy = NULL;
    PfdQmlProperties *oldPfdQmlProperties = NULL;
    if (m_quickWidgetProxy) {
        oldQuickWidgetProxy = m_quickWidgetProxy;
        oldPfdQmlProperties = m_pfdQmlProperties;
        m_quickWidgetProxy = NULL;
        m_pfdQmlProperties = NULL;
    }

    if (!m_quickWidgetProxy) {
        init();
    }

    // here we first clear the Qml file
    // then set all the properties
    // and finally set the desired Qml file
    // TODO this is a work around... some OSG Quick items don't yet handle properties updates well

    // clear widget
    // setQmlFile("");

    // no need to go further is Qml file is empty
    // if (config->qmlFile().isEmpty()) {
    // return;
    // }

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

    // deleting and recreating the PfdQmlGadgetWidget is workaround to avoid crashes in osgearth when
    // switching between configurations. Please remove this workaround once osgearth is stabilized
    if (oldQuickWidgetProxy) {
        layout()->removeWidget(oldQuickWidgetProxy->widget());
        delete oldQuickWidgetProxy;
        delete oldPfdQmlProperties;
    }
    layout()->addWidget(m_quickWidgetProxy->widget());
}

void PfdQmlGadgetWidget::setQmlFile(QString fn)
{
// if (m_qmlFileName == fn) {
// return;
// }
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
    case QQuickWidget::Null:
        qDebug() << "PfdQmlGadgetWidget - status Null";
        break;
    case QQuickWidget::Ready:
        qDebug() << "PfdQmlGadgetWidget - status Ready";
        break;
    case QQuickWidget::Loading:
        qDebug() << "PfdQmlGadgetWidget - status Loading";
        break;
    case QQuickWidget::Error:
        qDebug() << "PfdQmlGadgetWidget - status Error";
        foreach(const QQmlError &error, errors()) {
            qWarning() << error.description();
        }
        break;
    }
}

void PfdQmlGadgetWidget::onStatusChanged(QQuickView::Status status)
{
    switch (status) {
    case QQuickView::Null:
        qDebug() << "PfdQmlGadgetWidget - status Null";
        break;
    case QQuickView::Ready:
        qDebug() << "PfdQmlGadgetWidget - status Ready";
        break;
    case QQuickView::Loading:
        qDebug() << "PfdQmlGadgetWidget - status Loading";
        break;
    case QQuickView::Error:
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
