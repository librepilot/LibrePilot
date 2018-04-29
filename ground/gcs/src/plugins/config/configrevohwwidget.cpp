/**
 ******************************************************************************
 *
 * @file       configrevohwwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Revolution hardware configuration panel
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
#include "configrevohwwidget.h"

#include "ui_configrevohwwidget.h"

#include "hwsettings.h"

#include <QDebug>

ConfigRevoHWWidget::ConfigRevoHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_RevoHWWidget();
    m_ui->setupUi(this);

    m_ui->boardImg->load(QString(":/configgadget/images/revolution.svg"));
    QSize picSize = m_ui->boardImg->sizeHint();
    picSize.scale(360, 360, Qt::KeepAspectRatio);
    m_ui->boardImg->setFixedSize(picSize);

    // must be done before auto binding !
    setWikiURL("Revolution+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");

    addWidgetBinding("HwSettings", "RM_FlexiPort", m_ui->cbFlexi);
    addWidgetBinding("HwSettings", "RM_MainPort", m_ui->cbMain);
    addWidgetBinding("HwSettings", "RM_RcvrPort", m_ui->cbRcvr);

    addWidgetBinding("HwSettings", "USB_HIDPort", m_ui->cbUSBHIDFunction);
    addWidgetBinding("HwSettings", "USB_VCPPort", m_ui->cbUSBVCPFunction);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbFlexiTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbFlexiGPSSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbMainTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbMainGPSSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbRcvrTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbRcvrGPSSpeed);

    // Add Gps protocol configuration
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbMainGPSProtocol);
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbFlexiGPSProtocol);
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbRcvrGPSProtocol);

    addWidgetBinding("HwSettings", "RadioAuxStream", m_ui->cbRadioAux);

    setupCustomCombos();
}

ConfigRevoHWWidget::~ConfigRevoHWWidget()
{
    // Do nothing
}

void ConfigRevoHWWidget::setupCustomCombos()
{
    connect(m_ui->cbUSBHIDFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbHIDPortChanged(int)));
    connect(m_ui->cbUSBVCPFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbVCPPortChanged(int)));

    m_ui->cbSonar->addItem(tr("Disabled"));
    m_ui->cbSonar->setCurrentIndex(0);
    m_ui->cbSonar->setEnabled(false);

    connect(m_ui->cbFlexi, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged(int)));
    connect(m_ui->cbMain, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged(int)));
    connect(m_ui->cbRcvr, SIGNAL(currentIndexChanged(int)), this, SLOT(rcvrPortChanged(int)));
}

void ConfigRevoHWWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    usbVCPPortChanged(0);
    mainPortChanged(0);
    flexiPortChanged(0);
    rcvrPortChanged(0);
}

void ConfigRevoHWWidget::updateObjectsFromWidgetsImpl()
{
    // If any port is configured to be GPS port, enable GPS module if it is not enabled.
    // GPS module will be already built in for Revo board, keep this check just in case.
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    if ((hwSettings->optionalModulesGPS() == HwSettings_OptionalModules::Disabled) &&
        (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_GPS) ||
         isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_GPS))) {
        hwSettings->setOptionalModulesGPS(HwSettings_OptionalModules::Enabled);
    }
}

void ConfigRevoHWWidget::usbVCPPortChanged(int index)
{
    Q_UNUSED(index);

    bool vcpComBridgeEnabled = isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_COMBRIDGE);

    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
    }
    enableComboBoxOptionItem(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE, vcpComBridgeEnabled);

    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
    }
    enableComboBoxOptionItem(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE, vcpComBridgeEnabled);

    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
    }
    if (!vcpComBridgeEnabled && isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMCOMBRIDGE)) {
        setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
    }

    enableComboBoxOptionItem(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE, vcpComBridgeEnabled);
    enableComboBoxOptionItem(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMCOMBRIDGE, vcpComBridgeEnabled);

    // _DEBUGCONSOLE modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMDEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
    }

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_DISABLED);
    }
}

void ConfigRevoHWWidget::usbHIDPortChanged(int index)
{
    Q_UNUSED(index);

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
    }
}

void ConfigRevoHWWidget::flexiPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbFlexiTelemSpeed->setVisible(false);
    m_ui->cbFlexiGPSSpeed->setVisible(false);
    m_ui->lblFlexiSpeed->setVisible(true);

    // Add Gps protocol configuration
    m_ui->cbFlexiGPSProtocol->setVisible(false);
    m_ui->lbFlexiGPSProtocol->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbFlexi)) {
    case HwSettings::RM_FLEXIPORT_TELEMETRY:
        m_ui->cbFlexiTelemSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMTELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_FLEXIPORT_GPS:
        // Add Gps protocol configuration
        m_ui->cbFlexiGPSProtocol->setVisible(true);
        m_ui->lbFlexiGPSProtocol->setVisible(true);

        m_ui->cbFlexiGPSSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMGPS)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }

        break;
    case HwSettings::RM_FLEXIPORT_COMBRIDGE:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMCOMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    case HwSettings::RM_FLEXIPORT_DEBUGCONSOLE:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMDEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_FLEXIPORT_OSDHK:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_OSDHK)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_FLEXIPORT_MSP:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMMSP)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    case HwSettings::RM_FLEXIPORT_MAVLINK:
        m_ui->lblFlexiSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMMAVLINK)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    default:
        m_ui->lblFlexiSpeed->setVisible(false);
        break;
    }
}

void ConfigRevoHWWidget::mainPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbMainTelemSpeed->setVisible(false);
    m_ui->cbMainGPSSpeed->setVisible(false);
    m_ui->lblMainSpeed->setVisible(true);

    // Add Gps protocol configuration
    m_ui->cbMainGPSProtocol->setVisible(false);
    m_ui->lbMainGPSProtocol->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbMain)) {
    case HwSettings::RM_MAINPORT_TELEMETRY:
        m_ui->cbMainTelemSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMTELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_MAINPORT_GPS:
        // Add Gps protocol configuration
        m_ui->cbMainGPSProtocol->setVisible(true);
        m_ui->lbMainGPSProtocol->setVisible(true);

        m_ui->cbMainGPSSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMGPS)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_MAINPORT_COMBRIDGE:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMCOMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    case HwSettings::RM_MAINPORT_DEBUGCONSOLE:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMDEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_MAINPORT_OSDHK:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_OSDHK)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        break;
    case HwSettings::RM_MAINPORT_MSP:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMMSP)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    case HwSettings::RM_MAINPORT_MAVLINK:
        m_ui->lblMainSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMMAVLINK)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPM);
        }
        break;
    default:
        m_ui->lblMainSpeed->setVisible(false);
        break;
    }
}

void ConfigRevoHWWidget::rcvrPortChanged(int index)
{
    Q_UNUSED(index);
    m_ui->lblRcvrSpeed->setVisible(true);
    m_ui->cbRcvrTelemSpeed->setVisible(false);
    m_ui->cbRcvrGPSSpeed->setVisible(false);

    // Add Gps protocol configuration
    m_ui->cbRcvrGPSProtocol->setVisible(false);
    m_ui->lblRcvrGPSProtocol->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbRcvr)) {
    case HwSettings::RM_RCVRPORT_TELEMETRY:
    case HwSettings::RM_RCVRPORT_PPMTELEMETRY:
        m_ui->cbRcvrTelemSpeed->setVisible(true);

        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_COMBRIDGE:
    case HwSettings::RM_RCVRPORT_PPMCOMBRIDGE:
        m_ui->lblRcvrSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_DEBUGCONSOLE:
    case HwSettings::RM_RCVRPORT_PPMDEBUGCONSOLE:
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_GPS:
    case HwSettings::RM_RCVRPORT_PPMGPS:
        // Add Gps protocol configuration
        m_ui->cbRcvrGPSProtocol->setVisible(true);
        m_ui->lblRcvrGPSProtocol->setVisible(true);

        m_ui->cbRcvrGPSSpeed->setVisible(true);

        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_GPS)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_MSP:
    case HwSettings::RM_RCVRPORT_PPMMSP:
        m_ui->lblRcvrSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_MSP)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_MAVLINK:
    case HwSettings::RM_RCVRPORT_PPMMAVLINK:
        m_ui->lblRcvrSpeed->setVisible(false);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_MAVLINK)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    default:
        m_ui->lblRcvrSpeed->setVisible(false);
        break;
    }
}
