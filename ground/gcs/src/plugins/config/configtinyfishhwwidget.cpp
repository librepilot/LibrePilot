/**
 ******************************************************************************
 *
 * @file       configtinyfishhwwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief TinyFISH FC hardware configuration panel
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
#include "configtinyfishhwwidget.h"

#include "ui_configtinyfishhwwidget.h"

#include "hwsettings.h"
#include "hwtinyfishsettings.h"

#include <QDebug>

ConfigTinyFISHHWWidget::ConfigTinyFISHHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_TinyFISHHWWidget();
    m_ui->setupUi(this);

    m_ui->boardImg->load(QString(":/configgadget/images/tinyfish.svg"));
    QSize picSize = m_ui->boardImg->sizeHint();
    picSize.scale(360, 360, Qt::KeepAspectRatio);
    m_ui->boardImg->setFixedSize(picSize);

    // must be done before auto binding !
    setWikiURL("TinyFISH+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");
    addUAVObject("HwTinyFISHSettings");

    addWidgetBinding("HwTinyFISHSettings", "UART3Port", m_ui->cbUART3, 0, 1, true);
    addWidgetBinding("HwTinyFISHSettings", "LEDPort", m_ui->cbLEDPort);

    connect(m_ui->cbUART3, static_cast<void(QComboBox::*) (int)>(&QComboBox::currentIndexChanged), this, &ConfigTinyFISHHWWidget::UARTxChanged);

    m_ui->commonHWSettings->registerWidgets(*this);

    connect(m_ui->commonHWSettings, &CommonHWSettingsWidget::USBVCPFunctionChanged, this, &ConfigTinyFISHHWWidget::USBVCPFunctionChanged);

    updateFeatures();
}

ConfigTinyFISHHWWidget::~ConfigTinyFISHHWWidget()
{
    delete m_ui;
}

void ConfigTinyFISHHWWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
// UART3Changed(0);

    m_ui->commonHWSettings->refreshWidgetsValues(obj);
}

void ConfigTinyFISHHWWidget::updateObjectsFromWidgetsImpl()
{
    updateFeatures();
}

void ConfigTinyFISHHWWidget::updateFeatures()
{
    quint32 features = CommonHWSettingsWidget::F_USB;

    switch (getComboboxSelectedOption(m_ui->cbUART3)) {
    case HwTinyFISHSettings::UART3PORT_TELEMETRY:
        features |= CommonHWSettingsWidget::F_TELEMETRY;
        break;
    case HwTinyFISHSettings::UART3PORT_DSM:
        features |= CommonHWSettingsWidget::F_DSM;
        break;
    case HwTinyFISHSettings::UART3PORT_SBUS:
        features |= CommonHWSettingsWidget::F_SBUS;
        break;
    case HwTinyFISHSettings::UART3PORT_GPS:
        features |= CommonHWSettingsWidget::F_GPS;
        break;
    case HwTinyFISHSettings::UART3PORT_DEBUGCONSOLE:
        features |= CommonHWSettingsWidget::F_DEBUGCONSOLE;
        break;
    default:
        break;
    }

    m_ui->commonHWSettings->setFeatures(features);

    HwSettings::GetInstance(getObjectManager())
    ->setOptionalModules(HwSettings::OPTIONALMODULES_GPS,
                         (features & CommonHWSettingsWidget::F_GPS)
                         ? HwSettings::OPTIONALMODULES_ENABLED : HwSettings::OPTIONALMODULES_DISABLED);
}

bool ConfigTinyFISHHWWidget::optionConflict(int uartOption, int vcpOption)
{
    return (vcpOption == HwSettings::USB_VCPPORT_DEBUGCONSOLE
            && uartOption == HwTinyFISHSettings::UART3PORT_DEBUGCONSOLE)
           || (vcpOption == HwSettings::USB_VCPPORT_MAVLINK
               && uartOption == HwTinyFISHSettings::UART3PORT_MAVLINK);
}

void ConfigTinyFISHHWWidget::UARTxChanged(int index)
{
    Q_UNUSED(index);

    QComboBox *cbUARTx = qobject_cast<QComboBox *>(sender());

    if (!cbUARTx) {
        return;
    }

    int option = getComboboxSelectedOption(cbUARTx);

    if (option != HwTinyFISHSettings::UART3PORT_DISABLED && option != HwTinyFISHSettings::UART3PORT_DSM) {
        QComboBox *cbUSBVCP = m_ui->commonHWSettings->USBVCPComboBox();

        if (optionConflict(option, getComboboxSelectedOption(cbUSBVCP))) {
            setComboboxSelectedOption(cbUSBVCP, HwSettings::USB_VCPPORT_DISABLED);
        }
    }

    updateFeatures();
}

void ConfigTinyFISHHWWidget::USBVCPFunctionChanged(int index)
{
    Q_UNUSED(index);

    int vcpOption = getComboboxSelectedOption(m_ui->commonHWSettings->USBVCPComboBox());

    for (int i = 0; i < 3; ++i) {
        if (optionConflict(getComboboxSelectedOption(m_ui->cbUART3), vcpOption)) {
            setComboboxSelectedOption(m_ui->cbUART3, HwTinyFISHSettings::UART3PORT_DISABLED);
        }
    }

    updateFeatures();
}
