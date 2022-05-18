/**
 ******************************************************************************
 *
 * @file       configccpmwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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
#include "configccpmwidget.h"

#include "ui_airframe_ccpm.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutilmanager.h>
#include <uavobjecthelper.h>

#include "mixersettings.h"
#include "systemsettings.h"
#include "actuatorcommand.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QBrush>
#include <QMessageBox>
#include <QSpinBox>
#include <QtSvg/QGraphicsSvgItem>
#include <QGraphicsEllipseItem>

#include <math.h>

QStringList ConfigCcpmWidget::getChannelDescriptions()
{
    // init a channel_numelem list of channel desc defaults
    QStringList channelDesc;

    for (int i = 0; i < (int)ConfigCcpmWidget::CHANNEL_NUMELEM; i++) {
        channelDesc.append("-");
    }

    // get the gui config data
    GUIConfigDataUnion configData = getConfigData();
    heliGUISettingsStruct heli    = configData.heli;

    if (heli.Throttle > 0) {
        channelDesc[heli.Throttle - 1] = "Throttle";
    }
    if (heli.Tail > 0) {
        channelDesc[heli.Tail - 1] = "Tail";
    }

    switch (heli.FirstServoIndex) {
    case 0:
        // front
        if (heli.ServoIndexW > 0) {
            channelDesc[heli.ServoIndexW - 1] = "Elevator";
        }
        if (heli.ServoIndexX > 0) {
            channelDesc[heli.ServoIndexX - 1] = "Roll1";
        }
        if (heli.ServoIndexY > 0) {
            channelDesc[heli.ServoIndexY - 1] = "Roll2";
        }
        break;
    case 1:
        // right
        if (heli.ServoIndexW > 0) {
            channelDesc[heli.ServoIndexW - 1] = "ServoW";
        }
        if (heli.ServoIndexX > 0) {
            channelDesc[heli.ServoIndexX - 1] = "ServoX";
        }
        if (heli.ServoIndexY > 0) {
            channelDesc[heli.ServoIndexY - 1] = "ServoY";
        }
        break;
    case 2:
        // rear
        if (heli.ServoIndexW > 0) {
            channelDesc[heli.ServoIndexW - 1] = "Elevator";
        }
        if (heli.ServoIndexX > 0) {
            channelDesc[heli.ServoIndexX - 1] = "Roll1";
        }
        if (heli.ServoIndexY > 0) {
            channelDesc[heli.ServoIndexY - 1] = "Roll2";
        }
        break;
    case 3:
        // left
        if (heli.ServoIndexW > 0) {
            channelDesc[heli.ServoIndexW - 1] = "ServoW";
        }
        if (heli.ServoIndexX > 0) {
            channelDesc[heli.ServoIndexX - 1] = "ServoX";
        }
        if (heli.ServoIndexY > 0) {
            channelDesc[heli.ServoIndexY - 1] = "ServoY";
        }
        break;
    }
    if (heli.ServoIndexZ > 0) {
        channelDesc[heli.ServoIndexZ - 1] = "ServoZ";
    }
    return channelDesc;
}

ConfigCcpmWidget::ConfigCcpmWidget(QWidget *parent) :
    VehicleConfig(parent)
{
    m_aircraft = new Ui_CcpmConfigWidget();
    m_aircraft->setupUi(this);

    SwashLvlConfigurationInProgress = 0;
    SwashLvlState = 0;
    SwashLvlServoInterlock = 0;
    updatingFromHardware   = false;
    updatingToHardware     = false;

    // Initialization of the swashplaye widget
    m_aircraft->SwashplateImage->setScene(new QGraphicsScene(this));

    m_aircraft->SwashLvlSwashplateImage->setScene(m_aircraft->SwashplateImage->scene());
    m_aircraft->SwashLvlSwashplateImage->setSceneRect(-50, -50, 500, 500);
    // m_aircraft->SwashLvlSwashplateImage->scale(.85,.85);

    // m_aircraft->SwashplateImage->setSceneRect(SwashplateImg->boundingRect());
    m_aircraft->SwashplateImage->setSceneRect(-50, -30, 500, 500);
    // m_aircraft->SwashplateImage->scale(.85,.85);

    QSvgRenderer *renderer = new QSvgRenderer();
    renderer->load(QString(":/configgadget/images/ccpm_setup.svg"));

    SwashplateImg = new QGraphicsSvgItem();
    SwashplateImg->setSharedRenderer(renderer);
    SwashplateImg->setElementId("Swashplate");
    SwashplateImg->setObjectName("Swashplate");
    // SwashplateImg->setScale(0.75);
    m_aircraft->SwashplateImage->scene()->addItem(SwashplateImg);

    QFont serifFont("Times", 24, QFont::Bold);
    // creates a default pen
    QPen pen;

    pen.setStyle(Qt::DotLine);
    pen.setWidth(2);
    pen.setBrush(Qt::gray);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    QBrush brush(Qt::darkYellow);
    // creates a default pen
    QPen pen2;

    // pen2.setStyle(Qt::DotLine);
    pen2.setWidth(1);
    pen2.setBrush(Qt::yellow);
    // pen2.setCapStyle(Qt::RoundCap);
    // pen2.setJoinStyle(Qt::RoundJoin);

    // brush.setStyle(Qt::RadialGradientPattern);

    QList<QString> ServoNames;
    ServoNames << "ServoW" << "ServoX" << "ServoY" << "ServoZ";

    for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        ServoLines[i] = m_aircraft->SwashLvlSwashplateImage->scene()->addLine(0, 0, 100 * i, i * i * 100, pen);

        Servos[i]     = new QGraphicsSvgItem();
        Servos[i]->setSharedRenderer(renderer);
        Servos[i]->setElementId(ServoNames.at(i));
        Servos[i]->setZValue(20);
        m_aircraft->SwashplateImage->scene()->addItem(Servos[i]);

        ServosText[i] = new QGraphicsTextItem();
        ServosText[i]->setDefaultTextColor(Qt::yellow);
        ServosText[i]->setPlainText("-");
        ServosText[i]->setFont(serifFont);
        ServosText[i]->setZValue(31);

        ServosTextCircles[i] = new QGraphicsEllipseItem(1, 1, 30, 30);
        ServosTextCircles[i]->setBrush(brush);
        ServosTextCircles[i]->setPen(pen2);
        ServosTextCircles[i]->setZValue(30);
        m_aircraft->SwashplateImage->scene()->addItem(ServosTextCircles[i]);
        m_aircraft->SwashplateImage->scene()->addItem(ServosText[i]);

        SwashLvlSpinBoxes[i] = new QSpinBox(m_aircraft->SwashLvlSwashplateImage); // use QGraphicsView
        m_aircraft->SwashLvlSwashplateImage->scene()->addWidget(SwashLvlSpinBoxes[i]);
        SwashLvlSpinBoxes[i]->setMaximum(10000);
        SwashLvlSpinBoxes[i]->setMinimum(0);
        SwashLvlSpinBoxes[i]->setValue(0);
    }

    // initialize our two mixer curves
    // mixercurve defaults to mixercurve_throttle
    m_aircraft->ThrottleCurve->initLinearCurve(5, 1.0, 0.0);

    // tell mixercurve this is a pitch curve
    m_aircraft->PitchCurve->setMixerType(MixerCurve::MIXERCURVE_PITCH);
    m_aircraft->PitchCurve->initLinearCurve(5, 1.0, -1.0);

    // initialize channel names
    m_aircraft->ccpmEngineChannel->addItems(channelNames);
    m_aircraft->ccpmEngineChannel->setCurrentIndex(0);
    m_aircraft->ccpmTailChannel->addItems(channelNames);
    m_aircraft->ccpmTailChannel->setCurrentIndex(0);
    m_aircraft->ccpmServoWChannel->addItems(channelNames);
    m_aircraft->ccpmServoWChannel->setCurrentIndex(0);
    m_aircraft->ccpmServoXChannel->addItems(channelNames);
    m_aircraft->ccpmServoXChannel->setCurrentIndex(0);
    m_aircraft->ccpmServoYChannel->addItems(channelNames);
    m_aircraft->ccpmServoYChannel->setCurrentIndex(0);
    m_aircraft->ccpmServoZChannel->addItems(channelNames);
    m_aircraft->ccpmServoZChannel->setCurrentIndex(0);

    QStringList Types;
    Types << "CCPM 2 Servo 90º" << "CCPM 3 Servo 90º"
          << "CCPM 4 Servo 90º" << "CCPM 3 Servo 120º"
          << "CCPM 3 Servo 140º" << "FP 2 Servo 90º"
          << "Coax 2 Servo 90º" << "Custom - User Angles"
          << "Custom - Advanced Settings";
    m_aircraft->ccpmType->addItems(Types);
    m_aircraft->ccpmType->setCurrentIndex(m_aircraft->ccpmType->count() - 1);

    connect(m_aircraft->ccpmAngleW, SIGNAL(valueChanged(double)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmAngleX, SIGNAL(valueChanged(double)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmAngleY, SIGNAL(valueChanged(double)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmAngleZ, SIGNAL(valueChanged(double)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmCorrectionAngle, SIGNAL(valueChanged(double)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmServoWChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmServoXChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmServoYChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmServoZChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(ccpmSwashplateUpdate()));
    connect(m_aircraft->ccpmEngineChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmTailChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmRevoSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmREVOspinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmCollectiveSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmCollectivespinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateMixer()));
    connect(m_aircraft->ccpmType, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateType()));
    connect(m_aircraft->ccpmSingleServo, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateType()));
    connect(m_aircraft->TabObject, SIGNAL(currentChanged(int)), this, SLOT(UpdateType()));

    connect(m_aircraft->SwashLvlStartButton, SIGNAL(clicked()), this, SLOT(SwashLvlStartButtonPressed()));
    connect(m_aircraft->SwashLvlNextButton, SIGNAL(clicked()), this, SLOT(SwashLvlNextButtonPressed()));
    connect(m_aircraft->SwashLvlPrevButton, SIGNAL(clicked()), this, SLOT(SwashLvlPrevButtonPressed()));
    connect(m_aircraft->SwashLvlCancelButton, SIGNAL(clicked()), this, SLOT(SwashLvlCancelButtonPressed()));
    connect(m_aircraft->SwashLvlFinishButton, SIGNAL(clicked()), this, SLOT(SwashLvlFinishButtonPressed()));

    connect(m_aircraft->ccpmCollectivePassthrough, SIGNAL(clicked()), this, SLOT(SetUIComponentVisibilities()));
    connect(m_aircraft->ccpmLinkCyclic, SIGNAL(clicked()), this, SLOT(SetUIComponentVisibilities()));
    connect(m_aircraft->ccpmLinkRoll, SIGNAL(clicked()), this, SLOT(SetUIComponentVisibilities()));

    UpdateType();
}

ConfigCcpmWidget::~ConfigCcpmWidget()
{
    delete m_aircraft;
}

QString ConfigCcpmWidget::getFrameType()
{
    return "HeliCP";
}

void ConfigCcpmWidget::setupUI(QString frameType)
{
    Q_UNUSED(frameType);
}

void ConfigCcpmWidget::registerWidgets(ConfigTaskWidget &parent)
{
    parent.addWidget(m_aircraft->ccpmType);
    parent.addWidget(m_aircraft->ccpmTailChannel);
    parent.addWidget(m_aircraft->ccpmEngineChannel);
    parent.addWidget(m_aircraft->ccpmServoWChannel);
    parent.addWidget(m_aircraft->ccpmServoXChannel);
    parent.addWidget(m_aircraft->ccpmServoYChannel);
    parent.addWidget(m_aircraft->ccpmSingleServo);
    parent.addWidget(m_aircraft->ccpmServoZChannel);
    parent.addWidget(m_aircraft->ccpmAngleW);
    parent.addWidget(m_aircraft->ccpmAngleX);
    parent.addWidget(m_aircraft->ccpmCorrectionAngle);
    parent.addWidget(m_aircraft->ccpmAngleZ);
    parent.addWidget(m_aircraft->ccpmAngleY);
    parent.addWidget(m_aircraft->ccpmCollectivePassthrough);
    parent.addWidget(m_aircraft->ccpmLinkRoll);
    parent.addWidget(m_aircraft->ccpmLinkCyclic);
    parent.addWidget(m_aircraft->ccpmRevoSlider);
    parent.addWidget(m_aircraft->ccpmREVOspinBox);
    parent.addWidget(m_aircraft->ccpmCollectiveSlider);
    parent.addWidget(m_aircraft->ccpmCollectivespinBox);
    parent.addWidget(m_aircraft->ccpmCollectiveScale);
    parent.addWidget(m_aircraft->ccpmCollectiveScaleBox);
    parent.addWidget(m_aircraft->ccpmCyclicScale);
    parent.addWidget(m_aircraft->ccpmPitchScale);
    parent.addWidget(m_aircraft->ccpmPitchScaleBox);
    parent.addWidget(m_aircraft->ccpmRollScale);
    parent.addWidget(m_aircraft->ccpmRollScaleBox);
    parent.addWidget(m_aircraft->SwashLvlPositionSlider);
    parent.addWidget(m_aircraft->SwashLvlPositionSpinBox);
    parent.addWidget(m_aircraft->PitchCurve->getCurveWidget());
    parent.addWidget(m_aircraft->PitchCurve);
    parent.addWidget(m_aircraft->ThrottleCurve->getCurveWidget());
    parent.addWidget(m_aircraft->ThrottleCurve);
    parent.addWidget(m_aircraft->ccpmAdvancedSettingsTable);
}

void ConfigCcpmWidget::resetActuators(GUIConfigDataUnion *configData)
{
    configData->heli.Throttle    = 0;
    configData->heli.Tail        = 0;
    configData->heli.ServoIndexW = 0;
    configData->heli.ServoIndexX = 0;
    configData->heli.ServoIndexY = 0;
    configData->heli.ServoIndexZ = 0;
}

void ConfigCcpmWidget::enableControls(bool enable)
{
    if (enable) {
        SetUIComponentVisibilities();
    }
}

void ConfigCcpmWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    GUIConfigDataUnion config = getConfigData();

    // swashplate config
    setComboCurrentIndex(m_aircraft->ccpmType, m_aircraft->ccpmType->count() - (config.heli.SwashplateType + 1));
    setComboCurrentIndex(m_aircraft->ccpmSingleServo, config.heli.FirstServoIndex);

    // ccpm mixing options
    m_aircraft->ccpmCollectivePassthrough->setChecked(config.heli.ccpmCollectivePassthroughState);
    m_aircraft->ccpmLinkCyclic->setChecked(config.heli.ccpmLinkCyclicState);
    m_aircraft->ccpmLinkRoll->setChecked(config.heli.ccpmLinkRollState);

    // correction angle
    m_aircraft->ccpmCorrectionAngle->setValue(config.heli.CorrectionAngle);

    // update sliders
    m_aircraft->ccpmCollectiveScale->setValue(config.heli.SliderValue0);
    m_aircraft->ccpmCollectiveScaleBox->setValue(config.heli.SliderValue0);
    m_aircraft->ccpmCyclicScale->setValue(config.heli.SliderValue1);
    m_aircraft->ccpmCyclicScaleBox->setValue(config.heli.SliderValue1);
    m_aircraft->ccpmPitchScale->setValue(config.heli.SliderValue1);
    m_aircraft->ccpmPitchScaleBox->setValue(config.heli.SliderValue1);
    m_aircraft->ccpmRollScale->setValue(config.heli.SliderValue2);
    m_aircraft->ccpmRollScaleBox->setValue(config.heli.SliderValue2);
    m_aircraft->ccpmCollectiveSlider->setValue(config.heli.SliderValue0);
    m_aircraft->ccpmCollectivespinBox->setValue(config.heli.SliderValue0);

    // servo assignments
    setComboCurrentIndex(m_aircraft->ccpmServoWChannel, config.heli.ServoIndexW);
    setComboCurrentIndex(m_aircraft->ccpmServoXChannel, config.heli.ServoIndexX);
    setComboCurrentIndex(m_aircraft->ccpmServoYChannel, config.heli.ServoIndexY);
    setComboCurrentIndex(m_aircraft->ccpmServoZChannel, config.heli.ServoIndexZ);

    // throttle
    setComboCurrentIndex(m_aircraft->ccpmEngineChannel, config.heli.Throttle);
    // tail
    setComboCurrentIndex(m_aircraft->ccpmTailChannel, config.heli.Tail);

    getMixer();
}

void ConfigCcpmWidget::updateObjectsFromWidgetsImpl()
{
    updateConfigObjects();
    setMixer();
}

void ConfigCcpmWidget::UpdateType()
{
    int SingleServoIndex, NumServosDefined;
    double AdjustmentAngle = 0;

    typeText = m_aircraft->ccpmType->currentText();
    SingleServoIndex = m_aircraft->ccpmSingleServo->currentIndex();

    AdjustmentAngle  = SingleServoIndex * 90;

    m_aircraft->PitchCurve->setVisible(1);

    NumServosDefined = 4;
    // set values for pre defined heli types
    if (typeText.compare("CCPM 2 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 90, 360));
        m_aircraft->ccpmAngleY->setValue(0);
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleY->setEnabled(false);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoYChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoYChannel->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setEnabled(false);
        NumServosDefined = 2;
    } else if (typeText.compare("CCPM 3 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 90, 360));
        m_aircraft->ccpmAngleY->setValue(fmod(AdjustmentAngle + 180, 360));
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setEnabled(false);
        NumServosDefined = 3;
    } else if (typeText.compare("CCPM 4 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 90, 360));
        m_aircraft->ccpmAngleY->setValue(fmod(AdjustmentAngle + 180, 360));
        m_aircraft->ccpmAngleZ->setValue(fmod(AdjustmentAngle + 270, 360));
        m_aircraft->ccpmSingleServo->setEnabled(false);
        m_aircraft->ccpmSingleServo->setCurrentIndex(0);
        NumServosDefined = 4;
    } else if (typeText.compare("CCPM 3 Servo 120º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 120, 360));
        m_aircraft->ccpmAngleY->setValue(fmod(AdjustmentAngle + 240, 360));
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setEnabled(false);
        NumServosDefined = 3;
    } else if (typeText.compare("CCPM 3 Servo 140º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 140, 360));
        m_aircraft->ccpmAngleY->setValue(fmod(AdjustmentAngle + 220, 360));
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setEnabled(false);
        NumServosDefined = 3;
    } else if (typeText.compare("FP 2 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 90, 360));
        m_aircraft->ccpmAngleY->setValue(0);
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleY->setEnabled(false);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoYChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoYChannel->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setEnabled(false);

        m_aircraft->ccpmCollectivespinBox->setEnabled(false);
        m_aircraft->ccpmCollectiveSlider->setEnabled(false);
        m_aircraft->ccpmCollectivespinBox->setValue(0);
        m_aircraft->ccpmCollectiveSlider->setValue(0);
        m_aircraft->PitchCurve->setVisible(0);
        NumServosDefined = 2;
    } else if (typeText.compare("Coax 2 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmAngleW->setValue(AdjustmentAngle + 0);
        m_aircraft->ccpmAngleX->setValue(fmod(AdjustmentAngle + 90, 360));
        m_aircraft->ccpmAngleY->setValue(0);
        m_aircraft->ccpmAngleZ->setValue(0);
        m_aircraft->ccpmAngleY->setEnabled(false);
        m_aircraft->ccpmAngleZ->setEnabled(false);
        m_aircraft->ccpmServoYChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoZChannel->setCurrentIndex(0);
        m_aircraft->ccpmServoYChannel->setEnabled(false);
        m_aircraft->ccpmServoZChannel->setEnabled(false);

        m_aircraft->ccpmCollectivespinBox->setEnabled(false);
        m_aircraft->ccpmCollectiveSlider->setEnabled(false);
        m_aircraft->ccpmCollectivespinBox->setValue(0);
        m_aircraft->ccpmCollectiveSlider->setValue(0);
        m_aircraft->PitchCurve->setVisible(0);
        NumServosDefined = 2;
    }

    // Set the text of the motor boxes
    if (typeText.compare("Coax 2 Servo 90º", Qt::CaseInsensitive) == 0) {
        m_aircraft->ccpmEngineLabel->setText("CW motor");
        m_aircraft->ccpmTailLabel->setText("CCW motor");
    } else {
        m_aircraft->ccpmEngineLabel->setText("Engine");
        m_aircraft->ccpmTailLabel->setText("Tail rotor");
    }

    // set the visibility of the swashplate servo selection boxes
    m_aircraft->ccpmServoWLabel->setVisible(NumServosDefined >= 1);
    m_aircraft->ccpmServoXLabel->setVisible(NumServosDefined >= 2);
    m_aircraft->ccpmServoYLabel->setVisible(NumServosDefined >= 3);
    m_aircraft->ccpmServoZLabel->setVisible(NumServosDefined >= 4);
    m_aircraft->ccpmServoWChannel->setVisible(NumServosDefined >= 1);
    m_aircraft->ccpmServoXChannel->setVisible(NumServosDefined >= 2);
    m_aircraft->ccpmServoYChannel->setVisible(NumServosDefined >= 3);
    m_aircraft->ccpmServoZChannel->setVisible(NumServosDefined >= 4);

    // set the visibility of the swashplate angle selection boxes
    m_aircraft->ccpmServoWLabel_2->setVisible(NumServosDefined >= 1);
    m_aircraft->ccpmServoXLabel_2->setVisible(NumServosDefined >= 2);
    m_aircraft->ccpmServoYLabel_2->setVisible(NumServosDefined >= 3);
    m_aircraft->ccpmServoZLabel_2->setVisible(NumServosDefined >= 4);
    m_aircraft->ccpmAngleW->setVisible(NumServosDefined >= 1);
    m_aircraft->ccpmAngleX->setVisible(NumServosDefined >= 2);
    m_aircraft->ccpmAngleY->setVisible(NumServosDefined >= 3);
    m_aircraft->ccpmAngleZ->setVisible(NumServosDefined >= 4);

    QTableWidget *table = m_aircraft->ccpmAdvancedSettingsTable;
    table->resizeColumnsToContents();
    for (int i = 0; i < 6; i++) {
        table->setColumnWidth(i, (table->width() - table->verticalHeader()->width()) / 6);
    }

    // update UI
    ccpmSwashplateUpdate();
}

void ConfigCcpmWidget::ccpmSwashplateRedraw()
{
    double angle[CCPM_MAX_SWASH_SERVOS], CorrectionAngle, x, y, w, h, radius, CenterX, CenterY;
    int used[CCPM_MAX_SWASH_SERVOS], defined[CCPM_MAX_SWASH_SERVOS], i;
    QRectF bounds;
    QRect size;
    double scale, xscale, yscale;

    size = m_aircraft->SwashplateImage->rect();
    // If size = default, get size from other Img/tab
    if (size.width() == 100) {
        size = m_aircraft->SwashLvlSwashplateImage->rect();
    }

    xscale = size.width();
    yscale = size.height();
    scale  = xscale;
    if (yscale < scale) {
        scale = yscale;
    }
    scale /= 540.00;
    m_aircraft->SwashplateImage->resetTransform();
    m_aircraft->SwashplateImage->scale(scale, scale);

    size   = m_aircraft->SwashLvlSwashplateImage->rect();
    xscale = size.width();
    yscale = size.height();
    scale  = xscale;
    if (yscale < scale) {
        scale = yscale;
    }
    scale /= 590.00;
    m_aircraft->SwashLvlSwashplateImage->resetTransform();
    m_aircraft->SwashLvlSwashplateImage->scale(scale, scale);

    CorrectionAngle = m_aircraft->ccpmCorrectionAngle->value();

    CenterX = 200;
    CenterY = 200;

    bounds  = SwashplateImg->boundingRect();

    SwashplateImg->setPos(CenterX - bounds.width() / 2, CenterY - bounds.height() / 2);

    defined[0] = (m_aircraft->ccpmServoWChannel->isEnabled());
    defined[1] = (m_aircraft->ccpmServoXChannel->isEnabled());
    defined[2] = (m_aircraft->ccpmServoYChannel->isEnabled());
    defined[3] = (m_aircraft->ccpmServoZChannel->isEnabled());
    used[0]    = ((m_aircraft->ccpmServoWChannel->currentIndex() > 0) && (m_aircraft->ccpmServoWChannel->isEnabled()));
    used[1]    = ((m_aircraft->ccpmServoXChannel->currentIndex() > 0) && (m_aircraft->ccpmServoXChannel->isEnabled()));
    used[2]    = ((m_aircraft->ccpmServoYChannel->currentIndex() > 0) && (m_aircraft->ccpmServoYChannel->isEnabled()));
    used[3]    = ((m_aircraft->ccpmServoZChannel->currentIndex() > 0) && (m_aircraft->ccpmServoZChannel->isEnabled()));
    angle[0]   = (CorrectionAngle + 180 + m_aircraft->ccpmAngleW->value()) * M_PI / 180.00;
    angle[1]   = (CorrectionAngle + 180 + m_aircraft->ccpmAngleX->value()) * M_PI / 180.00;
    angle[2]   = (CorrectionAngle + 180 + m_aircraft->ccpmAngleY->value()) * M_PI / 180.00;
    angle[3]   = (CorrectionAngle + 180 + m_aircraft->ccpmAngleZ->value()) * M_PI / 180.00;


    for (i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        radius = 210;
        x = CenterX - (radius * sin(angle[i])) - 10.00;
        y = CenterY + (radius * cos(angle[i])) - 10.00;
        Servos[i]->setPos(x, y);
        Servos[i]->setVisible(used[i] != 0);

        radius = 150;
        bounds = ServosText[i]->boundingRect();
        x = CenterX - (radius * sin(angle[i])) - bounds.width() / 2;
        y = CenterY + (radius * cos(angle[i])) - bounds.height() / 2;

        ServosText[i]->setPos(x, y);
        ServosText[i]->setVisible(used[i] != 0);

        if (bounds.width() > bounds.height()) {
            bounds.setHeight(bounds.width());
        } else {
            bounds.setWidth(bounds.height());
        }
        x = CenterX - (radius * sin(angle[i])) - bounds.width() / 2;
        y = CenterY + (radius * cos(angle[i])) - bounds.height() / 2;

        ServosTextCircles[i]->setRect(bounds);
        ServosTextCircles[i]->setPos(x, y);
        ServosTextCircles[i]->setVisible(used[i] != 0);

        w = SwashLvlSpinBoxes[i]->width() / 2;
        h = SwashLvlSpinBoxes[i]->height() / 2;
        radius = (215.00 + w + h);
        x = CenterX - (radius * sin(angle[i])) - w;
        y = CenterY + (radius * cos(angle[i])) - h;
        SwashLvlSpinBoxes[i]->move(m_aircraft->SwashLvlSwashplateImage->mapFromScene(x, y));
        SwashLvlSpinBoxes[i]->setVisible(used[i] != 0);

        radius = 220;
        x = CenterX - (radius * sin(angle[i]));
        y = CenterY + (radius * cos(angle[i]));
        ServoLines[i]->setLine(CenterX, CenterY, x, y);
        ServoLines[i]->setVisible(defined[i] != 0);
    }

    // m_aircraft->SwashplateImage->centerOn(CenterX, CenterY);

    // m_aircraft->SwashplateImage->fitInView(SwashplateImg, Qt::KeepAspectRatio);
}

void ConfigCcpmWidget::ccpmSwashplateUpdate()
{
    UpdateMixer();
    SetUIComponentVisibilities();
    ccpmSwashplateRedraw();
}

void ConfigCcpmWidget::UpdateMixer()
{
    bool useCCPM;
    bool useCyclic;
    int ThisEnable[6];
    float CollectiveConstant, PitchConstant, RollConstant;
    float ThisAngle[6];
    QString Channel;

    int typeInt = m_aircraft->ccpmType->count() - m_aircraft->ccpmType->currentIndex() - 1;

    // Exit if currently updatingToHardware or ConfigError
    // Avoid mixer changes if something wrong in config
    if (throwConfigError(typeInt) || updatingToHardware) {
        return;
    }

    GUIConfigDataUnion config = getConfigData();

    useCCPM   = !(config.heli.ccpmCollectivePassthroughState || !config.heli.ccpmLinkCyclicState);
    useCyclic = config.heli.ccpmLinkRollState;

    CollectiveConstant = (float)config.heli.SliderValue0 / 100.00;

    if (useCCPM) { // cyclic = 1 - collective
        PitchConstant = 1 - CollectiveConstant;
        RollConstant  = PitchConstant;
    } else {
        PitchConstant = (float)config.heli.SliderValue1 / 100.00;
        ;
        if (useCyclic) {
            RollConstant = PitchConstant;
        } else {
            RollConstant = (float)config.heli.SliderValue2 / 100.00;
            ;
        }
    }

    // get the channel data from the ui
    MixerChannelData[0] = m_aircraft->ccpmEngineChannel->currentIndex();
    MixerChannelData[1] = m_aircraft->ccpmTailChannel->currentIndex();
    MixerChannelData[2] = m_aircraft->ccpmServoWChannel->currentIndex();
    MixerChannelData[3] = m_aircraft->ccpmServoXChannel->currentIndex();
    MixerChannelData[4] = m_aircraft->ccpmServoYChannel->currentIndex();
    MixerChannelData[5] = m_aircraft->ccpmServoZChannel->currentIndex();

    QTableWidget *table = m_aircraft->ccpmAdvancedSettingsTable;

    if (typeInt != 0) { // not advanced settings
        // get the angle data from the ui
        ThisAngle[2]  = m_aircraft->ccpmAngleW->value();
        ThisAngle[3]  = m_aircraft->ccpmAngleX->value();
        ThisAngle[4]  = m_aircraft->ccpmAngleY->value();
        ThisAngle[5]  = m_aircraft->ccpmAngleZ->value();

        // get the angle data from the ui
        ThisEnable[2] = m_aircraft->ccpmServoWChannel->isEnabled();
        ThisEnable[3] = m_aircraft->ccpmServoXChannel->isEnabled();
        ThisEnable[4] = m_aircraft->ccpmServoYChannel->isEnabled();
        ThisEnable[5] = m_aircraft->ccpmServoZChannel->isEnabled();

        ServosText[0]->setPlainText(QString("%1").arg(MixerChannelData[2]));
        ServosText[1]->setPlainText(QString("%1").arg(MixerChannelData[3]));
        ServosText[2]->setPlainText(QString("%1").arg(MixerChannelData[4]));
        ServosText[3]->setPlainText(QString("%1").arg(MixerChannelData[5]));

        // go through the user data and update the mixer matrix
        for (int i = 0; i < 6; i++) {
            if (((MixerChannelData[i] > 0) && ThisEnable[i]) || (i < 2)) {
                table->item(i, 0)->setText(QString("%1").arg(MixerChannelData[i]));
                // Generate the mixer vector
                if (i == 0) { // main motor-engine
                    table->item(i, 1)->setText(QString("%1").arg(127)); // ThrottleCurve1
                    table->item(i, 2)->setText(QString("%1").arg(0)); // ThrottleCurve2
                    table->item(i, 3)->setText(QString("%1").arg(0)); // Roll
                    table->item(i, 4)->setText(QString("%1").arg(0)); // Pitch

                    if (typeText.compare("Coax 2 Servo 90º", Qt::CaseInsensitive) == 0) {
                        // Yaw
                        table->item(i, 5)->setText(QString("%1").arg(-127));
                    } else {
                        // Yaw
                        table->item(i, 5)->setText(QString("%1").arg(0));
                    }
                }
                if (i == 1) {
                    // tailrotor --or-- counter-clockwise motor
                    if (typeText.compare("Coax 2 Servo 90º", Qt::CaseInsensitive) == 0) {
                        // ThrottleCurve1
                        table->item(i, 1)->setText(QString("%1").arg(127));
                        // Yaw
                        table->item(i, 5)->setText(QString("%1").arg(127));
                    } else {
                        // ThrottleCurve1
                        table->item(i, 1)->setText(QString("%1").arg(0));
                        // Yaw
                        table->item(i, 5)->setText(QString("%1").arg(127));
                    }
                    // ThrottleCurve2
                    table->item(i, 2)->setText(QString("%1").arg(0));
                    // Roll
                    table->item(i, 3)->setText(QString("%1").arg(0));
                    // Pitch
                    table->item(i, 4)->setText(QString("%1").arg(0));
                }
                if (i > 1) {
                    // Swashplate
                    // ThrottleCurve1
                    table->item(i, 1)->setText(QString("%1").arg(0));
                    // ThrottleCurve2
                    table->item(i, 2)->setText(QString("%1").arg((int)(127.0 * CollectiveConstant)));
                    table->item(i, 3)->setText(
                        QString("%1").arg(
                            (int)(127.0 * (RollConstant)
                                  * sin((180 + config.heli.CorrectionAngle + ThisAngle[i]) * M_PI / 180.00)))); // Roll
                    table->item(i, 4)->setText(
                        QString("%1").arg(
                            (int)(127.0 * (PitchConstant)
                                  * cos((config.heli.CorrectionAngle + ThisAngle[i]) * M_PI / 180.00)))); // Pitch
                    // Yaw
                    table->item(i, 5)->setText(QString("%1").arg(0));
                }
            } else {
                for (int j = 0; j < 6; j++) {
                    table->item(i, j)->setText("-");
                }
            }
        }
    } else {
        // advanced settings
        UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
        Q_ASSERT(mixer);

        // Populate custom mixer table from board values
        for (int i = 0; i < 6; i++) {
            if (MixerChannelData[i] > 0) {
                // Channel number
                table->item(i, 0)->setText(QString("%1").arg(MixerChannelData[i]));
                // Throttle1
                table->item(i, 1)->setText(QString("%1").arg(getMixerVectorValue(mixer, MixerChannelData[i] - 1,
                                                                                 VehicleConfig::MIXERVECTOR_THROTTLECURVE1)));
                // Throttle2
                table->item(i, 2)->setText(QString("%1").arg(getMixerVectorValue(mixer, MixerChannelData[i] - 1,
                                                                                 VehicleConfig::MIXERVECTOR_THROTTLECURVE2)));
                // Roll
                table->item(i, 3)->setText(QString("%1").arg(getMixerVectorValue(mixer, MixerChannelData[i] - 1,
                                                                                 VehicleConfig::MIXERVECTOR_ROLL)));
                // Pitch
                table->item(i, 4)->setText(QString("%1").arg(getMixerVectorValue(mixer, MixerChannelData[i] - 1,
                                                                                 VehicleConfig::MIXERVECTOR_PITCH)));
                // Yaw
                table->item(i, 5)->setText(QString("%1").arg(getMixerVectorValue(mixer, MixerChannelData[i] - 1,
                                                                                 VehicleConfig::MIXERVECTOR_YAW)));
            } else {
                for (int j = 0; j < 6; j++) {
                    table->item(i, j)->setText("-");
                }
            }
        }
    }
}

void ConfigCcpmWidget::updateConfigObjects()
{
    if (updatingFromHardware == true) {
        return;
    }
    updatingFromHardware = true;

    // get the user options
    GUIConfigDataUnion config = getConfigData();

    // swashplate config
    config.heli.SwashplateType  = m_aircraft->ccpmType->count() - m_aircraft->ccpmType->currentIndex() - 1;
    config.heli.FirstServoIndex = m_aircraft->ccpmSingleServo->currentIndex();

    // ccpm mixing options
    config.heli.ccpmCollectivePassthroughState = m_aircraft->ccpmCollectivePassthrough->isChecked();
    config.heli.ccpmLinkCyclicState = m_aircraft->ccpmLinkCyclic->isChecked();
    config.heli.ccpmLinkRollState   = m_aircraft->ccpmLinkRoll->isChecked();
    bool useCCPM   = !(config.heli.ccpmCollectivePassthroughState || !config.heli.ccpmLinkCyclicState);
    bool useCyclic = config.heli.ccpmLinkRollState;

    // correction angle
    config.heli.CorrectionAngle = m_aircraft->ccpmCorrectionAngle->value();

    // update sliders
    if (useCCPM) {
        config.heli.SliderValue0 = m_aircraft->ccpmCollectiveSlider->value();
    } else {
        config.heli.SliderValue0 = m_aircraft->ccpmCollectiveScale->value();
    }
    if (useCyclic) {
        config.heli.SliderValue1 = m_aircraft->ccpmCyclicScale->value();
    } else {
        config.heli.SliderValue1 = m_aircraft->ccpmPitchScale->value();
    }
    config.heli.SliderValue2 = m_aircraft->ccpmRollScale->value();

    // servo assignments
    config.heli.ServoIndexW  = m_aircraft->ccpmServoWChannel->currentIndex();
    config.heli.ServoIndexX  = m_aircraft->ccpmServoXChannel->currentIndex();
    config.heli.ServoIndexY  = m_aircraft->ccpmServoYChannel->currentIndex();
    config.heli.ServoIndexZ  = m_aircraft->ccpmServoZChannel->currentIndex();

    // throttle
    config.heli.Throttle     = m_aircraft->ccpmEngineChannel->currentIndex();
    // tail
    config.heli.Tail = m_aircraft->ccpmTailChannel->currentIndex();

    setConfigData(config);

    updatingFromHardware = false;
}

void ConfigCcpmWidget::SetUIComponentVisibilities()
{
    m_aircraft->ccpmRevoMixingBox->setVisible(0);

    m_aircraft->ccpmPitchMixingBox->setVisible(
        !m_aircraft->ccpmCollectivePassthrough->isChecked() && m_aircraft->ccpmLinkCyclic->isChecked());

    m_aircraft->ccpmCollectiveScalingBox->setVisible(
        m_aircraft->ccpmCollectivePassthrough->isChecked() || !m_aircraft->ccpmLinkCyclic->isChecked());

    m_aircraft->ccpmLinkCyclic->setVisible(!m_aircraft->ccpmCollectivePassthrough->isChecked());

    m_aircraft->ccpmCyclicScalingBox->setVisible(
        (m_aircraft->ccpmCollectivePassthrough->isChecked() || !m_aircraft->ccpmLinkCyclic->isChecked())
        && m_aircraft->ccpmLinkRoll->isChecked());

    if (!m_aircraft->ccpmCollectivePassthrough->checkState() && m_aircraft->ccpmLinkCyclic->isChecked()) {
        m_aircraft->ccpmPitchScalingBox->setVisible(0);
        m_aircraft->ccpmRollScalingBox->setVisible(0);
        m_aircraft->ccpmLinkRoll->setVisible(0);
    } else {
        m_aircraft->ccpmPitchScalingBox->setVisible(!m_aircraft->ccpmLinkRoll->isChecked());
        m_aircraft->ccpmRollScalingBox->setVisible(!m_aircraft->ccpmLinkRoll->isChecked());
        m_aircraft->ccpmLinkRoll->setVisible(1);
    }
    // clear status check boxes
    m_aircraft->SwashLvlStepList->item(0)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(1)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(2)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(3)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(0)->setBackground(Qt::transparent);
    m_aircraft->SwashLvlStepList->item(1)->setBackground(Qt::transparent);
    m_aircraft->SwashLvlStepList->item(2)->setBackground(Qt::transparent);
    m_aircraft->SwashLvlStepList->item(3)->setBackground(Qt::transparent);

    // Enable / disable by typeInt : 0 is custom
    int typeInt = m_aircraft->ccpmType->count() - m_aircraft->ccpmType->currentIndex() - 1;

    // set visibility of user settings (When Custom)
    m_aircraft->ccpmAdvancedSettingsTable->setEnabled(typeInt == 0);

    m_aircraft->ccpmAngleW->setEnabled(typeInt == 1);
    m_aircraft->ccpmAngleX->setEnabled(typeInt == 1);
    m_aircraft->ccpmAngleY->setEnabled(typeInt == 1);
    m_aircraft->ccpmAngleZ->setEnabled(typeInt == 1);
    m_aircraft->ccpmCorrectionAngle->setEnabled(typeInt != 0);

    m_aircraft->ccpmServoWChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmServoXChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmServoYChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmServoZChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmSingleServo->setEnabled(typeInt > 1);

    m_aircraft->ccpmEngineChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmTailChannel->setEnabled(typeInt > 0);
    m_aircraft->ccpmCollectiveSlider->setEnabled(typeInt > 0);
    m_aircraft->ccpmCollectivespinBox->setEnabled(typeInt > 0);
    m_aircraft->ccpmRevoSlider->setEnabled(typeInt > 0);
    m_aircraft->ccpmREVOspinBox->setEnabled(typeInt > 0);
}

/**
   Request the current value of the SystemSettings which holds the ccpm type
 */
void ConfigCcpmWidget::getMixer()
{
    if (SwashLvlConfigurationInProgress) {
        return;
    }
    if (updatingToHardware) {
        return;
    }

    updatingFromHardware = true;

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("MixerSettings"));
    Q_ASSERT(mixer);

    QList<double> curveValues;
    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE1, &curveValues);

    // is at least one of the curve values != 0?
    if (isValidThrottleCurve(&curveValues)) {
        m_aircraft->ThrottleCurve->setCurve(&curveValues);
    } else {
        m_aircraft->ThrottleCurve->ResetCurve();
    }

    getThrottleCurve(mixer, VehicleConfig::MIXER_THROTTLECURVE2, &curveValues);
    // is at least one of the curve values != 0?
    if (isValidThrottleCurve(&curveValues)) {
        m_aircraft->PitchCurve->setCurve(&curveValues);
    } else {
        m_aircraft->PitchCurve->ResetCurve();
    }

    updatingFromHardware = false;

    ccpmSwashplateUpdate();
}

/**
   Sends the config to the board (ccpm type)
 */
void ConfigCcpmWidget::setMixer()
{
    int i, j;

    int typeInt = m_aircraft->ccpmType->count() - m_aircraft->ccpmType->currentIndex() - 1;

    // Exit if currently updatingToHardware, ConfigError or LevelConfigInProgress
    // Avoid mixer changes if something wrong in config
    if (throwConfigError(typeInt) || updatingToHardware || SwashLvlConfigurationInProgress) {
        return;
    }

    UpdateMixer();
    updatingToHardware = true;

    MixerSettings *mixerSettings = MixerSettings::GetInstance(getObjectManager());
    Q_ASSERT(mixerSettings);
    MixerSettings::DataFields mixerSettingsData = mixerSettings->getData();

    // Set up some helper pointers
    qint8 *mixers[12] = { mixerSettingsData.Mixer1Vector,
                          mixerSettingsData.Mixer2Vector,
                          mixerSettingsData.Mixer3Vector,
                          mixerSettingsData.Mixer4Vector,
                          mixerSettingsData.Mixer5Vector,
                          mixerSettingsData.Mixer6Vector,
                          mixerSettingsData.Mixer7Vector,
                          mixerSettingsData.Mixer8Vector,
                          mixerSettingsData.Mixer9Vector,
                          mixerSettingsData.Mixer10Vector,
                          mixerSettingsData.Mixer11Vector,
                          mixerSettingsData.Mixer12Vector };

    quint8 *mixerTypes[12] = {
        &mixerSettingsData.Mixer1Type,
        &mixerSettingsData.Mixer2Type,
        &mixerSettingsData.Mixer3Type,
        &mixerSettingsData.Mixer4Type,
        &mixerSettingsData.Mixer5Type,
        &mixerSettingsData.Mixer6Type,
        &mixerSettingsData.Mixer7Type,
        &mixerSettingsData.Mixer8Type,
        &mixerSettingsData.Mixer9Type,
        &mixerSettingsData.Mixer10Type,
        &mixerSettingsData.Mixer11Type,
        &mixerSettingsData.Mixer12Type
    };

    // reset all outputs to Disabled
    for (i = 0; i < (int)ConfigCcpmWidget::CHANNEL_NUMELEM; i++) {
        *mixerTypes[i] = 0;
    }

    // go through the user data and update the mixer matrix
    for (i = 0; i < 6; i++) {
        if ((MixerChannelData[i] > 0) && (MixerChannelData[i] < (int)ConfigCcpmWidget::CHANNEL_NUMELEM + 1)) {
            // Set the mixer type. If Coax, then first two are motors. Otherwise, only first is motor
            if (typeText.compare("Coax 2 Servo 90º", Qt::CaseInsensitive) == 0) {
                *(mixerTypes[MixerChannelData[i] - 1]) = i > 1 ?
                                                         MixerSettings::MIXER1TYPE_SERVO :
                                                         MixerSettings::MIXER1TYPE_MOTOR;
            } else {
                *(mixerTypes[MixerChannelData[i] - 1]) = i > 0 ?
                                                         MixerSettings::MIXER1TYPE_SERVO :
                                                         MixerSettings::MIXER1TYPE_MOTOR;
            }
            // Configure the vector
            for (j = 0; j < 5; j++) {
                mixers[MixerChannelData[i] - 1][j] = m_aircraft->ccpmAdvancedSettingsTable->item(i, j + 1)->text().toInt();
            }
        }
    }

    // get the user data for the curve into the mixer settings
    QList<double> curve1 = m_aircraft->ThrottleCurve->getCurve();
    QList<double> curve2 = m_aircraft->PitchCurve->getCurve();
    for (i = 0; i < 5; i++) {
        mixerSettingsData.ThrottleCurve1[i] = curve1.at(i);
        mixerSettingsData.ThrottleCurve2[i] = curve2.at(i);
    }

    // mapping of collective input to curve 2...
    // MixerSettings.Curve2Source = Throttle,Roll,Pitch,Yaw,Accessory0,Accessory1,Accessory2,Accessory3,Accessory4,Accessory5
    // check if we are using throttle or directly from a channel...
    if (m_aircraft->ccpmCollectivePassthrough->isChecked()) {
        mixerSettingsData.Curve2Source = MixerSettings::CURVE2SOURCE_COLLECTIVE;
    } else {
        mixerSettingsData.Curve2Source = MixerSettings::CURVE2SOURCE_THROTTLE;
    }

    UAVObjectUpdaterHelper updateHelper;

    mixerSettings->setData(mixerSettingsData, false);
    updateHelper.doObjectAndWait(mixerSettings);

    updatingToHardware = false;
}

void ConfigCcpmWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    // Make the custom table columns autostretch:
    m_aircraft->ccpmAdvancedSettingsTable->resizeColumnsToContents();
    for (int i = 0; i < 6; i++) {
        m_aircraft->ccpmAdvancedSettingsTable->setColumnWidth(i, (m_aircraft->ccpmAdvancedSettingsTable->width() -
                                                                  m_aircraft->ccpmAdvancedSettingsTable->verticalHeader()->width()) / 6);
    }
    ccpmSwashplateRedraw();
}
void ConfigCcpmWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    m_aircraft->ccpmAdvancedSettingsTable->resizeColumnsToContents();
    for (int i = 0; i < 6; i++) {
        m_aircraft->ccpmAdvancedSettingsTable->setColumnWidth(i, (m_aircraft->ccpmAdvancedSettingsTable->width() -
                                                                  m_aircraft->ccpmAdvancedSettingsTable->verticalHeader()->width()) / 6);
    }
    ccpmSwashplateRedraw();
}


void ConfigCcpmWidget::SwashLvlStartButtonPressed()
{
    QMessageBox msgBox;
    int i;

    msgBox.setText(tr("<h1>Swashplate Leveling Routine</h1>"));
    msgBox.setInformativeText(tr("<b>You are about to start the Swashplate levelling routine.</b><p>This process will start by downloading the current configuration from the GCS to the OP hardware and will adjust your configuration at various stages.<p>The final state of your system should match the current configuration in the GCS config gadget.</p><p>Please ensure all ccpm settings in the GCS are correct before continuing.</p><p>If this process is interrupted, then the state of your OP board may not match the GCS configuration.</p><p><i>After completing this process, please check all settings before attempting to fly.</i></p><p><font color=red><b>Please disconnect your motor to ensure it will not spin up.</b></font><p><hr><i>Do you wish to proceed?</i></p>"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Information);
    int ret = msgBox.exec();

    UAVObjectField *MinField;
    UAVObjectField *NeutralField;
    UAVObjectField *MaxField;
    UAVDataObject *obj;
    ExtensionSystem::PluginManager *pm;
    UAVObjectManager *objManager;

    switch (ret) {
    case QMessageBox::Yes:
        // Yes was clicked
        SwashLvlState = 0;
        // remove Flight control of ActuatorCommand
        enableSwashplateLevellingControl(true);

        m_aircraft->SwashLvlStartButton->setEnabled(false);
        m_aircraft->SwashLvlNextButton->setEnabled(true);
        m_aircraft->SwashLvlPrevButton->setEnabled(false);
        m_aircraft->SwashLvlCancelButton->setEnabled(true);
        m_aircraft->SwashLvlFinishButton->setEnabled(false);
        // clear status check boxes
        m_aircraft->SwashLvlStepList->item(0)->setCheckState(Qt::Unchecked);
        m_aircraft->SwashLvlStepList->item(1)->setCheckState(Qt::Unchecked);
        m_aircraft->SwashLvlStepList->item(2)->setCheckState(Qt::Unchecked);
        m_aircraft->SwashLvlStepList->item(3)->setCheckState(Qt::Unchecked);


        // download the current settings to the OP hw
        // sendccpmUpdate();
        setMixer();

        // change control mode to gcs control / disarmed
        // set throttle to 0


        // save off the old ActuatorSettings for the swashplate servos
        pm = ExtensionSystem::PluginManager::instance();
        objManager = pm->getObject<UAVObjectManager>();


        // Get the channel assignements:
        obj = dynamic_cast<UAVDataObject *>(objManager->getObject("ActuatorSettings"));
        Q_ASSERT(obj);
        // obj->requestUpdate();
        MinField     = obj->getField("ChannelMin");
        NeutralField = obj->getField("ChannelNeutral");
        MaxField     = obj->getField("ChannelMax");

        // channel assignments
        oldSwashLvlConfiguration.ServoChannels[0] = m_aircraft->ccpmServoWChannel->currentIndex() - 1;
        oldSwashLvlConfiguration.ServoChannels[1] = m_aircraft->ccpmServoXChannel->currentIndex() - 1;
        oldSwashLvlConfiguration.ServoChannels[2] = m_aircraft->ccpmServoYChannel->currentIndex() - 1;
        oldSwashLvlConfiguration.ServoChannels[3] = m_aircraft->ccpmServoZChannel->currentIndex() - 1;
        // if servos are used
        oldSwashLvlConfiguration.Used[0] = ((m_aircraft->ccpmServoWChannel->isEnabled()) && (m_aircraft->ccpmServoWChannel->currentIndex() > 0));
        oldSwashLvlConfiguration.Used[1] = ((m_aircraft->ccpmServoXChannel->isEnabled()) && (m_aircraft->ccpmServoXChannel->currentIndex() > 0));
        oldSwashLvlConfiguration.Used[2] = ((m_aircraft->ccpmServoYChannel->isEnabled()) && (m_aircraft->ccpmServoYChannel->currentIndex() > 0));
        oldSwashLvlConfiguration.Used[3] = ((m_aircraft->ccpmServoZChannel->isEnabled()) && (m_aircraft->ccpmServoZChannel->currentIndex() > 0));
        // min,neutral,max values for the servos
        for (i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
            oldSwashLvlConfiguration.Min[i]     = MinField->getValue(oldSwashLvlConfiguration.ServoChannels[i]).toInt();
            oldSwashLvlConfiguration.Neutral[i] = NeutralField->getValue(oldSwashLvlConfiguration.ServoChannels[i]).toInt();
            oldSwashLvlConfiguration.Max[i]     = MaxField->getValue(oldSwashLvlConfiguration.ServoChannels[i]).toInt();
        }

        // copy to new Actuator settings.
        memcpy((void *)&newSwashLvlConfiguration, (void *)&oldSwashLvlConfiguration, sizeof(SwashplateServoSettingsStruct));

        // goto the first step
        SwashLvlNextButtonPressed();
        break;
    case QMessageBox::Cancel:
        // Cancel was clicked
        SwashLvlState = 0;
        // restore Flight control of ActuatorCommand
        enableSwashplateLevellingControl(false);

        m_aircraft->SwashLvlStartButton->setEnabled(true);
        m_aircraft->SwashLvlNextButton->setEnabled(false);
        m_aircraft->SwashLvlPrevButton->setEnabled(false);
        m_aircraft->SwashLvlCancelButton->setEnabled(false);
        m_aircraft->SwashLvlFinishButton->setEnabled(false);
        break;
    default:
        // should never be reached
        break;
    }
}

void ConfigCcpmWidget::SwashLvlPrevButtonPressed()
{
    SwashLvlState--;
    SwashLvlPrevNextButtonPressed();
}

void ConfigCcpmWidget::SwashLvlNextButtonPressed()
{
    SwashLvlState++;
    SwashLvlPrevNextButtonPressed();
}
void ConfigCcpmWidget::SwashLvlPrevNextButtonPressed()
{
    switch (SwashLvlState) {
    case 0:
        break;
    case 1: // Neutral levelling
        m_aircraft->SwashLvlPrevButton->setEnabled(false);
        m_aircraft->SwashLvlStepList->setCurrentRow(0);
        // set spin boxes and swashplate servos to Neutral values
        setSwashplateLevel(50);
        // disable position slider
        m_aircraft->SwashLvlPositionSlider->setEnabled(false);
        m_aircraft->SwashLvlPositionSpinBox->setEnabled(false);
        // set position slider to 50%
        m_aircraft->SwashLvlPositionSlider->setValue(50);
        m_aircraft->SwashLvlPositionSpinBox->setValue(50);
        // connect spinbox signals to slots and ebnable them
        for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
            connect(SwashLvlSpinBoxes[i], SIGNAL(valueChanged(int)), this, SLOT(SwashLvlSpinBoxChanged(int)));
            SwashLvlSpinBoxes[i]->setEnabled(true);
        }
        // issue user instructions
        m_aircraft->SwashLvlStepInstruction->setHtml(
            tr("<h2>Neutral levelling</h2><p>Using adjustment of:<ul><li>Servo horns,</li><li>Link lengths,</li><li>Neutral triming spinboxes to the right</li></ul><br>Ensure that the swashplate is in the center of desired travel range and is level."));
        break;
    case 2: // Max levelling
        m_aircraft->SwashLvlPrevButton->setEnabled(true);
        // check Neutral status as complete
        m_aircraft->SwashLvlStepList->item(0)->setCheckState(Qt::Checked);
        m_aircraft->SwashLvlStepList->setCurrentRow(1);
        // set spin boxes and swashplate servos to Max values
        setSwashplateLevel(100);
        // set position slider to 100%
        m_aircraft->SwashLvlPositionSlider->setValue(100);
        m_aircraft->SwashLvlPositionSpinBox->setValue(100);
        // issue user instructions
        m_aircraft->SwashLvlStepInstruction->setText(
            tr("<h2>Max levelling</h2><p>Using adjustment of:<ul><li>Max triming spinboxes to the right ONLY</li></ul><br>Ensure that the swashplate is at the top of desired travel range and is level."));
        break;
    case 3: // Min levelling
        // check Max status as complete
        m_aircraft->SwashLvlStepList->item(1)->setCheckState(Qt::Checked);
        m_aircraft->SwashLvlStepList->setCurrentRow(2);
        // set spin boxes and swashplate servos to Min values
        setSwashplateLevel(0);
        // set position slider to 0%
        m_aircraft->SwashLvlPositionSlider->setValue(0);
        m_aircraft->SwashLvlPositionSpinBox->setValue(0);
        // issue user instructions
        m_aircraft->SwashLvlStepInstruction->setText(
            tr("<h2>Min levelling</h2><p>Using adjustment of:<ul><li>Min triming spinboxes to the right ONLY</li></ul><br>Ensure that the swashplate is at the bottom of desired travel range and is level."));
        break;
    case 4: // levelling verification
        // check Min status as complete
        m_aircraft->SwashLvlNextButton->setEnabled(true);
        m_aircraft->SwashLvlStepList->item(2)->setCheckState(Qt::Checked);
        m_aircraft->SwashLvlStepList->setCurrentRow(3);
        // enable position slider
        m_aircraft->SwashLvlPositionSlider->setEnabled(true);
        m_aircraft->SwashLvlPositionSpinBox->setEnabled(true);
        // make heli respond to slider movement
        connect(m_aircraft->SwashLvlPositionSlider, SIGNAL(valueChanged(int)), this, SLOT(setSwashplateLevel(int)));
        // disable spin boxes
        for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
            SwashLvlSpinBoxes[i]->setEnabled(false);
        }

        // issue user instructions
        m_aircraft->SwashLvlStepInstruction->setText(
            tr("<h2>Levelling verification</h2><p>Adjust the slider to the right over it's full range and observe the swashplate motion. It should remain level over the entire range of travel.</p>"));
        break;
    case 5: // levelling complete
        // check verify status as complete
        m_aircraft->SwashLvlStepList->item(3)->setCheckState(Qt::Checked);
        // issue user instructions
        m_aircraft->SwashLvlStepInstruction->setText(
            tr("<h2>Levelling complete</h2><p>Press the Finish button to save these settings to the SD card</p><p>Press the cancel button to return to the pre-levelling settings</p>"));
        // disable position slider
        m_aircraft->SwashLvlPositionSlider->setEnabled(false);
        m_aircraft->SwashLvlPositionSpinBox->setEnabled(false);
        // disconnect levelling slots from signals
        disconnect(m_aircraft->SwashLvlPositionSlider, SIGNAL(valueChanged(int)), this, SLOT(setSwashplateLevel(int)));
        for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
            disconnect(SwashLvlSpinBoxes[i], SIGNAL(valueChanged(int)), this, SLOT(SwashLvlSpinBoxChanged(int)));
        }

        m_aircraft->SwashLvlStartButton->setEnabled(false);
        m_aircraft->SwashLvlNextButton->setEnabled(false);
        m_aircraft->SwashLvlPrevButton->setEnabled(true);
        m_aircraft->SwashLvlCancelButton->setEnabled(true);
        m_aircraft->SwashLvlFinishButton->setEnabled(true);
        break;
    default:
        // restore collective/cyclic setting
        // restore pitch curve
        // clear spin boxes
        // change control mode to gcs control (OFF) / disarmed
        // issue user confirmation
        break;
    }
}

void ConfigCcpmWidget::SwashLvlCancelButtonPressed()
{
    SwashLvlState = 0;

    UAVObjectField *MinField;
    UAVObjectField *NeutralField;
    UAVObjectField *MaxField;

    m_aircraft->SwashLvlStartButton->setEnabled(true);
    m_aircraft->SwashLvlNextButton->setEnabled(false);
    m_aircraft->SwashLvlPrevButton->setEnabled(false);
    m_aircraft->SwashLvlCancelButton->setEnabled(false);
    m_aircraft->SwashLvlFinishButton->setEnabled(false);

    m_aircraft->SwashLvlStepList->item(0)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(1)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(2)->setCheckState(Qt::Unchecked);
    m_aircraft->SwashLvlStepList->item(3)->setCheckState(Qt::Unchecked);

    // restore old Actuator Settings
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    UAVDataObject *obj = dynamic_cast<UAVDataObject *>(objManager->getObject("ActuatorSettings"));
    Q_ASSERT(obj);
    // update settings to match our changes.
    MinField     = obj->getField("ChannelMin");
    NeutralField = obj->getField("ChannelNeutral");
    MaxField     = obj->getField("ChannelMax");

    // min,neutral,max values for the servos
    for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        MinField->setValue(oldSwashLvlConfiguration.Min[i], oldSwashLvlConfiguration.ServoChannels[i]);
        NeutralField->setValue(oldSwashLvlConfiguration.Neutral[i], oldSwashLvlConfiguration.ServoChannels[i]);
        MaxField->setValue(oldSwashLvlConfiguration.Max[i], oldSwashLvlConfiguration.ServoChannels[i]);
    }

    obj->updated();

    // restore Flight control of ActuatorCommand
    enableSwashplateLevellingControl(false);

    m_aircraft->SwashLvlStepInstruction->setText(
        tr("<h2>Levelling Cancelled</h2><p>Previous settings have been restored."));

    ccpmSwashplateUpdate();
}


void ConfigCcpmWidget::SwashLvlFinishButtonPressed()
{
    UAVObjectField *MinField;
    UAVObjectField *NeutralField;
    UAVObjectField *MaxField;

    m_aircraft->SwashLvlStartButton->setEnabled(true);
    m_aircraft->SwashLvlNextButton->setEnabled(false);
    m_aircraft->SwashLvlPrevButton->setEnabled(false);
    m_aircraft->SwashLvlCancelButton->setEnabled(false);
    m_aircraft->SwashLvlFinishButton->setEnabled(false);

    // save new Actuator Settings to memory and SD card
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    UAVDataObject *obj = dynamic_cast<UAVDataObject *>(objManager->getObject("ActuatorSettings"));
    Q_ASSERT(obj);

    // update settings to match our changes.
    MinField     = obj->getField("ChannelMin");
    NeutralField = obj->getField("ChannelNeutral");
    MaxField     = obj->getField("ChannelMax");

    // min,neutral,max values for the servos
    for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        MinField->setValue(newSwashLvlConfiguration.Min[i], newSwashLvlConfiguration.ServoChannels[i]);
        NeutralField->setValue(newSwashLvlConfiguration.Neutral[i], newSwashLvlConfiguration.ServoChannels[i]);
        MaxField->setValue(newSwashLvlConfiguration.Max[i], newSwashLvlConfiguration.ServoChannels[i]);
    }

    obj->updated();
    saveObjectToSD(obj);

    // restore Flight control of ActuatorCommand
    enableSwashplateLevellingControl(false);

    m_aircraft->SwashLvlStepInstruction->setText(
        tr("<h2>Levelling Completed</h2><p>New settings have been saved to the SD card"));

    ShowDisclaimer(0);
    // ShowDisclaimer(2);

    ccpmSwashplateUpdate();
}

void ConfigCcpmWidget::saveObjectToSD(UAVObject *obj)
{
    // saveObjectToSD is now handled by the UAVUtils plugin in one
    // central place (and one central queue)
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();

    utilMngr->saveObjectToSD(obj);
}

int ConfigCcpmWidget::ShowDisclaimer(int messageID)
{
    QMessageBox msgBox;

    msgBox.setText(tr("<font color=red><h1>Warning!!!</h2></font>"));
    int ret;
    switch (messageID) {
    case 0:
        // Basic disclaimer
        msgBox.setInformativeText(
            tr("<h2>This code has many configurations.</h2><p>Please double check all settings before attempting flight!"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Information);
        ret = msgBox.exec();
        return 0;

        break;
    case 1:
        // Not Tested disclaimer
        msgBox.setInformativeText(
            tr("<h2>The CCPM mixer code needs more testing!</h2><p><font color=red>Use it at your own risk!</font><p>Do you wish to continue?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Warning);
        ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::Cancel:
            return -1;

        case QMessageBox::Yes:
            return 0;
        }
        break;
    case 2:
        // DO NOT use
        msgBox.setInformativeText(
            tr("<h2>The CCPM swashplate levelling code is NOT complete!</h2><p><font color=red>DO NOT use it for flight!</font>"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Critical);
        ret = msgBox.exec();
        return 0;

        break;
    default:
        // should never be reached
        break;
    }
    return -1;
}


/**
   Toggles the channel testing mode by making the GCS take over
   the ActuatorCommand objects
 */
void ConfigCcpmWidget::enableSwashplateLevellingControl(bool state)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    UAVDataObject *obj = dynamic_cast<UAVDataObject *>(objManager->getObject("ActuatorCommand"));
    UAVObject::Metadata mdata    = obj->getMetadata();

    if (state) {
        SwashLvlaccInitialData = mdata;
        UAVObject::SetFlightAccess(mdata, UAVObject::ACCESS_READONLY);
        UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_ONCHANGE);
        UAVObject::SetGcsTelemetryAcked(mdata, false);
        UAVObject::SetGcsTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_ONCHANGE);
        mdata.gcsTelemetryUpdatePeriod  = 100;
        SwashLvlConfigurationInProgress = 1;
        m_aircraft->TabObject->setTabEnabled(0, 0);
        m_aircraft->TabObject->setTabEnabled(2, 0);
        m_aircraft->TabObject->setTabEnabled(3, 0);
        m_aircraft->ccpmType->setEnabled(false);
    } else {
        // Restore metadata
        mdata = SwashLvlaccInitialData;
        SwashLvlConfigurationInProgress = 0;

        m_aircraft->TabObject->setTabEnabled(0, 1);
        m_aircraft->TabObject->setTabEnabled(2, 1);
        m_aircraft->TabObject->setTabEnabled(3, 1);
        m_aircraft->ccpmType->setEnabled(true);
    }
    obj->setMetadata(mdata);
}

/**
   Sets the swashplate level to a given value based on current settings for Max, Neutral and Min values.
   level ranges -1 to +1
 */
void ConfigCcpmWidget::setSwashplateLevel(int percent)
{
    if (percent < 0) {
        return; // -1;
    }
    if (percent > 100) {
        return; // -1;
    }
    if (SwashLvlConfigurationInProgress != 1) {
        return; // -1;
    }

    double level = ((double)percent / 50.00) - 1.00;

    SwashLvlServoInterlock = 1;

    ActuatorCommand *actuatorCommand = ActuatorCommand::GetInstance(getObjectManager());
    ActuatorCommand::DataFields actuatorCommandData = actuatorCommand->getData();

    for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        double value = 0;
        if (level == 0) {
            value = newSwashLvlConfiguration.Neutral[i];
        } else if (level > 0) {
            value = (newSwashLvlConfiguration.Max[i] - newSwashLvlConfiguration.Neutral[i]) * level
                    + newSwashLvlConfiguration.Neutral[i];
        } else if (level < 0) {
            value = (newSwashLvlConfiguration.Neutral[i] - newSwashLvlConfiguration.Min[i]) * level
                    + newSwashLvlConfiguration.Neutral[i];
        }
        actuatorCommandData.Channel[newSwashLvlConfiguration.ServoChannels[i]] = value;
        SwashLvlSpinBoxes[i]->setValue(value);
    }

    UAVObjectUpdaterHelper updateHelper;

    actuatorCommand->setData(actuatorCommandData, false);
    updateHelper.doObjectAndWait(actuatorCommand);

    SwashLvlServoInterlock = 0;
}


void ConfigCcpmWidget::SwashLvlSpinBoxChanged(int value)
{
    Q_UNUSED(value);

    if (SwashLvlServoInterlock == 1) {
        return;
    }

    ActuatorCommand *actuatorCommand = ActuatorCommand::GetInstance(getObjectManager());
    ActuatorCommand::DataFields actuatorCommandData = actuatorCommand->getData();

    for (int i = 0; i < CCPM_MAX_SWASH_SERVOS; i++) {
        value = SwashLvlSpinBoxes[i]->value();

        switch (SwashLvlState) {
        case 1:
            // Neutral levelling
            newSwashLvlConfiguration.Neutral[i] = value;
            break;
        case 2:
            // Max levelling
            newSwashLvlConfiguration.Max[i] = value;
            break;
        case 3:
            // Min levelling
            newSwashLvlConfiguration.Min[i] = value;
            break;
        case 4:
            // levelling verification
            break;
        case 5:
            // levelling complete
            break;
        default:
            break;
        }

        actuatorCommandData.Channel[newSwashLvlConfiguration.ServoChannels[i]] = value;
    }

    UAVObjectUpdaterHelper updateHelper;

    actuatorCommand->setData(actuatorCommandData, false);
    updateHelper.doObjectAndWait(actuatorCommand);
}

/**
   This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigCcpmWidget::throwConfigError(int typeInt)
{
    bool error = false;

    // Custom no need check, always return no error
    if (typeInt == 0) {
        return false;
    }

    QList<QComboBox *> comboChannelsName;
    comboChannelsName << m_aircraft->ccpmEngineChannel << m_aircraft->ccpmTailChannel << m_aircraft->ccpmServoWChannel
                      << m_aircraft->ccpmServoXChannel << m_aircraft->ccpmServoYChannel << m_aircraft->ccpmServoZChannel;
    QString channelNames = "";

    for (int i = 0; i < 6; i++) {
        QComboBox *combobox = comboChannelsName[i];
        if (combobox && (combobox->isVisible())) {
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
                combobox->setToolTip(tr("Channel already used"));
                error = true;
            } else {
                for (int index = 0; index < (int)ConfigCcpmWidget::CHANNEL_NUMELEM; index++) {
                    combobox->setItemData(index, 0, Qt::DecorationRole); // Reset all color palettes
                    combobox->setToolTip("");
                }
            }
            channelNames += (combobox->currentText() == "None") ? "" : combobox->currentText();
        }
    }
    return error;
}
