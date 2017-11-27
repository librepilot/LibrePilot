/**
 ******************************************************************************
 *
 * @file       commonhwsettingswidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Common hardware configuration panel
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
#include "commonhwsettingswidget.h"
#include "ui_commonhwsettingswidget.h"
#include "hwsettings.h"

#include <QDebug>
#include <extensionsystem/pluginmanager.h>
#include "uavobjectmanager.h"


CommonHWSettingsWidget::CommonHWSettingsWidget(QWidget *parent) : ConfigTaskWidget(parent, Child)
{
    m_ui = new Ui_CommonHWSettingsWidget();
    m_ui->setupUi(this);

    m_ui->cbDSMxBind->addItem(tr("Disabled"), 0);

    setFeatures(0);

    // Relay signals from private members
    connect(m_ui->cbUSBHID, SIGNAL(currentIndexChanged(int)), this, SIGNAL(USBHIDFunctionChanged(int)));
    connect(m_ui->cbUSBVCP, SIGNAL(currentIndexChanged(int)), this, SIGNAL(USBVCPFunctionChanged(int)));

    // And these are here to handle conflicting VCP & HID options (such as USBTelemetry).
    connect(m_ui->cbUSBHID, SIGNAL(currentIndexChanged(int)), this, SLOT(USBHIDComboChanged(int)));
    connect(m_ui->cbUSBVCP, SIGNAL(currentIndexChanged(int)), this, SLOT(USBVCPComboChanged(int)));
}

CommonHWSettingsWidget::~CommonHWSettingsWidget()
{
    delete m_ui;
}

void CommonHWSettingsWidget::registerWidgets(ConfigTaskWidget &ct)
{
    ct.addWidgetBinding("HwSettings", "USB_HIDPort", m_ui->cbUSBHID);
    ct.addWidgetBinding("HwSettings", "USB_VCPPort", m_ui->cbUSBVCP);

    ct.addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbTelemetrySpeed);
    ct.addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbGPSSpeed);
    ct.addWidgetBinding("HwSettings", "DebugConsoleSpeed", m_ui->cbDebugConsoleSpeed);
    ct.addWidgetBinding("HwSettings", "SBusMode", m_ui->cbSBUSMode);

    ct.addWidgetBinding("HwSettings", "DSMxBind", m_ui->cbDSMxBind, 0, 1, true);

    ct.addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbGPSProtocol);
}

void CommonHWSettingsWidget::refreshWidgetsValues(UAVObject *obj)
{
    Q_UNUSED(obj);

    int option = HwSettings::GetInstance(getObjectManager())->getDSMxBind();

    if (m_ui->cbDSMxBind->count() == 0) {
        m_ui->cbDSMxBind->addItem(tr("None"), 0);
        m_ui->cbDSMxBind->addItem(tr("DSM2 1024bit/22ms"), 3);
        m_ui->cbDSMxBind->addItem(tr("DSM2 2048bit/11ms"), 5);
        m_ui->cbDSMxBind->addItem(tr("DSMX 1024bit/22ms"), 7);
        m_ui->cbDSMxBind->addItem(tr("DSMX 2048bit/22ms"), 8);
        m_ui->cbDSMxBind->addItem(tr("DSMX 2048bit/11ms"), 9);
    }

    int index = m_ui->cbDSMxBind->findData(option);

    if (index == -1) {
        m_ui->cbDSMxBind->addItem(tr("%1 Pulses").arg(option), option);
        m_ui->cbDSMxBind->setCurrentIndex(m_ui->cbDSMxBind->count() - 1);
    } else {
        m_ui->cbDSMxBind->setCurrentIndex(index);
    }
}

void CommonHWSettingsWidget::setFeatures(quint32 features)
{
    bool flag = features != 0;

    setVisible(flag);

    flag = (features & F_USB) != 0;

    m_ui->lbUSBHID->setVisible(flag);
    m_ui->cbUSBHID->setVisible(flag);
    m_ui->lbUSBVCP->setVisible(flag);
    m_ui->cbUSBVCP->setVisible(flag);

    flag = (features & F_SBUS) != 0;

    m_ui->lbSBUSMode->setVisible(flag);
    m_ui->cbSBUSMode->setVisible(flag);

    flag = (features & F_DSM) != 0;

    m_ui->lbDSMxBind->setVisible(flag);
    m_ui->cbDSMxBind->setVisible(flag);

    flag = (features & F_TELEMETRY) != 0;

    m_ui->lbTelemetrySpeed->setVisible(flag);
    m_ui->cbTelemetrySpeed->setVisible(flag);

    flag = (features & F_DEBUGCONSOLE) != 0;

    m_ui->lbDebugConsoleSpeed->setVisible(flag);
    m_ui->cbDebugConsoleSpeed->setVisible(flag);

    flag = (features & F_GPS) != 0;

    m_ui->lbGPSSpeed->setVisible(flag);
    m_ui->cbGPSSpeed->setVisible(flag);
    m_ui->lbGPSProtocol->setVisible(flag);
    m_ui->cbGPSProtocol->setVisible(flag);
}

QComboBox *CommonHWSettingsWidget::USBVCPComboBox()
{
    return m_ui->cbUSBVCP;
}

bool CommonHWSettingsWidget::USBFunctionConflict()
{
    return (getComboboxSelectedOption(m_ui->cbUSBHID) == HwSettings::USB_HIDPORT_USBTELEMETRY)
           && (getComboboxSelectedOption(m_ui->cbUSBVCP) == HwSettings::USB_VCPPORT_USBTELEMETRY);
}

void CommonHWSettingsWidget::USBHIDComboChanged(int index)
{
    Q_UNUSED(index);

    if (USBFunctionConflict()) {
        setComboboxSelectedOption(m_ui->cbUSBVCP, HwSettings::USB_VCPPORT_DISABLED);
    } else if (getComboboxSelectedOption(m_ui->cbUSBHID) != HwSettings::USB_HIDPORT_USBTELEMETRY) {
        setComboboxSelectedOption(m_ui->cbUSBVCP, HwSettings::USB_VCPPORT_USBTELEMETRY);
    }
}

void CommonHWSettingsWidget::USBVCPComboChanged(int index)
{
    Q_UNUSED(index);

    if (USBFunctionConflict()) {
        setComboboxSelectedOption(m_ui->cbUSBHID, HwSettings::USB_HIDPORT_DISABLED);
    } else if (getComboboxSelectedOption(m_ui->cbUSBVCP) != HwSettings::USB_VCPPORT_USBTELEMETRY) {
        setComboboxSelectedOption(m_ui->cbUSBHID, HwSettings::USB_HIDPORT_USBTELEMETRY);
    }
}
