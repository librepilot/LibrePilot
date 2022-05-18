/**
 ******************************************************************************
 *
 * @file       configmultirotorwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief ccpm configuration panel
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
#include "configmultirotorwidget.h"

#include "ui_airframe_multirotor.h"

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
#include <QComboBox>
#include <QBrush>
#include <math.h>
#include <QMessageBox>

const QString ConfigMultiRotorWidget::CHANNELBOXNAME = "multiMotorChannelBox";

QStringList ConfigMultiRotorWidget::getChannelDescriptions()
{
    // init a channel_numelem list of channel desc defaults
    QStringList channelDesc;

    for (int i = 0; i < (int)ConfigMultiRotorWidget::CHANNEL_NUMELEM; i++) {
        channelDesc.append("-");
    }

    // get the gui config data
    GUIConfigDataUnion configData = getConfigData();
    multiGUISettingsStruct multi  = configData.multi;

    // Octocopter X motor definition
    if (multi.VTOLMotorNNE > 0 && multi.VTOLMotorNNE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorNNE - 1] = "VTOLMotorNNE";
    }
    if (multi.VTOLMotorENE > 0 && multi.VTOLMotorENE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorENE - 1] = "VTOLMotorENE";
    }
    if (multi.VTOLMotorESE > 0 && multi.VTOLMotorESE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorESE - 1] = "VTOLMotorESE";
    }
    if (multi.VTOLMotorSSE > 0 && multi.VTOLMotorSSE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorSSE - 1] = "VTOLMotorSSE";
    }
    if (multi.VTOLMotorSSW > 0 && multi.VTOLMotorSSW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorSSW - 1] = "VTOLMotorSSW";
    }
    if (multi.VTOLMotorWSW > 0 && multi.VTOLMotorWSW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorWSW - 1] = "VTOLMotorWSW";
    }
    if (multi.VTOLMotorWNW > 0 && multi.VTOLMotorWNW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorWNW - 1] = "VTOLMotorWNW";
    }
    if (multi.VTOLMotorNNW > 0 && multi.VTOLMotorNNW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorNNW - 1] = "VTOLMotorNNW";
    }
    // End OctocopterX

    if (multi.VTOLMotorN > 0 && multi.VTOLMotorN <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorN - 1] = "VTOLMotorN";
    }
    if (multi.VTOLMotorNE > 0 && multi.VTOLMotorNE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorNE - 1] = "VTOLMotorNE";
    }
    if (multi.VTOLMotorNW > 0 && multi.VTOLMotorNW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorNW - 1] = "VTOLMotorNW";
    }
    if (multi.VTOLMotorS > 0 && multi.VTOLMotorS <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorS - 1] = "VTOLMotorS";
    }
    if (multi.VTOLMotorSE > 0 && multi.VTOLMotorSE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorSE - 1] = "VTOLMotorSE";
    }
    if (multi.VTOLMotorSW > 0 && multi.VTOLMotorSW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorSW - 1] = "VTOLMotorSW";
    }
    if (multi.VTOLMotorW > 0 && multi.VTOLMotorW <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorW - 1] = "VTOLMotorW";
    }
    if (multi.VTOLMotorE > 0 && multi.VTOLMotorE <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.VTOLMotorE - 1] = "VTOLMotorE";
    }
    if (multi.TRIYaw > 0 && multi.TRIYaw <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.TRIYaw - 1] = "Tri-Yaw";
    }

    if (multi.Accessory0 > 0 && multi.Accessory0 <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.Accessory0 - 1] = "Accessory0";
    }
    if (multi.Accessory1 > 0 && multi.Accessory1 <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.Accessory1 - 1] = "Accessory1";
    }
    if (multi.Accessory2 > 0 && multi.Accessory2 <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.Accessory2 - 1] = "Accessory2";
    }
    if (multi.Accessory3 > 0 && multi.Accessory3 <= ConfigMultiRotorWidget::CHANNEL_NUMELEM) {
        channelDesc[multi.Accessory3 - 1] = "Accessory3";
    }

    return channelDesc;
}

ConfigMultiRotorWidget::ConfigMultiRotorWidget(QWidget *parent) :
    VehicleConfig(parent), invertMotors(false)
{
    m_aircraft = new Ui_MultiRotorConfigWidget();
    m_aircraft->setupUi(this);

    populateChannelComboBoxes();

    QStringList mixerCurveList;
    mixerCurveList << "Curve1" << "Curve2";
    m_aircraft->rcOutputCurveBox1->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBox2->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBox3->addItems(mixerCurveList);
    m_aircraft->rcOutputCurveBox4->addItems(mixerCurveList);

    // Setup the Multirotor picture in the Quad settings interface
    m_aircraft->quadShape->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_aircraft->quadShape->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QSvgRenderer *renderer = new QSvgRenderer();
    renderer->load(QString(":/configgadget/images/multirotor-shapes.svg"));

    quad = new QGraphicsSvgItem();
    quad->setSharedRenderer(renderer);
    quad->setElementId("quad-x");

    QGraphicsScene *scene = new QGraphicsScene();
    scene->addItem(quad);
    scene->setSceneRect(quad->boundingRect());
    m_aircraft->quadShape->setScene(scene);

    QStringList multiRotorTypes;
    multiRotorTypes << "Tricopter Y" << "Quad +" << "Quad X" << "Hexacopter" << "Hexacopter X" << "Hexacopter H"
                    << "Hexacopter Y6" << "Octocopter" << "Octocopter X" << "Octocopter V" << "Octo Coax +" << "Octo Coax X";
    m_aircraft->multirotorFrameType->addItems(multiRotorTypes);

    // Set default model to "Quad X"
    m_aircraft->multirotorFrameType->setCurrentIndex(m_aircraft->multirotorFrameType->findText("Quad X"));

    connect(m_aircraft->multirotorFrameType, SIGNAL(currentIndexChanged(QString)), this, SLOT(frameTypeChanged(QString)));

    // Connect the multirotor motor reverse checkbox
    connect(m_aircraft->MultirotorRevMixerCheckBox, SIGNAL(clicked(bool)), this, SLOT(reverseMultirotorMotor()));

    m_aircraft->multiThrottleCurve->setXAxisLabel(tr("Input"));
    m_aircraft->multiThrottleCurve->setYAxisLabel(tr("Output"));
}

ConfigMultiRotorWidget::~ConfigMultiRotorWidget()
{
    delete m_aircraft;
}

QString ConfigMultiRotorWidget::getFrameType()
{
    QString frameType = "QuadX";

    if (m_aircraft->multirotorFrameType->currentText() == "Quad +") {
        frameType = "QuadP";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Quad X") {
        frameType = "QuadX";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter") {
        frameType = "Hexa";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter X") {
        frameType = "HexaX";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter H") {
        frameType = "HexaH";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter Y6") {
        frameType = "HexaCoax";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter") {
        frameType = "Octo";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter X") {
        frameType = "OctoX";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter V") {
        frameType = "OctoV";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octo Coax +") {
        frameType = "OctoCoaxP";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octo Coax X") {
        frameType = "OctoCoaxX";
    } else if (m_aircraft->multirotorFrameType->currentText() == "Tricopter Y") {
        frameType = "Tri";
    }
    return frameType;
}

void ConfigMultiRotorWidget::setupUI(QString frameType)
{
    Q_ASSERT(m_aircraft);
    Q_ASSERT(quad);

    QStringList motorLabels;

    if (frameType == "Tri" || frameType == "Tricopter Y") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Tricopter Y"));

        motorLabels << "NW" << "NE" << "S";

        m_aircraft->mrRollMixLevel->setValue(100);
        m_aircraft->mrPitchMixLevel->setValue(100);
        setYawMixLevel(100);
    } else if (frameType == "QuadX" || frameType == "Quad X") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Quad X"));

        motorLabels << "NW" << "NE" << "SE" << "SW";

        // init mixer levels
        m_aircraft->mrRollMixLevel->setValue(50);
        m_aircraft->mrPitchMixLevel->setValue(50);
        setYawMixLevel(50);
    } else if (frameType == "QuadP" || frameType == "Quad +") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Quad +"));

        motorLabels << "N" << "E" << "S" << "W";

        m_aircraft->mrRollMixLevel->setValue(100);
        m_aircraft->mrPitchMixLevel->setValue(100);
        setYawMixLevel(50);
    } else if (frameType == "Hexa" || frameType == "Hexacopter") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Hexacopter"));

        motorLabels << "N" << "NE" << "SE" << "S" << "SW" << "NW";

        m_aircraft->mrRollMixLevel->setValue(100); // Old Roll 50 - Pitch 33 - Yaw 33
        m_aircraft->mrPitchMixLevel->setValue(100); // Do not alter mixer matrix
        setYawMixLevel(100);
    } else if (frameType == "HexaX" || frameType == "Hexacopter X") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType,
                             m_aircraft->multirotorFrameType->findText("Hexacopter X"));

        motorLabels << "NE" << "E" << "SE" << "SW" << "W" << "NW";

        m_aircraft->mrRollMixLevel->setValue(100); // Old: Roll 33 - Pitch 50 - Yaw 33
        m_aircraft->mrPitchMixLevel->setValue(100); // Do not alter mixer matrix
        setYawMixLevel(100);
    } else if (frameType == "HexaH" || frameType == "Hexacopter H") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType,
                             m_aircraft->multirotorFrameType->findText("Hexacopter H"));

        motorLabels << "NE" << "E" << "SE" << "SW" << "W" << "NW";

        m_aircraft->mrRollMixLevel->setValue(100); // Do not alter mixer matrix
        m_aircraft->mrPitchMixLevel->setValue(100); // All mixers RPY levels = 100%
        setYawMixLevel(100);
    } else if (frameType == "HexaCoax" || frameType == "Hexacopter Y6") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType,
                             m_aircraft->multirotorFrameType->findText("Hexacopter Y6"));

        motorLabels << "NW Top" << "NW Bottom" << "NE Top" << "NE Bottom" << "S Top" << "S Bottom";

        m_aircraft->mrRollMixLevel->setValue(86);
        m_aircraft->mrPitchMixLevel->setValue(100);
        setYawMixLevel(100);
    } else if (frameType == "Octo" || frameType == "Octocopter") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Octocopter"));

        motorLabels << "N" << "NE" << "E" << "SE" << "S" << "SW" << "W" << "NW";

        m_aircraft->mrRollMixLevel->setValue(100); // Do not alter mixer matrix
        m_aircraft->mrPitchMixLevel->setValue(100); // All mixers RPY levels = 100%
        setYawMixLevel(100);
    } else if (frameType == "OctoX" || frameType == "Octocopter X") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Octocopter X"));

        motorLabels << "NNE" << "ENE" << "ESE" << "SSE" << "SSW" << "WSW" << "WNW" << "NNW";

        m_aircraft->mrRollMixLevel->setValue(100); // Do not alter mixer matrix
        m_aircraft->mrPitchMixLevel->setValue(100); // All mixers RPY levels = 100%
        setYawMixLevel(100);
    } else if (frameType == "OctoV" || frameType == "Octocopter V") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType,
                             m_aircraft->multirotorFrameType->findText("Octocopter V"));

        motorLabels << "N" << "NE" << "E" << "SE" << "S" << "SW" << "W" << "NW";

        m_aircraft->mrRollMixLevel->setValue(25);
        m_aircraft->mrPitchMixLevel->setValue(25);
        setYawMixLevel(25);
    } else if (frameType == "OctoCoaxP" || frameType == "Octo Coax +") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Octo Coax +"));

        motorLabels << "N Top" << "N Bottom" << "E Top" << "E Bottom" << "S Top" << "S Bottom" << "W Top" << "W Bottom";

        m_aircraft->mrRollMixLevel->setValue(100);
        m_aircraft->mrPitchMixLevel->setValue(100);
        setYawMixLevel(50);
    } else if (frameType == "OctoCoaxX" || frameType == "Octo Coax X") {
        setComboCurrentIndex(m_aircraft->multirotorFrameType, m_aircraft->multirotorFrameType->findText("Octo Coax X"));

        motorLabels << "NW Top" << "NW Bottom" << "NE Top" << "NE Bottom" << "SE Top" << "SE Bottom" << "SW Top" << "SW Bottom";


        m_aircraft->mrRollMixLevel->setValue(50);
        m_aircraft->mrPitchMixLevel->setValue(50);
        setYawMixLevel(50);
    }

    // Enable/Disable controls
    setupEnabledControls(frameType);

    // Update motor position labels
    updateMotorsPositionLabels(motorLabels);

    // Draw the appropriate airframe
    updateAirframe(frameType);
}

void ConfigMultiRotorWidget::setupEnabledControls(QString frameType)
{
    // disable triyaw channel
    m_aircraft->triYawChannelBox->setEnabled(false);

    // disable all motor channel boxes
    for (int i = 1; i <= 8; i++) {
        // do it manually so we can turn off any error decorations
        QComboBox *combobox = this->findChild<QComboBox *>("multiMotorChannelBox" + QString::number(i));
        if (combobox) {
            combobox->setEnabled(false);
            combobox->setItemData(0, 0, Qt::DecorationRole);
        }
    }

    if (frameType == "Tri" || frameType == "Tricopter Y") {
        enableComboBoxes(this, CHANNELBOXNAME, 3, true);
        m_aircraft->triYawChannelBox->setEnabled(true);
    } else if (frameType == "QuadX" || frameType == "Quad X") {
        enableComboBoxes(this, CHANNELBOXNAME, 4, true);
    } else if (frameType == "QuadP" || frameType == "Quad +") {
        enableComboBoxes(this, CHANNELBOXNAME, 4, true);
    } else if (frameType == "Hexa" || frameType == "Hexacopter") {
        enableComboBoxes(this, CHANNELBOXNAME, 6, true);
    } else if (frameType == "HexaX" || frameType == "Hexacopter X") {
        enableComboBoxes(this, CHANNELBOXNAME, 6, true);
    } else if (frameType == "HexaH" || frameType == "Hexacopter H") {
        enableComboBoxes(this, CHANNELBOXNAME, 6, true);
    } else if (frameType == "HexaCoax" || frameType == "Hexacopter Y6") {
        enableComboBoxes(this, CHANNELBOXNAME, 6, true);
    } else if (frameType == "Octo" || frameType == "Octocopter") {
        enableComboBoxes(this, CHANNELBOXNAME, 8, true);
    } else if (frameType == "OctoX" || frameType == "Octocopter X") {
        enableComboBoxes(this, CHANNELBOXNAME, 8, true);
    } else if (frameType == "OctoV" || frameType == "Octocopter V") {
        enableComboBoxes(this, CHANNELBOXNAME, 8, true);
    } else if (frameType == "OctoCoaxP" || frameType == "Octo Coax +") {
        enableComboBoxes(this, CHANNELBOXNAME, 8, true);
    } else if (frameType == "OctoCoaxX" || frameType == "Octo Coax X") {
        enableComboBoxes(this, CHANNELBOXNAME, 8, true);
    }
}

void ConfigMultiRotorWidget::registerWidgets(ConfigTaskWidget &parent)
{
    parent.addWidget(m_aircraft->multiThrottleCurve->getCurveWidget());
    parent.addWidget(m_aircraft->multiThrottleCurve);
    parent.addWidget(m_aircraft->multirotorFrameType);
    parent.addWidget(m_aircraft->multiMotorChannelBox1);
    parent.addWidget(m_aircraft->multiMotorChannelBox2);
    parent.addWidget(m_aircraft->multiMotorChannelBox3);
    parent.addWidget(m_aircraft->multiMotorChannelBox4);
    parent.addWidget(m_aircraft->multiMotorChannelBox5);
    parent.addWidget(m_aircraft->multiMotorChannelBox6);
    parent.addWidget(m_aircraft->multiMotorChannelBox7);
    parent.addWidget(m_aircraft->multiMotorChannelBox8);
    parent.addWidget(m_aircraft->mrPitchMixLevel);
    parent.addWidget(m_aircraft->mrRollMixLevel);
    parent.addWidget(m_aircraft->mrYawMixLevel);
    parent.addWidget(m_aircraft->triYawChannelBox);
    parent.addWidget(m_aircraft->MultirotorRevMixerCheckBox);
    parent.addWidget(m_aircraft->rcOutputChannelBox1);
    parent.addWidget(m_aircraft->rcOutputChannelBox2);
    parent.addWidget(m_aircraft->rcOutputChannelBox3);
    parent.addWidget(m_aircraft->rcOutputChannelBox4);
    parent.addWidget(m_aircraft->rcOutputCurveBox1);
    parent.addWidget(m_aircraft->rcOutputCurveBox2);
    parent.addWidget(m_aircraft->rcOutputCurveBox3);
    parent.addWidget(m_aircraft->rcOutputCurveBox4);
}

void ConfigMultiRotorWidget::resetActuators(GUIConfigDataUnion *configData)
{
    configData->multi.VTOLMotorN   = 0;
    configData->multi.VTOLMotorNE  = 0;
    configData->multi.VTOLMotorE   = 0;
    configData->multi.VTOLMotorSE  = 0;
    configData->multi.VTOLMotorS   = 0;
    configData->multi.VTOLMotorSW  = 0;
    configData->multi.VTOLMotorW   = 0;
    configData->multi.VTOLMotorNW  = 0;
    configData->multi.TRIYaw = 0;
    configData->multi.VTOLMotorNNE = 0;
    configData->multi.VTOLMotorENE = 0;
    configData->multi.VTOLMotorESE = 0;
    configData->multi.VTOLMotorSSE = 0;
    configData->multi.VTOLMotorSSW = 0;
    configData->multi.VTOLMotorWSW = 0;
    configData->multi.VTOLMotorWNW = 0;
    configData->multi.VTOLMotorNNW = 0;
}

void ConfigMultiRotorWidget::resetRcOutputs(GUIConfigDataUnion *configData)
{
    configData->multi.Accessory0 = 0;
    configData->multi.Accessory1 = 0;
    configData->multi.Accessory2 = 0;
    configData->multi.Accessory3 = 0;
}

void ConfigMultiRotorWidget::updateRcCurvesUsed()
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    setComboCurrentIndex(m_aircraft->rcOutputCurveBox1, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBox2, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBox3, VehicleConfig::MIXER_THROTTLECURVE1);
    setComboCurrentIndex(m_aircraft->rcOutputCurveBox4, VehicleConfig::MIXER_THROTTLECURVE1);

    for (int channel = 0; channel < (int)ConfigMultiRotorWidget::CHANNEL_NUMELEM; channel++) {
        QString mixerType = getMixerType(mixer, channel);
        if (mixerType == "Accessory0" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBox1, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory1" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBox2, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory2" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBox3, VehicleConfig::MIXER_THROTTLECURVE2);
        } else if (mixerType == "Accessory3" && getMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2)) {
            setComboCurrentIndex(m_aircraft->rcOutputCurveBox4, VehicleConfig::MIXER_THROTTLECURVE2);
        }
    }
}

/**
   Helper function to refresh the UI widget values
 */
void ConfigMultiRotorWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);

    QList<double> curveValues;
    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, &curveValues);

    // is at least one of the curve values != 0?
    if (isValidThrottleCurve(&curveValues)) {
        // yes, use the curve we just read from mixersettings
        m_aircraft->multiThrottleCurve->initCurve(&curveValues);
    } else {
        // no, init a straight curve
        m_aircraft->multiThrottleCurve->initLinearCurve(curveValues.count(), 1.0);
    }

    GUIConfigDataUnion config    = getConfigData();
    multiGUISettingsStruct multi = config.multi;

    QString frameType = getFrameType();
    if (frameType == "QuadP") {
        // Motors 1/2/3/4 are: N / E / S / W
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorN);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorW);
    } else if (frameType == "QuadX") {
        // Motors 1/2/3/4 are: NW / NE / SE / SW
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorSW);
    } else if (frameType == "Hexa") {
        // Motors 1/2/3 4/5/6 are: N / NE / SE / S / SW / NW
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorN);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorNW);
    } else if (frameType == "HexaX") {
        // Motors 1/2/3 4/5/6 are: NE / E / SE / SW / W / NW
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorNW);
    } else if (frameType == "HexaH") {
        // Motors 1/2/3 4/5/6 are: NE / E / SE / SW / W / NW
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorNW);
    } else if (frameType == "HexaCoax") {
        // Motors 1/2/3 4/5/6 are: NW/W NE/E S/SE
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorSE);
    } else if (frameType == "Octo" || frameType == "OctoV" || frameType == "OctoCoaxP") {
        // Motors 1 to 8 are N / NE / E / etc
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorN);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox7, multi.VTOLMotorW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox8, multi.VTOLMotorNW);
    } else if (frameType == "OctoX") {
        // Motors 1 to 8 are NNE / ENE / ESE / etc
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorENE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorESE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorSSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorSSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorWSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox7, multi.VTOLMotorWNW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox8, multi.VTOLMotorNNW);
    } else if (frameType == "OctoCoaxX") {
        // Motors 1 to 8 are N / NE / E / etc
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorN);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox4, multi.VTOLMotorE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox5, multi.VTOLMotorSE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox6, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox7, multi.VTOLMotorSW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox8, multi.VTOLMotorW);
    } else if (frameType == "Tri") {
        // Motors 1 to 8 are N / NE / E / etc
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox1, multi.VTOLMotorNW);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox2, multi.VTOLMotorNE);
        setComboCurrentIndex(m_aircraft->multiMotorChannelBox3, multi.VTOLMotorS);
        setComboCurrentIndex(m_aircraft->triYawChannelBox, multi.TRIYaw);
    }

    setComboCurrentIndex(m_aircraft->rcOutputChannelBox1, multi.Accessory0);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBox2, multi.Accessory1);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBox3, multi.Accessory2);
    setComboCurrentIndex(m_aircraft->rcOutputChannelBox4, multi.Accessory3);

    updateRcCurvesUsed();

    // Now, read mixing values stored on board and applies values on sliders.
    m_aircraft->mrPitchMixLevel->setValue(getMixerValue(mixer, "MixerValuePitch"));
    m_aircraft->mrRollMixLevel->setValue(getMixerValue(mixer, "MixerValueRoll"));

    // MixerValueYaw : negative = reversed
    setYawMixLevel(getMixerValue(mixer, "MixerValueYaw"));

    updateAirframe(frameType);
}

/**
   Helper function to update the UI MotorPositionLabels text
 */
void ConfigMultiRotorWidget::updateMotorsPositionLabels(QStringList motorLabels)
{
    QList<QLabel *> mpList;
    mpList << m_aircraft->motorPositionLabel1 << m_aircraft->motorPositionLabel2
           << m_aircraft->motorPositionLabel3 << m_aircraft->motorPositionLabel4
           << m_aircraft->motorPositionLabel5 << m_aircraft->motorPositionLabel6
           << m_aircraft->motorPositionLabel7 << m_aircraft->motorPositionLabel8;

    int motorCount    = motorLabels.count();
    int uiLabelsCount = mpList.count();

    if (motorCount < uiLabelsCount) {
        // Fill labels for unused motors
        for (int index = motorCount; index < uiLabelsCount; index++) {
            motorLabels.insert(index, "Not used");
        }
    }

    foreach(QString posLabel, motorLabels) {
        mpList.takeFirst()->setText(posLabel);
    }
}

/**
   Function to update the UI widget objects
 */
void ConfigMultiRotorWidget::updateObjectsFromWidgetsImpl()
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    // Reset all Mixers types
    resetAllMixersType(mixer);

    QList<QString> rcOutputList;
    rcOutputList << "Accessory0" << "Accessory1" << "Accessory2" << "Accessory3";
    setupRcOutputs(rcOutputList);

    // Curve is also common to all quads:
    setThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, m_aircraft->multiThrottleCurve->getCurve());

    QList<QString> motorList;
    if (m_aircraft->multirotorFrameType->currentText() == "Quad +") {
        setupQuad(true);
    } else if (m_aircraft->multirotorFrameType->currentText() == "Quad X") {
        setupQuad(false);
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter") {
        setupHexa(true);
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter X") {
        setupHexa(false);
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter H") {
        // Show any config errors in GUI
        if (throwConfigError(6)) {
            return;
        }
        motorList << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorSW" << "VTOLMotorW" << "VTOLMotorNW";
        setupMotors(motorList);

        // Motor 1 to 6, H layout
        // ---------------------------------------------
        // Motor:   1      2      3     4     5     6
        // ROLL  {-0.50, -1.00, -0.50, 0.50, 1.00, 0.50}
        // PITCH {1.00, 0.00, -1.00, -1.00, -0.00, 1.00}
        // YAW   {-0.50, 1.00, -0.50, 0.50, -1.00, 0.50}
        // ---------------------------------------------
        // http://wiki.paparazziuav.org/wiki/RotorcraftMixing
        // pitch  roll  yaw
        double hMixer[8][3] = {
            { 1,  -0.5, -0.5 },
            { 0,  -1,   1    },
            { -1, -0.5, -0.5 },
            { -1, 0.5,  0.5  },
            { 0,  1,    -1   },
            { 1,  0.5,  0.5  },
            { 0,  0,    0    },
            { 0,  0,    0    }
        };
        setupMultiRotorMixer(hMixer);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Hexacopter Y6") {
        // Show any config errors in GUI
        if (throwConfigError(6)) {
            return;
        }
        motorList << "VTOLMotorNW" << "VTOLMotorW" << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorS" << "VTOLMotorSE";
        setupMotors(motorList);

        // Motor 1 to 6, Y6 Layout:
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 0.5, 1,  -1 },
            { 0.5, 1,  1  },
            { 0.5, -1, -1 },
            { 0.5, -1, 1  },
            { -1,  0,  -1 },
            { -1,  0,  1  },
            { 0,   0,  0  },
            { 0,   0,  0  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter") {
        // Show any config errors in GUI
        if (throwConfigError(8)) {
            return;
        }
        motorList << "VTOLMotorN" << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorS" << "VTOLMotorSW"
                  << "VTOLMotorW" << "VTOLMotorNW";
        setupMotors(motorList);
        // Motor 1 to 8, OctoP
        // -------------------------------------------------------
        // Motor:   1      2      3     4     5     6     7     8
        // ROLL  {0.00, -0.71, -1.00, -0.71, 0.00, 0.71, 1.00, 0.71}
        // PITCH {1.00,  0.71, -0.00, -0.71,-1.00,-0.71,-0.00, 0.71}
        // YAW   {-1.00, 1.00, -1.00, 1.00, -1.00, 1.00,-1.00, 1.00}
        // -------------------------------------------------------
        // http://wiki.paparazziuav.org/wiki/RotorcraftMixing
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 1,     0,     -1 },
            { 0.71,  -0.71, 1  },
            { 0,     -1,    -1 },
            { -0.71, -0.71, 1  },
            { -1,    0,     -1 },
            { -0.71, 0.71,  1  },
            { 0,     1,     -1 },
            { 0.71,  0.71,  1  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter X") {
        // Show any config errors in GUI
        if (throwConfigError(8)) {
            return;
        }
        motorList << "VTOLMotorNNE" << "VTOLMotorENE" << "VTOLMotorESE" << "VTOLMotorSSE" << "VTOLMotorSSW" << "VTOLMotorWSW"
                  << "VTOLMotorWNW" << "VTOLMotorNNW";
        setupMotors(motorList);
        // Motor 1 to 8, OctoX
        // --------------------------------------------------------
        // Motor:   1      2      3      4     5     6     7     8
        // ROLL  {-0.41, -1.00, -1.00, -0.41, 0.41, 1.00, 1.00, 0.41}
        // PITCH {1.00, 0.41, -0.41, -1.00, -1.00, -0.41, 0.41, 1.00}
        // YAW   {-1.00, 1.00, -1.00, 1.00, -1.00, 1.00, -1.00, 1.00}
        // --------------------------------------------------------
        // http://wiki.paparazziuav.org/wiki/RotorcraftMixing
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 1,     -0.41, -1 },
            { 0.41,  -1,    1  },
            { -0.41, -1,    -1 },
            { -1,    -0.41, 1  },
            { -1,    0.41,  -1 },
            { -0.41, 1,     1  },
            { 0.41,  1,     -1 },
            { 1,     0.41,  1  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octocopter V") {
        // Show any config errors in GUI
        if (throwConfigError(8)) {
            return;
        }
        motorList << "VTOLMotorN" << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorS" << "VTOLMotorSW"
                  << "VTOLMotorW" << "VTOLMotorNW";
        setupMotors(motorList);
        // Motor 1 to 8:
        // IMPORTANT: Assumes evenly spaced engines
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 0.33,  -1, -1 },
            { 1,     -1, 1  },
            { -1,    -1, -1 },
            { -0.33, -1, 1  },
            { -0.33, 1,  -1 },
            { -1,    1,  1  },
            { 1,     1,  -1 },
            { 0.33,  1,  1  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octo Coax +") {
        // Show any config errors in GUI
        if (throwConfigError(8)) {
            return;
        }
        motorList << "VTOLMotorN" << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorS" << "VTOLMotorSW"
                  << "VTOLMotorW" << "VTOLMotorNW";
        setupMotors(motorList);
        // Motor 1 to 8:
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 1,  0,  -1 },
            { 1,  0,  1  },
            { 0,  -1, -1 },
            { 0,  -1, 1  },
            { -1, 0,  -1 },
            { -1, 0,  1  },
            { 0,  1,  -1 },
            { 0,  1,  1  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Octo Coax X") {
        // Show any config errors in GUI
        if (throwConfigError(8)) {
            return;
        }
        motorList << "VTOLMotorNW" << "VTOLMotorN" << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorS"
                  << "VTOLMotorSW" << "VTOLMotorW";
        setupMotors(motorList);
        // Motor 1 to 8:
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 1,  1,  -1 },
            { 1,  1,  1  },
            { 1,  -1, -1 },
            { 1,  -1, 1  },
            { -1, -1, -1 },
            { -1, -1, 1  },
            { -1, 1,  -1 },
            { -1, 1,  1  }
        };
        setupMultiRotorMixer(mixerMatrix);
        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    } else if (m_aircraft->multirotorFrameType->currentText() == "Tricopter Y") {
        // Show any config errors in GUI
        if (throwConfigError(3)) {
            return;
        }
        if (m_aircraft->triYawChannelBox->currentText() == "None") {
            m_aircraft->mrStatusLabel->setText(tr("<font color='red'>ERROR: Assign a Yaw channel</font>"));
            return;
        }
        motorList << "VTOLMotorNW" << "VTOLMotorNE" << "VTOLMotorS";
        setupMotors(motorList);

        GUIConfigDataUnion config = getConfigData();
        config.multi.TRIYaw = m_aircraft->triYawChannelBox->currentIndex();
        setConfigData(config);

        // Motor 1 to 3, Tricopter Y Layout:
        // pitch   roll    yaw
        double mixerMatrix[8][3] = {
            { 0.5, 1,  0 },
            { 0.5, -1, 0 },
            { -1,  0,  0 },
            { 0,   0,  0 },
            { 0,   0,  0 },
            { 0,   0,  0 },
            { 0,   0,  0 },
            { 0,   0,  0 }
        };
        setupMultiRotorMixer(mixerMatrix);

        // tell the mixer about tricopter yaw channel

        int channel = m_aircraft->triYawChannelBox->currentIndex() - 1;
        if (channel > -1) {
            setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_SERVO);

            // Tricopter : Yaw mix slider value applies to servo (was fixed)
            // Get absolute MixerValueYaw, no servo reverse when Reverse All Motors is checked
            setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, abs(int(getMixerValue(mixer, "MixerValueYaw"))) * 1.27);
        }

        m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    }
}

void ConfigMultiRotorWidget::setYawMixLevel(int value)
{
    if (value < 0) {
        m_aircraft->mrYawMixLevel->setValue(-value);
        m_aircraft->MultirotorRevMixerCheckBox->setChecked(true);
    } else {
        m_aircraft->mrYawMixLevel->setValue(value);
        m_aircraft->MultirotorRevMixerCheckBox->setChecked(false);
    }
}

void ConfigMultiRotorWidget::reverseMultirotorMotor()
{
    QString frameType = m_aircraft->multirotorFrameType->currentText();

    updateAirframe(frameType);
}

void ConfigMultiRotorWidget::updateAirframe(QString frameType)
{
    // qDebug() << "ConfigMultiRotorWidget::updateAirframe - frame type" << frameType;

    QString elementId;

    if (frameType == "Tri" || frameType == "Tricopter Y") {
        elementId = "tri";
    } else if (frameType == "QuadX" || frameType == "Quad X") {
        elementId = "quad-x";
    } else if (frameType == "QuadP" || frameType == "Quad +") {
        elementId = "quad-plus";
    } else if (frameType == "Hexa" || frameType == "Hexacopter") {
        elementId = "quad-hexa";
    } else if (frameType == "HexaX" || frameType == "Hexacopter X") {
        elementId = "quad-hexa-X";
    } else if (frameType == "HexaH" || frameType == "Hexacopter H") {
        elementId = "quad-hexa-H";
    } else if (frameType == "HexaCoax" || frameType == "Hexacopter Y6") {
        elementId = "hexa-coax";
    } else if (frameType == "Octo" || frameType == "Octocopter") {
        elementId = "quad-octo";
    } else if (frameType == "OctoX" || frameType == "Octocopter X") {
        elementId = "quad-octo-X";
    } else if (frameType == "OctoV" || frameType == "Octocopter V") {
        elementId = "quad-octo-v";
    } else if (frameType == "OctoCoaxP" || frameType == "Octo Coax +") {
        elementId = "octo-coax-P";
    } else if (frameType == "OctoCoaxX" || frameType == "Octo Coax X") {
        elementId = "octo-coax-X";
    }

    invertMotors = m_aircraft->MultirotorRevMixerCheckBox->isChecked();
    if (invertMotors) {
        elementId += "_reverse";
    }

    if (elementId != "" && elementId != quad->elementId()) {
        quad->setElementId(elementId);
        m_aircraft->quadShape->setSceneRect(quad->boundingRect());
        m_aircraft->quadShape->fitInView(quad, Qt::KeepAspectRatio);
    }
}

/**
   Helper function: setupQuadMotor
 */
void ConfigMultiRotorWidget::setupQuadMotor(int channel, double pitch, double roll, double yaw)
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);

    setMixerType(mixer, channel, VehicleConfig::MIXERTYPE_MOTOR);

    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 0);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_ROLL, roll * 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_PITCH, pitch * 127);
    setMixerVectorValue(mixer, channel, VehicleConfig::MIXERVECTOR_YAW, yaw * 127);
}

/**
   Helper function: setup rc outputs. Takes a list of channel names in input.
 */
void ConfigMultiRotorWidget::setupRcOutputs(QList<QString> rcOutputList)
{
    QList<QComboBox *> rcList;
    rcList << m_aircraft->rcOutputChannelBox1 << m_aircraft->rcOutputChannelBox2
           << m_aircraft->rcOutputChannelBox3 << m_aircraft->rcOutputChannelBox4;

    GUIConfigDataUnion configData = getConfigData();
    resetRcOutputs(&configData);

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);

    int curveAccessory0  = m_aircraft->rcOutputCurveBox1->currentIndex();
    int curveAccessory1  = m_aircraft->rcOutputCurveBox2->currentIndex();
    int curveAccessory2  = m_aircraft->rcOutputCurveBox3->currentIndex();
    int curveAccessory3  = m_aircraft->rcOutputCurveBox4->currentIndex();

    foreach(QString rc_output, rcOutputList) {
        int index = rcList.takeFirst()->currentIndex();

        if (rc_output == "Accessory0") {
            configData.multi.Accessory0 = index;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY0);
                if (curveAccessory0) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory1") {
            configData.multi.Accessory1 = index;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY1);
                if (curveAccessory1) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory2") {
            configData.multi.Accessory2 = index;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY2);
                if (curveAccessory2) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        } else if (rc_output == "Accessory3") {
            configData.multi.Accessory3 = index;
            if (index) {
                setMixerType(mixer, index - 1, VehicleConfig::MIXERTYPE_ACCESSORY3);
                if (curveAccessory3) {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE2, 127);
                } else {
                    setMixerVectorValue(mixer, index - 1, VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);
                }
            }
        }
    }
    setConfigData(configData);
}

/**
   Helper function: setup motors. Takes a list of channel names in input.
 */
void ConfigMultiRotorWidget::setupMotors(QList<QString> motorList)
{
    QList<QComboBox *> mmList;
    mmList << m_aircraft->multiMotorChannelBox1 << m_aircraft->multiMotorChannelBox2
           << m_aircraft->multiMotorChannelBox3 << m_aircraft->multiMotorChannelBox4
           << m_aircraft->multiMotorChannelBox5 << m_aircraft->multiMotorChannelBox6
           << m_aircraft->multiMotorChannelBox7 << m_aircraft->multiMotorChannelBox8;

    GUIConfigDataUnion configData = getConfigData();
    resetActuators(&configData);

    foreach(QString motor, motorList) {
        int index = mmList.takeFirst()->currentIndex();

        if (motor == "VTOLMotorN") {
            configData.multi.VTOLMotorN = index;
        } else if (motor == "VTOLMotorNE") {
            configData.multi.VTOLMotorNE = index;
        } else if (motor == "VTOLMotorE") {
            configData.multi.VTOLMotorE = index;
        } else if (motor == "VTOLMotorSE") {
            configData.multi.VTOLMotorSE = index;
        } else if (motor == "VTOLMotorS") {
            configData.multi.VTOLMotorS = index;
        } else if (motor == "VTOLMotorSW") {
            configData.multi.VTOLMotorSW = index;
        } else if (motor == "VTOLMotorW") {
            configData.multi.VTOLMotorW = index;
        } else if (motor == "VTOLMotorNW") {
            configData.multi.VTOLMotorNW = index;
            // OctoX
        } else if (motor == "VTOLMotorNNE") {
            configData.multi.VTOLMotorNNE = index;
        } else if (motor == "VTOLMotorENE") {
            configData.multi.VTOLMotorENE = index;
        } else if (motor == "VTOLMotorESE") {
            configData.multi.VTOLMotorESE = index;
        } else if (motor == "VTOLMotorSSE") {
            configData.multi.VTOLMotorSSE = index;
        } else if (motor == "VTOLMotorSSW") {
            configData.multi.VTOLMotorSSW = index;
        } else if (motor == "VTOLMotorWSW") {
            configData.multi.VTOLMotorWSW = index;
        } else if (motor == "VTOLMotorWNW") {
            configData.multi.VTOLMotorWNW = index;
        } else if (motor == "VTOLMotorNNW") {
            configData.multi.VTOLMotorNNW = index;
        }
    }
    setConfigData(configData);
}

/**
   Set up a Quad-X or Quad-P mixer
 */
bool ConfigMultiRotorWidget::setupQuad(bool pLayout)
{
    // Check coherence:

    // Show any config errors in GUI
    if (throwConfigError(4)) {
        return false;
    }

    QList<QString> motorList;
    if (pLayout) {
        motorList << "VTOLMotorN" << "VTOLMotorE" << "VTOLMotorS" << "VTOLMotorW";
    } else {
        motorList << "VTOLMotorNW" << "VTOLMotorNE" << "VTOLMotorSE" << "VTOLMotorSW";
    }
    setupMotors(motorList);

    // Now, setup the mixer:
    // Motor 1 to 4, X Layout and Hlayout
    // pitch   roll    yaw
    // {0.5    ,0.5    ,-0.5     //Front left motor (CW)
    // {0.5    ,-0.5   ,0.5   //Front right motor(CCW)
    // {-0.5  ,-0.5    ,-0.5    //rear right motor (CW)
    // {-0.5   ,0.5    ,0.5   //Rear left motor  (CCW)
    double xMixer[8][3] = {
        { 1,  1,  -1 },
        { 1,  -1, 1  },
        { -1, -1, -1 },
        { -1, 1,  1  },
        { 0,  0,  0  },
        { 0,  0,  0  },
        { 0,  0,  0  },
        { 0,  0,  0  }
    };

    // Motor 1 to 4, P Layout:
    // pitch   roll    yaw
    // {1      ,0      ,-0.5    //Front motor (CW)
    // {0      ,-1     ,0.5   //Right  motor(CCW)
    // {-1     ,0      ,-0.5    //Rear motor  (CW)
    // {0      ,1      ,0.5   //Left motor  (CCW)
    double pMixer[8][3] = {
        { 1,  0,  -1 },
        { 0,  -1, 1  },
        { -1, 0,  -1 },
        { 0,  1,  1  },
        { 0,  0,  0  },
        { 0,  0,  0  },
        { 0,  0,  0  },
        { 0,  0,  0  }
    };

    if (pLayout) {
        setupMultiRotorMixer(pMixer);
    } else {
        setupMultiRotorMixer(xMixer);
    }
    m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    return true;
}

/**
   Set up a Hexa-X or Hexa-P mixer
 */
bool ConfigMultiRotorWidget::setupHexa(bool pLayout)
{
    // Check coherence:
    // Show any config errors in GUI
    if (throwConfigError(6)) {
        return false;
    }

    QList<QString> motorList;
    if (pLayout) {
        motorList << "VTOLMotorN" << "VTOLMotorNE" << "VTOLMotorSE" << "VTOLMotorS" << "VTOLMotorSW" << "VTOLMotorNW";
    } else {
        motorList << "VTOLMotorNE" << "VTOLMotorE" << "VTOLMotorSE" << "VTOLMotorSW" << "VTOLMotorW" << "VTOLMotorNW";
    }
    setupMotors(motorList);

    // and set only the relevant channels:

    // Motor 1 to 6, HexaP Layout
    // ---------------------------------------------
    // Motor:   1      2      3     4     5     6
    // ROLL  {0.00, -1.00, -1.00, 0.00, 1.00, 1.00}
    // PITCH {1.00, 0.50, -0.50, -1.00, -0.50, 0.50}
    // YAW   {-1.00, 1.00, -1.00, 1.00, -1.00, 1.00}
    // ---------------------------------------------
    // http://wiki.paparazziuav.org/wiki/RotorcraftMixing
    // pitch   roll    yaw
    double pMixer[8][3] = {
        { 1,    0,  -1 },
        { 0.5,  -1, 1  },
        { -0.5, -1, -1 },
        { -1,   0,  1  },
        { -0.5, 1,  -1 },
        { 0.5,  1,  1  },
        { 0,    0,  0  },
        { 0,    0,  0  }
    };

    // Motor 1 to 6, HexaX Layout
    // ---------------------------------------------
    // Motor:   1      2      3     4     5     6
    // ROLL  {-0.50, -1.00, -0.50, 0.50, 1.00, 0.50}
    // PITCH {1.00, 0.00, -1.00, -1.00, -0.00, 1.00}
    // YAW   {-1.00, 1.00, -1.00, 1.00, -1.00, 1.00}
    // ---------------------------------------------
    // http://wiki.paparazziuav.org/wiki/RotorcraftMixing
    // pitch   roll    yaw
    double xMixer[8][3] = {
        { 1,  -0.5, -1 },
        { 0,  -1,   1  },
        { -1, -0.5, -1 },
        { -1, 0.5,  1  },
        { 0,  1,    -1 },
        { 1,  0.5,  1  },
        { 0,  0,    0  },
        { 0,  0,    0  }
    };

    if (pLayout) {
        setupMultiRotorMixer(pMixer);
    } else {
        setupMultiRotorMixer(xMixer);
    }
    m_aircraft->mrStatusLabel->setText(tr("Configuration OK"));
    return true;
}


/**
   This function sets up the multirotor mixer values.
 */
bool ConfigMultiRotorWidget::setupMultiRotorMixer(double mixerFactors[8][3])
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));

    Q_ASSERT(mixer);
    resetMotorAndServoMixers(mixer);

    // and enable only the relevant channels:
    double pFactor = (double)m_aircraft->mrPitchMixLevel->value() / 100.0;
    double rFactor = (double)m_aircraft->mrRollMixLevel->value() / 100.0;
    invertMotors = m_aircraft->MultirotorRevMixerCheckBox->isChecked();
    double yFactor = (invertMotors ? -1.0 : 1.0) * (double)m_aircraft->mrYawMixLevel->value() / 100.0;

    setMixerValue(mixer, "MixerValueRoll", m_aircraft->mrRollMixLevel->value());
    setMixerValue(mixer, "MixerValuePitch", m_aircraft->mrPitchMixLevel->value());

    // Store negative value if ReverseAllMotors is checked
    setMixerValue(mixer, "MixerValueYaw", m_aircraft->mrYawMixLevel->value() * (invertMotors ? -1.0 : 1.0));

    QList<QComboBox *> mmList;
    mmList << m_aircraft->multiMotorChannelBox1 << m_aircraft->multiMotorChannelBox2
           << m_aircraft->multiMotorChannelBox3 << m_aircraft->multiMotorChannelBox4
           << m_aircraft->multiMotorChannelBox5 << m_aircraft->multiMotorChannelBox6
           << m_aircraft->multiMotorChannelBox7 << m_aircraft->multiMotorChannelBox8;
    for (int i = 0; i < 8; i++) {
        if (mmList.at(i)->isEnabled()) {
            int channel = mmList.at(i)->currentIndex() - 1;
            if (channel > -1) {
                setupQuadMotor(channel, mixerFactors[i][0] * pFactor, rFactor * mixerFactors[i][1],
                               yFactor * mixerFactors[i][2]);
            }
        }
    }
    return true;
}

/**
   This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigMultiRotorWidget::throwConfigError(int numMotors)
{
    // Initialize configuration error flag
    bool error = false;
    QString channelNames = "";

    // Iterate through all instances of multiMotorChannelBox
    for (int i = 0; i < numMotors; i++) {
        // Find widgets with his name "multiMotorChannelBox.x", where x is an integer
        QComboBox *combobox = this->findChild<QComboBox *>("multiMotorChannelBox" + QString::number(i + 1));
        if (combobox) {
            if (combobox->currentText() == "None") {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("red"));
                combobox->setItemData(0, pixmap, Qt::DecorationRole); // Set color palettes
                error = true;
            } else if (channelNames.contains(combobox->currentText(), Qt::CaseInsensitive)) {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("orange"));
                combobox->setItemData(combobox->currentIndex(), pixmap, Qt::DecorationRole); // Set color palettes
                combobox->setToolTip(tr("Duplicate channel in motor outputs"));
            } else {
                for (int index = 0; index < (int)ConfigMultiRotorWidget::CHANNEL_NUMELEM; index++) {
                    combobox->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox->setToolTip("");
                }
            }
            channelNames += (combobox->currentText() == "None") ? "" : combobox->currentText();
        }
    }

    // Iterate through all instances of rcOutputChannelBox
    for (int i = 0; i < 5; i++) {
        // Find widgets with his name "rcOutputChannelBox.x", where x is an integer
        QComboBox *combobox = this->findChild<QComboBox *>("rcOutputChannelBox" + QString::number(i + 1));
        if (combobox) {
            if (channelNames.contains(combobox->currentText(), Qt::CaseInsensitive)) {
                int size = combobox->style()->pixelMetric(QStyle::PM_SmallIconSize);
                QPixmap pixmap(size, size);
                pixmap.fill(QColor("orange"));
                combobox->setItemData(combobox->currentIndex(), pixmap, Qt::DecorationRole); // Set color palettes
                combobox->setToolTip(tr("Channel already used"));
            } else {
                for (int index = 0; index < (int)ConfigMultiRotorWidget::CHANNEL_NUMELEM; index++) {
                    combobox->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox->setToolTip(tr("Select output channel for Accessory%1 RcInput").arg(i));
                }
            }
            channelNames += (combobox->currentText() == "None") ? "" : combobox->currentText();
        }
    }

    if (error) {
        m_aircraft->mrStatusLabel->setText(
            tr("<font color='red'>ERROR: Assign all %1 motor channels</font>").arg(numMotors));
    }
    return error;
}

/**
   WHAT DOES THIS DO???
 */
void ConfigMultiRotorWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    // Thit fitInView method should only be called now, once the
    // widget is shown, otherwise it cannot compute its values and
    // the result is usually a ahrsbargraph that is way too small.
    m_aircraft->quadShape->fitInView(quad, Qt::KeepAspectRatio);
}

/**
   Resize the GUI contents when the user changes the window size
 */
void ConfigMultiRotorWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_aircraft->quadShape->fitInView(quad, Qt::KeepAspectRatio);
}

void ConfigMultiRotorWidget::enableControls(bool enable)
{
    if (enable) {
        setupEnabledControls(m_aircraft->multirotorFrameType->currentText());
    }
}
