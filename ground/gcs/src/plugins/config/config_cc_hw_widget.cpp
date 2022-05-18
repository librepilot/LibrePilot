/**
 ******************************************************************************
 *
 * @file       config_cc_hw_widget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update hardware settings in the firmware
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

#include "config_cc_hw_widget.h"

#include "ui_cc_hw_settings.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutilmanager.h>

#include "hwsettings.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QSvgRenderer>

ConfigCCHWWidget::ConfigCCHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_telemetry = new Ui_CC_HW_Widget();
    m_telemetry->setupUi(this);

    // must be done before auto binding !
    setWikiURL("CC+Hardware+Configuration");

    addAutoBindings();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();
    int id = utilMngr->getBoardModel();
    switch (id) {
    case 0x0101:
        m_telemetry->boardImg->load(QString(":/uploader/images/deviceID-0101.svg"));
        break;
    case 0x0201:
        m_telemetry->boardImg->load(QString(":/uploader/images/deviceID-0201.svg"));
        break;
    case 0x0301:
        m_telemetry->boardImg->load(QString(":/uploader/images/deviceID-0301.svg"));
        break;
    case 0x0401:
        m_telemetry->boardImg->load(QString(":/configgadget/images/coptercontrol.svg"));
        break;
    case 0x0402:
        m_telemetry->boardImg->load(QString(":/configgadget/images/cc3d.svg"));
        break;
    default:
        m_telemetry->boardImg->load(QString(":/configgadget/images/cc3d.svg"));
        break;
    }

    QSize picSize = m_telemetry->boardImg->sizeHint();
    picSize.scale(350, 350, Qt::KeepAspectRatio);
    m_telemetry->boardImg->setFixedSize(picSize);

    addWidgetBinding("HwSettings", "CC_FlexiPort", m_telemetry->cbFlexi);
    addWidgetBinding("HwSettings", "CC_MainPort", m_telemetry->cbTele);
    addWidgetBinding("HwSettings", "CC_RcvrPort", m_telemetry->cbRcvr);
    addWidgetBinding("HwSettings", "USB_HIDPort", m_telemetry->cbUsbHid);
    addWidgetBinding("HwSettings", "USB_VCPPort", m_telemetry->cbUsbVcp);
    addWidgetBinding("HwSettings", "TelemetrySpeed", m_telemetry->telemetrySpeed);
    addWidgetBinding("HwSettings", "GPSSpeed", m_telemetry->gpsSpeed);
    // Add Gps protocol configuration

    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    if (hwSettings->getOptionalModules(HwSettings::OPTIONALMODULES_GPS) != HwSettings::OPTIONALMODULES_ENABLED) {
        m_telemetry->gpsProtocol->setEnabled(false);
        m_telemetry->gpsProtocol->setToolTip(tr("Enable GPS module and reboot the board to be able to select GPS protocol"));
    } else {
        addWidgetBinding("GPSSettings", "DataProtocol", m_telemetry->gpsProtocol);
    }
}

ConfigCCHWWidget::~ConfigCCHWWidget()
{
    // Do nothing
}

void ConfigCCHWWidget::refreshValues()
{}

void ConfigCCHWWidget::widgetsContentsChanged()
{
    ConfigTaskWidget::widgetsContentsChanged();

    if (((m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_DEBUGCONSOLE) &&
         (m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_DEBUGCONSOLE)) ||
        ((m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_DEBUGCONSOLE) &&
         (m_telemetry->cbUsbVcp->currentIndex() == HwSettings::USB_VCPPORT_DEBUGCONSOLE)) ||
        ((m_telemetry->cbUsbVcp->currentIndex() == HwSettings::USB_VCPPORT_DEBUGCONSOLE) &&
         (m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_DEBUGCONSOLE))) {
        enableSaveButtons(false);
        m_telemetry->problems->setText(tr("Warning: you have configured more than one DebugConsole, this currently is not supported"));
    } else if (((m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_TELEMETRY) && (m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_TELEMETRY)) ||
               ((m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_GPS) && (m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_GPS)) ||
               ((m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_DEBUGCONSOLE) && (m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_DEBUGCONSOLE)) ||
               ((m_telemetry->cbTele->currentIndex() == HwSettings::CC_MAINPORT_COMBRIDGE) && (m_telemetry->cbFlexi->currentIndex() == HwSettings::CC_FLEXIPORT_COMBRIDGE))) {
        enableSaveButtons(false);
        m_telemetry->problems->setText(tr("Warning: you have configured both MainPort and FlexiPort for the same function, this currently is not supported"));
    } else if ((m_telemetry->cbUsbHid->currentIndex() == HwSettings::USB_HIDPORT_USBTELEMETRY) && (m_telemetry->cbUsbVcp->currentIndex() == HwSettings::USB_VCPPORT_USBTELEMETRY)) {
        enableSaveButtons(false);
        m_telemetry->problems->setText(tr("Warning: you have configured both USB HID Port and USB VCP Port for the same function, this currently is not supported"));
    } else if ((m_telemetry->cbUsbHid->currentIndex() != HwSettings::USB_HIDPORT_USBTELEMETRY) && (m_telemetry->cbUsbVcp->currentIndex() != HwSettings::USB_VCPPORT_USBTELEMETRY)) {
        enableSaveButtons(false);
        m_telemetry->problems->setText(tr("Warning: you have disabled USB Telemetry on both USB HID Port and USB VCP Port, this currently is not supported"));
    } else {
        m_telemetry->problems->setText("");
        enableSaveButtons(true);
    }
}

void ConfigCCHWWidget::enableSaveButtons(bool enable)
{
    m_telemetry->applyButton->setEnabled(enable);
    m_telemetry->saveButton->setEnabled(enable);
}
