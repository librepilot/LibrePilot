/**
 ******************************************************************************
 *
 * @file       configpikoblxhwwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief PikoBLX hardware configuration panel
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
#include "configpikoblxhwwidget.h"

#include "ui_configpikoblxhwwidget.h"

#include "hwsettings.h"

#include <QDebug>

ConfigPikoBLXHWWidget::ConfigPikoBLXHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_PikoBLXHWWidget();
    m_ui->setupUi(this);

    m_ui->boardImg->load(QString(":/configgadget/images/pikoblx.svg"));
    QSize picSize = m_ui->boardImg->sizeHint();
    picSize.scale(360, 360, Qt::KeepAspectRatio);
    m_ui->boardImg->setFixedSize(picSize);

    // must be done before auto binding !
    setWikiURL("PikoBLX+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");
    addUAVObject("HwPikoBLXSettings");

    addWidgetBinding("HwPikoBLXSettings", "UARTPort", m_ui->cbUART1, 0, 1, true);
    addWidgetBinding("HwPikoBLXSettings", "UARTPort", m_ui->cbUART2, 1, 1, true);
    addWidgetBinding("HwPikoBLXSettings", "UARTPort", m_ui->cbUART3, 2, 1, true);
    addWidgetBinding("HwPikoBLXSettings", "LEDPort", m_ui->cbLEDPort);
    addWidgetBinding("HwPikoBLXSettings", "PPMPort", m_ui->cbPPMPort);

    m_cbUART[0] = m_ui->cbUART1;
    m_cbUART[1] = m_ui->cbUART2;
    m_cbUART[2] = m_ui->cbUART3;

    for (quint32 i = 0; i < HwPikoBLXSettings::UARTPORT_NUMELEM; ++i) {
        connect(m_cbUART[i], static_cast<void(QComboBox::*) (int)>(&QComboBox::currentIndexChanged), this, &ConfigPikoBLXHWWidget::UARTxChanged);
    }

    m_ui->commonHWSettings->registerWidgets(*this);

    connect(m_ui->commonHWSettings, &CommonHWSettingsWidget::USBVCPFunctionChanged, this, &ConfigPikoBLXHWWidget::USBVCPFunctionChanged);

    updateFeatures();
}

ConfigPikoBLXHWWidget::~ConfigPikoBLXHWWidget()
{
    delete m_ui;
}

void ConfigPikoBLXHWWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
// is this needed? This is to force sane state
// UART1Changed(0);
// UART2Changed(0);
// UART3Changed(0);

    m_ui->commonHWSettings->refreshWidgetsValues(obj);
}

void ConfigPikoBLXHWWidget::updateObjectsFromWidgetsImpl()
{
    updateFeatures();
}

void ConfigPikoBLXHWWidget::updateFeatures()
{
    quint32 features = CommonHWSettingsWidget::F_USB;

    for (quint32 i = 0; i < HwPikoBLXSettings::UARTPORT_NUMELEM; ++i) {
        switch (getComboboxSelectedOption(m_cbUART[i])) {
        case HwPikoBLXSettings::UARTPORT_TELEMETRY:
            features |= CommonHWSettingsWidget::F_TELEMETRY;
            break;
        case HwPikoBLXSettings::UARTPORT_DSM:
            features |= CommonHWSettingsWidget::F_DSM;
            break;
        case HwPikoBLXSettings::UARTPORT_SBUS:
            features |= CommonHWSettingsWidget::F_SBUS;
            break;
        case HwPikoBLXSettings::UARTPORT_GPS:
            features |= CommonHWSettingsWidget::F_GPS;
            break;
        case HwPikoBLXSettings::UARTPORT_DEBUGCONSOLE:
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

bool ConfigPikoBLXHWWidget::optionConflict(int uartOption, int vcpOption)
{
    return (vcpOption == HwSettings::USB_VCPPORT_DEBUGCONSOLE
            && uartOption == HwPikoBLXSettings::UARTPORT_DEBUGCONSOLE)
           || (vcpOption == HwSettings::USB_VCPPORT_MAVLINK
               && uartOption == HwPikoBLXSettings::UARTPORT_MAVLINK);
}

void ConfigPikoBLXHWWidget::UARTxChanged(int index)
{
    Q_UNUSED(index);

    QComboBox *cbUARTx = qobject_cast<QComboBox *>(sender());

    if (!cbUARTx) {
        return;
    }

    // Everything except HwPikoBLXSettings::UARTPORT_DISABLED and  HwPikoBLXSettings::UARTPORT_DSM
    // is allowed on single port only.
    // HoTT SUMD & SUMH belong to the same receiver group, therefore cannot be configure at the same time
    //

    int option = getComboboxSelectedOption(cbUARTx);

    if (option == HwPikoBLXSettings::UARTPORT_HOTTSUMD) {
        option = HwPikoBLXSettings::UARTPORT_HOTTSUMH;
    }

    if (option != HwPikoBLXSettings::UARTPORT_DISABLED && option != HwPikoBLXSettings::UARTPORT_DSM) {
        for (quint32 i = 0; i < HwPikoBLXSettings::UARTPORT_NUMELEM; ++i) {
            if (m_cbUART[i] == cbUARTx) {
                continue;
            }
            int other = getComboboxSelectedOption(m_cbUART[i]);
            if (other == HwPikoBLXSettings::UARTPORT_HOTTSUMD) {
                other = HwPikoBLXSettings::UARTPORT_HOTTSUMH;
            }
            if (other == option) {
                setComboboxSelectedOption(m_cbUART[i], HwPikoBLXSettings::UARTPORT_DISABLED);
            }
        }

        QComboBox *cbUSBVCP = m_ui->commonHWSettings->USBVCPComboBox();

        if (optionConflict(option, getComboboxSelectedOption(cbUSBVCP))) {
            setComboboxSelectedOption(cbUSBVCP, HwSettings::USB_VCPPORT_DISABLED);
        }
    }

    updateFeatures();
}

void ConfigPikoBLXHWWidget::USBVCPFunctionChanged(int index)
{
    Q_UNUSED(index);

    int vcpOption = getComboboxSelectedOption(m_ui->commonHWSettings->USBVCPComboBox());

    for (quint32 i = 0; i < HwPikoBLXSettings::UARTPORT_NUMELEM; ++i) {
        if (optionConflict(getComboboxSelectedOption(m_cbUART[i]), vcpOption)) {
            setComboboxSelectedOption(m_cbUART[i], HwPikoBLXSettings::UARTPORT_DISABLED);
        }
    }

    updateFeatures();
}
