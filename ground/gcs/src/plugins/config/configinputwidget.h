/**
 ******************************************************************************
 *
 * @file       configservowidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo input/output configuration panel for the config gadget
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
#ifndef CONFIGINPUTWIDGET_H
#define CONFIGINPUTWIDGET_H

#include "ui_input.h"
#include "ui_input_wizard.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include <QWidget>
#include <QList>
#include "inputchannelform.h"
#include "ui_inputchannelform.h"
#include <QRadioButton>
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "actuatorsettings.h"
#include "mixersettings.h"
#include "flightmodesettings.h"
#include "receiveractivity.h"
#include <QGraphicsView>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include "flightstatus.h"
#include "accessorydesired.h"
#include <QPointer>
#include "systemsettings.h"

class Ui_InputWidget;

class ConfigInputWidget : public ConfigTaskWidget {
    Q_OBJECT
public:
    ConfigInputWidget(QWidget *parent = 0);
    ~ConfigInputWidget();
    enum wizardSteps { wizardWelcome, wizardChooseMode, wizardChooseType, wizardIdentifySticks, wizardIdentifyCenter, wizardIdentifyLimits, wizardIdentifyInverted, wizardFinish, wizardNone };
    enum txMode { mode1, mode2, mode3, mode4 };
    enum txMovements { moveLeftVerticalStick, moveRightVerticalStick, moveLeftHorizontalStick, moveRightHorizontalStick, moveAccess0, moveAccess1, moveAccess2, moveFlightMode, centerAll, moveAll, nothing };
    enum txMovementType { vertical, horizontal, jump, mix };
    enum txType { acro, heli, ground };
    void startInputWizard()
    {
        goToWizard();
    }
    void enableControls(bool enable);
    bool shouldObjectBeSaved(UAVObject *object);

private:
    bool throttleError;
    bool growing;
    bool reverse[ManualControlSettings::CHANNELNEUTRAL_NUMELEM];
    txMovements currentMovement;
    int movePos;
    void setTxMovement(txMovements movement);
    Ui_InputWidget *ui;
    Ui_InputWizardWidget *wizardUi;

    wizardSteps wizardStep;
    QList<QPointer<QWidget> > extraWidgets;
    txMode transmitterMode;
    txType transmitterType;
    struct channelsStruct {
        bool operator ==(const channelsStruct & rhs) const
        {
            return (group == rhs.group) && (number == rhs.number);
        }
        bool operator !=(const channelsStruct & rhs)  const
        {
            return !operator==(rhs);
        }
        int group;
        int number;
        int channelIndex;
    } lastChannel;
    channelsStruct currentChannel;
    QList<channelsStruct> usedChannels;
    bool channelDetected;
    QEventLoop *loop;
    bool skipflag;

    // Variables to support delayed transitions when detecting input controls.
    QTimer nextDelayedTimer;
    int nextDelayedTick;
    int nextDelayedLatestActivityTick;

    int currentChannelNum;
    QList<int> heliChannelOrder;
    QList<int> acroChannelOrder;
    QList<int> groundChannelOrder;

    UAVObject::Metadata manualControlMdata;
    ManualControlCommand *manualCommandObj;
    ManualControlCommand::DataFields manualCommandData;

    FlightStatus *flightStatusObj;
    FlightStatus::DataFields flightStatusData;

    UAVObject::Metadata accessoryDesiredMdata0;
    AccessoryDesired *accessoryDesiredObj0;
    AccessoryDesired *accessoryDesiredObj1;
    AccessoryDesired *accessoryDesiredObj2;

    ManualControlSettings *manualSettingsObj;
    ManualControlSettings::DataFields manualSettingsData;

    ActuatorSettings *actuatorSettingsObj;
    ActuatorSettings::DataFields actuatorSettingsData;

    FlightModeSettings *flightModeSettingsObj;
    FlightModeSettings::DataFields flightModeSettingsData;
    ReceiverActivity *receiverActivityObj;
    ReceiverActivity::DataFields receiverActivityData;

    SystemSettings *systemSettingsObj;
    SystemSettings::DataFields systemSettingsData;

    typedef struct {
        ManualControlSettings::DataFields manualSettingsData;
        ActuatorSettings::DataFields actuatorSettingsData;
        FlightModeSettings::DataFields    flightModeSettingsData;
        SystemSettings::DataFields systemSettingsData;
    } Memento;
    Memento memento;

    QSvgRenderer *m_renderer;

    // Background: background
    QGraphicsSvgItem *m_txMainBody;
    QGraphicsSvgItem *m_txLeftStick;
    QGraphicsSvgItem *m_txRightStick;
    QGraphicsSvgItem *m_txAccess0;
    QGraphicsSvgItem *m_txAccess1;
    QGraphicsSvgItem *m_txAccess2;
    QGraphicsSvgItem *m_txFlightMode;
    QGraphicsSvgItem *m_txBackground;
    QGraphicsSvgItem *m_txArrows;
    QTransform m_txLeftStickOrig;
    QTransform m_txRightStickOrig;
    QTransform m_txAccess0Orig;
    QTransform m_txAccess1Orig;
    QTransform m_txAccess2Orig;
    QTransform m_txFlightModeCOrig;
    QTransform m_txFlightModeLOrig;
    QTransform m_txFlightModeROrig;
    QTransform m_txMainBodyOrig;
    QTransform m_txArrowsOrig;
    QTimer *animate;
    void resetTxControls();
    void setMoveFromCommand(int command);

    void fastMdata();
    void restoreMdata();

    void setChannel(int);
    void nextChannel();
    void prevChannel();

    void wizardSetUpStep(enum wizardSteps);
    void wizardTearDownStep(enum wizardSteps);

    void registerControlActivity();

    void wzNextDelayedStart();
    void wzNextDelayedCancel();

    AccessoryDesired *getAccessoryDesiredInstance(int instance);
    float getAccessoryDesiredValue(int instance);

private slots:
    void wzNext();
    void wzNextDelayed();
    void wzBack();
    void wzCancel();
    void goToWizard();
    void disableWizardButton(int);
    void openHelp();
    void identifyControls();
    void identifyLimits();
    void moveTxControls();
    void moveSticks();
    void dimOtherControls(bool value);
    void moveFMSlider();
    void updatePositionSlider();
    void invertControls();
    void simpleCalibration(bool state);
    void adjustSpecialNeutrals();
    void checkThrottleRange();
    void updateCalibration();
    void resetChannelSettings();
    void resetActuatorSettings();
    void forceOneFlightMode();

protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // ifndef CONFIGINPUTWIDGET_H
