/**
 ******************************************************************************
 *
 * @file       EscCalibrationPage.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup
 * @{
 * @addtogroup EscCalibrationPage
 * @{
 * @brief
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


#include "esccalibrationpage.h"
#include "ui_esccalibrationpage.h"
#include "setupwizard.h"
#include "mixersettings.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "vehicleconfigurationhelper.h"
#include "actuatorsettings.h"

#include <QThread>

EscCalibrationPage::EscCalibrationPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),
    ui(new Ui::EscCalibrationPage), m_isCalibrating(false)
{
    ui->setupUi(this);

    ui->outputHigh->setEnabled(false);
    ui->outputLow->setEnabled(true);
    ui->outputLevel->setEnabled(true);
    ui->outputLevel->setText(QString(tr("%1 µs")).arg(LOW_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS));

    connect(ui->startButton, SIGNAL(clicked()), this, SLOT(startButtonClicked()));
    connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(stopButtonClicked()));

    connect(ui->securityCheckBox1, SIGNAL(toggled(bool)), this, SLOT(securityCheckBoxesToggled()));
    connect(ui->securityCheckBox2, SIGNAL(toggled(bool)), this, SLOT(securityCheckBoxesToggled()));
    connect(ui->securityCheckBox3, SIGNAL(toggled(bool)), this, SLOT(securityCheckBoxesToggled()));
}

EscCalibrationPage::~EscCalibrationPage()
{
    delete ui;
}

void EscCalibrationPage::initializePage()
{
    resetAllSecurityCheckboxes();
}

bool EscCalibrationPage::validatePage()
{
    return true;
}

void EscCalibrationPage::enableButtons(bool enable)
{
    getWizard()->button(QWizard::NextButton)->setEnabled(enable);
    getWizard()->button(QWizard::CancelButton)->setEnabled(enable);
    getWizard()->button(QWizard::BackButton)->setEnabled(enable);
    getWizard()->button(QWizard::CustomButton1)->setEnabled(enable);
    ui->securityCheckBox1->setEnabled(enable);
    ui->securityCheckBox2->setEnabled(enable);
    ui->securityCheckBox3->setEnabled(enable);
    QApplication::processEvents();
}

void EscCalibrationPage::resetAllSecurityCheckboxes()
{
    ui->securityCheckBox1->setChecked(false);
    ui->securityCheckBox2->setChecked(false);
    ui->securityCheckBox3->setChecked(false);
}

int EscCalibrationPage::getHighOutputRate()
{
    if (getWizard()->getEscType() == SetupWizard::ESC_ONESHOT125 ||
        getWizard()->getEscType() == SetupWizard::ESC_ONESHOT42 ||
        getWizard()->getEscType() == SetupWizard::ESC_MULTISHOT) {
        return HIGH_ONESHOT_MULTISHOT_OUTPUT_PULSE_LENGTH_MICROSECONDS;
    } else {
        return HIGH_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS;
    }
}

void EscCalibrationPage::startButtonClicked()
{
    if (!m_isCalibrating) {
        m_isCalibrating = true;
        ui->startButton->setEnabled(false);
        enableButtons(false);
        ui->outputHigh->setEnabled(true);
        ui->outputLow->setEnabled(false);
        ui->nonconnectedLabel->setEnabled(false);
        ui->connectedLabel->setEnabled(true);
        ui->outputLevel->setText(QString(tr("%1 µs")).arg(getHighOutputRate()));
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(uavoManager);
        MixerSettings *mSettings = MixerSettings::GetInstance(uavoManager);
        Q_ASSERT(mSettings);
        QString mixerTypePattern = "Mixer%1Type";

        OutputCalibrationUtil::startOutputCalibration();
        // First check if any servo and set his value to 1500 (like Tricopter)
        for (quint32 i = 0; i < ActuatorSettings::CHANNELADDR_NUMELEM; i++) {
            UAVObjectField *field = mSettings->getField(mixerTypePattern.arg(i + 1));
            Q_ASSERT(field);
            if (field->getValue().toString() == field->getOptions().at(VehicleConfigurationHelper::MIXER_TYPE_SERVO)) {
                m_outputUtil.startChannelOutput(i, 1500);
                m_outputUtil.stopChannelOutput();
            }
        }
        // Find motors and start Esc procedure
        for (quint32 i = 0; i < ActuatorSettings::CHANNELADDR_NUMELEM; i++) {
            UAVObjectField *field = mSettings->getField(mixerTypePattern.arg(i + 1));
            Q_ASSERT(field);
            if (field->getValue().toString() == field->getOptions().at(VehicleConfigurationHelper::MIXER_TYPE_MOTOR)) {
                m_outputChannels << i;
            }
        }
        m_outputUtil.startChannelOutput(m_outputChannels, OFF_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS);
        QThread::msleep(100);
        m_outputUtil.setChannelOutputValue(getHighOutputRate());

        ui->stopButton->setEnabled(true);
    }
}

void EscCalibrationPage::stopButtonClicked()
{
    if (m_isCalibrating) {
        ui->stopButton->setEnabled(false);
        ui->outputHigh->setEnabled(false);
        ui->outputLow->setEnabled(true);

        // Set to low pwm out
        m_outputUtil.setChannelOutputValue(LOW_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS);
        ui->outputLevel->setText(QString(tr("%1 µs")).arg(LOW_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS));
        QApplication::processEvents();
        QThread::msleep(2000);

        // Stop output, back to minimal value (1000) defined in vehicleconfigurationsource.h
        m_outputUtil.stopChannelOutput();
        OutputCalibrationUtil::stopOutputCalibration();

        ui->nonconnectedLabel->setEnabled(true);
        ui->connectedLabel->setEnabled(false);
        m_outputChannels.clear();
        m_isCalibrating = false;
        resetAllSecurityCheckboxes();
        enableButtons(true);
    }
}

void EscCalibrationPage::securityCheckBoxesToggled()
{
    ui->startButton->setEnabled(ui->securityCheckBox1->isChecked() &&
                                ui->securityCheckBox2->isChecked() &&
                                ui->securityCheckBox3->isChecked());
}
