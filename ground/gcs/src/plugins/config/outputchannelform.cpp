/**
 ******************************************************************************
 *
 * @file       outputchannelform.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo output configuration form for the config output gadget
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

#include "outputchannelform.h"

#include "ui_outputchannelform.h"
#include <QDebug>

OutputChannelForm::OutputChannelForm(const int index, QWidget *parent) :
    ChannelForm(index, parent), ui(new Ui::outputChannelForm), m_inChannelTest(false), m_updateChannelRangeEnabled(true)
{
    ui->setupUi(this);

    // The convention for OP is Channel 1 to Channel 10.
    ui->actuatorNumber->setText(QString("%1").arg(index + 1));
    setBank("-");
    // Register for ActuatorSettings changes:
    connect(ui->actuatorMin, SIGNAL(editingFinished()), this, SLOT(setChannelRange()));
    connect(ui->actuatorMax, SIGNAL(editingFinished()), this, SLOT(setChannelRange()));
    connect(ui->actuatorRev, SIGNAL(toggled(bool)), this, SLOT(reverseChannel(bool)));
    // Now connect the channel out sliders to our signal to send updates in test mode
    connect(ui->actuatorNeutral, SIGNAL(valueChanged(int)), this, SLOT(sendChannelTest(int)));

    ui->actuatorLink->setChecked(false);
    connect(ui->actuatorLink, SIGNAL(toggled(bool)), this, SLOT(linkToggled(bool)));

    setChannelRange();

    disableMouseWheelEvents();
}

OutputChannelForm::~OutputChannelForm()
{
    delete ui;
}

QString OutputChannelForm::name()
{
    return ui->actuatorName->text();
}

QString OutputChannelForm::bank()
{
    return ui->actuatorBankNumber->text();
}

/**
 * Set the channel assignment label.
 */
void OutputChannelForm::setName(const QString &name)
{
    ui->actuatorName->setText(name);
}

void OutputChannelForm::setColor(const QColor &color)
{
    QString stylesheet = ui->actuatorNumberFrame->styleSheet();

    stylesheet = stylesheet.split("background-color").first();
    stylesheet.append(
        QString("background-color: rgb(%1, %2, %3)")
        .arg(color.red()).arg(color.green()).arg(color.blue()));
    ui->actuatorNumberFrame->setStyleSheet(stylesheet);
}

/**
 * Set the channel bank label.
 */
void OutputChannelForm::setBank(const QString &bank)
{
    ui->actuatorBankNumber->setText(bank);
}

/**
 * Restrict UI to protect users from accidental misuse.
 */
void OutputChannelForm::enableChannelTest(bool state)
{
    if (m_inChannelTest == state) {
        return;
    }
    m_inChannelTest = state;

    if (m_inChannelTest) {
        // Prevent stupid users from touching the minimum & maximum ranges while
        // moving the sliders. Thanks Ivan for the tip :)
        ui->actuatorMin->setEnabled(false);
        ui->actuatorMax->setEnabled(false);
        ui->actuatorRev->setEnabled(false);
    } else if (!isDisabledOutput()) {
        ui->actuatorMin->setEnabled(true);
        ui->actuatorMax->setEnabled(true);
        if (!isNormalMotorOutput()) {
            ui->actuatorRev->setEnabled(true);
        }
    }
}

/**
 * Enable/Disable setChannelRange
 */
void OutputChannelForm::setChannelRangeEnabled(bool state)
{
    if (m_updateChannelRangeEnabled == state) {
        return;
    }
    m_updateChannelRangeEnabled = state;
}

/**
 * Toggles the channel linked state for use in testing mode
 */
void OutputChannelForm::linkToggled(bool state)
{
    Q_UNUSED(state)

    if (!m_inChannelTest) {
        return; // we are not in Test Output mode
    }
    // find the minimum slider value for the linked ones
    if (!parent()) {
        return;
    }
    int min = ui->actuatorMax->maximum();
    int linked_count = 0;
    QList<OutputChannelForm *> outputChannelForms = parent()->findChildren<OutputChannelForm *>();
    // set the linked channels of the parent widget to the same value
    foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
        if (!outputChannelForm->ui->actuatorLink->checkState()) {
            continue;
        }
        if (this == outputChannelForm) {
            continue;
        }
        int value = outputChannelForm->ui->actuatorNeutral->value();
        if (min > value) {
            min = value;
        }
        linked_count++;
    }

    if (linked_count <= 0) {
        return; // no linked channels
    }
    // set the linked channels to the same value
    foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
        if (!outputChannelForm->ui->actuatorLink->checkState()) {
            continue;
        }
        outputChannelForm->ui->actuatorNeutral->setValue(min);
    }
}

int OutputChannelForm::max() const
{
    return ui->actuatorMax->value();
}

/**
 * Set maximal channel value.
 */
void OutputChannelForm::setMax(int maximum)
{
    setRange(ui->actuatorMax->value(), maximum);
}

int OutputChannelForm::min() const
{
    return ui->actuatorMin->value();
}

/**
 * Set minimal channel value.
 */
void OutputChannelForm::setMin(int minimum)
{
    setRange(minimum, ui->actuatorMin->value());
}

int OutputChannelForm::neutral() const
{
    return ui->actuatorNeutral->value();
}

/**
 * Set neutral of channel.
 */
void OutputChannelForm::setNeutral(int value)
{
    ui->actuatorNeutral->setValue(value);
}

/**
 *
 * Set
 */
void OutputChannelForm::setLimits(int actuatorMinMinimum, int actuatorMinMaximum, int actuatorMaxMinimum, int actuatorMaxMaximum)
{
    ui->actuatorMin->setMaximum(actuatorMinMaximum);
    ui->actuatorMax->setMaximum(actuatorMaxMaximum);
    ui->actuatorMin->setMinimum(actuatorMinMinimum);
    ui->actuatorMax->setMinimum(actuatorMaxMinimum);
    // Neutral slider limits
    ui->actuatorNeutral->setMinimum(actuatorMinMinimum);
    ui->actuatorNeutral->setMaximum(actuatorMaxMaximum);
}

/**
 * Set minimal and maximal channel value.
 */
void OutputChannelForm::setRange(int minimum, int maximum)
{
    ui->actuatorMin->setValue(minimum);
    ui->actuatorMax->setValue(maximum);
    setChannelRange();
}

/**
 * Sets the minimum/maximum value of the channel output sliders.
 * Have to do it here because setMinimum is not a slot.
 *
 * One added trick: if the slider is at its min when the value
 * is changed, then keep it on the min.
 */
void OutputChannelForm::setChannelRange()
{
    // Disable outputs not already set in MixerSettings
    if (isDisabledOutput()) {
        setLimits(1000, 1000, 1000, 1000);
        ui->actuatorMin->setValue(1000);
        ui->actuatorMax->setValue(1000);
        ui->actuatorRev->setChecked(false);
        ui->actuatorLink->setChecked(false);
        setControlsEnabled(false);
        return;
    }

    if (!m_updateChannelRangeEnabled) {
        // Nothing to do here
        return;
    }

    setControlsEnabled(true);

    int minValue = ui->actuatorMin->value();
    int maxValue = ui->actuatorMax->value();

    int oldMini  = ui->actuatorNeutral->minimum();
    int oldMaxi  = ui->actuatorNeutral->maximum();

    // Red handle for Motors
    if (isNormalMotorOutput() || isReversibleMotorOutput()) {
        ui->actuatorNeutral->setStyleSheet("QSlider::handle:horizontal { background: rgb(255, 100, 100); width: 18px; height: 28px;"
                                           "margin: -3px 0; border-radius: 3px; border: 1px solid #777; }");
    } else {
        ui->actuatorNeutral->setStyleSheet("QSlider::handle:horizontal { background: rgb(196, 196, 196); width: 18px; height: 28px;"
                                           "margin: -3px 0; border-radius: 3px; border: 1px solid #777; }");
    }

    // Normal motor will be *** never *** reversed : without arming a "Min" value (like 1900) can be applied !
    if (isNormalMotorOutput()) {
        if (minValue > maxValue) {
            // Keep old values
            ui->actuatorMin->setValue(oldMini);
            ui->actuatorMax->setValue(oldMaxi);
        }
        ui->actuatorRev->setChecked(false);
        ui->actuatorRev->setEnabled(false);
        ui->actuatorNeutral->setInvertedAppearance(false);
        ui->actuatorNeutral->setRange(ui->actuatorMin->value(), ui->actuatorMax->value());
    } else {
        // Others output (!Motor)
        // Auto check reverse checkbox SpinBox Min/Max changes
        ui->actuatorRev->setEnabled(true);
        if (minValue <= maxValue) {
            ui->actuatorRev->setChecked(false);
            ui->actuatorNeutral->setInvertedAppearance(false);
            ui->actuatorNeutral->setRange(minValue, maxValue);
        } else {
            ui->actuatorRev->setChecked(true);
            ui->actuatorNeutral->setInvertedAppearance(true);
            ui->actuatorNeutral->setRange(maxValue, minValue);
        }
    }
    // If old neutral was Min, stay Min
    if (ui->actuatorNeutral->value() == oldMini) {
        ui->actuatorNeutral->setValue(ui->actuatorNeutral->minimum());
    }
}

/**
 * Reverses the channel when the checkbox is clicked
 */
void OutputChannelForm::reverseChannel(bool state)
{
    // if 'state' (reverse channel requested) apply only if not already reversed
    if ((state && (ui->actuatorMax->value() > ui->actuatorMin->value()))
        || (!state && (ui->actuatorMax->value() < ui->actuatorMin->value()))) {
        // Now, swap the min & max values (spin boxes)
        int temp = ui->actuatorMax->value();
        ui->actuatorMax->setValue(ui->actuatorMin->value());
        ui->actuatorMin->setValue(temp);
        ui->actuatorNeutral->setInvertedAppearance(state);

        setChannelRange();
        return;
    }
}

/**
 * Enable/Disable UI controls
 */
void OutputChannelForm::setControlsEnabled(bool state)
{
    if (isDisabledOutput()) {
        state = false;
    }
    ui->actuatorMin->setEnabled(state);
    ui->actuatorMax->setEnabled(state);
    ui->actuatorValue->setEnabled(state);
    ui->actuatorLink->setEnabled(state);
    // Reverse checkbox will be never checked
    // or enabled for normal motor
    if (isNormalMotorOutput()) {
        ui->actuatorRev->setChecked(false);
        ui->actuatorRev->setEnabled(false);
    } else {
        ui->actuatorRev->setEnabled(state);
    }
}

/**
 * Emits the channel value which will be send to the UAV to move the servo.
 * Returns immediately if we are not in testing mode.
 */
void OutputChannelForm::sendChannelTest(int value)
{
    int in_value = value;

    QSlider *ob  = (QSlider *)QObject::sender();

    if (!ob) {
        return;
    }

    // update the label
    ui->actuatorValue->setValue(value);

    if (ui->actuatorLink->checkState() && parent()) {
        // the channel is linked to other channels
        QList<OutputChannelForm *> outputChannelForms = parent()->findChildren<OutputChannelForm *>();
        // set the linked channels of the parent widget to the same value
        foreach(OutputChannelForm * outputChannelForm, outputChannelForms) {
            if (this == outputChannelForm) {
                continue;
            }
            if (!outputChannelForm->ui->actuatorLink->checkState()) {
                continue;
            }

            int val = in_value;
            if (val < outputChannelForm->ui->actuatorNeutral->minimum()) {
                val = outputChannelForm->ui->actuatorNeutral->minimum();
            }
            if (val > outputChannelForm->ui->actuatorNeutral->maximum()) {
                val = outputChannelForm->ui->actuatorNeutral->maximum();
            }

            if (outputChannelForm->ui->actuatorNeutral->value() == val) {
                continue;
            }

            outputChannelForm->ui->actuatorNeutral->setValue(val);
            outputChannelForm->ui->actuatorValue->setValue(val);
        }
    }

    if (!m_inChannelTest) {
        // we are not in Test Output mode
        return;
    }
    emit channelChanged(index(), value);
}

/**
 *
 * Returns current neutral value
 */
int OutputChannelForm::neutralValue()
{
    return ui->actuatorNeutral->value();
}

/**
 *
 * Returns MixerType
 */
QString OutputChannelForm::outputMixerType()
{
    UAVDataObject *mixer = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject(QString("MixerSettings")));

    Q_ASSERT(mixer);

    QString mixerNumType  = QString("Mixer%1Type").arg(index() + 1);
    UAVObjectField *field = mixer->getField(mixerNumType);
    Q_ASSERT(field);
    QString mixerType     = field->getValue().toString();

    return mixerType;
}

/**
 *
 * Returns true if a servo output
 */
bool OutputChannelForm::isServoOutput()
{
    return !isNormalMotorOutput() && !isReversibleMotorOutput() && !isDisabledOutput();
}

/**
 *
 * Returns true if output is a normal Motor
 */
bool OutputChannelForm::isNormalMotorOutput()
{
    return outputMixerType() == "Motor";
}

/**
 *
 * Returns true if output is a reversible Motor
 */
bool OutputChannelForm::isReversibleMotorOutput()
{
    return outputMixerType() == "ReversableMotor";
}

/**
 *
 * Returns true if output is disabled
 */
bool OutputChannelForm::isDisabledOutput()
{
    return outputMixerType() == "Disabled";
}
