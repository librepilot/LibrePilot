/**
 ******************************************************************************
 *
 * @file       configgroundvehiclewidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             K. Sebesta & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief ground vehicle configuration panel
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
#include "configgroundvehiclewidget.h"

#include "ui_airframe_ground.h"

#include "uavobjectmanager.h"

#include "mixersettings.h"
#include "systemsettings.h"
#include "actuatorsettings.h"
#include "actuatorcommand.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QBrush>
#include <QMessageBox>

#include <math.h>

QStringList ConfigGroundVehicleWidget::getChannelDescriptions()
{
    // init a channel_numelem list of channel desc defaults
    QStringList channelDesc;

    for (int i = 0; i < (int)ConfigGroundVehicleWidget::CHANNEL_NUMELEM; i++) {
        channelDesc.append("-");
    }

    // get the gui config data
    GUIConfigDataUnion configData = getConfigData();

    if (configData.ground.GroundVehicleSteering1 > 0) {
        channelDesc[configData.ground.GroundVehicleSteering1 - 1] = "GroundSteering1";
    }
    if (configData.ground.GroundVehicleSteering2 > 0) {
        channelDesc[configData.ground.GroundVehicleSteering2 - 1] = "GroundSteering2";
    }
    if (configData.ground.GroundVehicleThrottle1 > 0) {
        channelDesc[configData.ground.GroundVehicleThrottle1 - 1] = "GroundMotor1";
    }
    if (configData.ground.GroundVehicleThrottle2 > 0) {
        channelDesc[configData.ground.GroundVehicleThrottle2 - 1] = "GroundMotor2";
    }
    return channelDesc;
}

ConfigGroundVehicleWidget::ConfigGroundVehicleWidget(QWidget *parent) :
    VehicleConfig(parent)
{
    m_aircraft = new Ui_GroundConfigWidget();
    m_aircraft->setupUi(this);

    populateChannelComboBoxes();

    QStringList groundVehicleTypes;
    groundVehicleTypes << "Car (Turnable)" << "Tank (Differential)" << "Motorcycle" << "Boat (Turnable)" << "Boat (Differential)";
    m_aircraft->groundVehicleType->addItems(groundVehicleTypes);

    m_aircraft->groundShape->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_aircraft->groundShape->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set default model to "Car (Turnable)"
    m_aircraft->groundVehicleType->setCurrentIndex(m_aircraft->groundVehicleType->findText("Car (Turnable)"));

    connect(m_aircraft->groundVehicleType, SIGNAL(currentIndexChanged(QString)), this, SLOT(frameTypeChanged(QString)));

    setupUI(m_aircraft->groundVehicleType->currentText());
}

ConfigGroundVehicleWidget::~ConfigGroundVehicleWidget()
{
    delete m_aircraft;
}

QString ConfigGroundVehicleWidget::getFrameType()
{
    QString frameType = "GroundVehicleCar";

    // All frame types must start with "GroundVehicle"
    if (m_aircraft->groundVehicleType->currentText() == "Boat (Differential)") {
        frameType = "GroundVehicleDifferentialBoat";
    } else if (m_aircraft->groundVehicleType->currentText() == "Boat (Turnable)") {
        frameType = "GroundVehicleBoat";
    } else if (m_aircraft->groundVehicleType->currentText() == "Car (Turnable)") {
        frameType = "GroundVehicleCar";
    } else if (m_aircraft->groundVehicleType->currentText() == "Tank (Differential)") {
        frameType = "GroundVehicleDifferential";
    } else {
        frameType = "GroundVehicleMotorcycle";
    }
    return frameType;
}

/**
   Virtual function to setup the UI
 */
void ConfigGroundVehicleWidget::setupUI(QString frameType)
{
    // Setup the UI

    Q_ASSERT(m_aircraft);
    QSvgRenderer *renderer = new QSvgRenderer();
    renderer->load(QString(":/configgadget/images/ground-shapes.svg"));
    m_vehicleImg = new QGraphicsSvgItem();
    m_vehicleImg->setSharedRenderer(renderer);

    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));
    Q_ASSERT(system);
    QPointer<UAVObjectField> frameTypeSaved = system->getField("AirframeType");

    m_aircraft->differentialSteeringSlider1->setEnabled(false);
    m_aircraft->differentialSteeringSlider2->setEnabled(false);

    m_aircraft->gvThrottleCurve1GroupBox->setEnabled(true);
    m_aircraft->gvThrottleCurve2GroupBox->setEnabled(true);

    m_aircraft->groundVehicleThrottle1->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);
    m_aircraft->groundVehicleThrottle2->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);

    initMixerCurves(frameType);

    if (frameType == "GroundVehicleBoat" || frameType == "Boat (Turnable)") {
        setComboCurrentIndex(m_aircraft->groundVehicleType, m_aircraft->groundVehicleType->findText("Boat (Turnable)"));
        // Boat
        m_vehicleImg->setElementId("boat");

        m_aircraft->gvMotor1ChannelBox->setEnabled(true);
        m_aircraft->gvMotor2ChannelBox->setEnabled(true);

        m_aircraft->gvMotor1Label->setText("First motor");
        m_aircraft->gvMotor2Label->setText("Second motor");

        m_aircraft->gvSteering1ChannelBox->setEnabled(true);
        m_aircraft->gvSteering2ChannelBox->setEnabled(true);

        m_aircraft->gvSteering1Label->setText("First rudder");
        m_aircraft->gvSteering2Label->setText("Second rudder");

        m_aircraft->gvThrottleCurve1GroupBox->setTitle("Throttle Curve 1");
        m_aircraft->gvThrottleCurve1GroupBox->setEnabled(true);
        m_aircraft->gvThrottleCurve2GroupBox->setTitle("Throttle Curve 2");
        m_aircraft->gvThrottleCurve2GroupBox->setEnabled(false);

        m_aircraft->groundVehicleThrottle2->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);
        m_aircraft->groundVehicleThrottle1->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);

        initMixerCurves(frameType);

        // If new setup, set curves values
        if (frameTypeSaved->getValue().toString() != "GroundVehicleBoat") {
            m_aircraft->groundVehicleThrottle1->initLinearCurve(5, 1.0, 0.0);
            m_aircraft->groundVehicleThrottle2->initLinearCurve(5, 1.0, 0.0);
        }
    } else if ((frameType == "GroundVehicleDifferential") || (frameType == "Tank (Differential)") ||
               (frameType == "GroundVehicleDifferentialBoat") || (frameType == "Boat (Differential)")) {
        bool isBoat = frameType.contains("Boat");

        if (isBoat) {
            setComboCurrentIndex(m_aircraft->groundVehicleType, m_aircraft->groundVehicleType->findText("Boat (Differential)"));
            // Boat differential
            m_vehicleImg->setElementId("boat_diff");
            m_aircraft->gvSteering1Label->setText("First rudder");
            m_aircraft->gvSteering2Label->setText("Second rudder");
        } else {
            setComboCurrentIndex(m_aircraft->groundVehicleType, m_aircraft->groundVehicleType->findText("Tank (Differential)"));
            // Tank
            m_vehicleImg->setElementId("tank");
            m_aircraft->gvSteering1Label->setText("Front steering");
            m_aircraft->gvSteering2Label->setText("Rear steering");
        }

        m_aircraft->gvMotor1ChannelBox->setEnabled(true);
        m_aircraft->gvMotor2ChannelBox->setEnabled(true);

        m_aircraft->gvMotor1Label->setText("Left motor");
        m_aircraft->gvMotor2Label->setText("Right motor");

        m_aircraft->gvSteering1ChannelBox->setEnabled(false);
        m_aircraft->gvSteering2ChannelBox->setEnabled(false);

        m_aircraft->differentialSteeringSlider1->setEnabled(true);
        m_aircraft->differentialSteeringSlider2->setEnabled(true);

        m_aircraft->gvThrottleCurve1GroupBox->setTitle("Throttle Curve 1");
        m_aircraft->gvThrottleCurve1GroupBox->setEnabled(true);
        m_aircraft->gvThrottleCurve2GroupBox->setTitle("Throttle Curve 2 ");
        m_aircraft->gvThrottleCurve2GroupBox->setEnabled(false);

        m_aircraft->groundVehicleThrottle2->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);
        m_aircraft->groundVehicleThrottle1->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);

        initMixerCurves(frameType);

        // If new setup, set sliders to defaults and set curves values
        if ((!isBoat && (frameTypeSaved->getValue().toString() != "GroundVehicleDifferential")) ||
            (isBoat && (frameTypeSaved->getValue().toString() != "GroundVehicleDifferentialBoat"))) {
            m_aircraft->differentialSteeringSlider1->setValue(100);
            m_aircraft->differentialSteeringSlider2->setValue(100);
            m_aircraft->groundVehicleThrottle1->initLinearCurve(5, 0.8, 0.0);
            m_aircraft->groundVehicleThrottle2->initLinearCurve(5, 0.8, 0.0);
        }
    } else if (frameType == "GroundVehicleMotorcycle" || frameType == "Motorcycle") {
        setComboCurrentIndex(m_aircraft->groundVehicleType, m_aircraft->groundVehicleType->findText("Motorcycle"));
        // Motorcycle
        m_vehicleImg->setElementId("motorbike");

        m_aircraft->gvMotor1ChannelBox->setEnabled(false);
        m_aircraft->gvMotor2ChannelBox->setEnabled(true);

        m_aircraft->gvMotor2Label->setText("Rear motor");

        m_aircraft->gvSteering1ChannelBox->setEnabled(true);
        m_aircraft->gvSteering2ChannelBox->setEnabled(true);

        m_aircraft->gvSteering1Label->setText("Front steering");
        m_aircraft->gvSteering2Label->setText("Balancing");

        // Curve1 for Motorcyle
        m_aircraft->gvThrottleCurve1GroupBox->setTitle("Throttle Curve 1");
        m_aircraft->gvThrottleCurve1GroupBox->setEnabled(true);
        m_aircraft->gvThrottleCurve2GroupBox->setTitle("Throttle Curve 2");
        m_aircraft->gvThrottleCurve2GroupBox->setEnabled(false);

        m_aircraft->groundVehicleThrottle2->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);
        m_aircraft->groundVehicleThrottle1->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);

        initMixerCurves(frameType);

        // If new setup, set curves values
        if (frameTypeSaved->getValue().toString() != "GroundVehicleMotorCycle") {
            m_aircraft->groundVehicleThrottle2->initLinearCurve(5, 1.0, 0.0);
            m_aircraft->groundVehicleThrottle1->initLinearCurve(5, 1.0, 0.0);
        }
    } else if (frameType == "GroundVehicleCar" || frameType == "Car (Turnable)") {
        setComboCurrentIndex(m_aircraft->groundVehicleType, m_aircraft->groundVehicleType->findText("Car (Turnable)"));
        // Car
        m_vehicleImg->setElementId("car");

        m_aircraft->gvMotor1ChannelBox->setEnabled(true);
        m_aircraft->gvMotor2ChannelBox->setEnabled(true);

        m_aircraft->gvMotor1Label->setText("Front motor");
        m_aircraft->gvMotor2Label->setText("Rear motor");

        m_aircraft->gvSteering1ChannelBox->setEnabled(true);
        m_aircraft->gvSteering2ChannelBox->setEnabled(true);

        m_aircraft->gvSteering1Label->setText("Front steering");
        m_aircraft->gvSteering2Label->setText("Rear steering");

        m_aircraft->gvThrottleCurve1GroupBox->setTitle("Front Motor Throttle Curve");
        m_aircraft->gvThrottleCurve1GroupBox->setEnabled(true);
        m_aircraft->gvThrottleCurve2GroupBox->setTitle("Rear Motor Throttle Curve");
        m_aircraft->gvThrottleCurve2GroupBox->setEnabled(true);

        m_aircraft->groundVehicleThrottle2->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);
        m_aircraft->groundVehicleThrottle1->setMixerType(MixerCurve::MIXERCURVE_THROTTLE);

        initMixerCurves(frameType);

        // If new setup, set curves values
        if (frameTypeSaved->getValue().toString() != "GroundVehicleCar") {
            m_aircraft->groundVehicleThrottle1->initLinearCurve(5, 1.0, 0.0);
            m_aircraft->groundVehicleThrottle2->initLinearCurve(5, 1.0, 0.0);
        }
    }

    QGraphicsScene *scene = new QGraphicsScene();
    scene->addItem(m_vehicleImg);
    scene->setSceneRect(m_vehicleImg->boundingRect());
    m_aircraft->groundShape->fitInView(m_vehicleImg, Qt::KeepAspectRatio);
    m_aircraft->groundShape->setScene(scene);
}

void ConfigGroundVehicleWidget::enableControls(bool enable)
{
    if (enable) {
        setupUI(m_aircraft->groundVehicleType->currentText());
    }
}

void ConfigGroundVehicleWidget::registerWidgets(ConfigTaskWidget &parent)
{
    parent.addWidget(m_aircraft->groundVehicleThrottle1->getCurveWidget());
    parent.addWidget(m_aircraft->groundVehicleThrottle1);
    parent.addWidget(m_aircraft->groundVehicleThrottle2->getCurveWidget());
    parent.addWidget(m_aircraft->groundVehicleThrottle2);
    parent.addWidget(m_aircraft->groundVehicleType);
    parent.addWidget(m_aircraft->gvMotor1ChannelBox);
    parent.addWidget(m_aircraft->gvMotor2ChannelBox);
    parent.addWidget(m_aircraft->gvSteering1ChannelBox);
    parent.addWidget(m_aircraft->gvSteering2ChannelBox);
}

void ConfigGroundVehicleWidget::resetActuators(GUIConfigDataUnion *configData)
{
    configData->ground.GroundVehicleSteering1 = 0;
    configData->ground.GroundVehicleSteering2 = 0;
    configData->ground.GroundVehicleThrottle1 = 0;
    configData->ground.GroundVehicleThrottle2 = 0;
}

/**
   Virtual function to refresh the UI widget values
 */
void ConfigGroundVehicleWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    QString frameType = getFrameType();

    initMixerCurves(frameType);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);

    GUIConfigDataUnion config = getConfigData();

    // THIS SECTION STILL NEEDS WORK. FOR THE MOMENT, USE THE FIXED-WING ONBOARD SETTING IN ORDER TO MINIMIZE CHANCES OF BOLLOXING REAL CODE
    // Retrieve channel setup values
    setComboCurrentIndex(m_aircraft->gvMotor1ChannelBox, config.ground.GroundVehicleThrottle1);
    setComboCurrentIndex(m_aircraft->gvMotor2ChannelBox, config.ground.GroundVehicleThrottle2);
    setComboCurrentIndex(m_aircraft->gvSteering1ChannelBox, config.ground.GroundVehicleSteering1);
    setComboCurrentIndex(m_aircraft->gvSteering2ChannelBox, config.ground.GroundVehicleSteering2);

    if (frameType.contains("GroundVehicleDifferential")) {
        // Find the channel number for Motor1
        int channel = m_aircraft->gvMotor1ChannelBox->currentIndex() - 1;
        if (channel > -1) {
            // If for some reason the actuators were incoherent, we might fail here, hence the check.
            m_aircraft->differentialSteeringSlider1->setValue(
                getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW) / 1.27);
        }
        channel = m_aircraft->gvMotor2ChannelBox->currentIndex() - 1;
        if (channel > -1) {
            m_aircraft->differentialSteeringSlider2->setValue(
                -getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW) / 1.27);
        }
    }
}

/**
   Virtual function to update curve values from board
 */
void ConfigGroundVehicleWidget::initMixerCurves(QString frameType)
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    QList<double> curveValues;
    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, &curveValues);

    // is at least one of the curve values != 0?
    if (isValidThrottleCurve(&curveValues)) {
        // yes, use the curve we just read from mixersettings
        m_aircraft->groundVehicleThrottle1->initCurve(&curveValues);
    } else {
        // no, init a straight curve
        if (frameType.contains("GroundVehicleDifferential")) {
            m_aircraft->groundVehicleThrottle1->initLinearCurve(curveValues.count(), 0.8, 0.0);
        } else if (frameType == "GroundVehicleCar") {
            m_aircraft->groundVehicleThrottle1->initLinearCurve(curveValues.count(), 1.0, 0.0);
        } else {
            m_aircraft->groundVehicleThrottle1->initLinearCurve(curveValues.count(), 1.0, 0.0);
        }
    }

    // Setup all Throttle2 curves for all types of frames
    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE2, &curveValues);

    if (isValidThrottleCurve(&curveValues)) {
        m_aircraft->groundVehicleThrottle2->initCurve(&curveValues);
    } else {
        // no, init a straight curve
        if (frameType.contains("GroundVehicleDifferential")) {
            m_aircraft->groundVehicleThrottle2->initLinearCurve(curveValues.count(), 0.8, 0.0);
        } else if (frameType == "GroundVehicleCar") {
            m_aircraft->groundVehicleThrottle2->initLinearCurve(curveValues.count(), 1.0, 0.0);
        } else {
            m_aircraft->groundVehicleThrottle2->initLinearCurve(curveValues.count(), 1.0, 0.0);
        }
    }
}

/**
   Virtual function to update the UI widget objects
 */
void ConfigGroundVehicleWidget::updateObjectsFromWidgetsImpl()
{
    // Save the curve (common to all ground vehicle frames)
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    // set the throttle curves
    setThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, m_aircraft->groundVehicleThrottle1->getCurve());
    setThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE2, m_aircraft->groundVehicleThrottle2->getCurve());

    QString frameType = getFrameType();
    if (m_aircraft->groundVehicleType->currentText() == "Boat (Differential)") {
        setupGroundVehicleDifferential(frameType);
    } else if (m_aircraft->groundVehicleType->currentText() == "Boat (Turnable)") {
        setupGroundVehicleTurnable(frameType);
    } else if (m_aircraft->groundVehicleType->currentText() == "Car (Turnable)") {
        setupGroundVehicleTurnable(frameType);
    } else if (m_aircraft->groundVehicleType->currentText() == "Tank (Differential)") {
        setupGroundVehicleDifferential(frameType);
    } else {
        setupGroundVehicleMotorcycle(frameType);
    }
}

/**
   Setup balancing ground vehicle.

   Returns False if impossible to create the mixer.
 */
bool ConfigGroundVehicleWidget::setupGroundVehicleMotorcycle(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(frameType)) {
        return false;
    }

    // Now setup the channels:
    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.ground.GroundVehicleThrottle2 = m_aircraft->gvMotor2ChannelBox->currentIndex();
    config.ground.GroundVehicleSteering1 = m_aircraft->gvSteering1ChannelBox->currentIndex();
    config.ground.GroundVehicleSteering2 = m_aircraft->gvSteering2ChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    // motor
    int channel = m_aircraft->gvMotor2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, 127);

    // steering
    channel = m_aircraft->gvSteering1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, -127);

    // balance
    channel = m_aircraft->gvSteering2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, 127);

    m_aircraft->gvStatusLabel->setText("Mixer generated");

    return true;
}

/**
   Setup differentially steered ground vehicle.

   Returns False if impossible to create the mixer.
 */
bool ConfigGroundVehicleWidget::setupGroundVehicleDifferential(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI

    if (throwConfigError(frameType)) {
        return false;
    }

    // Now setup the channels:
    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.ground.GroundVehicleThrottle1 = m_aircraft->gvMotor1ChannelBox->currentIndex();
    config.ground.GroundVehicleThrottle2 = m_aircraft->gvMotor2ChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    double yawmotor1 = m_aircraft->differentialSteeringSlider1->value() * 1.27;
    double yawmotor2 = m_aircraft->differentialSteeringSlider2->value() * 1.27;

    // left motor
    int channel = m_aircraft->gvMotor1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_REVERSABLEMOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, yawmotor1);

    // right motor
    channel = m_aircraft->gvMotor2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_REVERSABLEMOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -yawmotor2);

    // Output success message
    m_aircraft->gvStatusLabel->setText("Mixer generated");

    return true;
}

/**
   Setup steerable ground vehicle.

   Returns False if impossible to create the mixer.
 */
bool ConfigGroundVehicleWidget::setupGroundVehicleTurnable(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(frameType)) {
        return false;
    }

    // Now setup the channels:
    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.ground.GroundVehicleThrottle1 = m_aircraft->gvMotor1ChannelBox->currentIndex();
    config.ground.GroundVehicleThrottle2 = m_aircraft->gvMotor2ChannelBox->currentIndex();
    config.ground.GroundVehicleSteering1 = m_aircraft->gvSteering1ChannelBox->currentIndex();
    config.ground.GroundVehicleSteering2 = m_aircraft->gvSteering2ChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    int channel = m_aircraft->gvSteering1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, 127);

    channel = m_aircraft->gvSteering2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -127);

    channel = m_aircraft->gvMotor1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_REVERSABLEMOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    channel = m_aircraft->gvMotor2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_REVERSABLEMOTOR);
    if (frameType == "GroundVehicleCar") {
        // Car: Throttle2 curve for 2nd motor
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
    } else {
        // Boat: Throttle1 curve for both motors
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
    }
    // Output success message
    m_aircraft->gvStatusLabel->setText("Mixer generated");

    return true;
}

/**
   This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigGroundVehicleWidget::throwConfigError(QString frameType)
{
    // Initialize configuration error flag
    bool error = false;

    // Create a red block. All combo boxes are the same size, so any one should do as a model
    int size   = m_aircraft->gvMotor1ChannelBox->style()->pixelMetric(QStyle::PM_SmallIconSize);
    QPixmap pixmap(size, size);

    pixmap.fill(QColor("red"));

    if ((frameType == "GroundVehicleCar") || (frameType == "GroundVehicleBoat")) { // Car
        if (m_aircraft->gvMotor1ChannelBox->currentText() == "None"
            && m_aircraft->gvMotor2ChannelBox->currentText() == "None") {
            m_aircraft->gvMotor1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            m_aircraft->gvMotor2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->gvMotor1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
            m_aircraft->gvMotor2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->gvSteering1ChannelBox->currentText() == "None"
            && m_aircraft->gvSteering2ChannelBox->currentText() == "None") {
            m_aircraft->gvSteering1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            m_aircraft->gvSteering2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->gvSteering1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
            m_aircraft->gvSteering2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }
    } else if (frameType.contains("GroundVehicleDifferential")) { // differential Tank and Boat
        if (m_aircraft->gvMotor1ChannelBox->currentText() == "None"
            || m_aircraft->gvMotor2ChannelBox->currentText() == "None") {
            m_aircraft->gvMotor1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            m_aircraft->gvMotor2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->gvMotor1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
            m_aircraft->gvMotor2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        // Always reset
        m_aircraft->gvSteering1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        m_aircraft->gvSteering2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
    } else if (frameType == "GroundVehicleMotorcycle") { // Motorcycle
        if (m_aircraft->gvMotor2ChannelBox->currentText() == "None") {
            m_aircraft->gvMotor2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->gvMotor2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->gvSteering1ChannelBox->currentText() == "None"
            && m_aircraft->gvSteering2ChannelBox->currentText() == "None") {
            m_aircraft->gvSteering1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->gvSteering1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        // Always reset
        m_aircraft->gvMotor1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        m_aircraft->gvSteering2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
    }

    if (error) {
        m_aircraft->gvStatusLabel->setText("<font color='red'>ERROR: Assign all necessary channels</font>");
    }
    return error;
}

void ConfigGroundVehicleWidget::resizeEvent(QResizeEvent *)
{
    if (m_vehicleImg) {
        m_aircraft->groundShape->fitInView(m_vehicleImg, Qt::KeepAspectRatio);
    }
}

void ConfigGroundVehicleWidget::showEvent(QShowEvent *)
{
    if (m_vehicleImg) {
        m_aircraft->groundShape->fitInView(m_vehicleImg, Qt::KeepAspectRatio);
    }
}
