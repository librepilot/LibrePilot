/**
 ******************************************************************************
 *
 * @file       configsparky2hwwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Sparky2 hardware configuration panel
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
#include "configsparky2hwwidget.h"

#include "ui_configsparky2hwwidget.h"

#include "hwsettings.h"

#include <QDebug>

ConfigSparky2HWWidget::ConfigSparky2HWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_Sparky2HWWidget();
    m_ui->setupUi(this);

    m_ui->boardImg->load(QString(":/configgadget/images/sparky2.svg"));
    QSize picSize = m_ui->boardImg->sizeHint();
    picSize.scale(360, 360, Qt::KeepAspectRatio);
    m_ui->boardImg->setFixedSize(picSize);

    // must be done before auto binding !
    setWikiURL("Sparky2+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");

    addWidgetBinding("HwSettings", "SPK2_FlexiPort", m_ui->cbFlexi);
    addWidgetBinding("HwSettings", "SPK2_MainPort", m_ui->cbMain);
    addWidgetBinding("HwSettings", "SPK2_RcvrPort", m_ui->cbRcvr);
    addWidgetBinding("HwSettings", "SPK2_I2CPort", m_ui->cbI2C);

    addWidgetBinding("HwSettings", "USB_HIDPort", m_ui->cbUSBHIDFunction);
    addWidgetBinding("HwSettings", "USB_VCPPort", m_ui->cbUSBVCPFunction);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbFlexiTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbFlexiGPSSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbMainTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbMainGPSSpeed);

    // Add Gps protocol configuration
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbMainGPSProtocol);
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbFlexiGPSProtocol);

    addWidgetBinding("HwSettings", "RadioAuxStream", m_ui->cbRadioAux);

    setupCustomCombos();
}

ConfigSparky2HWWidget::~ConfigSparky2HWWidget()
{
    // Do nothing
}

void ConfigSparky2HWWidget::setupCustomCombos()
{
    connect(m_ui->cbUSBHIDFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbHIDPortChanged(int)));
    connect(m_ui->cbUSBVCPFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbVCPPortChanged(int)));

    // m_ui->cbSonar->addItem(tr("Disabled"));
    // m_ui->cbSonar->setCurrentIndex(0);
    // m_ui->cbSonar->setEnabled(false);

    connect(m_ui->cbFlexi, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged(int)));
    connect(m_ui->cbMain, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged(int)));
}

void ConfigSparky2HWWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    usbVCPPortChanged(0);
    mainPortChanged(0);
    flexiPortChanged(0);
}

void ConfigSparky2HWWidget::updateObjectsFromWidgetsImpl()
{
    // If any port is configured to be GPS port, enable GPS module if it is not enabled.
    // GPS will be already built in for Sparky2 board, keep this check just in case.
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    if ((hwSettings->optionalModulesGPS() == HwSettings_OptionalModules::Disabled) &&
        (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_GPS) ||
         isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_GPS))) {
        hwSettings->setOptionalModulesGPS(HwSettings_OptionalModules::Enabled);
    }
}

void ConfigSparky2HWWidget::usbVCPPortChanged(int index)
{
    Q_UNUSED(index);

    bool vcpComBridgeEnabled = isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_COMBRIDGE);

    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_COMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
    }
    enableComboBoxOptionItem(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_COMBRIDGE, vcpComBridgeEnabled);

    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_COMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
    }
    enableComboBoxOptionItem(m_ui->cbMain, HwSettings::SPK2_MAINPORT_COMBRIDGE, vcpComBridgeEnabled);

    // _DEBUGCONSOLE modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
        }
    }

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_DISABLED);
    }
}

void ConfigSparky2HWWidget::usbHIDPortChanged(int index)
{
    Q_UNUSED(index);

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
    }
}

void ConfigSparky2HWWidget::flexiPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbFlexiTelemSpeed->setVisible(false);
    m_ui->cbFlexiGPSSpeed->setVisible(false);
    m_ui->lblFlexiSpeed->setVisible(true);

    // Add Gps protocol configuration
    m_ui->cbFlexiGPSProtocol->setVisible(false);
    m_ui->lbFlexiGPSProtocol->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbFlexi)) {
    case HwSettings::SPK2_FLEXIPORT_TELEMETRY:
        m_ui->cbFlexiTelemSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_FLEXIPORT_GPS:
        // Add Gps protocol configuration
        m_ui->cbFlexiGPSProtocol->setVisible(true);
        m_ui->lbFlexiGPSProtocol->setVisible(true);

        m_ui->cbFlexiGPSSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_FLEXIPORT_COMBRIDGE:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_FLEXIPORT_DEBUGCONSOLE:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::SPK2_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
        }
        break;
    default:
        m_ui->lblFlexiSpeed->setVisible(false);
        break;
    }
}

void ConfigSparky2HWWidget::mainPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbMainTelemSpeed->setVisible(false);
    m_ui->cbMainGPSSpeed->setVisible(false);
    m_ui->lblMainSpeed->setVisible(true);

    // Add Gps protocol configuration
    m_ui->cbMainGPSProtocol->setVisible(false);
    m_ui->lbMainGPSProtocol->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbMain)) {
    case HwSettings::SPK2_MAINPORT_TELEMETRY:
        m_ui->cbMainTelemSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_MAINPORT_GPS:
        // Add Gps protocol configuration
        m_ui->cbMainGPSProtocol->setVisible(true);
        m_ui->lbMainGPSProtocol->setVisible(true);

        m_ui->cbMainGPSSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_MAINPORT_COMBRIDGE:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
        }
        break;
    case HwSettings::SPK2_MAINPORT_DEBUGCONSOLE:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::SPK2_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
        }
        break;
    default:
        m_ui->lblMainSpeed->setVisible(false);
        break;
    }
}
