/**
 ******************************************************************************
 *
 * @file       configfixedwingwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief fixed wing configuration panel
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
#include "configfixedwingwidget.h"
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
#include <math.h>
#include <QMessageBox>

QStringList ConfigFixedWingWidget::getChannelDescriptions()
{
    // init a channel_numelem list of channel desc defaults
    QStringList channelDesc;

    for (int i = 0; i < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; i++) {
        channelDesc.append(QString("-"));
    }

    // get the gui config data
    GUIConfigDataUnion configData = getConfigData();

    if (configData.fixedwing.FixedWingPitch1 > 0) {
        channelDesc[configData.fixedwing.FixedWingPitch1 - 1] = QString("FixedWingPitch1");
    }
    if (configData.fixedwing.FixedWingPitch2 > 0) {
        channelDesc[configData.fixedwing.FixedWingPitch2 - 1] = QString("FixedWingPitch2");
    }
    if (configData.fixedwing.FixedWingRoll1 > 0) {
        channelDesc[configData.fixedwing.FixedWingRoll1 - 1] = QString("FixedWingRoll1");
    }
    if (configData.fixedwing.FixedWingRoll2 > 0) {
        channelDesc[configData.fixedwing.FixedWingRoll2 - 1] = QString("FixedWingRoll2");
    }
    if (configData.fixedwing.FixedWingYaw1 > 0) {
        channelDesc[configData.fixedwing.FixedWingYaw1 - 1] = QString("FixedWingYaw1");
    }
    if (configData.fixedwing.FixedWingYaw2 > 0) {
        channelDesc[configData.fixedwing.FixedWingYaw2 - 1] = QString("FixedWingYaw2");
    }
    if (configData.fixedwing.FixedWingThrottle > 0) {
        channelDesc[configData.fixedwing.FixedWingThrottle - 1] = QString("FixedWingThrottle");
    }
    return channelDesc;
}

ConfigFixedWingWidget::ConfigFixedWingWidget(QWidget *parent) :
    VehicleConfig(parent), m_aircraft(new Ui_FixedWingConfigWidget())
{
    m_aircraft->setupUi(this);

    populateChannelComboBoxes();

    QStringList fixedWingTypes;
    fixedWingTypes << "Aileron" << "Elevon" << "Vtail";
    m_aircraft->fixedWingType->addItems(fixedWingTypes);

    m_aircraft->planeShape->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_aircraft->planeShape->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set default model to "Aileron"
    connect(m_aircraft->fixedWingType, SIGNAL(currentIndexChanged(QString)), this, SLOT(setupUI(QString)));
    m_aircraft->fixedWingType->setCurrentIndex(m_aircraft->fixedWingType->findText("Aileron"));
    setupUI(m_aircraft->fixedWingType->currentText());
}

ConfigFixedWingWidget::~ConfigFixedWingWidget()
{
    delete m_aircraft;
}

/**
   Virtual function to setup the UI
 */
void ConfigFixedWingWidget::setupUI(QString frameType)
{
    Q_ASSERT(m_aircraft);
    QSvgRenderer *renderer = new QSvgRenderer();
    renderer->load(QString(":/configgadget/images/fixedwing-shapes.svg"));
    planeimg = new QGraphicsSvgItem();
    planeimg->setSharedRenderer(renderer);

    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("SystemSettings")));
    Q_ASSERT(system);
    QPointer<UAVObjectField> field = system->getField(QString("AirframeType"));

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    if (frameType == "FixedWing" || frameType == "Aileron") {
        planeimg->setElementId("aileron");
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Aileron"));
        m_aircraft->fwRudder1ChannelBox->setEnabled(true);
        m_aircraft->fwRudder2ChannelBox->setEnabled(true);
        m_aircraft->fwElevator1ChannelBox->setEnabled(true);
        m_aircraft->fwElevator2ChannelBox->setEnabled(true);
        m_aircraft->fwAileron1ChannelBox->setEnabled(true);
        m_aircraft->fwAileron2ChannelBox->setEnabled(true);

        m_aircraft->fwAileron1Label->setText("Aileron 1");
        m_aircraft->fwAileron2Label->setText("Aileron 2");
        m_aircraft->fwElevator1Label->setText("Elevator 1");
        m_aircraft->fwElevator2Label->setText("Elevator 2");

        m_aircraft->elevonSlider1->setEnabled(false);
        m_aircraft->elevonSlider2->setEnabled(false);

        m_aircraft->elevonSlider1->setValue(100);
        m_aircraft->elevonSlider2->setValue(100);
    } else if (frameType == "FixedWingElevon" || frameType == "Elevon") {
        planeimg->setElementId("elevon");
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Elevon"));
        m_aircraft->fwAileron1Label->setText("Elevon 1");
        m_aircraft->fwAileron2Label->setText("Elevon 2");
        m_aircraft->fwElevator1ChannelBox->setEnabled(false);
        m_aircraft->fwElevator2ChannelBox->setEnabled(false);
        m_aircraft->fwRudder1ChannelBox->setEnabled(true);
        m_aircraft->fwElevator1ChannelBox->setCurrentText("None");
        m_aircraft->fwRudder2ChannelBox->setEnabled(true);
        m_aircraft->fwElevator2ChannelBox->setCurrentText("None");
        m_aircraft->fwElevator1Label->setText("Elevator 1");
        m_aircraft->fwElevator2Label->setText("Elevator 2");
        m_aircraft->elevonLabel1->setText("Roll");
        m_aircraft->elevonLabel2->setText("Pitch");

        m_aircraft->elevonSlider1->setEnabled(true);
        m_aircraft->elevonSlider2->setEnabled(true);

        // Get values saved if frameType = current frameType set on board
        if (field->getValue().toString() == "FixedWingElevon") {
            m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueRoll"));
            m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
        } else {
            m_aircraft->elevonSlider1->setValue(100);
            m_aircraft->elevonSlider2->setValue(100);
        }
    } else if (frameType == "FixedWingVtail" || frameType == "Vtail") {
        planeimg->setElementId("vtail");
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Vtail"));
        m_aircraft->fwRudder1ChannelBox->setEnabled(false);
        m_aircraft->fwRudder1ChannelBox->setCurrentText("None");
        m_aircraft->fwRudder2ChannelBox->setEnabled(false);
        m_aircraft->fwRudder2ChannelBox->setCurrentText("None");

        m_aircraft->fwElevator1Label->setText("Vtail 1");
        m_aircraft->fwElevator1ChannelBox->setEnabled(true);

        m_aircraft->fwElevator2Label->setText("Vtail 2");
        m_aircraft->fwElevator2ChannelBox->setEnabled(true);

        m_aircraft->fwAileron1Label->setText("Aileron 1");
        m_aircraft->fwAileron2Label->setText("Aileron 2");
        m_aircraft->elevonLabel1->setText("Rudder");
        m_aircraft->elevonLabel2->setText("Pitch");

        m_aircraft->elevonSlider1->setEnabled(true);
        m_aircraft->elevonSlider2->setEnabled(true);

        // Get values saved if frameType = current frameType set on board
        if (field->getValue().toString() == "FixedWingVtail") {
            m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueYaw"));
            m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
        } else {
            m_aircraft->elevonSlider1->setValue(100);
            m_aircraft->elevonSlider2->setValue(100);
        }
    }

    QGraphicsScene *scene = new QGraphicsScene();
    scene->addItem(planeimg);
    scene->setSceneRect(planeimg->boundingRect());
    m_aircraft->planeShape->fitInView(planeimg, Qt::KeepAspectRatio);
    m_aircraft->planeShape->setScene(scene);
}

void ConfigFixedWingWidget::registerWidgets(ConfigTaskWidget &parent)
{
    parent.addWidget(m_aircraft->fixedWingThrottle->getCurveWidget());
    parent.addWidget(m_aircraft->fixedWingThrottle);
    parent.addWidget(m_aircraft->fixedWingType);

    parent.addWidget(m_aircraft->fwEngineChannelBox);
    parent.addWidget(m_aircraft->fwAileron1ChannelBox);
    parent.addWidget(m_aircraft->fwAileron2ChannelBox);
    parent.addWidget(m_aircraft->fwElevator1ChannelBox);
    parent.addWidget(m_aircraft->fwElevator2ChannelBox);
    parent.addWidget(m_aircraft->fwRudder1ChannelBox);
    parent.addWidget(m_aircraft->fwRudder2ChannelBox);
    parent.addWidget(m_aircraft->elevonSlider1);
    parent.addWidget(m_aircraft->elevonSlider2);
}

void ConfigFixedWingWidget::resetActuators(GUIConfigDataUnion *configData)
{
    configData->fixedwing.FixedWingPitch1   = 0;
    configData->fixedwing.FixedWingPitch2   = 0;
    configData->fixedwing.FixedWingRoll1    = 0;
    configData->fixedwing.FixedWingRoll2    = 0;
    configData->fixedwing.FixedWingYaw1     = 0;
    configData->fixedwing.FixedWingYaw2     = 0;
    configData->fixedwing.FixedWingThrottle = 0;
}

/**
   Virtual function to refresh the UI widget values
 */
void ConfigFixedWingWidget::refreshWidgetsValues(QString frameType)
{
    Q_ASSERT(m_aircraft);

    setupUI(frameType);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    QList<double> curveValues;
    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, &curveValues);

    // is at least one of the curve values != 0?
    if (isValidThrottleCurve(&curveValues)) {
        // yes, use the curve we just read from mixersettings
        m_aircraft->fixedWingThrottle->initCurve(&curveValues);
    } else {
        // no, init a straight curve
        m_aircraft->fixedWingThrottle->initLinearCurve(curveValues.count(), 1.0);
    }

    GUIConfigDataUnion config    = getConfigData();
    fixedGUISettingsStruct fixed = config.fixedwing;

    // Then retrieve how channels are setup
    setComboCurrentIndex(m_aircraft->fwEngineChannelBox, fixed.FixedWingThrottle);
    setComboCurrentIndex(m_aircraft->fwAileron1ChannelBox, fixed.FixedWingRoll1);
    setComboCurrentIndex(m_aircraft->fwAileron2ChannelBox, fixed.FixedWingRoll2);
    setComboCurrentIndex(m_aircraft->fwElevator1ChannelBox, fixed.FixedWingPitch1);
    setComboCurrentIndex(m_aircraft->fwElevator2ChannelBox, fixed.FixedWingPitch2);
    setComboCurrentIndex(m_aircraft->fwRudder1ChannelBox, fixed.FixedWingYaw1);
    setComboCurrentIndex(m_aircraft->fwRudder2ChannelBox, fixed.FixedWingYaw2);

    // Get mixing values for GUI sliders (values stored onboard)
    if (frameType == "FixedWingElevon" || frameType == "Elevon") {
        m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueRoll"));
        m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
    } else if (frameType == "FixedWingVtail" || frameType == "Vtail") {
        m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueYaw"));
        m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
    }
}

/**
   Virtual function to update the UI widget objects
 */
QString ConfigFixedWingWidget::updateConfigObjectsFromWidgets()
{
    QString airframeType = "FixedWing";

    // Save the curve (common to all Fixed wing frames)
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));

    Q_ASSERT(mixer);

    // Remove Feed Forward, it is pointless on a plane:
    setMixerValue(mixer, "FeedForward", 0.0);

    // Set the throttle curve
    setThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, m_aircraft->fixedWingThrottle->getCurve());

    // All airframe types must start with "FixedWing"
    if (m_aircraft->fixedWingType->currentText() == "Aileron") {
        airframeType = "FixedWing";
        setupFrameFixedWing(airframeType);
    } else if (m_aircraft->fixedWingType->currentText() == "Elevon") {
        airframeType = "FixedWingElevon";
        setupFrameElevon(airframeType);
    } else { // "Vtail"
        airframeType = "FixedWingVtail";
        setupFrameVtail(airframeType);
    }

    return airframeType;
}

/**
   Setup Elevator/Aileron/Rudder airframe.

   If both Aileron channels are set to 'None' (EasyStar), do Pitch/Rudder mixing

   Returns False if impossible to create the mixer.
 */
bool ConfigFixedWingWidget::setupFrameFixedWing(QString airframeType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(airframeType)) {
        return false;
    }

    // Now setup the channels:
    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.fixedwing.FixedWingPitch1   = m_aircraft->fwElevator1ChannelBox->currentIndex();
    config.fixedwing.FixedWingPitch2   = m_aircraft->fwElevator2ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll1    = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2    = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw1     = m_aircraft->fwRudder1ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    // motor
    int channel = m_aircraft->fwEngineChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    // rudder
    channel = m_aircraft->fwRudder1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -127);

    // ailerons
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, 127);

        channel = m_aircraft->fwAileron2ChannelBox->currentIndex() - 1;
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, 127);
    }

    // elevators
    channel = m_aircraft->fwElevator1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, 127);

        channel = m_aircraft->fwElevator2ChannelBox->currentIndex() - 1;
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, 127);
    }

    m_aircraft->fwStatusLabel->setText("Mixer generated");

    return true;
}

/**
   Setup Elevon
 */
bool ConfigFixedWingWidget::setupFrameElevon(QString airframeType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(airframeType)) {
        return false;
    }

    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.fixedwing.FixedWingRoll1    = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2    = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw1     = m_aircraft->fwRudder1ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw2     = m_aircraft->fwRudder2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    // Save the curve:
    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    double pitch;
    double roll;
    double yaw;

    // motor
    int channel = m_aircraft->fwEngineChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    // Yaw mixer value, currently no slider (should be added for Rudder response ?)
    yaw     = 127;
    setMixerValue(mixer, "MixerValueYaw", 100);

    channel = m_aircraft->fwRudder1ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, yaw);

    channel = m_aircraft->fwRudder2ChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -yaw);

    // ailerons
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Compute mixer absolute values
        pitch = (double)(m_aircraft->elevonSlider2->value() * 1.27);
        roll  = (double)(m_aircraft->elevonSlider1->value() * 1.27);

        // Store sliders values onboard
        setMixerValue(mixer, "MixerValuePitch", m_aircraft->elevonSlider2->value());
        setMixerValue(mixer, "MixerValueRoll", m_aircraft->elevonSlider1->value());

        // First servo (left aileron)
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, -pitch);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, roll);

        // Second servo (right aileron)
        channel = m_aircraft->fwAileron2ChannelBox->currentIndex() - 1;
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, pitch);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, roll);
    }

    m_aircraft->fwStatusLabel->setText("Mixer generated");
    return true;
}

/**
   Setup VTail
 */
bool ConfigFixedWingWidget::setupFrameVtail(QString airframeType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(airframeType)) {
        return false;
    }

    GUIConfigDataUnion config = getConfigData();
    resetActuators(&config);

    config.fixedwing.FixedWingPitch1   = m_aircraft->fwElevator1ChannelBox->currentIndex();
    config.fixedwing.FixedWingPitch2   = m_aircraft->fwElevator2ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll1    = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2    = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    // Save the curve:
    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    double pitch;
    double roll;
    double yaw;

    // motor
    int channel = m_aircraft->fwEngineChannelBox->currentIndex() - 1;
    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    // ailerons
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Roll mixer value, currently no slider (should be added for Ailerons response ?)
        roll = 127;
        // Store Roll fixed value onboard
        setMixerValue(mixer, "MixerValueRoll", 100);

        // First Aileron (left)
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, roll);

        // Second Aileron (right)
        channel = m_aircraft->fwAileron2ChannelBox->currentIndex() - 1;
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, roll);
    }

    // vtail (pitch / yaw mixing)
    channel = m_aircraft->fwElevator1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Compute mixer absolute values
        pitch = (double)(m_aircraft->elevonSlider2->value() * 1.27);
        yaw   = (double)(m_aircraft->elevonSlider1->value() * 1.27);

        // Store sliders values onboard
        setMixerValue(mixer, "MixerValuePitch", m_aircraft->elevonSlider2->value());
        setMixerValue(mixer, "MixerValueYaw", m_aircraft->elevonSlider1->value());

        // First Vtail servo
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, -pitch);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -yaw);

        // Second Vtail servo
        channel = m_aircraft->fwElevator2ChannelBox->currentIndex() - 1;
        setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, pitch);
        setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, -yaw);
    }

    m_aircraft->fwStatusLabel->setText("Mixer generated");
    return true;
}

void ConfigFixedWingWidget::enableControls(bool enable)
{
    ConfigTaskWidget::enableControls(enable);

    if (enable) {
        setupUI(m_aircraft->fixedWingType->currentText());
    }
}

/**
   This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigFixedWingWidget::throwConfigError(QString airframeType)
{
    // Initialize configuration error flag
    bool error = false;

    // Create a red block. All combo boxes are the same size, so any one should do as a model
    int size   = m_aircraft->fwEngineChannelBox->style()->pixelMetric(QStyle::PM_SmallIconSize);
    QPixmap pixmap(size, size);

    pixmap.fill(QColor("red"));

    if (airframeType == "FixedWing") {
        if (m_aircraft->fwEngineChannelBox->currentText() == "None") {
            m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->fwElevator1ChannelBox->currentText() == "None") {
            m_aircraft->fwElevator1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwElevator1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if ((m_aircraft->fwAileron1ChannelBox->currentText() == "None")
            && (m_aircraft->fwRudder1ChannelBox->currentText() == "None")) {
            pixmap.fill(QColor("green"));
            m_aircraft->fwAileron1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            m_aircraft->fwRudder1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwAileron1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
            m_aircraft->fwRudder1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }
    } else if (airframeType == "FixedWingElevon") {
        if (m_aircraft->fwEngineChannelBox->currentText() == "None") {
            m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->fwAileron1ChannelBox->currentText() == "None") {
            m_aircraft->fwAileron1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwAileron1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->fwAileron2ChannelBox->currentText() == "None") {
            m_aircraft->fwAileron2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwAileron2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }
    } else if (airframeType == "FixedWingVtail") {
        if (m_aircraft->fwEngineChannelBox->currentText() == "None") {
            m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->fwElevator1ChannelBox->currentText() == "None") {
            m_aircraft->fwElevator1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwElevator1ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }

        if (m_aircraft->fwElevator2ChannelBox->currentText() == "None") {
            m_aircraft->fwElevator2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            error = true;
        } else {
            m_aircraft->fwElevator2ChannelBox->setItemData(0, 0, Qt::DecorationRole); // Reset color palettes
        }
    }

    if (error) {
        m_aircraft->fwStatusLabel->setText(QString("<font color='red'>ERROR: Assign all necessary channels</font>"));
    }

    return error;
}


void ConfigFixedWingWidget::resizeEvent(QResizeEvent *)
{
    if (planeimg) {
        m_aircraft->planeShape->fitInView(planeimg, Qt::KeepAspectRatio);
    }
}

void ConfigFixedWingWidget::showEvent(QShowEvent *)
{
    if (planeimg) {
        m_aircraft->planeShape->fitInView(planeimg, Qt::KeepAspectRatio);
    }
}
