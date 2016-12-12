/**
 ****************************************************************************************
 *
 * @file       configoplinkwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to configure the OPLink, Revo and Sparky2 modems
 ***************************************************************************************/
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

#include "configoplinkwidget.h"

#include "ui_oplink.h"

#include <uavobjectutilmanager.h>

#include <oplinksettings.h>
#include <oplinkstatus.h>

#include <QMessageBox>
#include <QDebug>

// Channel range and Frequency display
static const int MAX_CHANNEL_NUM   = 250;
static const int MIN_CHANNEL_RANGE = 10;
static const float FIRST_FREQUENCY = 430.000;
static const float FREQUENCY_STEP  = 0.040;

ConfigOPLinkWidget::ConfigOPLinkWidget(QWidget *parent) : ConfigTaskWidget(parent, false), statusUpdated(false)
{
    m_oplink = new Ui_OPLinkWidget();
    m_oplink->setupUi(this);

    // must be done before auto binding !
    setWikiURL("OPLink+Configuration");

    addAutoBindings();

    disableMouseWheelEvents();

    connect(this, SIGNAL(connected()), this, SLOT(connected()));

    oplinkStatusObj   = dynamic_cast<OPLinkStatus *>(getObject("OPLinkStatus"));
    Q_ASSERT(oplinkStatusObj);

    oplinkSettingsObj = dynamic_cast<OPLinkSettings *>(getObject("OPLinkSettings"));
    Q_ASSERT(oplinkSettingsObj);

    addWidget(m_oplink->FirmwareVersion);
    addWidget(m_oplink->SerialNumber);
    addWidget(m_oplink->MinFreq);
    addWidget(m_oplink->MaxFreq);
    addWidget(m_oplink->UnbindButton);
    addWidget(m_oplink->PairSignalStrengthBar1);
    addWidget(m_oplink->PairSignalStrengthLabel1);

    addWidgetBinding("OPLinkSettings", "Protocol", m_oplink->Protocol);
    addWidgetBinding("OPLinkSettings", "LinkType", m_oplink->LinkType);
    addWidgetBinding("OPLinkSettings", "CoordID", m_oplink->CoordID);
    addWidgetBinding("OPLinkSettings", "CustomDeviceID", m_oplink->CustomDeviceID);
    addWidgetBinding("OPLinkSettings", "MinChannel", m_oplink->MinimumChannel);
    addWidgetBinding("OPLinkSettings", "MaxChannel", m_oplink->MaximumChannel);
    addWidgetBinding("OPLinkSettings", "MaxRFPower", m_oplink->MaxRFTxPower);
    addWidgetBinding("OPLinkSettings", "ComSpeed", m_oplink->ComSpeed);
    addWidgetBinding("OPLinkSettings", "MainPort", m_oplink->MainPort);
    addWidgetBinding("OPLinkSettings", "FlexiPort", m_oplink->FlexiPort);
    addWidgetBinding("OPLinkSettings", "VCPPort", m_oplink->VCPPort);

    addWidgetBinding("OPLinkStatus", "DeviceID", m_oplink->DeviceID);
    addWidgetBinding("OPLinkStatus", "RxGood", m_oplink->Good);
    addWidgetBinding("OPLinkStatus", "RxCorrected", m_oplink->Corrected);
    addWidgetBinding("OPLinkStatus", "RxErrors", m_oplink->Errors);
    addWidgetBinding("OPLinkStatus", "RxMissed", m_oplink->Missed);
    addWidgetBinding("OPLinkStatus", "RxFailure", m_oplink->RxFailure);
    addWidgetBinding("OPLinkStatus", "UAVTalkErrors", m_oplink->UAVTalkErrors);
    addWidgetBinding("OPLinkStatus", "TxDropped", m_oplink->Dropped);
    addWidgetBinding("OPLinkStatus", "TxFailure", m_oplink->TxFailure);
    addWidgetBinding("OPLinkStatus", "Resets", m_oplink->Resets);
    addWidgetBinding("OPLinkStatus", "Timeouts", m_oplink->Timeouts);
    addWidgetBinding("OPLinkStatus", "RSSI", m_oplink->RSSI);
    addWidgetBinding("OPLinkStatus", "HeapRemaining", m_oplink->FreeHeap);
    addWidgetBinding("OPLinkStatus", "LinkQuality", m_oplink->LinkQuality);
    addWidgetBinding("OPLinkStatus", "RXSeq", m_oplink->RXSeq);
    addWidgetBinding("OPLinkStatus", "TXSeq", m_oplink->TXSeq);
    addWidgetBinding("OPLinkStatus", "RXRate", m_oplink->RXRate);
    addWidgetBinding("OPLinkStatus", "TXRate", m_oplink->TXRate);
    addWidgetBinding("OPLinkStatus", "RXPacketRate", m_oplink->RXPacketRate);
    addWidgetBinding("OPLinkStatus", "TXPacketRate", m_oplink->TXPacketRate);

    // initially hide port combo boxes
    setPortsVisible(false);

    // Connect the selection changed signals.
    connect(m_oplink->Protocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolChanged()));
    connect(m_oplink->LinkType, SIGNAL(currentIndexChanged(int)), this, SLOT(linkTypeChanged()));
    connect(m_oplink->MinimumChannel, SIGNAL(valueChanged(int)), this, SLOT(minChannelChanged()));
    connect(m_oplink->MaximumChannel, SIGNAL(valueChanged(int)), this, SLOT(maxChannelChanged()));
    connect(m_oplink->MainPort, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged()));
    connect(m_oplink->FlexiPort, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged()));
    connect(m_oplink->VCPPort, SIGNAL(currentIndexChanged(int)), this, SLOT(vcpPortChanged()));

    // Connect the Unbind button
    connect(m_oplink->UnbindButton, SIGNAL(released()), this, SLOT(unbind()));

    // all upper case hex
    m_oplink->CustomDeviceID->setInputMask(">HHHHHHHH");
    m_oplink->CustomDeviceID->setPlaceholderText("AutoGen");

    m_oplink->CoordID->setInputMask(">HHHHHHHH");

    m_oplink->MinimumChannel->setKeyboardTracking(false);
    m_oplink->MaximumChannel->setKeyboardTracking(false);

    m_oplink->MaximumChannel->setMaximum(MAX_CHANNEL_NUM);
    m_oplink->MinimumChannel->setMaximum(MAX_CHANNEL_NUM - MIN_CHANNEL_RANGE);
}

ConfigOPLinkWidget::~ConfigOPLinkWidget()
{}

void ConfigOPLinkWidget::connected()
{
    // qDebug() << "ConfigOPLinkWidget::connected()";
    statusUpdated = false;

    // Request and update of the setting object if we haven't received it yet.
    // this is only really needed for OPLM
    oplinkSettingsObj->requestUpdate();

    updateSettings();
}

void ConfigOPLinkWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    // qDebug() << "ConfigOPLinkWidget::refreshWidgetsValuesImpl()" << obj;
    if (obj == oplinkStatusObj) {
        updateStatus();
    } else if (obj == oplinkSettingsObj) {
        updateSettings();
    }
}

void ConfigOPLinkWidget::updateStatus()
{
    // qDebug() << "ConfigOPLinkWidget::updateStatus";

    // Update the link state
    UAVObjectField *linkField = oplinkStatusObj->getField("LinkState");

    m_oplink->LinkState->setText(linkField->getValue().toString());
    bool linkConnected = (oplinkStatusObj->linkState() == OPLinkStatus_LinkState::Connected);

    m_oplink->PairSignalStrengthBar1->setValue(linkConnected ? m_oplink->RSSI->text().toInt() : -127);
    m_oplink->PairSignalStrengthLabel1->setText(QString("%1dB").arg(m_oplink->PairSignalStrengthBar1->value()));

    // Enable components based on the board type connected.
    switch (oplinkStatusObj->boardType()) {
    case 0x09: // Revolution, DiscoveryF4Bare, RevoNano, RevoProto
    case 0x92: // Sparky2
        setPortsVisible(false);
        break;
    case 0x03: // OPLinkMini
        setPortsVisible(true);
        break;
    default:
        // This shouldn't happen.
        break;
    }

    if (!statusUpdated) {
        statusUpdated = true;
        // update static info
        updateInfo();
    }
}

void ConfigOPLinkWidget::setPortsVisible(bool visible)
{
    m_oplink->MainPort->setVisible(visible);
    m_oplink->MainPortLabel->setVisible(visible);
    m_oplink->FlexiPort->setVisible(visible);
    m_oplink->FlexiPortLabel->setVisible(visible);
    m_oplink->VCPPort->setVisible(visible);
    m_oplink->VCPPortLabel->setVisible(visible);
}

void ConfigOPLinkWidget::updateInfo()
{
    // qDebug() << "ConfigOPLinkWidget::updateInfo";

    // Update the Description field

    // extract desc into byte array
    OPLinkStatus::DataFields oplinkStatusData = oplinkStatusObj->getData();
    quint8 *desc = oplinkStatusData.Description;
    QByteArray ba;

    for (unsigned int i = 0; i < OPLinkStatus::DESCRIPTION_NUMELEM; i++) {
        ba.append(desc[i]);
    }

    // parse byte array into device descriptor
    deviceDescriptorStruct dds;
    UAVObjectUtilManager::descriptionToStructure(ba, dds);

    // update UI
    if (!dds.gitTag.isEmpty()) {
        m_oplink->FirmwareVersion->setText(dds.gitTag + " " + dds.gitDate);
    } else {
        m_oplink->FirmwareVersion->setText(tr("Unknown"));
    }

    // Update the serial number field
    char buf[OPLinkStatus::CPUSERIAL_NUMELEM * 2 + 1];
    for (unsigned int i = 0; i < OPLinkStatus::CPUSERIAL_NUMELEM; ++i) {
        unsigned char val = oplinkStatusObj->cpuSerial(i) >> 4;
        buf[i * 2]     = ((val < 10) ? '0' : '7') + val;
        val = oplinkStatusObj->cpuSerial(i) & 0xf;
        buf[i * 2 + 1] = ((val < 10) ? '0' : '7') + val;
    }
    buf[OPLinkStatus::CPUSERIAL_NUMELEM * 2] = '\0';
    m_oplink->SerialNumber->setText(buf);
}

void ConfigOPLinkWidget::updateSettings()
{
    // qDebug() << "ConfigOPLinkWidget::updateSettings";

    bool is_enabled     = !isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_DISABLED);
    bool is_coordinator = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKCOORDINATOR);
    bool is_receiver    = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKRECEIVER);
    bool is_openlrs     = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPENLRS);
    bool is_ppm_only    = isComboboxOptionSelected(m_oplink->LinkType, OPLinkSettings::LINKTYPE_CONTROL);
    bool is_bound = (m_oplink->CoordID->text() != "");

    m_oplink->ComSpeed->setEnabled(is_enabled && !is_ppm_only && !is_openlrs);
    m_oplink->CoordID->setEnabled(is_enabled && is_receiver);
    m_oplink->UnbindButton->setEnabled(is_enabled && is_bound && !is_coordinator);
    m_oplink->CustomDeviceID->setEnabled(is_coordinator);

    m_oplink->MinimumChannel->setEnabled(is_receiver || is_coordinator);
    m_oplink->MaximumChannel->setEnabled(is_receiver || is_coordinator);

    enableComboBoxOptionItem(m_oplink->VCPPort, OPLinkSettings::VCPPORT_SERIAL, (is_receiver || is_coordinator));

    if (isComboboxOptionSelected(m_oplink->VCPPort, OPLinkSettings::VCPPORT_SERIAL) && !(is_receiver || is_coordinator)) {
        setComboboxSelectedOption(m_oplink->VCPPort, OPLinkSettings::VCPPORT_DISABLED);
    }

    m_oplink->LinkType->setEnabled(is_enabled && !is_openlrs);
    m_oplink->MaxRFTxPower->setEnabled(is_enabled && !is_openlrs);
}

void ConfigOPLinkWidget::protocolChanged()
{
    updateSettings();
}

void ConfigOPLinkWidget::linkTypeChanged()
{
    updateSettings();
}

void ConfigOPLinkWidget::minChannelChanged()
{
    channelChanged(false);
}

void ConfigOPLinkWidget::maxChannelChanged()
{
    channelChanged(true);
}

void ConfigOPLinkWidget::channelChanged(bool isMax)
{
    int minChannel = m_oplink->MinimumChannel->value();
    int maxChannel = m_oplink->MaximumChannel->value();

    if ((maxChannel - minChannel) < MIN_CHANNEL_RANGE) {
        if (isMax) {
            minChannel = maxChannel - MIN_CHANNEL_RANGE;
        } else {
            maxChannel = minChannel + MIN_CHANNEL_RANGE;
        }

        if (maxChannel > MAX_CHANNEL_NUM) {
            maxChannel = MAX_CHANNEL_NUM;
            minChannel = MAX_CHANNEL_NUM - MIN_CHANNEL_RANGE;
        }

        if (minChannel < 0) {
            minChannel = 0;
            maxChannel = MIN_CHANNEL_RANGE;
        }
    }

    m_oplink->MaximumChannel->setValue(maxChannel);
    m_oplink->MinimumChannel->setValue(minChannel);

    // Calculate and Display frequency in MHz
    float minFrequency = FIRST_FREQUENCY + (minChannel * FREQUENCY_STEP);
    float maxFrequency = FIRST_FREQUENCY + (maxChannel * FREQUENCY_STEP);

    m_oplink->MinFreq->setText("(" + QString::number(minFrequency, 'f', 3) + " MHz)");
    m_oplink->MaxFreq->setText("(" + QString::number(maxFrequency, 'f', 3) + " MHz)");
}

void ConfigOPLinkWidget::mainPortChanged()
{
    switch (getComboboxSelectedOption(m_oplink->MainPort)) {
    case OPLinkSettings::MAINPORT_TELEMETRY:
    case OPLinkSettings::MAINPORT_SERIAL:
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY)
            || isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        }
        break;
    case OPLinkSettings::MAINPORT_COMBRIDGE:
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        }
        break;
    case OPLinkSettings::MAINPORT_PPM:
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_PPM)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        }
        break;
    }
}

void ConfigOPLinkWidget::flexiPortChanged()
{
    switch (getComboboxSelectedOption(m_oplink->FlexiPort)) {
    case OPLinkSettings::FLEXIPORT_TELEMETRY:
    case OPLinkSettings::FLEXIPORT_SERIAL:
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY)
            || isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        }
        break;
    case OPLinkSettings::FLEXIPORT_COMBRIDGE:
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        }
        break;
    case OPLinkSettings::FLEXIPORT_PPM:
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_PPM)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        }
        break;
    }
}

void ConfigOPLinkWidget::vcpPortChanged()
{
    bool vcpComBridgeEnabled = isComboboxOptionSelected(m_oplink->VCPPort, OPLinkSettings::VCPPORT_COMBRIDGE);

    enableComboBoxOptionItem(m_oplink->MainPort, OPLinkSettings::MAINPORT_COMBRIDGE, vcpComBridgeEnabled);
    enableComboBoxOptionItem(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_COMBRIDGE, vcpComBridgeEnabled);

    if (!vcpComBridgeEnabled) {
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_COMBRIDGE)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        }
    }
}

void ConfigOPLinkWidget::unbind()
{
    // Clear the coordinator ID
    oplinkSettingsObj->setCoordID(0);
    m_oplink->CoordID->clear();

    // Clear the OpenLRS settings
    oplinkSettingsObj->setVersion((quint16)0);
    oplinkSettingsObj->setSerialBaudrate(0);
    oplinkSettingsObj->setRFFrequency(0);
    oplinkSettingsObj->setRFPower((quint16)0);
    oplinkSettingsObj->setRFChannelSpacing((quint16)0);
    oplinkSettingsObj->setModemParams((quint16)0);
    oplinkSettingsObj->setFlags((quint16)0);
}
