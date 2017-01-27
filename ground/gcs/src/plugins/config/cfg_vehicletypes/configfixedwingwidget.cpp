/**
 ******************************************************************************
 *
 * @file       configfixedwingwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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

#include "ui_airframe_fixedwing.h"

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
#include <math.h>
#include <QMessageBox>

QStringList ConfigFixedWingWidget::getChannelDescriptions()
{
    // init a channel_numelem list of channel desc defaults
    QStringList channelDesc;

    for (int i = 0; i < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; i++) {
        channelDesc.append("-");
    }

    // get the gui config data
    GUIConfigDataUnion configData    = getConfigData();
    fixedGUISettingsStruct fixedwing = configData.fixedwing;

    if (configData.fixedwing.FixedWingPitch1 > 0) {
        channelDesc[configData.fixedwing.FixedWingPitch1 - 1] = "FixedWingPitch1";
    }
    if (configData.fixedwing.FixedWingPitch2 > 0) {
        channelDesc[configData.fixedwing.FixedWingPitch2 - 1] = "FixedWingPitch2";
    }
    if (configData.fixedwing.FixedWingRoll1 > 0) {
        channelDesc[configData.fixedwing.FixedWingRoll1 - 1] = "FixedWingRoll1";
    }
    if (configData.fixedwing.FixedWingRoll2 > 0) {
        channelDesc[configData.fixedwing.FixedWingRoll2 - 1] = "FixedWingRoll2";
    }
    if (configData.fixedwing.FixedWingYaw1 > 0) {
        channelDesc[configData.fixedwing.FixedWingYaw1 - 1] = "FixedWingYaw1";
    }
    if (configData.fixedwing.FixedWingYaw2 > 0) {
        channelDesc[configData.fixedwing.FixedWingYaw2 - 1] = "FixedWingYaw2";
    }
    if (configData.fixedwing.FixedWingThrottle > 0) {
        channelDesc[configData.fixedwing.FixedWingThrottle - 1] = "FixedWingThrottle";
    }

    if (fixedwing.Accessory0 > 0 && fixedwing.Accessory0 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory0 - 1] = "Accessory0-1";
    }
    if (fixedwing.Accessory1 > 0 && fixedwing.Accessory1 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory1 - 1] = "Accessory1-1";
    }
    if (fixedwing.Accessory2 > 0 && fixedwing.Accessory2 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory2 - 1] = "Accessory2-1";
    }
    if (fixedwing.Accessory3 > 0 && fixedwing.Accessory3 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory3 - 1] = "Accessory3-1";
    }

    if (fixedwing.Accessory0_2 > 0 && fixedwing.Accessory0_2 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory0_2 - 1] = "Accessory0-2";
    }
    if (fixedwing.Accessory1_2 > 0 && fixedwing.Accessory1_2 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory1_2 - 1] = "Accessory1-2";
    }
    if (fixedwing.Accessory2_2 > 0 && fixedwing.Accessory2_2 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory2_2 - 1] = "Accessory2-2";
    }
    if (fixedwing.Accessory3_2 > 0 && fixedwing.Accessory3_2 <= ConfigFixedWingWidget::CHANNEL_NUMELEM) {
        channelDesc[fixedwing.Accessory3_2 - 1] = "Accessory3-2";
    }

    return channelDesc;
}

ConfigFixedWingWidget::ConfigFixedWingWidget(QWidget *parent) :
    VehicleConfig(parent)
{
    m_aircraft = new Ui_FixedWingConfigWidget();
    m_aircraft->setupUi(this);

    populateChannelComboBoxes();

    QStringList mixerCurveList;
    mixerCurveList << "Curve1" << "Curve2";
    m_aircraft->rcOutputCurveBoxFw1->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBoxFw2->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBoxFw3->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBoxFw4->addItems(mixerCurveList);

    QStringList fixedWingTypes;
    fixedWingTypes << "Aileron" << "Elevon" << "Vtail";
    m_aircraft->fixedWingType->addItems(fixedWingTypes);

    m_aircraft->planeShape->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_aircraft->planeShape->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set default model to "Aileron"
    connect(m_aircraft->fixedWingType, SIGNAL(currentIndexChanged(QString)), this, SLOT(frameTypeChanged(QString)));
    m_aircraft->fixedWingType->setCurrentIndex(m_aircraft->fixedWingType->findText("Aileron"));
    setupUI(m_aircraft->fixedWingType->currentText());
}

ConfigFixedWingWidget::~ConfigFixedWingWidget()
{
    delete m_aircraft;
}

QString ConfigFixedWingWidget::getFrameType()
{
    QString frameType = "FixedWing";

    // All airframe types must start with "FixedWing"
    if (m_aircraft->fixedWingType->currentText() == "Aileron") {
        frameType = "FixedWing";
    } else if (m_aircraft->fixedWingType->currentText() == "Elevon") {
        frameType = "FixedWingElevon";
    } else { // "Vtail"
        frameType = "FixedWingVtail";
    }
    return frameType;
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

    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));
    Q_ASSERT(system);
    QPointer<UAVObjectField> field = system->getField("AirframeType");

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
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
        m_aircraft->elevonSlider3->setEnabled(true);

        m_aircraft->elevonSlider1->setValue(100);
        m_aircraft->elevonSlider2->setValue(100);

        // Get values saved if frameType = current frameType set on board
        if (field->getValue().toString() == "FixedWing") {
            m_aircraft->elevonSlider3->setValue(getMixerValue(mixer, "RollDifferential"));
        } else {
            m_aircraft->elevonSlider3->setValue(0);
        }
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
        m_aircraft->elevonSlider3->setEnabled(true);

        // Get values saved if frameType = current frameType set on board
        if (field->getValue().toString() == "FixedWingElevon") {
            m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueRoll"));
            m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
            m_aircraft->elevonSlider3->setValue(getMixerValue(mixer, "RollDifferential"));
        } else {
            m_aircraft->elevonSlider1->setValue(100);
            m_aircraft->elevonSlider2->setValue(100);
            m_aircraft->elevonSlider3->setValue(0);
        }
    } else if (frameType == "FixedWingVtail" || frameType == "Vtail") {
        planeimg->setElementId("vtail");
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Vtail"));
        m_aircraft->fwRudder1ChannelBox->setEnabled(false);
        m_aircraft->fwRudder1ChannelBox->setCurrentText("None");
        m_aircraft->fwRudder1ChannelBox->setToolTip("");
        m_aircraft->fwRudder2ChannelBox->setEnabled(false);
        m_aircraft->fwRudder2ChannelBox->setCurrentText("None");
        m_aircraft->fwRudder2ChannelBox->setToolTip("");

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
        m_aircraft->elevonSlider3->setEnabled(true);

        // Get values saved if frameType = current frameType set on board
        if (field->getValue().toString() == "FixedWingVtail") {
            m_aircraft->elevonSlider1->setValue(getMixerValue(mixer, "MixerValueYaw"));
            m_aircraft->elevonSlider2->setValue(getMixerValue(mixer, "MixerValuePitch"));
            m_aircraft->elevonSlider3->setValue(getMixerValue(mixer, "RollDifferential"));
        } else {
            m_aircraft->elevonSlider1->setValue(100);
            m_aircraft->elevonSlider2->setValue(100);
            m_aircraft->elevonSlider3->setValue(0);
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
    parent.addWidget(m_aircraft->elevonSlider3);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw1);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw2);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw3);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw4);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw1_2);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw2_2);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw3_2);
    parent.addWidget(m_aircraft->rcOutputChannelBoxFw4_2);
    parent.addWidget(m_aircraft->rcOutputCurveBoxFw1);
    parent.addWidget(m_aircraft->rcOutputCurveBoxFw2);
    parent.addWidget(m_aircraft->rcOutputCurveBoxFw3);
    parent.addWidget(m_aircraft->rcOutputCurveBoxFw4);
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

void ConfigFixedWingWidget::resetRcOutputs(GUIConfigDataUnion *configData)
{
    configData->fixedwing.Accessory0   = 0;
    configData->fixedwing.Accessory1   = 0;
    configData->fixedwing.Accessory2   = 0;
    configData->fixedwing.Accessory3   = 0;
    configData->fixedwing.Accessory0_2 = 0;
    configData->fixedwing.Accessory1_2 = 0;
    configData->fixedwing.Accessory2_2 = 0;
    configData->fixedwing.Accessory3_2 = 0;
}


void ConfigFixedWingWidget::updateRcCurvesUsed()
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw1, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw2, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw3, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw4, VehicleConfig::MIXER_THROTTLECURVE1);

    for (int channel = 0; channel < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; channel++) {
        QString mixerType = getMixerType(mixer, channel);
        if (mixerType == "Accessory0" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw1, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory1" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw2, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory2" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw3, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory3" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBoxFw4, VehicleConfig::MIXER_THROTTLECURVE2);
        }
    }
}

/**
   Virtual function to refresh the UI widget values
 */
void ConfigFixedWingWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
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

    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw1, fixed.Accessory0);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw2, fixed.Accessory1);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw3, fixed.Accessory2);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw4, fixed.Accessory3);

    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw1_2, fixed.Accessory0_2);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw2_2, fixed.Accessory1_2);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw3_2, fixed.Accessory2_2);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBoxFw4_2, fixed.Accessory3_2);

    updateRcCurvesUsed();

    // Get mixing values for GUI sliders (values stored onboard)
    m_aircraft->elevonSlider3->setValue(getMixerValue(mixer, "RollDifferential"));
    QString frameType = getFrameType();
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
void ConfigFixedWingWidget::updateObjectsFromWidgetsImpl()
{
    // Save the curve (common to all Fixed wing frames)
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    // Reset all Mixers type
    resetAllMixersType(mixer);

    QList<QString> rcOutputList;
    rcOutputList << "Accessory0" << "Accessory1" << "Accessory2" << "Accessory3";
    setupRcOutputs(rcOutputList);

    // Set the throttle curve
    setThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, m_aircraft->fixedWingThrottle->getCurve());

    QString frameType = getFrameType();
    if (m_aircraft->fixedWingType->currentText() == "Aileron") {
        setupFrameFixedWing(frameType);
    } else if (m_aircraft->fixedWingType->currentText() == "Elevon") {
        setupFrameElevon(frameType);
    } else { // "Vtail"
        setupFrameVtail(frameType);
    }
}

/**
   Setup Elevator/Aileron/Rudder airframe.

   If both Aileron channels are set to 'None' (EasyStar), do Pitch/Rudder mixing

   Returns False if impossible to create the mixer.
 */
bool ConfigFixedWingWidget::setupFrameFixedWing(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(frameType)) {
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
    config.fixedwing.FixedWingYaw2     = m_aircraft->fwRudder2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    setConfigData(config);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
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
    setMixerValue(mixer, "FirstRollServo", m_aircraft->fwAileron1ChannelBox->currentIndex());
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Store differential value onboard
        setMixerValue(mixer, "RollDifferential", m_aircraft->elevonSlider3->value());

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
bool ConfigFixedWingWidget::setupFrameElevon(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(frameType)) {
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

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
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
    setMixerValue(mixer, "FirstRollServo", m_aircraft->fwAileron1ChannelBox->currentIndex());
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Compute mixer absolute values
        pitch = (double)(m_aircraft->elevonSlider2->value() * 1.27);
        roll  = (double)(m_aircraft->elevonSlider1->value() * 1.27);

        // Store sliders values onboard
        setMixerValue(mixer, "RollDifferential", m_aircraft->elevonSlider3->value());
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
bool ConfigFixedWingWidget::setupFrameVtail(QString frameType)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(frameType)) {
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

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
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
    setMixerValue(mixer, "FirstRollServo", m_aircraft->fwAileron1ChannelBox->currentIndex());
    channel = m_aircraft->fwAileron1ChannelBox->currentIndex() - 1;
    if (channel > -1) {
        // Roll mixer value, currently no slider (should be added for Ailerons response ?)
        roll = 127;
        // Store Roll fixed and RollDifferential values onboard
        setMixerValue(mixer, "MixerValueRoll", 100);
        setMixerValue(mixer, "RollDifferential", m_aircraft->elevonSlider3->value());

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

/**
   Helper function: setup rc outputs. Takes a list of channel names in input.
 */
void ConfigFixedWingWidget::setupRcOutputs(QList<QString> rcOutputList)
{
    QList<QComboBox *> rcList;
    rcList << m_aircraft->rcOutputChannelBoxFw1 << m_aircraft->rcOutputChannelBoxFw2
           << m_aircraft->rcOutputChannelBoxFw3 << m_aircraft->rcOutputChannelBoxFw4;

    QList<QComboBox *> rcList2;
    rcList2 << m_aircraft->rcOutputChannelBoxFw1_2 << m_aircraft->rcOutputChannelBoxFw2_2
            << m_aircraft->rcOutputChannelBoxFw3_2 << m_aircraft->rcOutputChannelBoxFw4_2;

    GUIConfigDataUnion configData = getConfigData();
    resetRcOutputs(&configData);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);

    int curveAccessory0  = m_aircraft->rcOutputCurveBoxFw1->currentIndex();
    int curveAccessory1  = m_aircraft->rcOutputCurveBoxFw2->currentIndex();
    int curveAccessory2  = m_aircraft->rcOutputCurveBoxFw3->currentIndex();
    int curveAccessory3  = m_aircraft->rcOutputCurveBoxFw4->currentIndex();

    foreach(QString rc_output, rcOutputList) {
        int index  = rcList.takeFirst()->currentIndex();
        int index2 = rcList2.takeFirst()->currentIndex();

        if (rc_output == "Accessory0") {
            // First output for Accessory0
            configData.fixedwing.Accessory0 = index;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY0);
                if (curveAccessory0) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
            // Second output for Accessory0
            configData.fixedwing.Accessory0_2 = index2;
            if (index2) {
                setMixerType(mixer, index2 - 1, VehicleConfig::MIXERTYPE_ACCESSORY0);
                if (curveAccessory0) {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory1") {
            configData.fixedwing.Accessory1   = index;
            configData.fixedwing.Accessory1_2 = index2;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY1);
                if (curveAccessory1) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
            if (index2) {
                setMixerType(mixer, index2 - 1, VehicleConfig::MIXERTYPE_ACCESSORY1);
                if (curveAccessory1) {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory2") {
            configData.fixedwing.Accessory2   = index;
            configData.fixedwing.Accessory2_2 = index2;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY2);
                if (curveAccessory2) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
            if (index2) {
                setMixerType(mixer, index2 - 1, VehicleConfig::MIXERTYPE_ACCESSORY2);
                if (curveAccessory2) {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory3") {
            configData.fixedwing.Accessory3   = index;
            configData.fixedwing.Accessory3_2 = index2;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY3);
                if (curveAccessory3) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
            if (index2) {
                setMixerType(mixer, index2 - 1, VehicleConfig::MIXERTYPE_ACCESSORY3);
                if (curveAccessory3) {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index2 - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        }
    }
    setConfigData(configData);
}


void ConfigFixedWingWidget::enableControls(bool enable)
{
    if (enable) {
        setupUI(m_aircraft->fixedWingType->currentText());
    }
}

/**
   This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigFixedWingWidget::throwConfigError(QString frameType)
{
    // Initialize configuration error flag
    bool error = false;

    QList<QComboBox *> comboChannelsName;
    comboChannelsName << m_aircraft->fwEngineChannelBox
                      << m_aircraft->fwAileron1ChannelBox << m_aircraft->fwAileron2ChannelBox
                      << m_aircraft->fwElevator1ChannelBox << m_aircraft->fwElevator2ChannelBox
                      << m_aircraft->fwRudder1ChannelBox << m_aircraft->fwRudder2ChannelBox;
    QString channelNames = "";

    for (int i = 0; i < 7; i++) {
        QComboBox *combobox = comboChannelsName[i];
        if (combobox && (combobox->isEnabled())) {
            if (combobox->currentText() == "None") {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                if ((frameType == "FixedWingElevon") && (i > 2)) {
                    pixmap.fill(QColor("green"));
                    // Rudders are optional for elevon frame
                    combobox->setToolTip(tr("Rudders are optional for Elevon frame"));
                } else if (((frameType == "FixedWing") || (frameType == "FixedWingVtail")) && (i == 2)) {
                    pixmap.fill(QColor("green"));
                    // Second aileron servo is optional for FixedWing frame
                    combobox->setToolTip(tr("Second aileron servo is optional"));
                } else if ((frameType == "FixedWing") && (i > 3)) {
                    pixmap.fill(QColor("green"));
                    // Second elevator and rudders are optional for FixedWing frame
                    combobox->setToolTip(tr("Second elevator servo is optional"));
                    if (i > 4) {
                        combobox->setToolTip(tr("Rudder is highly recommended for fixed wing."));
                    }
                } else {
                    pixmap.fill(QColor("red"));
                    combobox->setToolTip(tr("Please assign Channel"));
                    m_aircraft->fwStatusLabel->setText(tr("<font color='red'>ERROR: Assign all necessary channels</font>"));
                    error = true;
                }
                combobox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
            } else if (channelNames.contains(combobox->currentText(), Qt::CaseInsensitive)) {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("orange"));
                combobox->setItemData(combobox->currentIndex(), pixmap, Qt::DecorationRole); // Set color palettes
                combobox->setToolTip(tr("Channel already used"));
                error = true;
            } else {
                for (int index = 0; index < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; index++) {
                    combobox->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox->setToolTip("");
                }
            }
            channelNames += (combobox->currentText() == "None") ? "" : combobox->currentText();
        }
    }

    // Iterate through all instances of rcOutputChannelBoxFw
    for (int i = 0; i < 5; i++) {
        // Find widgets with his name "rcOutputChannelBoxFw.x", where x is an integer
        QComboBox *combobox = this->findChild<QComboBox *>("rcOutputChannelBoxFw" + QString::number(i + 1));
        if (combobox) {
            if (channelNames.contains(combobox->currentText(), Qt::CaseInsensitive)) {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("orange"));
                combobox->setItemData(combobox->currentIndex(), pixmap, Qt::DecorationRole); // Set color palettes
                combobox->setToolTip(tr("Channel already used"));
            } else {
                for (int index = 0; index < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; index++) {
                    combobox->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox->setToolTip(tr("Select first output channel for Accessory%1 RcInput").arg(i));
                }
            }
            channelNames += (combobox->currentText() == "None") ? "" : combobox->currentText();
        }
        // Find duplicates in second output comboboxes
        QComboBox *combobox2 = this->findChild<QComboBox *>("rcOutputChannelBoxFw" + QString::number(i + 1) + "_2");
        if (combobox2) {
            if (channelNames.contains(combobox2->currentText(), Qt::CaseInsensitive)) {
                int size = combobox2->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("orange"));
                combobox2->setItemData(combobox2->currentIndex(), pixmap, Qt::DecorationRole); // Set color palettes
                combobox2->setToolTip(tr("Channel already used"));
            } else {
                for (int index = 0; index < (int)ConfigFixedWingWidget::CHANNEL_NUMELEM; index++) {
                    combobox2->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox2->setToolTip(tr("Select second output channel for Accessory%1 RcInput").arg(i));
                }
            }
            channelNames += (combobox2->currentText() == "None") ? "" : combobox2->currentText();
        }
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
