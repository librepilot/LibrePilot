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

#include <QDebug>

CommonHWSettingsWidget::CommonHWSettingsWidget(QWidget *parent) : QWidget(parent)
{
    m_ui = new Ui_CommonHWSettingsWidget();
    m_ui->setupUi(this);

    setFeatures(0);

    /* Relay signals from private members */
    connect(m_ui->cbUSBHID, SIGNAL(currentIndexChanged(int)), this, SIGNAL(USBHIDFunctionChanged(int)));
    connect(m_ui->cbUSBVCP, SIGNAL(currentIndexChanged(int)), this, SIGNAL(USBVCPFunctionChanged(int)));
}

CommonHWSettingsWidget::~CommonHWSettingsWidget()
{
    delete m_ui;
}

void CommonHWSettingsWidget::registerWidgets(ConfigTaskWidget &ct)
{
// addAutoBindings();

// ct.addUAVObject("HwSettings");

    ct.addWidgetBinding("HwSettings", "USB_HIDPort", m_ui->cbUSBHID);
    ct.addWidgetBinding("HwSettings", "USB_VCPPort", m_ui->cbUSBVCP);

    ct.addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbTelemetrySpeed);
    ct.addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbGPSSpeed);
    ct.addWidgetBinding("HwSettings", "DebugConsoleSpeed", m_ui->cbDebugConsoleSpeed);
    ct.addWidgetBinding("HwSettings", "SBusMode", m_ui->cbSBUSMode);
    ct.addWidgetBinding("HwSettings", "DSMxBind", m_ui->cbDSMxBind, 0, 1, true);

    ct.addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbGPSProtocol);
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
