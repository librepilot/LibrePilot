/**
 ******************************************************************************
 *
 * @file       configinputwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo input configuration panel for the config gadget
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

#include "configinputwidget.h"

#include "ui_input.h"
#include "ui_input_wizard.h"

#include "inputchannelform.h"
#include "ui_inputchannelform.h"

#include "failsafechannelform.h"
#include "ui_failsafechannelform.h"

#include <uavobjectmanager.h>
#include <uavobjecthelper.h>
#include <utils/stylehelper.h>

#include <systemalarms.h>

#include <QDebug>
#include <QWidget>
#include <QMessageBox>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>

#define ACCESS_MIN_MOVE            -3
#define ACCESS_MAX_MOVE            3
#define STICK_MIN_MOVE             -8
#define STICK_MAX_MOVE             8

#define MIN_INPUT_US               100
#define MAX_INPUT_US               2500

#define CHANNEL_NUMBER_NONE        0
#define DEFAULT_FLIGHT_MODE_NUMBER 1

ConfigInputWidget::ConfigInputWidget(QWidget *parent) :
    ConfigTaskWidget(parent),
    wizardStep(wizardNone),
    // not currently stored in the settings UAVO
    transmitterMode(mode2),
    transmitterType(acro),
    //
    loop(NULL),
    skipflag(false),
    nextDelayedTimer(),
    nextDelayedTick(0),
    nextDelayedLatestActivityTick(0),
    accessoryDesiredObj0(NULL),
    accessoryDesiredObj1(NULL),
    accessoryDesiredObj2(NULL),
    accessoryDesiredObj3(NULL),
    rssiDesiredObj4(NULL)
{
    ui = new Ui_InputWidget();
    ui->setupUi(this);

    // must be done before auto binding !
    setWikiURL("Input+Configuration");

    addAutoBindings();

    connect(this, SIGNAL(enableControlsChanged(bool)), this, SLOT(enableControlsChanged(bool)));

    manualCommandObj      = ManualControlCommand::GetInstance(getObjectManager());
    manualSettingsObj     = ManualControlSettings::GetInstance(getObjectManager());
    flightModeSettingsObj = FlightModeSettings::GetInstance(getObjectManager());
    flightStatusObj = FlightStatus::GetInstance(getObjectManager());
    receiverActivityObj   = ReceiverActivity::GetInstance(getObjectManager());
    accessoryDesiredObj0  = AccessoryDesired::GetInstance(getObjectManager(), 0);
    accessoryDesiredObj1  = AccessoryDesired::GetInstance(getObjectManager(), 1);
    accessoryDesiredObj2  = AccessoryDesired::GetInstance(getObjectManager(), 2);
    accessoryDesiredObj3  = AccessoryDesired::GetInstance(getObjectManager(), 3);
    rssiDesiredObj4 = AccessoryDesired::GetInstance(getObjectManager(), 4);
    actuatorSettingsObj   = ActuatorSettings::GetInstance(getObjectManager());
    systemSettingsObj     = SystemSettings::GetInstance(getObjectManager());
    hwSettingsObj = HwSettings::GetInstance(getObjectManager());
    // Only instance 0 is present if the board is not connected.
    // The other instances are populated lazily.
    Q_ASSERT(accessoryDesiredObj0);

    // Generate the rows of buttons in the input channel form GUI
    quint32 index   = 0;
    quint32 indexRT = 0;
    foreach(QString name, manualSettingsObj->getField("ChannelNumber")->getElementNames()) {
        Q_ASSERT(index < ManualControlSettings::CHANNELGROUPS_NUMELEM);

        // Input channel setup
        InputChannelForm *inputChannelForm = new InputChannelForm(index, this);
        inputChannelForm->setName(name);

        inputChannelForm->moveTo(*(ui->channelLayout));

        // The order of the following binding calls is important. Since the values will be populated
        // in reverse order of the binding order otherwise the 'Reversed' logic will floor the neutral value
        // to the max value ( which is smaller than the neutral value when reversed ) and the channel number
        // will not be set correctly.
        addWidgetBinding("ManualControlSettings", "ChannelNumber", inputChannelForm->ui->channelNumber, index);
        addWidgetBinding("ManualControlSettings", "ChannelGroups", inputChannelForm->ui->channelGroup, index);
        // Slider position based on real time Rcinput (allow monitoring)
        addWidgetBinding("ManualControlCommand", "Channel", inputChannelForm->ui->channelNeutral, index);
        // Neutral value stored on board (SpinBox)
        // Rssi neutral is always set to min
        if (index == ManualControlSettings::CHANNELGROUPS_RSSI) {
            addWidgetBinding("ManualControlSettings", "ChannelMin", inputChannelForm->ui->neutralValue, index);
        } else {
            addWidgetBinding("ManualControlSettings", "ChannelNeutral", inputChannelForm->ui->neutralValue, index);
        }
        addWidgetBinding("ManualControlSettings", "ChannelMax", inputChannelForm->ui->channelMax, index);
        addWidgetBinding("ManualControlSettings", "ChannelMin", inputChannelForm->ui->channelMin, index);
        addWidgetBinding("ManualControlSettings", "ChannelMax", inputChannelForm->ui->channelMax, index);

        addWidget(inputChannelForm->ui->channelRev);

        // Reversing supported for some channels only
        bool reversable = ((index == ManualControlSettings::CHANNELGROUPS_THROTTLE) ||
                           (index == ManualControlSettings::CHANNELGROUPS_ROLL) ||
                           (index == ManualControlSettings::CHANNELGROUPS_PITCH) ||
                           (index == ManualControlSettings::CHANNELGROUPS_YAW));
        inputChannelForm->ui->channelRev->setVisible(reversable);

        // Input filter response time fields supported for some channels only
        // Set Neutral field to readonly for Rssi
        switch (index) {
        case ManualControlSettings::CHANNELGROUPS_RSSI:
            inputChannelForm->ui->neutralValue->setReadOnly(true);
        case ManualControlSettings::CHANNELGROUPS_ROLL:
        case ManualControlSettings::CHANNELGROUPS_PITCH:
        case ManualControlSettings::CHANNELGROUPS_YAW:
        case ManualControlSettings::CHANNELGROUPS_COLLECTIVE:
        case ManualControlSettings::CHANNELGROUPS_ACCESSORY0:
        case ManualControlSettings::CHANNELGROUPS_ACCESSORY1:
        case ManualControlSettings::CHANNELGROUPS_ACCESSORY2:
        case ManualControlSettings::CHANNELGROUPS_ACCESSORY3:
            addWidgetBinding("ManualControlSettings", "ResponseTime", inputChannelForm->ui->channelResponseTime, indexRT);
            ++indexRT;
            break;
        case ManualControlSettings::CHANNELGROUPS_THROTTLE:
        case ManualControlSettings::CHANNELGROUPS_FLIGHTMODE:
            inputChannelForm->ui->channelResponseTime->setVisible(false);
            break;
        default:
            Q_ASSERT(0);
            break;
        }
        ++index;
    }

    QList<int> failsafeReloadGroup;
    failsafeReloadGroup.append(555);

    addWidgetBinding("ManualControlSettings", "FailsafeFlightModeSwitchPosition", ui->failsafeFlightMode, 0, 1, true, new QList<int>(failsafeReloadGroup));

    addWidgetBinding("FlightModeSettings", "BatteryFailsafeSwitchPositions", ui->failsafeBatteryWarningFlightMode, 0, 1, true, new QList<int>(failsafeReloadGroup));
    addWidgetBinding("FlightModeSettings", "BatteryFailsafeSwitchPositions", ui->failsafeBatteryCriticalFlightMode, 1, 1, true, new QList<int>(failsafeReloadGroup));

    // Generate the rows for the failsafe channel form GUI
    index = 0;
    foreach(QString name, manualSettingsObj->getField("FailsafeChannel")->getElementNames()) {
        Q_ASSERT(index < ManualControlSettings::FAILSAFECHANNEL_NUMELEM);

        // Failsafe channels setup
        FailsafeChannelForm *failsafeChannelForm = new FailsafeChannelForm(index, this);
        addWidget(failsafeChannelForm->ui->channelValueSpinner);
        failsafeChannelForm->setName(name);
        failsafeChannelForm->moveTo(*(ui->failsafeChannelsLayout));
        addWidgetBinding("ManualControlSettings", "FailsafeChannel", failsafeChannelForm->ui->channelValue, index, 0.01, true, new QList<int>(failsafeReloadGroup));
        ++index;
    }
    addWidget(ui->failsafeDefault);

    addWidgetBinding("ManualControlSettings", "Deadband", ui->deadband, 0, 1);
    addWidgetBinding("ManualControlSettings", "DeadbandAssistedControl", ui->assistedControlDeadband, 0, 1);

    connect(ui->configurationWizard, SIGNAL(clicked()), this, SLOT(goToWizard()));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(disableWizardButton(int)));
    connect(ui->runCalibration, SIGNAL(toggled(bool)), this, SLOT(simpleCalibration(bool)));

    connect(ReceiverActivity::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateReceiverActivityStatus()));
    connect(FlightStatus::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateReceiverActivityStatus()));

    ui->receiverActivityStatus->setStyleSheet("QLabel { background-color: darkGreen; color: rgb(255, 255, 255); \
                                               border: 1px solid grey; border-radius: 5; margin:1px; font:bold;}");

    ui->stackedWidget->setCurrentIndex(0);
    QList<QWidget *> widgets = QList<QWidget *>() << ui->fmsModePos1 << ui->fmsModePos2 << ui->fmsModePos3 <<
                               ui->fmsModePos4 << ui->fmsModePos5 << ui->fmsModePos6;
    index = 0;
    foreach(QWidget * widget, widgets) {
        addWidgetBinding("FlightModeSettings", "FlightModePosition", widget, index++, 1, true);
    }

    addWidgetBinding("FlightModeSettings", "Stabilization1Settings", ui->fmsSsPos1Roll, "Roll", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization2Settings", ui->fmsSsPos2Roll, "Roll", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization3Settings", ui->fmsSsPos3Roll, "Roll", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization4Settings", ui->fmsSsPos4Roll, "Roll", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization5Settings", ui->fmsSsPos5Roll, "Roll", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization6Settings", ui->fmsSsPos6Roll, "Roll", 1, true);

    addWidgetBinding("FlightModeSettings", "Stabilization1Settings", ui->fmsSsPos1Pitch, "Pitch", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization2Settings", ui->fmsSsPos2Pitch, "Pitch", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization3Settings", ui->fmsSsPos3Pitch, "Pitch", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization4Settings", ui->fmsSsPos4Pitch, "Pitch", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization5Settings", ui->fmsSsPos5Pitch, "Pitch", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization6Settings", ui->fmsSsPos6Pitch, "Pitch", 1, true);

    addWidgetBinding("FlightModeSettings", "Stabilization1Settings", ui->fmsSsPos1Yaw, "Yaw", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization2Settings", ui->fmsSsPos2Yaw, "Yaw", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization3Settings", ui->fmsSsPos3Yaw, "Yaw", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization4Settings", ui->fmsSsPos4Yaw, "Yaw", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization5Settings", ui->fmsSsPos5Yaw, "Yaw", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization6Settings", ui->fmsSsPos6Yaw, "Yaw", 1, true);

    addWidgetBinding("FlightModeSettings", "Stabilization1Settings", ui->fmsSsPos1Thrust, "Thrust", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization2Settings", ui->fmsSsPos2Thrust, "Thrust", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization3Settings", ui->fmsSsPos3Thrust, "Thrust", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization4Settings", ui->fmsSsPos4Thrust, "Thrust", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization5Settings", ui->fmsSsPos5Thrust, "Thrust", 1, true);
    addWidgetBinding("FlightModeSettings", "Stabilization6Settings", ui->fmsSsPos6Thrust, "Thrust", 1, true);

    addWidgetBinding("ManualControlSettings", "FlightModeNumber", ui->fmsPosNum);

    addWidgetBinding("FlightModeSettings", "Arming", ui->armControl);
    addWidgetBinding("FlightModeSettings", "ArmedTimeout", ui->armTimeout, 0, 1000);
    connect(ManualControlCommand::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveFMSlider()));
    connect(ManualControlSettings::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updatePositionSlider()));
    connect(SystemAlarms::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateConfigAlarmStatus()));

    connect(ui->failsafeFlightMode, SIGNAL(currentIndexChanged(int)), this, SLOT(failsafeFlightModeChanged(int)));
    connect(ui->failsafeFlightModeCb, SIGNAL(toggled(bool)), this, SLOT(failsafeFlightModeCbToggled(bool)));

    connect(ui->failsafeBatteryWarningFlightMode, SIGNAL(currentIndexChanged(int)), this, SLOT(failsafeBatteryWarningFlightModeChanged(int)));
    connect(ui->failsafeBatteryWarningFlightModeCb, SIGNAL(toggled(bool)), this, SLOT(failsafeBatteryWarningFlightModeCbToggled(bool)));

    connect(ui->failsafeBatteryCriticalFlightMode, SIGNAL(currentIndexChanged(int)), this, SLOT(failsafeBatteryCriticalFlightModeChanged(int)));
    connect(ui->failsafeBatteryCriticalFlightModeCb, SIGNAL(toggled(bool)), this, SLOT(failsafeBatteryCriticalFlightModeCbToggled(bool)));


    addWidget(ui->configurationWizard);
    addWidget(ui->runCalibration);
    addWidget(ui->failsafeFlightModeCb);
    addWidget(ui->failsafeBatteryWarningFlightModeCb);
    addWidget(ui->failsafeBatteryCriticalFlightModeCb);

    // Wizard
    wizardUi = new Ui_InputWizardWidget();
    wizardUi->setupUi(ui->wizard);

    connect(wizardUi->wzNext, SIGNAL(clicked()), this, SLOT(wzNext()));
    connect(wizardUi->wzCancel, SIGNAL(clicked()), this, SLOT(wzCancel()));
    connect(wizardUi->wzBack, SIGNAL(clicked()), this, SLOT(wzBack()));

    wizardUi->graphicsView->setScene(new QGraphicsScene(this));
    wizardUi->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_renderer = new QSvgRenderer();
    QGraphicsScene *l_scene = wizardUi->graphicsView->scene();
    wizardUi->graphicsView->setBackgroundBrush(QBrush(Utils::StyleHelper::baseColor()));
    if (QFile::exists(":/configgadget/images/TX2.svg") && m_renderer->load(QString(":/configgadget/images/TX2.svg")) && m_renderer->isValid()) {
        l_scene->clear(); // Deletes all items contained in the scene as well.

        m_txBackground = new QGraphicsSvgItem();
        // All other items will be clipped to the shape of the background
        m_txBackground->setFlags(QGraphicsItem::ItemClipsChildrenToShape |
                                 QGraphicsItem::ItemClipsToShape);
        m_txBackground->setSharedRenderer(m_renderer);
        m_txBackground->setElementId("background");
        l_scene->addItem(m_txBackground);

        m_txMainBody = new QGraphicsSvgItem();
        m_txMainBody->setParentItem(m_txBackground);
        m_txMainBody->setSharedRenderer(m_renderer);
        m_txMainBody->setElementId("body");
        l_scene->addItem(m_txMainBody);

        m_txLeftStick = new QGraphicsSvgItem();
        m_txLeftStick->setParentItem(m_txBackground);
        m_txLeftStick->setSharedRenderer(m_renderer);
        m_txLeftStick->setElementId("ljoy");

        m_txRightStick = new QGraphicsSvgItem();
        m_txRightStick->setParentItem(m_txBackground);
        m_txRightStick->setSharedRenderer(m_renderer);
        m_txRightStick->setElementId("rjoy");

        m_txAccess0 = new QGraphicsSvgItem();
        m_txAccess0->setParentItem(m_txBackground);
        m_txAccess0->setSharedRenderer(m_renderer);
        m_txAccess0->setElementId("access0");

        m_txAccess1 = new QGraphicsSvgItem();
        m_txAccess1->setParentItem(m_txBackground);
        m_txAccess1->setSharedRenderer(m_renderer);
        m_txAccess1->setElementId("access1");

        m_txAccess2 = new QGraphicsSvgItem();
        m_txAccess2->setParentItem(m_txBackground);
        m_txAccess2->setSharedRenderer(m_renderer);
        m_txAccess2->setElementId("access2");

        m_txAccess3 = new QGraphicsSvgItem();
        m_txAccess3->setParentItem(m_txBackground);
        m_txAccess3->setSharedRenderer(m_renderer);
        m_txAccess3->setElementId("access3");

        m_txFlightMode = new QGraphicsSvgItem();
        m_txFlightMode->setParentItem(m_txBackground);
        m_txFlightMode->setSharedRenderer(m_renderer);
        m_txFlightMode->setElementId("flightModeCenter");
        m_txFlightMode->setZValue(-10);

        m_txFlightModeCountBG = new QGraphicsSvgItem();
        m_txFlightModeCountBG->setParentItem(m_txBackground);
        m_txFlightModeCountBG->setSharedRenderer(m_renderer);
        m_txFlightModeCountBG->setElementId("fm_count_bg");
        l_scene->addItem(m_txFlightModeCountBG);

        m_txFlightModeCountText = new QGraphicsSimpleTextItem("?", m_txFlightModeCountBG);
        m_txFlightModeCountText->setBrush(QColor(40, 40, 40));
        m_txFlightModeCountText->setFont(QFont("Arial Bold"));

        m_txArrows = new QGraphicsSvgItem();
        m_txArrows->setParentItem(m_txBackground);
        m_txArrows->setSharedRenderer(m_renderer);
        m_txArrows->setElementId("arrows");
        m_txArrows->setVisible(false);

        QRectF orig    = m_renderer->boundsOnElement("ljoy");
        QMatrix Matrix = m_renderer->matrixForElement("ljoy");
        orig = Matrix.mapRect(orig);
        m_txLeftStickOrig.translate(orig.x(), orig.y());
        m_txLeftStick->setTransform(m_txLeftStickOrig, false);

        orig   = m_renderer->boundsOnElement("arrows");
        Matrix = m_renderer->matrixForElement("arrows");
        orig   = Matrix.mapRect(orig);
        m_txArrowsOrig.translate(orig.x(), orig.y());
        m_txArrows->setTransform(m_txArrowsOrig, false);

        orig   = m_renderer->boundsOnElement("body");
        Matrix = m_renderer->matrixForElement("body");
        orig   = Matrix.mapRect(orig);
        m_txMainBodyOrig.translate(orig.x(), orig.y());
        m_txMainBody->setTransform(m_txMainBodyOrig, false);

        orig   = m_renderer->boundsOnElement("flightModeCenter");
        Matrix = m_renderer->matrixForElement("flightModeCenter");
        orig   = Matrix.mapRect(orig);
        m_txFlightModeCOrig.translate(orig.x(), orig.y());
        m_txFlightMode->setTransform(m_txFlightModeCOrig, false);

        orig   = m_renderer->boundsOnElement("flightModeLeft");
        Matrix = m_renderer->matrixForElement("flightModeLeft");
        orig   = Matrix.mapRect(orig);
        m_txFlightModeLOrig.translate(orig.x(), orig.y());
        orig   = m_renderer->boundsOnElement("flightModeRight");
        Matrix = m_renderer->matrixForElement("flightModeRight");
        orig   = Matrix.mapRect(orig);
        m_txFlightModeROrig.translate(orig.x(), orig.y());

        orig   = m_renderer->boundsOnElement("fm_count_bg");
        Matrix = m_renderer->matrixForElement("fm_count_bg");
        orig   = Matrix.mapRect(orig);
        m_txFlightModeCountBGOrig.translate(orig.x(), orig.y());
        m_txFlightModeCountBG->setTransform(m_txFlightModeCountBGOrig, false);

        QRectF flightModeBGRect   = m_txFlightModeCountBG->boundingRect();
        QRectF flightModeTextRect = m_txFlightModeCountText->boundingRect();
        qreal scale = 2.5;
        m_txFlightModeCountTextOrig.translate(flightModeBGRect.width() - (flightModeBGRect.height() / 2), flightModeBGRect.height() / 2);
        m_txFlightModeCountTextOrig.scale(scale, scale);
        m_txFlightModeCountTextOrig.translate(-flightModeTextRect.width() / 2, -flightModeTextRect.height() / 2);
        m_txFlightModeCountText->setTransform(m_txFlightModeCountTextOrig, false);

        orig   = m_renderer->boundsOnElement("rjoy");
        Matrix = m_renderer->matrixForElement("rjoy");
        orig   = Matrix.mapRect(orig);
        m_txRightStickOrig.translate(orig.x(), orig.y());
        m_txRightStick->setTransform(m_txRightStickOrig, false);

        orig   = m_renderer->boundsOnElement("access0");
        Matrix = m_renderer->matrixForElement("access0");
        orig   = Matrix.mapRect(orig);
        m_txAccess0Orig.translate(orig.x(), orig.y());
        m_txAccess0->setTransform(m_txAccess0Orig, false);

        orig   = m_renderer->boundsOnElement("access1");
        Matrix = m_renderer->matrixForElement("access1");
        orig   = Matrix.mapRect(orig);
        m_txAccess1Orig.translate(orig.x(), orig.y());
        m_txAccess1->setTransform(m_txAccess1Orig, false);

        orig   = m_renderer->boundsOnElement("access2");
        Matrix = m_renderer->matrixForElement("access2");
        orig   = Matrix.mapRect(orig);
        m_txAccess2Orig.translate(orig.x(), orig.y());
        m_txAccess2->setTransform(m_txAccess2Orig, true);

        orig   = m_renderer->boundsOnElement("access3");
        Matrix = m_renderer->matrixForElement("access3");
        orig   = Matrix.mapRect(orig);
        m_txAccess3Orig.translate(orig.x(), orig.y());
        m_txAccess3->setTransform(m_txAccess3Orig, true);
    }
    wizardUi->graphicsView->fitInView(m_txMainBody, Qt::KeepAspectRatio);
    animate = new QTimer(this);
    connect(animate, SIGNAL(timeout()), this, SLOT(moveTxControls()));

    heliChannelOrder << ManualControlSettings::CHANNELGROUPS_COLLECTIVE <<
        ManualControlSettings::CHANNELGROUPS_THROTTLE <<
        ManualControlSettings::CHANNELGROUPS_ROLL <<
        ManualControlSettings::CHANNELGROUPS_PITCH <<
        ManualControlSettings::CHANNELGROUPS_YAW <<
        ManualControlSettings::CHANNELGROUPS_FLIGHTMODE <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY0 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY1 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY2 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY3;

    acroChannelOrder << ManualControlSettings::CHANNELGROUPS_THROTTLE <<
        ManualControlSettings::CHANNELGROUPS_ROLL <<
        ManualControlSettings::CHANNELGROUPS_PITCH <<
        ManualControlSettings::CHANNELGROUPS_YAW <<
        ManualControlSettings::CHANNELGROUPS_FLIGHTMODE <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY0 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY1 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY2 <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY3;

    groundChannelOrder << ManualControlSettings::CHANNELGROUPS_THROTTLE <<
        ManualControlSettings::CHANNELGROUPS_YAW <<
        ManualControlSettings::CHANNELGROUPS_FLIGHTMODE <<
        ManualControlSettings::CHANNELGROUPS_ACCESSORY0;
}

void ConfigInputWidget::buildOptionComboBox(QComboBox *combo, UAVObjectField *field, int index, bool applyLimits)
{
    if (combo == ui->failsafeFlightMode || combo == ui->failsafeBatteryCriticalFlightMode || combo == ui->failsafeBatteryWarningFlightMode) {
        for (quint32 i = 0; i < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM; i++) {
            combo->addItem(QString("Position %1").arg(i + 1), QVariant(i));
        }
    } else {
        ConfigTaskWidget::buildOptionComboBox(combo, field, index, applyLimits);
    }
}

void ConfigInputWidget::resetTxControls()
{
    m_txLeftStick->setTransform(m_txLeftStickOrig, false);
    m_txRightStick->setTransform(m_txRightStickOrig, false);
    m_txAccess0->setTransform(m_txAccess0Orig, false);
    m_txAccess1->setTransform(m_txAccess1Orig, false);
    m_txAccess2->setTransform(m_txAccess2Orig, false);
    m_txAccess3->setTransform(m_txAccess3Orig, false);
    m_txFlightMode->setElementId("flightModeCenter");
    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
    m_txArrows->setVisible(false);
    m_txFlightModeCountText->setText("?");
    m_txFlightModeCountText->setVisible(false);
    m_txFlightModeCountBG->setVisible(false);
}

ConfigInputWidget::~ConfigInputWidget()
{}

void ConfigInputWidget::enableControls(bool enable)
{
    ConfigTaskWidget::enableControls(enable);

    if (enable) {
        updatePositionSlider();
    } else {
        // Hide configAlarmStatus when disconnected
        ui->configAlarmStatus->setVisible(false);
        if (wizardStep != wizardNone) {
            // Close input wizard
            wzCancel();
        }
        if (ui->runCalibration->isChecked()) {
            // Close manual calibration
            ui->runCalibration->setChecked(false);
            ui->runCalibration->setText(tr("Start Manual Calibration"));
            emit inputCalibrationStateChanged(false);
        }
    }
}

void ConfigInputWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    wizardUi->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
}

void ConfigInputWidget::goToWizard()
{
    if (!outputConfigIsSafe) {
        QMessageBox::warning(this, tr("Warning"), tr("There is something wrong in <b>Output</b> tab."
                                                     "<p>Please fix the issue before starting the Transmitter wizard.</p>"), QMessageBox::Ok);
        return;
    }

    QMessageBox msgBox;

    msgBox.setText(tr("Arming Settings are now set to 'Always Disarmed' for your safety."));
    msgBox.setDetailedText(tr("You will have to reconfigure the arming settings manually "
                              "when the wizard is finished. After the last step of the "
                              "wizard you will be taken to the Arming Settings screen."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    // Tell Output tab we freeze actuators soon
    emit inputCalibrationStateChanged(true);

    // Set correct tab visible before starting wizard.
    if (ui->tabWidget->currentIndex() != 0) {
        ui->tabWidget->setCurrentIndex(0);
    }

    // Stash current manual settings data in case the wizard is
    // cancelled or the user proceeds far enough into the wizard such
    // that the UAVO is changed, but then backs out to the start and
    // chooses a different TX type (which could otherwise result in
    // unexpected TX channels being enabled)
    manualSettingsData             = manualSettingsObj->getData();
    memento.manualSettingsData     = manualSettingsData;
    flightModeSettingsData         = flightModeSettingsObj->getData();
    memento.flightModeSettingsData = flightModeSettingsData;
    flightModeSettingsData.Arming  = FlightModeSettings::ARMING_ALWAYSDISARMED;
    flightModeSettingsObj->setData(flightModeSettingsData);
    // Stash actuatorSettings
    actuatorSettingsData           = actuatorSettingsObj->getData();
    memento.actuatorSettingsData   = actuatorSettingsData;

    // Stash systemSettings
    systemSettingsData             = systemSettingsObj->getData();
    memento.systemSettingsData     = systemSettingsData;

    // Now reset channel and actuator settings (disable outputs)
    resetChannelSettings();
    resetActuatorSettings();

    // Use faster input update rate.
    fastMdata();

    // start the wizard
    wizardSetUpStep(wizardWelcome);
    wizardUi->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
}

void ConfigInputWidget::disableWizardButton(int value)
{
    if (value != 0) {
        ui->groupBox_3->setVisible(false);
    } else {
        ui->groupBox_3->setVisible(true);
    }
}

void ConfigInputWidget::wzCancel()
{
    dimOtherControls(false);

    // Cancel any ongoing delayd next trigger.
    wzNextDelayedCancel();

    // Restore original input update rate.
    restoreMdata();

    ui->stackedWidget->setCurrentIndex(0);

    if (wizardStep != wizardNone) {
        wizardTearDownStep(wizardStep);
    }
    wizardStep = wizardNone;
    ui->stackedWidget->setCurrentIndex(0);

    // Load settings back from beginning of wizard
    manualSettingsObj->setData(memento.manualSettingsData);
    flightModeSettingsObj->setData(memento.flightModeSettingsData);
    actuatorSettingsObj->setData(memento.actuatorSettingsData);
    systemSettingsObj->setData(memento.systemSettingsData);

    // Tell Output tab the calibration is ended
    emit inputCalibrationStateChanged(false);
}

void ConfigInputWidget::registerControlActivity()
{
    nextDelayedLatestActivityTick = nextDelayedTick;
}

void ConfigInputWidget::wzNextDelayed()
{
    nextDelayedTick++;

    // Call next after the full 2500 ms timeout has been reached,
    // or if no input activity has occurred the last 500 ms.
    if (nextDelayedTick == 25 ||
        nextDelayedTick - nextDelayedLatestActivityTick >= 5) {
        wzNext();
    }
}

void ConfigInputWidget::wzNextDelayedStart()
{
    // Call wzNextDelayed every 100 ms, to see if it's time to go to the next page.
    connect(&nextDelayedTimer, SIGNAL(timeout()), this, SLOT(wzNextDelayed()));
    nextDelayedTimer.start(100);
}

// Cancel the delayed next timer, if it's active.
void ConfigInputWidget::wzNextDelayedCancel()
{
    nextDelayedTick = 0;
    nextDelayedLatestActivityTick = 0;
    if (nextDelayedTimer.isActive()) {
        nextDelayedTimer.stop();
        disconnect(&nextDelayedTimer, SIGNAL(timeout()), this, SLOT(wzNextDelayed()));
    }
}

void ConfigInputWidget::wzNext()
{
    wzNextDelayedCancel();

    // In identify sticks mode the next button can indicate
    // channel advance
    if (wizardStep != wizardNone &&
        wizardStep != wizardIdentifySticks) {
        wizardTearDownStep(wizardStep);
    }

    // State transitions for next button
    switch (wizardStep) {
    case wizardWelcome:
        wizardSetUpStep(wizardChooseType);
        break;
    case wizardChooseType:
        wizardSetUpStep(wizardChooseMode);
        break;
    case wizardChooseMode:
        wizardSetUpStep(wizardIdentifySticks);
        break;
    case wizardIdentifySticks:
        nextChannel();
        if (currentChannelNum == -1) { // Gone through all channels
            wizardTearDownStep(wizardIdentifySticks);
            wizardSetUpStep(wizardIdentifyCenter);
        }
        break;
    case wizardIdentifyCenter:
        resetFlightModeSettings();
        wizardSetUpStep(wizardIdentifyLimits);
        break;
    case wizardIdentifyLimits:
        wizardSetUpStep(wizardIdentifyInverted);
        break;
    case wizardIdentifyInverted:
        wizardSetUpStep(wizardFinish);
        break;
    case wizardFinish:
        wizardStep = wizardNone;

        // Restore original input update rate.
        restoreMdata();

        // Load actuator settings back from beginning of wizard
        actuatorSettingsObj->setData(memento.actuatorSettingsData);

        // Force flight mode neutral to middle and Throttle neutral at 4%
        adjustSpecialNeutrals();
        throttleError = false;
        checkThrottleRange();

        manualSettingsObj->setData(manualSettingsData);

        // move to Arming Settings tab
        ui->stackedWidget->setCurrentIndex(0);
        ui->tabWidget->setCurrentIndex(3);

        // Tell Output tab the calibration is ended
        emit inputCalibrationStateChanged(false);
        break;
    default:
        Q_ASSERT(0);
    }
}

void ConfigInputWidget::wzBack()
{
    wzNextDelayedCancel();

    if (wizardStep != wizardNone &&
        wizardStep != wizardIdentifySticks) {
        wizardTearDownStep(wizardStep);
    }

    // State transitions for back button
    switch (wizardStep) {
    case wizardChooseType:
        wizardSetUpStep(wizardWelcome);
        break;
    case wizardChooseMode:
        wizardSetUpStep(wizardChooseType);
        break;
    case wizardIdentifySticks:
        prevChannel();
        if (currentChannelNum == -1) {
            wizardTearDownStep(wizardIdentifySticks);
            wizardSetUpStep(wizardChooseMode);
        }
        break;
    case wizardIdentifyCenter:
        wizardSetUpStep(wizardIdentifySticks);
        break;
    case wizardIdentifyLimits:
        wizardSetUpStep(wizardIdentifyCenter);
        break;
    case wizardIdentifyInverted:
        resetFlightModeSettings();
        wizardSetUpStep(wizardIdentifyLimits);
        break;
    case wizardFinish:
        wizardSetUpStep(wizardIdentifyInverted);
        break;
    default:
        Q_ASSERT(0);
    }
}

void ConfigInputWidget::wizardSetUpStep(enum wizardSteps step)
{
    wizardUi->wzNext->setText(tr("Next"));

    switch (step) {
    case wizardWelcome:
        foreach(QPointer<QWidget> wd, extraWidgets) {
            if (!wd.isNull()) {
                delete wd;
            }
        }
        extraWidgets.clear();
        wizardUi->graphicsView->setVisible(false);
        setTxMovement(nothing);
        wizardUi->wzBack->setEnabled(false);
        wizardUi->pagesStack->setCurrentWidget(wizardUi->welcomePage);
        ui->stackedWidget->setCurrentIndex(1);
        break;
    case wizardChooseType:
    {
        wizardUi->graphicsView->setVisible(true);
        wizardUi->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
        setTxMovement(nothing);
        wizardUi->wzBack->setEnabled(true);
        if (transmitterType == heli) {
            wizardUi->typeHeli->setChecked(true);
        } else if (transmitterType == ground) {
            wizardUi->typeGround->setChecked(true);
        } else {
            wizardUi->typeAcro->setChecked(true);
        }
        wizardUi->pagesStack->setCurrentWidget(wizardUi->chooseTypePage);
    }
    break;
    case wizardChooseMode:
    {
        wizardUi->wzBack->setEnabled(true);
        QRadioButton *modeButtons[] = {
            wizardUi->mode1Button,
            wizardUi->mode2Button,
            wizardUi->mode3Button,
            wizardUi->mode4Button
        };

        for (int i = 0; i <= mode4; ++i) {
            QString label;
            txMode mode = static_cast<txMode>(i);
            if (transmitterType == heli) {
                switch (mode) {
                case mode1: label = tr("Mode 1: Fore/Aft Cyclic and Yaw on the left, Throttle/Collective and Left/Right Cyclic on the right"); break;
                case mode2: label = tr("Mode 2: Throttle/Collective and Yaw on the left, Cyclic on the right"); break;
                case mode3: label = tr("Mode 3: Cyclic on the left, Throttle/Collective and Yaw on the right"); break;
                case mode4: label = tr("Mode 4: Throttle/Collective and Left/Right Cyclic on the left, Fore/Aft Cyclic and Yaw on the right"); break;
                default:    Q_ASSERT(0); break;
                }
                wizardUi->typePageFooter->setText(" ");
            } else {
                switch (mode) {
                case mode1: label = tr("Mode 1: Elevator and Rudder on the left, Throttle and Ailerons on the right"); break;
                case mode2: label = tr("Mode 2: Throttle and Rudder on the left, Elevator and Ailerons on the right"); break;
                case mode3: label = tr("Mode 3: Elevator and Ailerons on the left, Throttle and Rudder on the right"); break;
                case mode4: label = tr("Mode 4: Throttle and Ailerons on the left, Elevator and Rudder on the right"); break;
                default:    Q_ASSERT(0); break;
                }
                wizardUi->typePageFooter->setText(tr("For a Quad: Elevator is Pitch, Ailerons are Roll, and Rudder is Yaw."));
            }
            modeButtons[i]->setText(label);
            if (transmitterMode == mode) {
                modeButtons[i]->setChecked(true);
            }
        }
        wizardUi->pagesStack->setCurrentWidget(wizardUi->chooseModePage);
    }
    break;
    case wizardIdentifySticks:
        usedChannels.clear();
        currentChannelNum  = -1;
        nextChannel();
        manualSettingsData = manualSettingsObj->getData();
        connect(receiverActivityObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(identifyControls()));
        wizardUi->wzNext->setEnabled(false);
        wizardUi->pagesStack->setCurrentWidget(wizardUi->identifySticksPage);
        break;
    case wizardIdentifyCenter:
        setTxMovement(centerAll);
        wizardUi->pagesStack->setCurrentWidget(wizardUi->identifyCenterPage);
        if (transmitterType == ground) {
            wizardUi->identifyCenterInstructions->setText(QString(tr("Please center all controls and trims and press Next when ready.\n\n"
                                                                     "For a ground vehicle, this center position will be used as neutral value of each channel.")));
        }
        break;
    case wizardIdentifyLimits:
    {
        setTxMovement(nothing);
        manualSettingsData = manualSettingsObj->getData();
        for (uint i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; ++i) {
            // Preserve the inverted status
            if (manualSettingsData.ChannelMin[i] <= manualSettingsData.ChannelMax[i]) {
                manualSettingsData.ChannelMin[i] = manualSettingsData.ChannelNeutral[i];
                manualSettingsData.ChannelMax[i] = manualSettingsData.ChannelNeutral[i];
            } else {
                // Make this detect as still inverted
                manualSettingsData.ChannelMin[i] = manualSettingsData.ChannelNeutral[i] + 1;
                manualSettingsData.ChannelMax[i] = manualSettingsData.ChannelNeutral[i];
            }
        }
        UAVObjectUpdaterHelper updateHelper;
        manualSettingsObj->setData(manualSettingsData, false);
        updateHelper.doObjectAndWait(manualSettingsObj);

        connect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(identifyLimits()));
        connect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        connect(flightStatusObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        connect(accessoryDesiredObj0, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));

        wizardUi->pagesStack->setCurrentWidget(wizardUi->identifyLimitsPage);
    }
    break;
    case wizardIdentifyInverted:
        dimOtherControls(true);
        setTxMovement(nothing);
        extraWidgets.clear();
        for (int index = 0; index < manualSettingsObj->getField("ChannelMax")->getElementNames().length(); index++) {
            QString name = manualSettingsObj->getField("ChannelMax")->getElementNames().at(index);
            if (!name.contains("Access") && !name.contains("Flight") && !name.contains("Rssi") &&
                (!name.contains("Collective") || transmitterType == heli)) {
                QCheckBox *cb = new QCheckBox(name, this);
                // Make sure checked status matches current one
                cb->setChecked(manualSettingsData.ChannelMax[index] < manualSettingsData.ChannelMin[index]);
                wizardUi->checkBoxesLayout->addWidget(cb, extraWidgets.size() / 5, extraWidgets.size() % 5);
                extraWidgets.append(cb);
                connect(cb, SIGNAL(toggled(bool)), this, SLOT(invertControls()));
            }
        }
        connect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        wizardUi->pagesStack->setCurrentWidget(wizardUi->identifyInvertedPage);
        break;
    case wizardFinish:
        dimOtherControls(false);
        connect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        connect(flightStatusObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        connect(accessoryDesiredObj0, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        wizardUi->pagesStack->setCurrentWidget(wizardUi->finishPage);
        break;
    default:
        Q_ASSERT(0);
    }
    wizardStep = step;
}

void ConfigInputWidget::wizardTearDownStep(enum wizardSteps step)
{
    UAVObjectUpdaterHelper updateHelper;

    Q_ASSERT(step == wizardStep);
    switch (step) {
    case wizardWelcome:
        break;
    case wizardChooseType:
        if (wizardUi->typeAcro->isChecked()) {
            transmitterType = acro;
        } else if (wizardUi->typeGround->isChecked()) {
            transmitterType    = ground;

            systemSettingsData = systemSettingsObj->getData();
            /* Make sure to tell controller, this is really a ground vehicle. */
            if ((systemSettingsData.AirframeType != SystemSettings::AIRFRAMETYPE_GROUNDVEHICLECAR) ||
                (systemSettingsData.AirframeType != SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIAL) ||
                (systemSettingsData.AirframeType != SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEMOTORCYCLE) ||
                (systemSettingsData.AirframeType != SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEBOAT) ||
                (systemSettingsData.AirframeType != SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIALBOAT)) {
                // Apply default ground vehicle airframe
                systemSettingsData.AirframeType = SystemSettings::AIRFRAMETYPE_GROUNDVEHICLECAR;
                systemSettingsObj->setData(systemSettingsData);
            }
        } else {
            transmitterType = heli;
        }
        break;
    case wizardChooseMode:
    {
        QRadioButton *modeButtons[] = {
            wizardUi->mode1Button,
            wizardUi->mode2Button,
            wizardUi->mode3Button,
            wizardUi->mode4Button
        };
        for (int i = mode1; i <= mode4; ++i) {
            if (modeButtons[i]->isChecked()) {
                transmitterMode = static_cast<txMode>(i);
            }
        }
    }
    break;
    case wizardIdentifySticks:
        disconnect(receiverActivityObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(identifyControls()));
        wizardUi->wzNext->setEnabled(true);
        setTxMovement(nothing);
        break;
    case wizardIdentifyCenter:
        manualCommandData  = manualCommandObj->getData();
        manualSettingsData = manualSettingsObj->getData();
        // Ignore last Rssi channel
        for (unsigned int i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; ++i) {
            // Set Accessory neutral to middle range
            if (i >= ManualControlSettings::CHANNELNUMBER_ACCESSORY0) {
                manualSettingsData.ChannelNeutral[i] = manualSettingsData.ChannelMin[i] + ((manualSettingsData.ChannelMax[i] - manualSettingsData.ChannelMin[i]) / 2);
            } else {
                manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
            }
        }
        manualSettingsObj->setData(manualSettingsData, false);
        updateHelper.doObjectAndWait(manualSettingsObj);
        setTxMovement(nothing);
        break;
    case wizardIdentifyLimits:
        disconnect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(identifyLimits()));
        disconnect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        disconnect(flightStatusObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        disconnect(accessoryDesiredObj0, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        manualSettingsObj->setData(manualSettingsData, false);
        updateHelper.doObjectAndWait(manualSettingsObj);
        setTxMovement(nothing);
        break;
    case wizardIdentifyInverted:
        dimOtherControls(false);
        foreach(QWidget * wd, extraWidgets) {
            QCheckBox *cb = qobject_cast<QCheckBox *>(wd);

            if (cb) {
                disconnect(cb, SIGNAL(toggled(bool)), this, SLOT(invertControls()));
                delete cb;
            }
        }
        extraWidgets.clear();
        disconnect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        break;
    case wizardFinish:
        dimOtherControls(false);
        setTxMovement(nothing);
        disconnect(manualCommandObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        disconnect(flightStatusObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        disconnect(accessoryDesiredObj0, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(moveSticks()));
        break;
    default:
        Q_ASSERT(0);
    }
}

static void fastMdataSingle(UAVDataObject *object, UAVObject::Metadata *savedMdata)
{
    *savedMdata = object->getMetadata();
    UAVObject::Metadata mdata = *savedMdata;
    UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_PERIODIC);
    mdata.flightTelemetryUpdatePeriod = 150;
    object->setMetadata(mdata);
}

static void restoreMdataSingle(UAVDataObject *object, UAVObject::Metadata *savedMdata)
{
    object->setMetadata(*savedMdata);
}

/**
 * Set manual control command to fast updates
 */
void ConfigInputWidget::fastMdata()
{
    fastMdataSingle(manualCommandObj, &manualControlMdata);
    fastMdataSingle(accessoryDesiredObj0, &accessoryDesiredMdata0);
}

/**
 * Restore previous update settings for manual control data
 */
void ConfigInputWidget::restoreMdata()
{
    restoreMdataSingle(manualCommandObj, &manualControlMdata);
    restoreMdataSingle(accessoryDesiredObj0, &accessoryDesiredMdata0);
}

/**
 * Set the display to indicate which channel the person should move
 */
void ConfigInputWidget::setChannel(int newChan)
{
    bool canBeSkipped = false;

    if (newChan == ManualControlSettings::CHANNELGROUPS_COLLECTIVE) {
        wizardUi->identifyStickInstructions->setText(QString(tr("<p>Please enable throttle hold mode.</p>"
                                                                "<p>Move the Collective Pitch stick.</p>")));
    } else if (newChan == ManualControlSettings::CHANNELGROUPS_FLIGHTMODE) {
        wizardUi->identifyStickInstructions->setText(QString(tr("<p>Please toggle the Flight Mode switch.</p>"
                                                                "<p>For switches you may have to repeat this rapidly.</p>"
                                                                "<p>Alternatively, you can click Next to skip this channel, but you will get only <b>ONE</b> Flight Mode.</p>")));
        canBeSkipped = true;
    } else if ((transmitterType == heli) && (newChan == ManualControlSettings::CHANNELGROUPS_THROTTLE)) {
        wizardUi->identifyStickInstructions->setText(QString(tr("<p>Please disable throttle hold mode.</p>"
                                                                "<p>Move the Throttle stick.</p>")));
    } else {
        wizardUi->identifyStickInstructions->setText(QString(tr("<p>Please move each control one at a time according to the instructions and picture below.</p>"
                                                                "<p>Move the %1 stick.</p>")).arg(manualSettingsObj->getField("ChannelGroups")->getElementNames().at(newChan)));
    }

    if (manualSettingsObj->getField("ChannelGroups")->getElementNames().at(newChan).contains("Accessory")) {
        wizardUi->identifyStickInstructions->setText(wizardUi->identifyStickInstructions->text() + tr("<p>Alternatively, click Next to skip this channel.</p>"));
        canBeSkipped = true;
    }

    if (canBeSkipped) {
        wizardUi->wzNext->setEnabled(true);
        wizardUi->wzNext->setText(tr("Next / Skip"));
    } else {
        wizardUi->wzNext->setEnabled(false);
    }

    setMoveFromCommand(newChan);
    currentChannelNum = newChan;
    channelDetected   = false;
}

/**
 * Unfortunately order of channel should be different in different conditions.  Selects
 * next channel based on heli or acro mode
 */
void ConfigInputWidget::nextChannel()
{
    QList <int> order;
    switch (transmitterType) {
    case heli:
        order = heliChannelOrder;
        break;
    case ground:
        order = groundChannelOrder;
        break;
    default:
        order = acroChannelOrder;
        break;
    }

    if (currentChannelNum == -1) {
        setChannel(order[0]);
        return;
    }
    for (int i = 0; i < order.length() - 1; i++) {
        if (order[i] == currentChannelNum) {
            setChannel(order[i + 1]);
            return;
        }
    }
    currentChannelNum = -1; // hit end of list
}

/**
 * Unfortunately order of channel should be different in different conditions.  Selects
 * previous channel based on heli or acro mode
 */
void ConfigInputWidget::prevChannel()
{
    QList <int> order;
    switch (transmitterType) {
    case heli:
        order = heliChannelOrder;
        break;
    case ground:
        order = groundChannelOrder;
        break;
    default:
        order = acroChannelOrder;
        break;
    }

    // No previous from unset channel or next state
    if (currentChannelNum == -1) {
        return;
    }

    for (int i = 1; i < order.length(); i++) {
        if (order[i] == currentChannelNum) {
            if (!usedChannels.isEmpty() &&
                usedChannels.back().channelIndex == order[i - 1]) {
                usedChannels.removeLast();
            }
            setChannel(order[i - 1]);
            return;
        }
    }
    currentChannelNum = -1; // hit end of list
}

void ConfigInputWidget::identifyControls()
{
    static const int DEBOUNCE_COUNT = 4;
    static int debounce = 0;

    receiverActivityData = receiverActivityObj->getData();

    if (receiverActivityData.ActiveChannel == 255) {
        return;
    }

    if (channelDetected) {
        registerControlActivity();
        return;
    }

    receiverActivityData  = receiverActivityObj->getData();
    currentChannel.group  = receiverActivityData.ActiveGroup;
    currentChannel.number = receiverActivityData.ActiveChannel;

    if (debounce == 0) {
        // Register a channel to be debounced.
        lastChannel.group  = currentChannel.group;
        lastChannel.number = currentChannel.number;
        lastChannel.channelIndex = currentChannelNum;
        ++debounce;
        return;
    }

    if (currentChannel != lastChannel) {
        // A new channel was seen. Only register it if we count down to 0.
        --debounce;
        return;
    }

    if (debounce < DEBOUNCE_COUNT) {
        // We still haven't seen enough enough activity on this channel yet.
        ++debounce;
        return;
    }

    // Channel has been debounced and it's enough record it.

    if (usedChannels.contains(lastChannel)) {
        // Channel is already recorded.
        return;
    }

    // Record the channel.

    channelDetected    = true;
    debounce = 0;
    usedChannels.append(lastChannel);
    manualSettingsData = manualSettingsObj->getData();
    manualSettingsData.ChannelGroups[currentChannelNum] = currentChannel.group;
    manualSettingsData.ChannelNumber[currentChannelNum] = currentChannel.number;
    manualSettingsObj->setData(manualSettingsData);

    // m_config->wzText->clear();
    setTxMovement(nothing);

    wzNextDelayedStart();
}

void ConfigInputWidget::identifyLimits()
{
    uint16_t inputValue;
    static uint16_t previousInputFMValue;
    bool newLimitValue = false;
    bool newFlightModeValue = false;

    manualCommandData = manualCommandObj->getData();
    for (uint i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; ++i) {
        inputValue = manualCommandData.Channel[i];
        // Check if channel is already detected and prevent glitches
        if ((manualSettingsData.ChannelNumber[i] != CHANNEL_NUMBER_NONE) &&
            (inputValue > MIN_INPUT_US) && (inputValue < MAX_INPUT_US)) {
            if (manualSettingsData.ChannelMin[i] <= manualSettingsData.ChannelMax[i]) {
                // Non inverted channel
                if (manualSettingsData.ChannelMin[i] > inputValue) {
                    manualSettingsData.ChannelMin[i] = inputValue;
                    newLimitValue = true;
                }
                if (manualSettingsData.ChannelMax[i] < inputValue) {
                    manualSettingsData.ChannelMax[i] = inputValue;
                    newLimitValue = true;
                }
            } else {
                // Inverted channel
                if (manualSettingsData.ChannelMax[i] > inputValue) {
                    manualSettingsData.ChannelMax[i] = inputValue;
                    newLimitValue = true;
                }
                if (manualSettingsData.ChannelMin[i] < inputValue) {
                    manualSettingsData.ChannelMin[i] = inputValue;
                    newLimitValue = true;
                }
            }
            // Flightmode channel
            if (i == ManualControlSettings::CHANNELGROUPS_FLIGHTMODE) {
                int deltaInput = abs(previousInputFMValue - inputValue);
                previousInputFMValue = inputValue;
                // Expecting two consecutive readings within a close value
                if (deltaInput < 5) {
                    // Avoid duplicate values too close and error due to RcTx drift
                    int minSpacing = 100; // 100µs
                    for (int pos = 0; pos < manualSettingsData.FlightModeNumber + 1; ++pos) {
                        if (flightModeSignalValue[pos] == 0) {
                            newFlightModeValue = true;
                            // A new flightmode value can be set now
                            for (int checkpos = 0; checkpos < manualSettingsData.FlightModeNumber + 1; ++checkpos) {
                                // Check if value is already used, MinSpacing needed between values.
                                if ((flightModeSignalValue[checkpos] < inputValue + minSpacing) &&
                                    (flightModeSignalValue[checkpos] > inputValue - minSpacing)) {
                                    newFlightModeValue = false;
                                }
                            }
                            // Be sure FlightModeNumber is < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM (6)
                            if ((manualSettingsData.FlightModeNumber < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM) && newFlightModeValue) {
                                // Start from 0, erase previous count
                                if (pos == 0) {
                                    manualSettingsData.FlightModeNumber = 0;
                                }
                                // Store new value and increase FlightModeNumber
                                flightModeSignalValue[pos] = inputValue;
                                manualSettingsData.FlightModeNumber++;
                                // Show flight mode number
                                m_txFlightModeCountText->setText(QString().number(manualSettingsData.FlightModeNumber));
                                m_txFlightModeCountText->setVisible(true);
                                m_txFlightModeCountBG->setVisible(true);
                            }
                        }
                    }
                }
            }
        }
    }
    // Save only if something changed
    if (newLimitValue || newFlightModeValue) {
        UAVObjectUpdaterHelper updateHelper;
        manualSettingsObj->setData(manualSettingsData, false);
        updateHelper.doObjectAndWait(manualSettingsObj);
    }
}

void ConfigInputWidget::setMoveFromCommand(int command)
{
    // ManualControlSettings::ChannelNumberElem:
    // CHANNELNUMBER_ROLL=0,
    // CHANNELNUMBER_PITCH=1,
    // CHANNELNUMBER_YAW=2,
    // CHANNELNUMBER_THROTTLE=3,
    // CHANNELNUMBER_FLIGHTMODE=4,
    // CHANNELNUMBER_ACCESSORY0=5,
    // CHANNELNUMBER_ACCESSORY1=6,
    // CHANNELNUMBER_ACCESSORY2=7

    txMovements movement = moveLeftVerticalStick;

    switch (command) {
    case ManualControlSettings::CHANNELNUMBER_ROLL:
        movement = ((transmitterMode == mode3 || transmitterMode == mode4) ?
                    moveLeftHorizontalStick : moveRightHorizontalStick);
        break;
    case ManualControlSettings::CHANNELNUMBER_PITCH:
        movement = (transmitterMode == mode1 || transmitterMode == mode3) ?
                   moveLeftVerticalStick : moveRightVerticalStick;
        break;
    case ManualControlSettings::CHANNELNUMBER_YAW:
        movement = ((transmitterMode == mode1 || transmitterMode == mode2) ?
                    moveLeftHorizontalStick : moveRightHorizontalStick);
        break;
    case ManualControlSettings::CHANNELNUMBER_THROTTLE:
        movement = (transmitterMode == mode2 || transmitterMode == mode4) ?
                   moveLeftVerticalStick : moveRightVerticalStick;
        break;
    case ManualControlSettings::CHANNELNUMBER_COLLECTIVE:
        movement = (transmitterMode == mode2 || transmitterMode == mode4) ?
                   moveLeftVerticalStick : moveRightVerticalStick;
        break;
    case ManualControlSettings::CHANNELNUMBER_FLIGHTMODE:
        movement = moveFlightMode;
        break;
    case ManualControlSettings::CHANNELNUMBER_ACCESSORY0:
        movement = moveAccess0;
        break;
    case ManualControlSettings::CHANNELNUMBER_ACCESSORY1:
        movement = moveAccess1;
        break;
    case ManualControlSettings::CHANNELNUMBER_ACCESSORY2:
        movement = moveAccess2;
        break;
    case ManualControlSettings::CHANNELNUMBER_ACCESSORY3:
        movement = moveAccess3;
        break;
    default:
        Q_ASSERT(0);
        break;
    }
    setTxMovement(movement);
}

void ConfigInputWidget::setTxMovement(txMovements movement)
{
    resetTxControls();
    switch (movement) {
    case moveLeftVerticalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveLeftVerticalStick;
        animate->start(100);
        break;
    case moveRightVerticalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveRightVerticalStick;
        animate->start(100);
        break;
    case moveLeftHorizontalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveLeftHorizontalStick;
        animate->start(100);
        break;
    case moveRightHorizontalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveRightHorizontalStick;
        animate->start(100);
        break;
    case moveAccess0:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess0;
        animate->start(100);
        break;
    case moveAccess1:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess1;
        animate->start(100);
        break;
    case moveAccess2:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess2;
        animate->start(100);
        break;
    case moveAccess3:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess3;
        animate->start(100);
        break;
    case moveFlightMode:
        movePos = 0;
        growing = true;
        currentMovement = moveFlightMode;
        animate->start(1000);
        break;
    case centerAll:
        movePos = 0;
        currentMovement = centerAll;
        animate->start(1000);
        break;
    case moveAll:
        movePos = 0;
        growing = true;
        currentMovement = moveAll;
        animate->start(50);
        break;
    case nothing:
        movePos = 0;
        animate->stop();
        break;
    default:
        Q_ASSERT(0);
        break;
    }
}

void ConfigInputWidget::moveTxControls()
{
    QTransform trans;
    QGraphicsItem *item = NULL;
    txMovementType move = vertical;
    int limitMax = 0;
    int limitMin = 0;
    static bool auxFlag = false;

    switch (currentMovement) {
    case moveLeftVerticalStick:
        item     = m_txLeftStick;
        trans    = m_txLeftStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move     = vertical;
        break;
    case moveRightVerticalStick:
        item     = m_txRightStick;
        trans    = m_txRightStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move     = vertical;
        break;
    case moveLeftHorizontalStick:
        item     = m_txLeftStick;
        trans    = m_txLeftStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move     = horizontal;
        break;
    case moveRightHorizontalStick:
        item     = m_txRightStick;
        trans    = m_txRightStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move     = horizontal;
        break;
    case moveAccess0:
        item     = m_txAccess0;
        trans    = m_txAccess0Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move     = horizontal;
        break;
    case moveAccess1:
        item     = m_txAccess1;
        trans    = m_txAccess1Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move     = horizontal;
        break;
    case moveAccess2:
        item     = m_txAccess2;
        trans    = m_txAccess2Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move     = horizontal;
        break;
    case moveAccess3:
        item     = m_txAccess3;
        trans    = m_txAccess3Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move     = horizontal;
        break;
    case moveFlightMode:
        item     = m_txFlightMode;
        move     = jump;
        break;
    case centerAll:
        item     = m_txArrows;
        move     = jump;
        break;
    case moveAll:
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move     = mix;
        break;
    default:
        break;
    }
    if (move == vertical) {
        item->setTransform(trans.translate(0, movePos * 10), false);
    } else if (move == horizontal) {
        item->setTransform(trans.translate(movePos * 10, 0), false);
    } else if (move == jump) {
        if (item == m_txArrows) {
            m_txArrows->setVisible(!m_txArrows->isVisible());
        } else if (item == m_txFlightMode) {
            QGraphicsSvgItem *svg;
            svg = (QGraphicsSvgItem *)item;
            if (svg) {
                if (svg->elementId() == "flightModeCenter") {
                    if (growing) {
                        svg->setElementId("flightModeRight");
                        m_txFlightMode->setTransform(m_txFlightModeROrig, false);
                    } else {
                        svg->setElementId("flightModeLeft");
                        m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
                    }
                } else if (svg->elementId() == "flightModeRight") {
                    growing = false;
                    svg->setElementId("flightModeCenter");
                    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
                } else if (svg->elementId() == "flightModeLeft") {
                    growing = true;
                    svg->setElementId("flightModeCenter");
                    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
                }
            }
        }
    } else if (move == mix) {
        trans = m_txAccess0Orig;
        m_txAccess0->setTransform(trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);
        trans = m_txAccess1Orig;
        m_txAccess1->setTransform(trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);
        trans = m_txAccess2Orig;
        m_txAccess2->setTransform(trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);
        trans = m_txAccess3Orig;
        m_txAccess3->setTransform(trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);

        if (auxFlag) {
            trans = m_txLeftStickOrig;
            m_txLeftStick->setTransform(trans.translate(0, movePos * 10), false);
            trans = m_txRightStickOrig;
            m_txRightStick->setTransform(trans.translate(0, movePos * 10), false);
        } else {
            trans = m_txLeftStickOrig;
            m_txLeftStick->setTransform(trans.translate(movePos * 10, 0), false);
            trans = m_txRightStickOrig;
            m_txRightStick->setTransform(trans.translate(movePos * 10, 0), false);
        }

        if (movePos == 0) {
            m_txFlightMode->setElementId("flightModeCenter");
            m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
        } else if (movePos == ACCESS_MAX_MOVE / 2) {
            m_txFlightMode->setElementId("flightModeRight");
            m_txFlightMode->setTransform(m_txFlightModeROrig, false);
        } else if (movePos == ACCESS_MIN_MOVE / 2) {
            m_txFlightMode->setElementId("flightModeLeft");
            m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
        }
    }
    if (move == horizontal || move == vertical || move == mix) {
        if (movePos == 0 && growing) {
            auxFlag = !auxFlag;
        }
        if (growing) {
            ++movePos;
        } else {
            --movePos;
        }
        if (movePos > limitMax) {
            movePos = movePos - 2;
            growing = false;
        }
        if (movePos < limitMin) {
            movePos = movePos + 2;
            growing = true;
        }
    }
}

AccessoryDesired *ConfigInputWidget::getAccessoryDesiredInstance(int instance)
{
    switch (instance) {
    case 0:
        if (accessoryDesiredObj0 == NULL) {
            accessoryDesiredObj0 = AccessoryDesired::GetInstance(getObjectManager(), 0);
        }
        return accessoryDesiredObj0;

    case 1:
        if (accessoryDesiredObj1 == NULL) {
            accessoryDesiredObj1 = AccessoryDesired::GetInstance(getObjectManager(), 1);
        }
        return accessoryDesiredObj1;

    case 2:
        if (accessoryDesiredObj2 == NULL) {
            accessoryDesiredObj2 = AccessoryDesired::GetInstance(getObjectManager(), 2);
        }
        return accessoryDesiredObj2;

    case 3:
        if (accessoryDesiredObj3 == NULL) {
            accessoryDesiredObj3 = AccessoryDesired::GetInstance(getObjectManager(), 3);
        }
        return accessoryDesiredObj3;

    default:
        Q_ASSERT(false);
    }

    return NULL;
}

float ConfigInputWidget::getAccessoryDesiredValue(int instance)
{
    AccessoryDesired *accessoryDesiredObj = getAccessoryDesiredInstance(instance);

    if (accessoryDesiredObj == NULL) {
        Q_ASSERT(false);
        return 0.0f;
    }

    AccessoryDesired::DataFields data = accessoryDesiredObj->getData();

    return data.AccessoryVal;
}

void ConfigInputWidget::moveSticks()
{
    QTransform trans;

    manualCommandData = manualCommandObj->getData();
    flightStatusData  = flightStatusObj->getData();

    switch (transmitterMode) {
    case mode1:
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10, manualCommandData.Pitch * STICK_MAX_MOVE * 10), false);
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10, -manualCommandData.Throttle * STICK_MAX_MOVE * 10), false);
        break;
    case mode2:
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10, -manualCommandData.Throttle * STICK_MAX_MOVE * 10), false);
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10, manualCommandData.Pitch * STICK_MAX_MOVE * 10), false);
        break;
    case mode3:
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10, manualCommandData.Pitch * STICK_MAX_MOVE * 10), false);
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10, -manualCommandData.Throttle * STICK_MAX_MOVE * 10), false);
        break;
    case mode4:
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10, -manualCommandData.Throttle * STICK_MAX_MOVE * 10), false);
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10, manualCommandData.Pitch * STICK_MAX_MOVE * 10), false);
        break;
    default:
        Q_ASSERT(0);
        break;
    }
    if ((flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[0]) ||
        (flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[5])) {
        m_txFlightMode->setElementId("flightModeLeft");
        m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
    } else if ((flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[1]) ||
               (flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[4])) {
        m_txFlightMode->setElementId("flightModeCenter");
        m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
    } else if ((flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[2]) ||
               (flightStatusData.FlightMode == flightModeSettingsData.FlightModePosition[3])) {
        m_txFlightMode->setElementId("flightModeRight");
        m_txFlightMode->setTransform(m_txFlightModeROrig, false);
    }

    m_txAccess0->setTransform(QTransform(m_txAccess0Orig).translate(getAccessoryDesiredValue(0) * ACCESS_MAX_MOVE * 10, 0), false);
    m_txAccess1->setTransform(QTransform(m_txAccess1Orig).translate(getAccessoryDesiredValue(1) * ACCESS_MAX_MOVE * 10, 0), false);
    m_txAccess2->setTransform(QTransform(m_txAccess2Orig).translate(getAccessoryDesiredValue(2) * ACCESS_MAX_MOVE * 10, 0), false);
    m_txAccess3->setTransform(QTransform(m_txAccess3Orig).translate(getAccessoryDesiredValue(3) * ACCESS_MAX_MOVE * 10, 0), false);
}

void ConfigInputWidget::dimOtherControls(bool value)
{
    qreal opac;

    if (value) {
        opac = 0.1;
    } else {
        opac = 1;
    }
    m_txAccess0->setOpacity(opac);
    m_txAccess1->setOpacity(opac);
    m_txAccess2->setOpacity(opac);
    m_txAccess3->setOpacity(opac);
    m_txFlightMode->setOpacity(opac);
}

void ConfigInputWidget::invertControls()
{
    manualSettingsData = manualSettingsObj->getData();
    foreach(QWidget * wd, extraWidgets) {
        QCheckBox *cb = qobject_cast<QCheckBox *>(wd);

        if (cb) {
            int index = manualSettingsObj->getField("ChannelNumber")->getElementNames().indexOf(cb->text());
            if ((cb->isChecked() && (manualSettingsData.ChannelMax[index] > manualSettingsData.ChannelMin[index])) ||
                (!cb->isChecked() && (manualSettingsData.ChannelMax[index] < manualSettingsData.ChannelMin[index]))) {
                qint16 aux;
                aux = manualSettingsData.ChannelMax[index];
                manualSettingsData.ChannelMax[index] = manualSettingsData.ChannelMin[index];
                manualSettingsData.ChannelMin[index] = aux;
            }
        }
    }
    UAVObjectUpdaterHelper updateHelper;
    manualSettingsObj->setData(manualSettingsData, false);
    updateHelper.doObjectAndWait(manualSettingsObj);
}

void ConfigInputWidget::moveFMSlider()
{
    ManualControlSettings::DataFields manualSettingsDataPriv = manualSettingsObj->getData();
    ManualControlCommand::DataFields manualCommandDataPriv   = manualCommandObj->getData();

    float valueScaled;
    int chMin     = manualSettingsDataPriv.ChannelMin[ManualControlSettings::CHANNELMIN_FLIGHTMODE];
    int chMax     = manualSettingsDataPriv.ChannelMax[ManualControlSettings::CHANNELMAX_FLIGHTMODE];
    int chNeutral = manualSettingsDataPriv.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_FLIGHTMODE];

    int value     = manualCommandDataPriv.Channel[ManualControlSettings::CHANNELMIN_FLIGHTMODE];

    if ((chMax > chMin && value >= chNeutral) || (chMin > chMax && value <= chNeutral)) {
        if (chMax != chNeutral) {
            valueScaled = (float)(value - chNeutral) / (float)(chMax - chNeutral);
        } else {
            valueScaled = 0;
        }
    } else {
        if (chMin != chNeutral) {
            valueScaled = (float)(value - chNeutral) / (float)(chNeutral - chMin);
        } else {
            valueScaled = 0;
        }
    }

    // Bound and scale FlightMode from [-1..+1] to [0..1] range
    if (valueScaled < -1.0) {
        valueScaled = -1.0;
    } else if (valueScaled > 1.0) {
        valueScaled = 1.0;
    }

    // Convert flightMode value into the switch position in the range [0..N-1]
    // This uses the same optimized computation as flight code to be consistent
    uint8_t pos = ((int16_t)(valueScaled * 256) + 256) * manualSettingsDataPriv.FlightModeNumber >> 9;
    if (pos >= manualSettingsDataPriv.FlightModeNumber) {
        pos = manualSettingsDataPriv.FlightModeNumber - 1;
    }
    ui->fmsSlider->setValue(pos);
    highlightStabilizationMode(pos);
}

void ConfigInputWidget::highlightStabilizationMode(int pos)
{
    QComboBox *comboboxFm    = this->findChild<QComboBox *>("fmsModePos" + QString::number(pos + 1));
    QString customStyleSheet = "QComboBox:editable:!on{background: #feb103;}";

    if (comboboxFm) {
        QString flightModeText = comboboxFm->currentText();
        comboboxFm->setStyleSheet("");
        for (uint8_t i = 0; i < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM; i++) {
            QLabel *label = this->findChild<QLabel *>("stab" + QString::number(i + 1) + "_label");
            QComboBox *comboRoll   = this->findChild<QComboBox *>("fmsSsPos" + QString::number(i + 1) + "Roll");
            QComboBox *comboPitch  = this->findChild<QComboBox *>("fmsSsPos" + QString::number(i + 1) + "Pitch");
            QComboBox *comboYaw    = this->findChild<QComboBox *>("fmsSsPos" + QString::number(i + 1) + "Yaw");
            QComboBox *comboThrust = this->findChild<QComboBox *>("fmsSsPos" + QString::number(i + 1) + "Thrust");
            QComboBox *comboboxFm2 = this->findChild<QComboBox *>("fmsModePos" + QString::number(i + 1));
            comboboxFm2->setStyleSheet("");

            // Highlight current stabilization mode if any.
            if ((flightModeText.contains("Stabilized", Qt::CaseInsensitive)) && (flightModeText.contains(QString::number(i + 1), Qt::CaseInsensitive))) {
                label->setStyleSheet("border-radius: 4px; border:3px solid #feb103;");
                comboRoll->setStyleSheet(customStyleSheet);
                comboPitch->setStyleSheet(customStyleSheet);
                comboYaw->setStyleSheet(customStyleSheet);
                comboThrust->setStyleSheet(customStyleSheet);
            } else {
                label->setStyleSheet("");
                comboRoll->setStyleSheet("");
                comboPitch->setStyleSheet("");
                comboYaw->setStyleSheet("");
                comboThrust->setStyleSheet("");
                if (!flightModeText.contains("Stabilized", Qt::CaseInsensitive)) {
                    // Highlight PosHold, Return to Base, ... flightmodes
                    comboboxFm->setStyleSheet(customStyleSheet);
                }
            }
        }
    }
}

void setComboBoxItemEnabled(QComboBox *combo, int index, bool enabled = true)
{
    combo->setItemData(index, enabled ? QVariant(1 | 32) : QVariant(0), Qt::UserRole - 1);
}

void ConfigInputWidget::updatePositionSlider()
{
    ManualControlSettings::DataFields manualSettingsDataPriv = manualSettingsObj->getData();

    QWidget *fmsModes[] = {
        ui->fmsModePos1,
        ui->fmsModePos2,
        ui->fmsModePos3,
        ui->fmsModePos4,
        ui->fmsModePos5,
        ui->fmsModePos6
    };
    QWidget *pidBanks[] = {
        ui->pidBankSs1_0,
        ui->pidBankSs1_1,
        ui->pidBankSs1_2,
        ui->pidBankSs1_3,
        ui->pidBankSs1_4,
        ui->pidBankSs1_5
    };
    QWidget *assistControls[] = {
        ui->assistControlPos1,
        ui->assistControlPos2,
        ui->assistControlPos3,
        ui->assistControlPos4,
        ui->assistControlPos5,
        ui->assistControlPos6
    };

    for (quint32 i = 0; i < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM; i++) {
        bool enabled = i < manualSettingsDataPriv.FlightModeNumber;

        fmsModes[i]->setEnabled(enabled);
        pidBanks[i]->setEnabled(enabled);
        assistControls[i]->setEnabled(enabled);
        setComboBoxItemEnabled(ui->failsafeFlightMode, i, enabled);
        setComboBoxItemEnabled(ui->failsafeBatteryCriticalFlightMode, i, enabled);
        setComboBoxItemEnabled(ui->failsafeBatteryWarningFlightMode, i, enabled);
    }

    QString fmNumber = QString().setNum(manualSettingsDataPriv.FlightModeNumber);
    int count = 0;
    foreach(QSlider * sp, findChildren<QSlider *>()) {
        // Find FlightMode slider and apply stylesheet
        if (sp->objectName() == "channelNeutral") {
            if (count == 4) {
                sp->setStyleSheet(
                    "QSlider::groove:horizontal {border: 2px solid rgb(196, 196, 196); margin: 0px 23px 0px 23px; height: 12px; border-radius: 5px; "
                    "border-image:url(:/configgadget/images/flightmode_bg" + fmNumber + ".png); }"
                    "QSlider::add-page:horizontal { background: none; border: none; }"
                    "QSlider::sub-page:horizontal { background: none; border: none; }"
                    "QSlider::handle:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                    "stop: 0 rgba(196, 196, 196, 180), stop: 0.45 rgba(196, 196, 196, 180), "
                    "stop: 0.46 rgba(255,0,0,100), stop: 0.54 rgba(255,0,0,100), "
                    "stop: 0.55 rgba(196, 196, 196, 180), stop: 1 rgba(196, 196, 196, 180)); "
                    "width: 46px; height: 28px; margin: -6px -23px -6px -23px; border-radius: 5px; border: 1px solid #777; }");
                count++;
            } else {
                count++;
            }
        }
    }
}

void ConfigInputWidget::updateConfigAlarmStatus()
{
    SystemAlarms *systemAlarmsObj = SystemAlarms::GetInstance(getObjectManager());
    SystemAlarms::DataFields systemAlarms = systemAlarmsObj->getData();

    QString message = tr("Config OK");
    QString tooltipMessage = tr("All fine, no config alarm!");
    QString bgColor = "green";

    if (systemAlarms.Alarm[SystemAlarms::ALARM_SYSTEMCONFIGURATION] > SystemAlarms::ALARM_WARNING) {
        switch (systemAlarms.ExtendedAlarmStatus[SystemAlarms::EXTENDEDALARMSTATUS_SYSTEMCONFIGURATION]) {
        case SystemAlarms::EXTENDEDALARMSTATUS_FLIGHTMODE:
            message = tr("Config error");
            tooltipMessage = tr("There is something wrong in the current config,\nusually a Thrust mode or Assisted mode not supported.\n\n"
                                "Tip: Reduce the Flight Mode Count to find the culprit.");
            bgColor = "red";
        }
    }
    ui->configAlarmStatus->setVisible(true);
    ui->configAlarmStatus->setStyleSheet(
        "QLabel { background-color: " + bgColor + ";"
        "color: rgb(255, 255, 255); border-radius: 5; margin:1px; font:bold; }");
    ui->configAlarmStatus->setText(message);
    ui->configAlarmStatus->setToolTip(tooltipMessage);
}

void ConfigInputWidget::updateCalibration()
{
    manualCommandData = manualCommandObj->getData();
    for (uint i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; ++i) {
        if ((!reverse[i] && manualSettingsData.ChannelMin[i] > manualCommandData.Channel[i]) ||
            (reverse[i] && manualSettingsData.ChannelMin[i] < manualCommandData.Channel[i])) {
            manualSettingsData.ChannelMin[i] = manualCommandData.Channel[i];
        }
        if ((!reverse[i] && manualSettingsData.ChannelMax[i] < manualCommandData.Channel[i]) ||
            (reverse[i] && manualSettingsData.ChannelMax[i] > manualCommandData.Channel[i])) {
            manualSettingsData.ChannelMax[i] = manualCommandData.Channel[i];
        }
        if ((i == ManualControlSettings::CHANNELNUMBER_FLIGHTMODE) || (i == ManualControlSettings::CHANNELNUMBER_THROTTLE)) {
            adjustSpecialNeutrals();
        } else {
            // Set Accessory neutral to middle range
            if (i >= ManualControlSettings::CHANNELNUMBER_ACCESSORY0) {
                manualSettingsData.ChannelNeutral[i] = manualSettingsData.ChannelMin[i] + ((manualSettingsData.ChannelMax[i] - manualSettingsData.ChannelMin[i]) / 2);
            } else {
                manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
            }
        }
    }
    UAVObjectUpdaterHelper updateHelper;
    manualSettingsObj->setData(manualSettingsData, false);
    updateHelper.doObjectAndWait(manualSettingsObj);
    manualSettingsObj->updated();
}

void ConfigInputWidget::simpleCalibration(bool enable)
{
    if (!isConnected()) {
        return;
    }

    if (!outputConfigIsSafe) {
        if (enable) {
            QMessageBox::warning(this, tr("Warning"), tr("There is something wrong in <b>Output</b> tab."
                                                         "<p>Please fix the issue before starting the Manual Calibration.</p>"), QMessageBox::Ok);
            ui->runCalibration->setChecked(false);
        }
        return;
    }

    if (enable) {
        ui->configurationWizard->setEnabled(false);
        ui->applyButton->setEnabled(false);
        ui->saveButton->setEnabled(false);
        ui->runCalibration->setText(tr("Stop Manual Calibration"));
        throttleError = false;

        QMessageBox msgBox;
        msgBox.setText(tr("<p>Arming Settings are now set to 'Always Disarmed' for your safety.</p>"
                          "<p>Be sure your receiver is powered with an external source and Transmitter is on.</p>"
                          "<p align='center'><b>Stop Manual Calibration</b> when done</p>"));
        msgBox.setDetailedText(tr("You will have to reconfigure the arming settings manually when the manual calibration is finished."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

        // Tell Output tab we freeze actuators soon
        emit inputCalibrationStateChanged(true);

        manualCommandData      = manualCommandObj->getData();

        manualSettingsData     = manualSettingsObj->getData();
        flightModeSettingsData = flightModeSettingsObj->getData();
        flightModeSettingsData.Arming = FlightModeSettings::ARMING_ALWAYSDISARMED;
        flightModeSettingsObj->setData(flightModeSettingsData);
        // Ignore last Rssi channel
        for (unsigned int i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; i++) {
            reverse[i] = manualSettingsData.ChannelMax[i] < manualSettingsData.ChannelMin[i];
            manualSettingsData.ChannelMin[i]     = manualCommandData.Channel[i];
            manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
            manualSettingsData.ChannelMax[i]     = manualCommandData.Channel[i];
        }

        fastMdataSingle(manualCommandObj, &manualControlMdata);

        // Stash actuatorSettings
        actuatorSettingsData = actuatorSettingsObj->getData();
        memento.actuatorSettingsData = actuatorSettingsData;

        // Disable all actuators
        resetActuatorSettings();

        connect(manualCommandObj, SIGNAL(objectUnpacked(UAVObject *)), this, SLOT(updateCalibration()));
    } else {
        manualCommandData  = manualCommandObj->getData();
        manualSettingsData = manualSettingsObj->getData();
        systemSettingsData = systemSettingsObj->getData();

        if ((systemSettingsData.AirframeType == SystemSettings::AIRFRAMETYPE_GROUNDVEHICLECAR) ||
            (systemSettingsData.AirframeType == SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIAL) ||
            (systemSettingsData.AirframeType == SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEMOTORCYCLE) ||
            (systemSettingsData.AirframeType == SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEBOAT) ||
            (systemSettingsData.AirframeType == SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIALBOAT)) {
            QMessageBox::warning(this, tr("Ground Vehicle"),
                                 tr("<p>Please <b>center</b> throttle control and press OK when ready.</p>"));

            transmitterType = ground;
            manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] =
                manualCommandData.Channel[ManualControlSettings::CHANNELNUMBER_THROTTLE];
        }

        restoreMdataSingle(manualCommandObj, &manualControlMdata);

        for (unsigned int i = 0; i < ManualControlSettings::CHANNELNUMBER_RSSI; i++) {
            if ((i == ManualControlSettings::CHANNELNUMBER_FLIGHTMODE) || (i == ManualControlSettings::CHANNELNUMBER_THROTTLE)) {
                adjustSpecialNeutrals();
                checkThrottleRange();
            } else {
                // Set Accessory neutral to middle range
                if (i >= ManualControlSettings::CHANNELNUMBER_ACCESSORY0) {
                    manualSettingsData.ChannelNeutral[i] = manualSettingsData.ChannelMin[i] + ((manualSettingsData.ChannelMax[i] - manualSettingsData.ChannelMin[i]) / 2);
                } else {
                    manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
                }
            }
        }
        manualSettingsObj->setData(manualSettingsData);

        // Load actuator settings back from beginning of manual calibration
        actuatorSettingsObj->setData(memento.actuatorSettingsData);

        ui->configurationWizard->setEnabled(true);
        ui->applyButton->setEnabled(true);
        ui->saveButton->setEnabled(true);
        ui->runCalibration->setText(tr("Start Manual Calibration"));

        // Tell Output tab the calibration is ended
        emit inputCalibrationStateChanged(false);

        disconnect(manualCommandObj, SIGNAL(objectUnpacked(UAVObject *)), this, SLOT(updateCalibration()));
    }
}

void ConfigInputWidget::adjustSpecialNeutrals()
{
    // FlightMode and Throttle need special neutral settings
    //
    // Force flight mode neutral to middle
    manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_FLIGHTMODE] =
        (manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_FLIGHTMODE] +
         manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_FLIGHTMODE]) / 2;

    // A ground vehicle has a reversible motor, the center position of throttle is the neutral setting.
    // So do not have to set a special neutral value for it.
    if (transmitterType == ground) {
        return;
    }

    // Force throttle to be near min, add 4% from total range to avoid arming issues
    manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] =
        manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE] +
        ((manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_THROTTLE] -
          manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE]) * 0.04);
}

void ConfigInputWidget::checkThrottleRange()
{
    int throttleRange = abs(manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_THROTTLE] -
                            manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE]);

    if (!throttleError && (throttleRange < 300)) {
        throttleError = true;
        QMessageBox::warning(this, tr("Warning"), tr("<p>There is something wrong with Throttle range. Please redo calibration and move <b>ALL sticks</b>, Throttle stick included.</p>"), QMessageBox::Ok);

        // Set Throttle neutral to max value so Throttle can't be positive
        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] =
            manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_THROTTLE];
    }
}

bool ConfigInputWidget::shouldObjectBeSaved(UAVObject *object)
{
    // ManualControlCommand no need to be saved
    return dynamic_cast<ManualControlCommand *>(object) == NULL;
}

void ConfigInputWidget::resetChannelSettings()
{
    manualSettingsData = manualSettingsObj->getData();
    // Clear all channel data : Channel Type (PPM,PWM..) and Number, except for Rssi
    for (unsigned int channel = 0; channel < ManualControlSettings::CHANNELNUMBER_RSSI; channel++) {
        manualSettingsData.ChannelGroups[channel] = ManualControlSettings::CHANNELGROUPS_NONE;
        manualSettingsData.ChannelNumber[channel] = CHANNEL_NUMBER_NONE;
        UAVObjectUpdaterHelper updateHelper;
        manualSettingsObj->setData(manualSettingsData, false);
        updateHelper.doObjectAndWait(manualSettingsObj);
    }
    resetFlightModeSettings();
}

void ConfigInputWidget::resetFlightModeSettings()
{
    // Reset FlightMode settings
    manualSettingsData.FlightModeNumber = DEFAULT_FLIGHT_MODE_NUMBER;
    UAVObjectUpdaterHelper updateHelper;
    manualSettingsObj->setData(manualSettingsData, false);
    updateHelper.doObjectAndWait(manualSettingsObj);
    for (uint8_t pos = 0; pos < FlightModeSettings::FLIGHTMODEPOSITION_NUMELEM; pos++) {
        flightModeSignalValue[pos] = 0;
    }
}

void ConfigInputWidget::resetActuatorSettings()
{
    actuatorSettingsData = actuatorSettingsObj->getData();

    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    QString mixerType;

    // Clear all output data : Min, max, neutral at same value
    // min value for motors and neutral for all others (Reversable motor, servo)
    for (unsigned int output = 0; output < ActuatorSettings::CHANNELMAX_NUMELEM; output++) {
        QString mixerNumType  = QString("Mixer%1Type").arg(output + 1);
        UAVObjectField *field = mixer->getField(mixerNumType);
        Q_ASSERT(field);

        if (field) {
            mixerType = field->getValue().toString();
        }
        if ((mixerType == "Motor") || (mixerType == "Disabled")) {
            // Apply current min setting to neutral/max values
            actuatorSettingsData.ChannelMax[output]     = actuatorSettingsData.ChannelMin[output];
            actuatorSettingsData.ChannelNeutral[output] = actuatorSettingsData.ChannelMin[output];
        } else {
            // Apply current neutral setting to min/max values
            actuatorSettingsData.ChannelMax[output] = actuatorSettingsData.ChannelNeutral[output];
            actuatorSettingsData.ChannelMin[output] = actuatorSettingsData.ChannelNeutral[output];
        }
        UAVObjectUpdaterHelper updateHelper;
        actuatorSettingsObj->setData(actuatorSettingsData, false);
        updateHelper.doObjectAndWait(actuatorSettingsObj);
    }
}

void ConfigInputWidget::updateReceiverActivityStatus()
{
    ReceiverActivity *receiverActivity = ReceiverActivity::GetInstance(getObjectManager());

    Q_ASSERT(receiverActivity);

    FlightStatus *flightStatus = FlightStatus::GetInstance(getObjectManager());

    Q_ASSERT(flightStatus);

    UAVObjectField *activeGroup   = receiverActivity->getField(QString("ActiveGroup"));
    Q_ASSERT(activeGroup);

    UAVObjectField *activeChannel = receiverActivity->getField(QString("ActiveChannel"));
    Q_ASSERT(activeChannel);

    QString activeGroupText   = activeGroup->getValue().toString();
    QString activeChannelText = activeChannel->getValue().toString();


    if (activeGroupText != "None") {
        ui->receiverActivityStatus->setText(tr("%1 input - Channel %2").arg(activeGroupText).arg(activeChannelText));
        ui->receiverActivityStatus->setStyleSheet("QLabel { background-color: green; color: rgb(255, 255, 255); \
                                                   border: 1px solid grey; border-radius: 5; margin:1px; font:bold;}");
    } else {
        bool armed = (flightStatus->getArmed() == FlightStatus::ARMED_ARMED);
        QString receiverActivityText = armed ? tr("Disabled (Armed)") : tr("No activity");
        ui->receiverActivityStatus->setText(receiverActivityText);
        ui->receiverActivityStatus->setStyleSheet("QLabel { background-color: darkGreen; color: rgb(255, 255, 255); \
                                                   border: 1px solid grey; border-radius: 5; margin:1px; font:bold;}");
    }
}

void ConfigInputWidget::failsafeBatteryWarningFlightModeChanged(int index)
{
    HwSettings::DataFields hwSettingsData = hwSettingsObj->getData();
    bool batteryModuleEnabled = (hwSettingsData.OptionalModules[HwSettings::OPTIONALMODULES_BATTERY] == HwSettings::OPTIONALMODULES_ENABLED);

    ui->failsafeBatteryWarningFlightMode->setEnabled(batteryModuleEnabled && index != -1);
    ui->failsafeBatteryWarningFlightModeCb->setChecked(index != -1);
}

void ConfigInputWidget::failsafeBatteryCriticalFlightModeChanged(int index)
{
    HwSettings::DataFields hwSettingsData = hwSettingsObj->getData();
    bool batteryModuleEnabled = (hwSettingsData.OptionalModules[HwSettings::OPTIONALMODULES_BATTERY] == HwSettings::OPTIONALMODULES_ENABLED);

    ui->failsafeBatteryCriticalFlightMode->setEnabled(batteryModuleEnabled && index != -1);
    ui->failsafeBatteryCriticalFlightModeCb->setChecked(index != -1);
}

void ConfigInputWidget::failsafeFlightModeChanged(int index)
{
    ui->failsafeFlightMode->setEnabled(index != -1);
    ui->failsafeFlightModeCb->setChecked(index != -1);
}

void ConfigInputWidget::failsafeFlightModeCbToggled(bool checked)
{
    ui->failsafeFlightMode->setCurrentIndex(checked ? 0 : -1);
}

void ConfigInputWidget::failsafeBatteryWarningFlightModeCbToggled(bool checked)
{
    ui->failsafeBatteryWarningFlightMode->setCurrentIndex(checked ? 0 : -1);
}

void ConfigInputWidget::failsafeBatteryCriticalFlightModeCbToggled(bool checked)
{
    ui->failsafeBatteryCriticalFlightMode->setCurrentIndex(checked ? 0 : -1);
}

void ConfigInputWidget::enableControlsChanged(bool enabled)
{
    ui->failsafeFlightMode->setEnabled(enabled && ui->failsafeFlightMode->currentIndex() != -1);

    HwSettings::DataFields hwSettingsData = hwSettingsObj->getData();
    bool batteryModuleEnabled = (hwSettingsData.OptionalModules[HwSettings::OPTIONALMODULES_BATTERY] == HwSettings::OPTIONALMODULES_ENABLED);
    ui->failsafeBatteryWarningFlightMode->setEnabled(batteryModuleEnabled && enabled && ui->failsafeBatteryWarningFlightMode->currentIndex() != -1);
    ui->failsafeBatteryCriticalFlightMode->setEnabled(batteryModuleEnabled && enabled && ui->failsafeBatteryCriticalFlightMode->currentIndex() != -1);
    ui->failsafeBatteryWarningFlightModeCb->setEnabled(enabled && batteryModuleEnabled);
    ui->failsafeBatteryCriticalFlightModeCb->setEnabled(enabled && batteryModuleEnabled);
}

void ConfigInputWidget::setOutputConfigSafe(bool status)
{
    outputConfigIsSafe = status;
}
