/**
 ******************************************************************************
 *
 * @file       configoutputwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo output configuration panel for the config gadget
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

#include "configoutputwidget.h"

#include "ui_output.h"
#include "ui_outputchannelform.h"

#include "outputchannelform.h"
#include "configvehicletypewidget.h"

#include "uavsettingsimportexport/uavsettingsimportexportfactory.h"
#include <extensionsystem/pluginmanager.h>
#include <uavobjecthelper.h>

#include "mixersettings.h"
#include "actuatorcommand.h"
#include "actuatorsettings.h"
#include "flightmodesettings.h"
#include "flightstatus.h"
#include "systemsettings.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QMessageBox>

// Motor settings
#define DSHOT_MAXOUTPUT_RANGE         2000
#define DSHOT_MINTOUTPUT_RANGE        0
#define PWMSYNC_MAXOUTPUT_RANGE       1900
#define DEFAULT_MAXOUTPUT_RANGE       2000
#define DEFAULT_MINOUTPUT_RANGE       900
#define DEFAULT_MINOUTPUT_VALUE       1000
#define REVMOTOR_NEUTRAL_TARGET_VALUE 1500
#define REVMOTOR_NEUTRAL_DIFF_VALUE   200
#define MOTOR_NEUTRAL_DIFF_VALUE      300

// Servo settings
#define SERVO_MAXOUTPUT_RANGE         2500
#define SERVO_MINOUTPUT_RANGE         500
#define SERVO_MAXOUTPUT_VALUE         2000
#define SERVO_MINOUTPUT_VALUE         1000
#define SERVO_NEUTRAL_VALUE           1500

ConfigOutputWidget::ConfigOutputWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_OutputWidget();
    m_ui->setupUi(this);

    // must be done before auto binding !
    setWikiURL("Output+Configuration");

    addAutoBindings();

    m_ui->boardWarningFrame->setVisible(false);
    m_ui->configWarningFrame->setVisible(false);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVSettingsImportExportFactory *importexportplugin = pm->getObject<UAVSettingsImportExportFactory>();
    connect(importexportplugin, SIGNAL(importAboutToBegin()), this, SLOT(stopTests()));

    connect(m_ui->channelOutTest, SIGNAL(clicked(bool)), this, SLOT(runChannelTests(bool)));

    // Configure the task widget

    // Track the ActuatorSettings object
    addUAVObject("ActuatorSettings");

    // NOTE: we have channel indices from 0 to 9, but the convention for OP is Channel 1 to Channel 10.
    // Register for ActuatorSettings changes:
    for (unsigned int i = 0; i < ActuatorCommand::CHANNEL_NUMELEM; i++) {
        OutputChannelForm *form = new OutputChannelForm(i, this);
        form->moveTo(*(m_ui->channelLayout));

        connect(m_ui->channelOutTest, SIGNAL(toggled(bool)), form, SLOT(enableChannelTest(bool)));
        connect(form, SIGNAL(channelChanged(int, int)), this, SLOT(sendChannelTest(int, int)));

        addWidget(form->ui->actuatorMin);
        addWidget(form->ui->actuatorNeutral);
        addWidget(form->ui->actuatorMax);
        addWidget(form->ui->actuatorRev);
        addWidget(form->ui->actuatorLink);
    }

    // Associate the buttons with their UAVO fields
    addWidget(m_ui->spinningArmed);
    connect(m_ui->spinningArmed, SIGNAL(clicked(bool)), this, SLOT(updateSpinStabilizeCheckComboBoxes()));

    addUAVObject("FlightModeSettings");
    addWidgetBinding("FlightModeSettings", "AlwaysStabilizeWhenArmedSwitch", m_ui->alwaysStabilizedSwitch);

    connect(FlightStatus::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateAlwaysStabilizeStatus()));

    MixerSettings *mixer = MixerSettings::GetInstance(getObjectManager());
    Q_ASSERT(mixer);
    m_banks << OutputBankControls(mixer, m_ui->chBank1, QColor("#C6ECAE"), m_ui->cb_outputRate1, m_ui->cb_outputMode1);
    m_banks << OutputBankControls(mixer, m_ui->chBank2, QColor("#91E5D3"), m_ui->cb_outputRate2, m_ui->cb_outputMode2);
    m_banks << OutputBankControls(mixer, m_ui->chBank3, QColor("#FCEC52"), m_ui->cb_outputRate3, m_ui->cb_outputMode3);
    m_banks << OutputBankControls(mixer, m_ui->chBank4, QColor("#C3A8FF"), m_ui->cb_outputRate4, m_ui->cb_outputMode4);
    m_banks << OutputBankControls(mixer, m_ui->chBank5, QColor("#F7F7F2"), m_ui->cb_outputRate5, m_ui->cb_outputMode5);
    m_banks << OutputBankControls(mixer, m_ui->chBank6, QColor("#FF9F51"), m_ui->cb_outputRate6, m_ui->cb_outputMode6);

    QList<int> rates;
    rates << 50 << 60 << 125 << 165 << 270 << 330 << 400 << 490;
    int i = 0;
    foreach(OutputBankControls controls, m_banks) {
        addWidget(controls.rateCombo());

        controls.rateCombo()->addItem(tr("-"), QVariant(0));
        controls.rateCombo()->model()->setData(controls.rateCombo()->model()->index(0, 0), QVariant(0), Qt::UserRole - 1);
        foreach(int rate, rates) {
            controls.rateCombo()->addItem(tr("%1 Hz").arg(rate), rate);
        }

        addWidgetBinding("ActuatorSettings", "BankMode", controls.modeCombo(), i++, 0, true);
        connect(controls.modeCombo(), SIGNAL(currentIndexChanged(int)), this, SLOT(onBankTypeChange()));
    }

    SystemAlarms *systemAlarmsObj = SystemAlarms::GetInstance(getObjectManager());
    connect(systemAlarmsObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateBoardWarnings(UAVObject *)));

    inputCalibrationStarted = false;
    channelTestsStarted     = false;

    // TODO why do we do that ?
    disconnect(this, SLOT(refreshWidgetsValues(UAVObject *)));
}

ConfigOutputWidget::~ConfigOutputWidget()
{
    SystemAlarms *systemAlarmsObj = SystemAlarms::GetInstance(getObjectManager());

    disconnect(systemAlarmsObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateBoardWarnings(UAVObject *)));
    foreach(OutputBankControls controls, m_banks) {
        disconnect(controls.modeCombo(), SIGNAL(currentIndexChanged(int)), this, SLOT(onBankTypeChange()));
    }
}

void ConfigOutputWidget::enableControls(bool enable)
{
    ConfigTaskWidget::enableControls(enable);

    if (!enable) {
        m_ui->channelOutTest->setChecked(false);
        channelTestsStarted = false;
    }
    m_ui->channelOutTest->setEnabled(enable);
}

/**
   Force update all channels with the values in the OutputChannelForms.
 */
void ConfigOutputWidget::sendAllChannelTests()
{
    for (unsigned int i = 0; i < ActuatorCommand::CHANNEL_NUMELEM; i++) {
        OutputChannelForm *form = getOutputChannelForm(i);
        sendChannelTest(i, form->neutral());
    }
}

/**
   Toggles the channel testing mode by making the GCS take over
   the ActuatorCommand objects
 */
void ConfigOutputWidget::runChannelTests(bool state)
{
    SystemAlarms *systemAlarmsObj = SystemAlarms::GetInstance(getObjectManager());
    SystemAlarms::DataFields systemAlarms = systemAlarmsObj->getData();

    if (state && systemAlarms.Alarm[SystemAlarms::ALARM_ACTUATOR] != SystemAlarms::ALARM_OK) {
        QMessageBox mbox;
        mbox.setText(QString(tr("The actuator module is in an error state. This can also occur because there are no inputs. "
                                "Please fix these before testing outputs.")));
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();

        // Unfortunately must cache this since callback will reoccur
        m_accInitialData = ActuatorCommand::GetInstance(getObjectManager())->getMetadata();

        m_ui->channelOutTest->setChecked(false);
        return;
    }

    // Confirm this is definitely what they want
    if (state) {
        QMessageBox mbox;
        mbox.setText(QString(tr("This option will start your motors by the amount selected on the sliders regardless of transmitter."
                                "It is recommended to remove any blades from motors. Are you sure you want to do this?")));
        mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        int retval = mbox.exec();
        if (retval != QMessageBox::Yes) {
            state = false;
            qDebug() << "Cancelled";
            m_ui->channelOutTest->setChecked(false);
            return;
        }
    }

    channelTestsStarted = state;

    // Emit signal to be received by Input tab
    emit outputConfigSafeChanged(!state);

    m_ui->spinningArmed->setEnabled(!state);
    m_ui->alwaysStabilizedSwitch->setEnabled((m_ui->spinningArmed->isChecked()) && !state);
    m_ui->alwayStabilizedLabel1->setEnabled((m_ui->spinningArmed->isChecked()) && !state);
    m_ui->alwayStabilizedLabel2->setEnabled((m_ui->spinningArmed->isChecked()) && !state);
    setBanksEnabled(!state);

    ActuatorCommand *obj = ActuatorCommand::GetInstance(getObjectManager());
    UAVObject::Metadata mdata = obj->getMetadata();
    if (state) {
        m_accInitialData = mdata;
        UAVObject::SetFlightAccess(mdata, UAVObject::ACCESS_READONLY);
        UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_ONCHANGE);
        UAVObject::SetGcsTelemetryAcked(mdata, false);
        UAVObject::SetGcsTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_ONCHANGE);
        mdata.gcsTelemetryUpdatePeriod = 100;
    } else {
        mdata = m_accInitialData; // Restore metadata
    }
    obj->setMetadata(mdata);
    obj->updated();

    // Setup the correct initial channel values when the channel testing mode is turned on.
    if (state) {
        sendAllChannelTests();
    }

    // Add info at end
    if (!state && isDirty()) {
        QMessageBox mbox;
        mbox.setText(QString(tr("You may want to save your neutral settings.")));
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.setIcon(QMessageBox::Information);
        mbox.exec();
    }
}

OutputChannelForm *ConfigOutputWidget::getOutputChannelForm(const int index) const
{
    QList<OutputChannelForm *> outputChannelForms = findChildren<OutputChannelForm *>();
    foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
        if (outputChannelForm->index() == index) {
            return outputChannelForm;
        }
    }

    // no OutputChannelForm found with given index
    return NULL;
}

/**
 * Set the label for a channel output assignement
 */
void ConfigOutputWidget::assignOutputChannel(UAVDataObject *obj, QString &str)
{
    // FIXME: use signal/ slot approach
    UAVObjectField *field = obj->getField(str);
    QStringList options   = field->getOptions();
    int index = options.indexOf(field->getValue().toString());

    OutputChannelForm *outputChannelForm = getOutputChannelForm(index);

    if (outputChannelForm) {
        outputChannelForm->setName(str);
    }
}

/**
   Sends the channel value to the UAV to move the servo.
   Returns immediately if we are not in testing mode
 */
void ConfigOutputWidget::sendChannelTest(int index, int value)
{
    if (!m_ui->channelOutTest->isChecked()) {
        return;
    }

    if (index < 0 || (unsigned)index >= ActuatorCommand::CHANNEL_NUMELEM) {
        return;
    }

    ActuatorCommand *actuatorCommand = ActuatorCommand::GetInstance(getObjectManager());
    Q_ASSERT(actuatorCommand);
    ActuatorCommand::DataFields actuatorCommandFields = actuatorCommand->getData();
    actuatorCommandFields.Channel[index] = value;
    actuatorCommand->setData(actuatorCommandFields);
}

void ConfigOutputWidget::setColor(QWidget *widget, const QColor color)
{
    QPalette p(palette());

    p.setColor(QPalette::Background, color);
    p.setColor(QPalette::Base, color);
    p.setColor(QPalette::Active, QPalette::Button, color);
    p.setColor(QPalette::Inactive, QPalette::Button, color);
    widget->setAutoFillBackground(true);
    widget->setPalette(p);
}

/********************************
 *  Output settings
 *******************************/

/**
   Request the current config from the board (RC Output)
 */
void ConfigOutputWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    // Get Actuator Settings
    ActuatorSettings *actuatorSettings = ActuatorSettings::GetInstance(getObjectManager());

    Q_ASSERT(actuatorSettings);
    ActuatorSettings::DataFields actuatorSettingsData = actuatorSettings->getData();

    // Get channel descriptions
    QStringList channelDesc = ConfigVehicleTypeWidget::getChannelDescriptions();

    // Initialize output forms
    QList<OutputChannelForm *> outputChannelForms = findChildren<OutputChannelForm *>();
    foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
        outputChannelForm->setName(channelDesc[outputChannelForm->index()]);

        // init min,max,neutral
        int minValue = actuatorSettingsData.ChannelMin[outputChannelForm->index()];
        int maxValue = actuatorSettingsData.ChannelMax[outputChannelForm->index()];
        outputChannelForm->setRange(minValue, maxValue);

        int neutral  = actuatorSettingsData.ChannelNeutral[outputChannelForm->index()];
        outputChannelForm->setNeutral(neutral);
    }

    // Get the SpinWhileArmed setting
    m_ui->spinningArmed->setChecked(actuatorSettingsData.MotorsSpinWhileArmed == ActuatorSettings::MOTORSSPINWHILEARMED_TRUE);

    for (int i = 0; i < m_banks.count(); i++) {
        OutputBankControls controls = m_banks.at(i);
        // Reset to all disabled
        controls.label()->setText("-");

        controls.rateCombo()->setEnabled(false);
        setColor(controls.rateCombo(), palette().color(QPalette::Background));
        controls.rateCombo()->setCurrentIndex(0);

        controls.modeCombo()->setEnabled(false);
        setColor(controls.modeCombo(), palette().color(QPalette::Background));
    }

    // Get connected board model
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(utilMngr);
    QStringList bankLabels;
    QList<int> channelBanks;

    if (utilMngr) {
        int board = utilMngr->getBoardModel();
        // Setup labels and combos for banks according to board type
        if ((board & 0xff00) == 0x0400) {
            // Coptercontrol family of boards 4 timer banks
            bankLabels << "1 (1-3)" << "2 (4)" << "3 (5,7-8)" << "4 (6,9-10)";
            channelBanks << 1 << 1 << 1 << 2 << 3 << 4 << 3 << 3 << 4 << 4;
        } else if (board == 0x0903) {
            // Revolution family of boards 6 timer banks
            bankLabels << "1 (1-2)" << "2 (3)" << "3 (4)" << "4 (5-6)" << "5 (7,12)" << "6 (8-11)";
            channelBanks << 1 << 1 << 2 << 3 << 4 << 4 << 5 << 6 << 6 << 6 << 6 << 5;
        } else if (board == 0x0905) {
            // Revolution Nano
            bankLabels << "1 (1)" << "2 (2,7,11)" << "3 (3)" << "4 (4)" << "5 (5-6)" << "6 (8-10,12)";
            channelBanks << 1 << 2 << 3 << 4 << 5 << 5 << 2 << 6 << 6 << 6 << 2 << 6;
        } else if (board == 0x9201) {
            // Sparky2
            bankLabels << "1 (1-2)" << "2 (3)" << "3 (4)" << "4 (5-6)" << "5 (7-8)" << "6 (9-10)";
            channelBanks << 1 << 1 << 2 << 3 << 4 << 4 << 5 << 5 << 6 << 6;
        } else if (board == 0x1001) {
            // SPRacingF3
            bankLabels << "1 (1-3,7)" << "2 (4,8)" << "3 (5)" << "4 (6)";
            channelBanks << 1 << 1 << 1 << 2 << 3 << 4 << 1 << 2;
        } else if (board == 0x1002 || board == 0x1003) {
            // SPRacingF3_EVO, NucleoF303RE
            bankLabels << "1 (1-3)" << "2 (4)" << "3 (5-8)";
            channelBanks << 1 << 1 << 1 << 2 << 3 << 3 << 3 << 3;
        } else if (board == 0x1005) { // PikoBLX
            bankLabels << "1 (1-4)" << "2 (5-6)" << "3 (7)" << "4 (8)";
            channelBanks << 1 << 1 << 1 << 1 << 2 << 2 << 3 << 4;
        } else if (board == 0x1006) { // tinyFISH
            bankLabels << "1 (1)" << "2 (2)" << "3 (3-4)" << "4 (5-6)";
            channelBanks << 1 << 2 << 3 << 3 << 4 << 4;
        }
    }

    // Store how many banks are active according to the board
    activeBanksCount = bankLabels.count();

    int i = 0;
    foreach(QString banklabel, bankLabels) {
        OutputBankControls controls = m_banks.at(i);

        controls.label()->setText(banklabel);
        int index = controls.rateCombo()->findData(actuatorSettingsData.BankUpdateFreq[i]);
        if (index == -1) {
            controls.rateCombo()->addItem(tr("%1 Hz").arg(actuatorSettingsData.BankUpdateFreq[i]), actuatorSettingsData.BankUpdateFreq[i]);
        }
        bool isPWM = (controls.modeCombo()->currentIndex() == ActuatorSettings::BANKMODE_PWM);
        controls.rateCombo()->setCurrentIndex(index);
        controls.rateCombo()->setEnabled(!inputCalibrationStarted && !channelTestsStarted && isPWM);
        setColor(controls.rateCombo(), controls.color());
        controls.modeCombo()->setEnabled(!inputCalibrationStarted && !channelTestsStarted);
        setColor(controls.modeCombo(), controls.color());
        i++;
    }

    // Get Channel ranges:
    i = 0;
    foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
        int minValue = actuatorSettingsData.ChannelMin[outputChannelForm->index()];
        int maxValue = actuatorSettingsData.ChannelMax[outputChannelForm->index()];

        if (channelBanks.count() > i) {
            int bankNumber = channelBanks.at(i);
            OutputBankControls bankControls = m_banks.at(bankNumber - 1);

            setChannelLimits(outputChannelForm, &bankControls);

            outputChannelForm->setBank(QString::number(bankNumber));
            outputChannelForm->setColor(bankControls.color());

            i++;
        }
        outputChannelForm->setRange(minValue, maxValue);
        int neutral = actuatorSettingsData.ChannelNeutral[outputChannelForm->index()];
        outputChannelForm->setNeutral(neutral);
    }

    updateSpinStabilizeCheckComboBoxes();
    checkOutputConfig();
}

/**
 * Sends the config to the board, without saving to the SD card (RC Output)
 */
void ConfigOutputWidget::updateObjectsFromWidgetsImpl()
{
    ActuatorSettings *actuatorSettings = ActuatorSettings::GetInstance(getObjectManager());

    Q_ASSERT(actuatorSettings);
    if (actuatorSettings) {
        ActuatorSettings::DataFields actuatorSettingsData = actuatorSettings->getData();

        // Set channel ranges
        QList<OutputChannelForm *> outputChannelForms     = findChildren<OutputChannelForm *>();
        foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
            actuatorSettingsData.ChannelMax[outputChannelForm->index()]     = outputChannelForm->max();
            actuatorSettingsData.ChannelMin[outputChannelForm->index()]     = outputChannelForm->min();
            actuatorSettingsData.ChannelNeutral[outputChannelForm->index()] = outputChannelForm->neutral();
        }

        // Set update rates
        actuatorSettingsData.BankUpdateFreq[0]    = m_ui->cb_outputRate1->currentData().toUInt();
        actuatorSettingsData.BankUpdateFreq[1]    = m_ui->cb_outputRate2->currentData().toUInt();
        actuatorSettingsData.BankUpdateFreq[2]    = m_ui->cb_outputRate3->currentData().toUInt();
        actuatorSettingsData.BankUpdateFreq[3]    = m_ui->cb_outputRate4->currentData().toUInt();
        actuatorSettingsData.BankUpdateFreq[4]    = m_ui->cb_outputRate5->currentData().toUInt();
        actuatorSettingsData.BankUpdateFreq[5]    = m_ui->cb_outputRate6->currentData().toUInt();

        actuatorSettingsData.MotorsSpinWhileArmed = m_ui->spinningArmed->isChecked() ?
                                                    ActuatorSettings::MOTORSSPINWHILEARMED_TRUE :
                                                    ActuatorSettings::MOTORSSPINWHILEARMED_FALSE;

        // Apply settings
        UAVObjectUpdaterHelper updateHelper;
        actuatorSettings->setData(actuatorSettingsData, false);
        updateHelper.doObjectAndWait(actuatorSettings);
    }

    FlightModeSettings *flightModeSettings = FlightModeSettings::GetInstance(getObjectManager());
    Q_ASSERT(flightModeSettings);

    if (flightModeSettings) {
        FlightModeSettings::DataFields flightModeSettingsData = flightModeSettings->getData();
        flightModeSettingsData.AlwaysStabilizeWhenArmedSwitch = m_ui->alwaysStabilizedSwitch->currentIndex();

        // Apply settings
        flightModeSettings->setData(flightModeSettingsData);
    }
}

void ConfigOutputWidget::updateSpinStabilizeCheckComboBoxes()
{
    m_ui->alwayStabilizedLabel1->setEnabled((m_ui->spinningArmed->isChecked()) && (m_ui->spinningArmed->isEnabled()));
    m_ui->alwayStabilizedLabel2->setEnabled((m_ui->spinningArmed->isChecked()) && (m_ui->spinningArmed->isEnabled()));
    m_ui->alwaysStabilizedSwitch->setEnabled((m_ui->spinningArmed->isChecked()) && (m_ui->spinningArmed->isEnabled()));

    if (!m_ui->spinningArmed->isChecked()) {
        m_ui->alwaysStabilizedSwitch->setCurrentIndex(FlightModeSettings::ALWAYSSTABILIZEWHENARMEDSWITCH_DISABLED);
    }
}

void ConfigOutputWidget::updateAlwaysStabilizeStatus()
{
    FlightStatus *flightStatusObj = FlightStatus::GetInstance(getObjectManager());
    FlightStatus::DataFields flightStatus = flightStatusObj->getData();

    if (flightStatus.AlwaysStabilizeWhenArmed == FlightStatus::ALWAYSSTABILIZEWHENARMED_TRUE) {
        m_ui->alwayStabilizedLabel2->setText(tr("AlwaysStabilizeWhenArmed is <b>ACTIVE</b>. This prevents arming!."));
    } else {
        m_ui->alwayStabilizedLabel2->setText(tr("(Really be careful!)."));
    }
}

void ConfigOutputWidget::setChannelLimits(OutputChannelForm *channelForm, OutputBankControls *bankControls)
{
    // Set UI limits according to the bankmode and destination
    switch (bankControls->modeCombo()->currentIndex()) {
    case ActuatorSettings::BANKMODE_DSHOT:
        // 0 - 2000 UI limits, DShot min value is fixed to zero
        if (channelForm->isServoOutput()) {
            // Driving a servo using DShot doest not make sense so break
            break;
        }
        channelForm->setLimits(DSHOT_MINTOUTPUT_RANGE, DSHOT_MINTOUTPUT_RANGE, DSHOT_MINTOUTPUT_RANGE, DSHOT_MAXOUTPUT_RANGE);
        channelForm->setRange(DSHOT_MINTOUTPUT_RANGE, DSHOT_MAXOUTPUT_RANGE);
        channelForm->setNeutral(DSHOT_MINTOUTPUT_RANGE);
        break;
    case ActuatorSettings::BANKMODE_PWMSYNC:
        // 900 - 1900 UI limits
        // Default values 1000 - 1900
        channelForm->setLimits(DEFAULT_MINOUTPUT_RANGE, PWMSYNC_MAXOUTPUT_RANGE, DEFAULT_MINOUTPUT_RANGE, PWMSYNC_MAXOUTPUT_RANGE);
        channelForm->setRange(DEFAULT_MINOUTPUT_VALUE, PWMSYNC_MAXOUTPUT_RANGE);
        channelForm->setNeutral(DEFAULT_MINOUTPUT_VALUE);
        if (channelForm->isServoOutput()) {
            // Servo: Some of them can handle PWMSync, 500 - 1900 UI limits
            // Default values 1000 - 1900 + neutral 1500
            channelForm->setRange(SERVO_MINOUTPUT_VALUE, PWMSYNC_MAXOUTPUT_RANGE);
            channelForm->setNeutral(SERVO_NEUTRAL_VALUE);
        }
        break;
    case ActuatorSettings::BANKMODE_PWM:
        if (channelForm->isServoOutput()) {
            // Servo: 500 - 2500 UI limits
            // Default values 1000 - 2000 + neutral 1500
            channelForm->setLimits(SERVO_MINOUTPUT_RANGE, SERVO_MAXOUTPUT_RANGE, SERVO_MINOUTPUT_RANGE, SERVO_MAXOUTPUT_RANGE);
            channelForm->setRange(SERVO_MINOUTPUT_VALUE, SERVO_MAXOUTPUT_VALUE);
            channelForm->setNeutral(SERVO_NEUTRAL_VALUE);
            break;
        }
    // PWM motor outputs fall to default
    case ActuatorSettings::BANKMODE_ONESHOT125:
    case ActuatorSettings::BANKMODE_ONESHOT42:
    case ActuatorSettings::BANKMODE_MULTISHOT:
        if (channelForm->isServoOutput()) {
            // Driving a servo using this mode does not make sense so break
            break;
        }
    default:
        // Motors 900 - 2000 UI limits
        // Default values 1000 - 2000, neutral set to min
        // This settings are used for PWM, OneShot125, OneShot42 and MultiShot
        channelForm->setLimits(DEFAULT_MINOUTPUT_RANGE, DEFAULT_MAXOUTPUT_RANGE, DEFAULT_MINOUTPUT_RANGE, DEFAULT_MAXOUTPUT_RANGE);
        channelForm->setRange(DEFAULT_MINOUTPUT_VALUE, DEFAULT_MAXOUTPUT_RANGE);
        channelForm->setNeutral(DEFAULT_MINOUTPUT_VALUE);
        break;
    }
}

ConfigOutputWidget::ChannelConfigWarning ConfigOutputWidget::checkChannelConfig(OutputChannelForm *channelForm, OutputBankControls *bankControls)
{
    ChannelConfigWarning warning = None;
    int currentNeutralValue = channelForm->neutralValue();

    // Check if RevMotor has neutral value around center
    if (channelForm->isReversibleMotorOutput()) {
        warning = IsReversibleMotorCheckNeutral;
        int neutralDiff = qAbs(REVMOTOR_NEUTRAL_TARGET_VALUE - currentNeutralValue);
        if (neutralDiff < REVMOTOR_NEUTRAL_DIFF_VALUE) {
            // Reset warning
            warning = None;
        }
    }

    // Check if NormalMotor neutral is not too high
    if (channelForm->isNormalMotorOutput()) {
        warning = IsNormalMotorCheckNeutral;
        int neutralDiff = currentNeutralValue - DEFAULT_MINOUTPUT_VALUE;
        if (neutralDiff < MOTOR_NEUTRAL_DIFF_VALUE) {
            // Reset warning
            warning = None;
        }
    }

    switch (bankControls->modeCombo()->currentIndex()) {
    case ActuatorSettings::BANKMODE_DSHOT:
        if (channelForm->isServoOutput()) {
            // Driving a servo using DShot doest not make sense
            warning = CannotDriveServo;
        } else if (channelForm->isReversibleMotorOutput()) {
            // Bi-directional DShot not yet supported
            warning = BiDirectionalDShotNotSupported;
        }
        break;
    case ActuatorSettings::BANKMODE_PWMSYNC:
    case ActuatorSettings::BANKMODE_PWM:
        break;
    case ActuatorSettings::BANKMODE_ONESHOT125:
    case ActuatorSettings::BANKMODE_ONESHOT42:
    case ActuatorSettings::BANKMODE_MULTISHOT:
        if (channelForm->isServoOutput()) {
            warning = CannotDriveServo;
            // Driving a servo using this mode does not make sense so break
        }
    default:
        break;
    }

    return warning;
}


void ConfigOutputWidget::onBankTypeChange()
{
    QComboBox *bankModeCombo = qobject_cast<QComboBox *>(sender());

    ChannelConfigWarning new_warning = None;

    if (bankModeCombo != NULL) {
        int bankNumber = 1;
        QList<OutputChannelForm *> outputChannelForms = findChildren<OutputChannelForm *>();
        foreach(OutputBankControls controls, m_banks) {
            if (controls.modeCombo() == bankModeCombo) {
                bool enabled = bankModeCombo->currentIndex() == ActuatorSettings::BANKMODE_PWM;
                controls.rateCombo()->setEnabled(enabled);
                controls.rateCombo()->setCurrentIndex(enabled ? 1 : 0);
                foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
                    if (outputChannelForm->bank().toInt() == bankNumber) {
                        setChannelLimits(outputChannelForm, &controls);
                        ChannelConfigWarning warning = checkChannelConfig(outputChannelForm, &controls);
                        if (warning > None) {
                            new_warning = warning;
                        }
                    }
                }
                break;
            }

            bankNumber++;
        }
    }

    updateChannelConfigWarning(new_warning);
}

bool ConfigOutputWidget::checkOutputConfig()
{
    ChannelConfigWarning new_warning = None;

    int bankNumber = 1;

    QList<OutputChannelForm *> outputChannelForms = findChildren<OutputChannelForm *>();

    foreach(OutputBankControls controls, m_banks) {
        foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
            if (!outputChannelForm->isDisabledOutput() && (outputChannelForm->bank().toInt() == bankNumber)) {
                ChannelConfigWarning warning = checkChannelConfig(outputChannelForm, &controls);
                if (warning > None) {
                    new_warning = warning;
                }
            }
        }

        bankNumber++;
    }

    updateChannelConfigWarning(new_warning);

    // Emit signal to be received by Input tab
    emit outputConfigSafeChanged(new_warning == None);

    return new_warning == None;
}

void ConfigOutputWidget::stopTests()
{
    m_ui->channelOutTest->setChecked(false);
    channelTestsStarted = false;
}

void ConfigOutputWidget::updateBoardWarnings(UAVObject *)
{
    SystemAlarms *systemAlarmsObj = SystemAlarms::GetInstance(getObjectManager());
    SystemAlarms::DataFields systemAlarms = systemAlarmsObj->getData();

    if (systemAlarms.Alarm[SystemAlarms::ALARM_SYSTEMCONFIGURATION] > SystemAlarms::ALARM_WARNING) {
        switch (systemAlarms.ExtendedAlarmStatus[SystemAlarms::EXTENDEDALARMSTATUS_SYSTEMCONFIGURATION]) {
        case SystemAlarms::EXTENDEDALARMSTATUS_UNSUPPORTEDCONFIG_ONESHOT:
            setBoardWarning(tr("OneShot and PWMSync output only works with Receiver Port settings marked with '+OneShot'<br>"
                               "When using Receiver Port setting 'PPM_PIN8+OneShot' "
                               "<b><font color='%1'>Bank %2</font></b> must be set to PWM")
                            .arg(m_banks.at(3).color().name()).arg(m_banks.at(3).label()->text()));
            return;
        }
    }
    setBoardWarning(NULL);
}

void ConfigOutputWidget::updateChannelConfigWarning(ChannelConfigWarning warning)
{
    QString warning_str;

    if (warning == BiDirectionalDShotNotSupported) {
        // TODO: Implement bi-directional DShot
        warning_str = "There is <b>one reversible motor</b> using DShot is configured.<br>"
                      "Bi-directional DShot is currently not supported. Please use PWM, OneShotXXX or MultiShot.";
    }

    if (warning == IsNormalMotorCheckNeutral) {
        warning_str = "There is at least one pretty <b>high neutral value</b> set in your configuration.<br>"
                      "Make sure all ESCs are calibrated and no mechanical stress in all motors.";
    }

    if (warning == IsReversibleMotorCheckNeutral) {
        warning_str = "A least one <b>reversible motor</b> is configured.<br>"
                      "Make sure a appropriate neutral value is set before saving and applying power to the vehicule.";
    }

    if (warning == CannotDriveServo) {
        warning_str = "One bank cannot drive a <b>servo output</b>!<br>"
                      "You must use PWM for this bank or move the servo output to another compatible bank.";
    }

    setConfigWarning(warning_str);
}

void ConfigOutputWidget::setBanksEnabled(bool state)
{
    // Disable/Enable banks
    for (int i = 0; i < m_banks.count(); i++) {
        OutputBankControls controls = m_banks.at(i);
        if (i < activeBanksCount) {
            controls.modeCombo()->setEnabled(state);
            controls.rateCombo()->setEnabled(state);
        } else {
            controls.modeCombo()->setEnabled(false);
            controls.rateCombo()->setEnabled(false);
        }
    }
}

void ConfigOutputWidget::setBoardWarning(QString message)
{
    m_ui->boardWarningFrame->setVisible(!message.isNull());
    m_ui->boardWarningPic->setPixmap(message.isNull() ? QPixmap() : QPixmap(":/configgadget/images/error.svg"));
    m_ui->boardWarningTxt->setText(message);
}

void ConfigOutputWidget::setConfigWarning(QString message)
{
    m_ui->configWarningFrame->setVisible(!message.isNull());
    m_ui->configWarningPic->setPixmap(message.isNull() ? QPixmap() : QPixmap(":/configgadget/images/error.svg"));
    m_ui->configWarningTxt->setText(message);
}

void ConfigOutputWidget::setInputCalibrationState(bool started)
{
    inputCalibrationStarted = started;

    // Disable UI when a input calibration is started
    // so user cannot manipulate settings.
    enableControls(!started);
    setBanksEnabled(!started);
    // Disable ASWA
    m_ui->spinningArmed->setEnabled(!started);
    m_ui->alwaysStabilizedSwitch->setEnabled((m_ui->spinningArmed->isChecked()) && !started);
    m_ui->alwayStabilizedLabel1->setEnabled((m_ui->spinningArmed->isChecked()) && !started);
    m_ui->alwayStabilizedLabel2->setEnabled((m_ui->spinningArmed->isChecked()) && !started);

    // Disable every channel form when needed
    for (unsigned int i = 0; i < ActuatorCommand::CHANNEL_NUMELEM; i++) {
        OutputChannelForm *form = getOutputChannelForm(i);
        form->ui->actuatorRev->setChecked(false);
        form->ui->actuatorLink->setChecked(false);
        form->setChannelRangeEnabled(!started);
        form->setControlsEnabled(!started);
    }
}

OutputBankControls::OutputBankControls(MixerSettings *mixer, QLabel *label, QColor color, QComboBox *rateCombo, QComboBox *modeCombo) :
    m_mixer(mixer), m_label(label), m_color(color), m_rateCombo(rateCombo), m_modeCombo(modeCombo)
{}

OutputBankControls::~OutputBankControls()
{}
