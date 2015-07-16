/**
 ******************************************************************************
 *
 * @file       configrevohwwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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
#include "configrevonanohwwidget.h"

#include <QDebug>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>
#include "hwsettings.h"
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>


ConfigRevoNanoHWWidget::ConfigRevoNanoHWWidget(QWidget *parent) : ConfigTaskWidget(parent), m_refreshing(true)
{
    m_ui = new Ui_RevoNanoHWWidget();
    m_ui->setupUi(this);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    if (!settings->useExpertMode()) {
        m_ui->saveTelemetryToRAM->setEnabled(false);
        m_ui->saveTelemetryToRAM->setVisible(false);
    }

    addApplySaveButtons(m_ui->saveTelemetryToRAM, m_ui->saveTelemetryToSD);

    forceConnectedState();

    addWidgetBinding("HwSettings", "RM_FlexiPort", m_ui->cbFlexi);
    addWidgetBinding("HwSettings", "RM_MainPort", m_ui->cbMain);
    addWidgetBinding("HwSettings", "RM_RcvrPort", m_ui->cbRcvr, 0, 1, true);

    addWidgetBinding("HwSettings", "USB_HIDPort", m_ui->cbUSBHIDFunction);
    addWidgetBinding("HwSettings", "USB_VCPPort", m_ui->cbUSBVCPFunction);
    addWidgetBinding("HwSettings", "ComUsbBridgeSpeed", m_ui->cbUSBVCPSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbFlexiTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbFlexiGPSSpeed);
    addWidgetBinding("HwSettings", "ComUsbBridgeSpeed", m_ui->cbFlexiComSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbMainTelemSpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_ui->cbMainGPSSpeed);
    addWidgetBinding("HwSettings", "ComUsbBridgeSpeed", m_ui->cbMainComSpeed);

    addWidgetBinding("HwSettings", "TelemetrySpeed", m_ui->cbRcvrTelemSpeed);
    addWidgetBinding("HwSettings", "ComUsbBridgeSpeed", m_ui->cbRcvrComSpeed);

    // Add Gps protocol configuration
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbMainGPSProtocol);
    addWidgetBinding("GPSSettings", "DataProtocol", m_ui->cbFlexiGPSProtocol);

    connect(m_ui->cchwHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
    setupCustomCombos();
    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    setDirty(false);
    m_refreshing = false;
}

ConfigRevoNanoHWWidget::~ConfigRevoNanoHWWidget()
{
    // Do nothing
}

void ConfigRevoNanoHWWidget::setupCustomCombos()
{
    connect(m_ui->cbUSBHIDFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbHIDPortChanged(int)));
    connect(m_ui->cbUSBVCPFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(usbVCPPortChanged(int)));

    connect(m_ui->cbFlexi, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged(int)));
    connect(m_ui->cbMain, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged(int)));
    connect(m_ui->cbRcvr, SIGNAL(currentIndexChanged(int)), this, SLOT(rcvrPortChanged(int)));
}

void ConfigRevoNanoHWWidget::refreshWidgetsValues(UAVObject *obj)
{
    m_refreshing = true;
    ConfigTaskWidget::refreshWidgetsValues(obj);

    usbVCPPortChanged(0);
    mainPortChanged(0);
    flexiPortChanged(0);
    rcvrPortChanged(0);
    m_refreshing = false;
}

void ConfigRevoNanoHWWidget::updateObjectsFromWidgets()
{
    ConfigTaskWidget::updateObjectsFromWidgets();

    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());
    HwSettings::DataFields data = hwSettings->getData();

    // If any port is configured to be GPS port, enable GPS module if it is not enabled.
    // Otherwise disable GPS module.
    if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_GPS)
        || isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_GPS)) {
        data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] = HwSettings::OPTIONALMODULES_ENABLED;
    } else {
        data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] = HwSettings::OPTIONALMODULES_DISABLED;
    }

    hwSettings->setData(data);
}

void ConfigRevoNanoHWWidget::usbVCPPortChanged(int index)
{
    Q_UNUSED(index);

    bool vcpComBridgeEnabled = isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_COMBRIDGE);

    m_ui->lblUSBVCPSpeed->setVisible(vcpComBridgeEnabled);
    m_ui->cbUSBVCPSpeed->setVisible(vcpComBridgeEnabled);

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
    enableComboBoxOptionItem(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE, vcpComBridgeEnabled);

    // _DEBUGCONSOLE modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DEBUGCONSOLE)) {
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
    }

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_DISABLED);
    }
}

void ConfigRevoNanoHWWidget::usbHIDPortChanged(int index)
{
    Q_UNUSED(index);

    // _USBTELEMETRY modes are mutual exclusive
    if (isComboboxOptionSelected(m_ui->cbUSBHIDFunction, HwSettings::USB_HIDPORT_USBTELEMETRY)
        && isComboboxOptionSelected(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_USBTELEMETRY)) {
        setComboboxSelectedOption(m_ui->cbUSBVCPFunction, HwSettings::USB_VCPPORT_DISABLED);
    }
}

void ConfigRevoNanoHWWidget::flexiPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbFlexiTelemSpeed->setVisible(false);
    m_ui->cbFlexiGPSSpeed->setVisible(false);
    m_ui->cbFlexiComSpeed->setVisible(false);
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
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMTELEMETRY)
            || isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_TELEMETRY)) {
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
        break;
    case HwSettings::RM_FLEXIPORT_COMBRIDGE:
        m_ui->cbFlexiComSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_FLEXIPORT_DEBUGCONSOLE:
        m_ui->cbFlexiComSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
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

void ConfigRevoNanoHWWidget::mainPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbMainTelemSpeed->setVisible(false);
    m_ui->cbMainGPSSpeed->setVisible(false);
    m_ui->cbMainComSpeed->setVisible(false);
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
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_PPMTELEMETRY)
            || isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_TELEMETRY)) {
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
        break;
    case HwSettings::RM_MAINPORT_COMBRIDGE:
        m_ui->cbMainComSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbRcvr, HwSettings::RM_RCVRPORT_DISABLED);
        }
        break;
    case HwSettings::RM_MAINPORT_DEBUGCONSOLE:
        m_ui->cbMainComSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DEBUGCONSOLE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
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

void ConfigRevoNanoHWWidget::rcvrPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->lblRcvrSpeed->setVisible(true);
    m_ui->cbRcvrTelemSpeed->setVisible(false);
    m_ui->cbRcvrComSpeed->setVisible(false);

    switch (getComboboxSelectedOption(m_ui->cbRcvr)) {
    case HwSettings::RM_RCVRPORT_TELEMETRY:
    case HwSettings::RM_RCVRPORT_PPMTELEMETRY:
        m_ui->cbRcvrTelemSpeed->setVisible(true);

        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        break;
    case HwSettings::RM_RCVRPORT_COMBRIDGE:
        m_ui->cbRcvrComSpeed->setVisible(true);
        if (isComboboxOptionSelected(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbFlexi, HwSettings::RM_FLEXIPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_ui->cbMain, HwSettings::RM_MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_ui->cbMain, HwSettings::RM_MAINPORT_DISABLED);
        }
        break;
    default:
        m_ui->lblRcvrSpeed->setVisible(false);
        break;
    }
}

void ConfigRevoNanoHWWidget::openHelp()
{
    QDesktopServices::openUrl(QUrl(tr("http://wiki.openpilot.org/x/GgDBAQ"), QUrl::StrictMode));
}
