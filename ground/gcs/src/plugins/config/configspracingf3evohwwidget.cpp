/**
 ******************************************************************************
 *
 * @file       configspracingf3evohwwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief SPRacingF3EVO hardware configuration panel
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
#include "configspracingf3evohwwidget.h"

#include "ui_configspracingf3evohwwidget.h"

#include "hwsettings.h"
#include "hwspracingf3evosettings.h"

#include <QDebug>

ConfigSPRacingF3EVOHWWidget::ConfigSPRacingF3EVOHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_SPRacingF3EVOHWWidget();
    m_ui->setupUi(this);

    // must be done before auto binding !
    setWikiURL("SPRacingF3EVO+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");
    addUAVObject("HwSPRacingF3EVOSettings");

    addWidgetBinding("HwSPRacingF3EVOSettings", "UARTPort", m_ui->cbUART1, 0, 1, true);
    addWidgetBinding("HwSPRacingF3EVOSettings", "UARTPort", m_ui->cbUART2, 1, 1, true);
    addWidgetBinding("HwSPRacingF3EVOSettings", "UARTPort", m_ui->cbUART3, 2, 1, true);
    addWidgetBinding("HwSPRacingF3EVOSettings", "LEDPort", m_ui->cbLEDPort);
    addWidgetBinding("HwSPRacingF3EVOSettings", "I2CPort", m_ui->cbI2C1);

    connect(m_ui->cbUART1, static_cast<void(QComboBox::*) (int)>(&QComboBox::currentIndexChanged), this, &ConfigSPRacingF3EVOHWWidget::UART1Changed);
    connect(m_ui->cbUART2, static_cast<void(QComboBox::*) (int)>(&QComboBox::currentIndexChanged), this, &ConfigSPRacingF3EVOHWWidget::UART2Changed);
    connect(m_ui->cbUART3, static_cast<void(QComboBox::*) (int)>(&QComboBox::currentIndexChanged), this, &ConfigSPRacingF3EVOHWWidget::UART3Changed);

    m_ui->commonHWSettings->registerWidgets(*this);

    connect(m_ui->commonHWSettings, &CommonHWSettingsWidget::USBVCPFunctionChanged, this, &ConfigSPRacingF3EVOHWWidget::USBVCPFunctionChanged);

    updateFeatures();
}

ConfigSPRacingF3EVOHWWidget::~ConfigSPRacingF3EVOHWWidget()
{
    // Do nothing
}

void ConfigSPRacingF3EVOHWWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    UART1Changed(0);
    UART2Changed(0);
    UART3Changed(0);
}

void ConfigSPRacingF3EVOHWWidget::updateObjectsFromWidgetsImpl()
{
    updateFeatures();
}

void ConfigSPRacingF3EVOHWWidget::updateFeatures()
{
    quint32 features    = CommonHWSettingsWidget::F_USB;

    QComboBox *ports[3] = { m_ui->cbUART1, m_ui->cbUART2, m_ui->cbUART3 };

    for (int i = 0; i < 3; ++i) {
        switch (getComboboxSelectedOption(ports[i])) {
        case HwSPRacingF3EVOSettings::UARTPORT_TELEMETRY:
            features |= CommonHWSettingsWidget::F_TELEMETRY;
            break;
        case HwSPRacingF3EVOSettings::UARTPORT_DSM:
            features |= CommonHWSettingsWidget::F_DSM;
            break;
        case HwSPRacingF3EVOSettings::UARTPORT_SBUS:
            features |= CommonHWSettingsWidget::F_SBUS;
            break;
        case HwSPRacingF3EVOSettings::UARTPORT_GPS:
            features |= CommonHWSettingsWidget::F_GPS;
            break;
        case HwSPRacingF3EVOSettings::UARTPORT_DEBUGCONSOLE:
            features |= CommonHWSettingsWidget::F_DEBUGCONSOLE;
            break;
        default:
            break;
        }
    }

    m_ui->commonHWSettings->setFeatures(features);

    HwSettings::GetInstance(getObjectManager())
    ->setOptionalModules(HwSettings::OPTIONALMODULES_GPS,
                         (features & CommonHWSettingsWidget::F_GPS)
                         ? HwSettings::OPTIONALMODULES_ENABLED : HwSettings::OPTIONALMODULES_DISABLED);
}

bool ConfigSPRacingF3EVOHWWidget::optionConflict(int uartOption, int vcpOption)
{
    return (vcpOption == HwSettings::USB_VCPPORT_DEBUGCONSOLE
            && uartOption == HwSPRacingF3EVOSettings::UARTPORT_DEBUGCONSOLE)
           || (vcpOption == HwSettings::USB_VCPPORT_MAVLINK
               && uartOption == HwSPRacingF3EVOSettings::UARTPORT_MAVLINK);
}

void ConfigSPRacingF3EVOHWWidget::UARTxChanged(QComboBox *cbUARTx)
{
    /* Everything except HwSPRacingF3EVOSettings::UARTPORT_DISABLED and  HwSPRacingF3EVOSettings::UARTPORT_DSM
     * is allowed on single port only.
     * HoTT SUMD & SUMH belong to the same receiver group, therefore cannot be configure at the same time
     */

    int option = getComboboxSelectedOption(cbUARTx);

    if (option == HwSPRacingF3EVOSettings::UARTPORT_HOTTSUMD) {
        option = HwSPRacingF3EVOSettings::UARTPORT_HOTTSUMH;
    }

    if (option != HwSPRacingF3EVOSettings::UARTPORT_DISABLED && option != HwSPRacingF3EVOSettings::UARTPORT_DSM) {
        QComboBox *ports[3] = { m_ui->cbUART1, m_ui->cbUART2, m_ui->cbUART3 };

        for (int i = 0; i < 3; ++i) {
            if (ports[i] == cbUARTx) {
                continue;
            }
            int other = getComboboxSelectedOption(ports[i]);
            if (other == HwSPRacingF3EVOSettings::UARTPORT_HOTTSUMD) {
                other = HwSPRacingF3EVOSettings::UARTPORT_HOTTSUMH;
            }
            if (other == option) {
                setComboboxSelectedOption(ports[i], HwSPRacingF3EVOSettings::UARTPORT_DISABLED);
            }
        }

        QComboBox *cbUSBVCP = m_ui->commonHWSettings->USBVCPComboBox();

        if (optionConflict(option, getComboboxSelectedOption(cbUSBVCP))) {
            setComboboxSelectedOption(cbUSBVCP, HwSettings::USB_VCPPORT_DISABLED);
        }
    }

    updateFeatures();
}

void ConfigSPRacingF3EVOHWWidget::USBVCPFunctionChanged(int index)
{
    Q_UNUSED(index);

    int vcpOption = getComboboxSelectedOption(m_ui->commonHWSettings->USBVCPComboBox());

    QComboBox *ports[3] = { m_ui->cbUART1, m_ui->cbUART2, m_ui->cbUART3 };

    for (int i = 0; i < 3; ++i) {
        if (optionConflict(getComboboxSelectedOption(ports[i]), vcpOption)) {
            setComboboxSelectedOption(ports[i], HwSPRacingF3EVOSettings::UARTPORT_DISABLED);
        }
    }

    updateFeatures();
}

void ConfigSPRacingF3EVOHWWidget::UART1Changed(int index)
{
    Q_UNUSED(index);
    UARTxChanged(m_ui->cbUART1);
}

void ConfigSPRacingF3EVOHWWidget::UART2Changed(int index)
{
    Q_UNUSED(index);
    UARTxChanged(m_ui->cbUART2);
}

void ConfigSPRacingF3EVOHWWidget::UART3Changed(int index)
{
    Q_UNUSED(index);
    UARTxChanged(m_ui->cbUART3);
}
