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
static const float FREQUENCY_STEP  = 0.040;

ConfigOPLinkWidget::ConfigOPLinkWidget(QWidget *parent) : ConfigTaskWidget(parent, OPLink), statusUpdated(false)
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
    addWidget(m_oplink->ClearDeviceButton);
    addWidget(m_oplink->SignalStrengthBar);
    addWidget(m_oplink->SignalStrengthLabel);

    addWidgetBinding("OPLinkSettings", "Protocol", m_oplink->Protocol);
    addWidgetBinding("OPLinkSettings", "LinkType", m_oplink->LinkType);
    addWidgetBinding("OPLinkSettings", "CoordID", m_oplink->CoordID);
    addWidgetBinding("OPLinkSettings", "CustomDeviceID", m_oplink->CustomDeviceID);
    addWidgetBinding("OPLinkSettings", "RFBand", m_oplink->RFBand);
    addWidgetBinding("OPLinkSettings", "MinChannel", m_oplink->MinimumChannel);
    addWidgetBinding("OPLinkSettings", "MaxChannel", m_oplink->MaximumChannel);
    addWidgetBinding("OPLinkSettings", "MaxRFPower", m_oplink->MaxRFTxPower);
    addWidgetBinding("OPLinkSettings", "MainPort", m_oplink->MainPort);
    addWidgetBinding("OPLinkSettings", "FlexiPort", m_oplink->FlexiPort);
    addWidgetBinding("OPLinkSettings", "PPMOutRSSI", m_oplink->PPMoutRssi);
    addWidgetBinding("OPLinkSettings", "RadioPriStream", m_oplink->RadioPriStream);
    addWidgetBinding("OPLinkSettings", "RadioAuxStream", m_oplink->RadioAuxStream);
    addWidgetBinding("OPLinkSettings", "VCPBridge", m_oplink->VCPBridge);
    addWidgetBinding("OPLinkSettings", "MainComSpeed", m_oplink->MainComSpeed);
    addWidgetBinding("OPLinkSettings", "FlexiComSpeed", m_oplink->FlexiComSpeed);
    addWidgetBinding("OPLinkSettings", "AirDataRate", m_oplink->AirDataRate);

    addWidgetBinding("OPLinkSettings", "RFXtalCap", m_oplink->RFXtalCapValue);
    addWidgetBinding("OPLinkSettings", "RFXtalCap", m_oplink->RFXtalCapSlider);

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
    addWidgetBinding("OPLinkStatus", "AFCCorrection", m_oplink->AFCCorrection);

    // initially hide Oplink Mini options
    setOPLMOptionsVisible(false);

    // Connect the selection changed signals.
    connect(m_oplink->Protocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolChanged()));
    connect(m_oplink->LinkType, SIGNAL(currentIndexChanged(int)), this, SLOT(linkTypeChanged()));
    connect(m_oplink->CustomDeviceID, SIGNAL(textChanged(QString)), this, SLOT(customIDChanged()));
    connect(m_oplink->CoordID, SIGNAL(textChanged(QString)), this, SLOT(coordIDChanged()));
    connect(m_oplink->RFBand, SIGNAL(currentIndexChanged(int)), this, SLOT(rfBandChanged()));
    connect(m_oplink->MinimumChannel, SIGNAL(valueChanged(int)), this, SLOT(minChannelChanged()));
    connect(m_oplink->MaximumChannel, SIGNAL(valueChanged(int)), this, SLOT(maxChannelChanged()));
    connect(m_oplink->MainPort, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged()));
    connect(m_oplink->FlexiPort, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged()));
    connect(m_oplink->RadioPriStream, SIGNAL(currentIndexChanged(int)), this, SLOT(radioPriStreamChanged()));
    connect(m_oplink->RadioAuxStream, SIGNAL(currentIndexChanged(int)), this, SLOT(radioAuxStreamChanged()));
    connect(m_oplink->VCPBridge, SIGNAL(currentIndexChanged(int)), this, SLOT(vcpBridgeChanged()));

    // Connect the Unbind and ClearDevice buttons
    connect(m_oplink->UnbindButton, SIGNAL(released()), this, SLOT(unbind()));
    connect(m_oplink->ClearDeviceButton, SIGNAL(released()), this, SLOT(clearDeviceID()));

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

    m_oplink->SignalStrengthBar->setValue(linkConnected ? m_oplink->RSSI->text().toInt() : -127);
    m_oplink->SignalStrengthLabel->setText(QString("%1dBm").arg(m_oplink->SignalStrengthBar->value()));

    int afc_valueKHz = m_oplink->AFCCorrection->text().toInt() / 1000;
    m_oplink->AFCCorrectionBar->setValue(afc_valueKHz);

    // Enable components based on the board type connected.
    switch (oplinkStatusObj->boardType()) {
    case 0x09: // Revolution, DiscoveryF4Bare, RevoNano, RevoProto
    case 0x92: // Sparky2
        setOPLMOptionsVisible(false);
        break;
    case 0x03: // OPLinkMini
        setOPLMOptionsVisible(true);
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

void ConfigOPLinkWidget::setOPLMOptionsVisible(bool visible)
{
    m_oplink->UartsGroupBox->setVisible(visible);
    m_oplink->ConnectionsGroupBox->setVisible(visible);
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

    bool is_openlrs      = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPENLRS);
    bool is_coordinator  = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKCOORDINATOR);
    bool is_receiver     = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKRECEIVER);
    bool is_oplink = (is_receiver || is_coordinator);
    bool is_ppm_only     = isComboboxOptionSelected(m_oplink->LinkType, OPLinkSettings::LINKTYPE_CONTROL);
    bool is_ppm = isComboboxOptionSelected(m_oplink->LinkType, OPLinkSettings::LINKTYPE_DATAANDCONTROL);
    bool is_main_serial  = isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL);
    bool is_main_telem   = isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY);
    bool is_flexi_serial = isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL);
    bool is_flexi_telem  = isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY);
    bool is_vcp_main     = isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_MAIN);
    bool is_vcp_flexi    = isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_FLEXI);
    bool is_custom_id    = !m_oplink->CustomDeviceID->text().isEmpty();
    bool is_bound = !m_oplink->CoordID->text().isEmpty();

    bool is_stream_main  = isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_MAIN) ||
                           isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_MAIN);
    bool is_stream_flexi = isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_FLEXI) ||
                           isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_FLEXI);

    bool is_flexi_ppm    = isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_PPM);
    bool is_main_ppm     = isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_PPM);

    if (!is_stream_main && !is_vcp_main && (is_main_serial || is_main_telem)) {
        setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        is_main_serial = false;
        is_main_telem  = false;
    }
    if (!is_stream_flexi && !is_vcp_flexi && (is_flexi_serial || is_flexi_telem)) {
        setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        is_flexi_serial = false;
        is_flexi_telem  = false;
    }

    enableComboBoxOptionItem(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY, is_stream_flexi);
    enableComboBoxOptionItem(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL, (is_stream_flexi || is_vcp_flexi));
    enableComboBoxOptionItem(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY, is_stream_main);
    enableComboBoxOptionItem(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL, (is_stream_main || is_vcp_main));

    m_oplink->MainPort->setEnabled(is_oplink || is_vcp_main);
    m_oplink->FlexiPort->setEnabled(is_oplink || is_vcp_flexi);

    m_oplink->PPMoutRssi->setEnabled(is_receiver && (is_ppm || is_ppm_only) && (is_flexi_ppm || is_main_ppm));

    m_oplink->MainComSpeed->setEnabled(is_oplink && !is_ppm_only && !is_vcp_main && (is_main_serial || is_main_telem));
    m_oplink->FlexiComSpeed->setEnabled(is_oplink && !is_ppm_only && !is_vcp_flexi && (is_flexi_serial || is_flexi_telem));
    m_oplink->CoordID->setEnabled(is_receiver || is_openlrs);
    m_oplink->CoordID->setReadOnly(is_openlrs);
    m_oplink->UnbindButton->setEnabled((is_receiver && is_bound) || is_openlrs);

    m_oplink->CustomDeviceID->setEnabled(is_coordinator);
    m_oplink->ClearDeviceButton->setEnabled(is_coordinator && is_custom_id);

    m_oplink->RadioPriStream->setEnabled(is_oplink && !is_ppm_only);
    m_oplink->RadioAuxStream->setEnabled(is_oplink && !is_ppm_only);

    m_oplink->AirDataRate->setEnabled(is_oplink && !is_ppm_only);
    m_oplink->RFBand->setEnabled(is_oplink);
    m_oplink->MinimumChannel->setEnabled(is_oplink);
    m_oplink->MaximumChannel->setEnabled(is_oplink);

    m_oplink->LinkType->setEnabled(is_oplink);
    m_oplink->MaxRFTxPower->setEnabled(is_oplink);
}

void ConfigOPLinkWidget::protocolChanged()
{
    updateSettings();
}

void ConfigOPLinkWidget::linkTypeChanged()
{
    updateSettings();
}

void ConfigOPLinkWidget::customIDChanged()
{
    updateSettings();
}

void ConfigOPLinkWidget::coordIDChanged()
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

void ConfigOPLinkWidget::rfBandChanged()
{
    switch (getComboboxSelectedOption(m_oplink->RFBand)) {
    case OPLinkSettings::RFBAND_915MHZ:
        frequency_base  = 900.0f;
        frequency_step  = FREQUENCY_STEP * 2.0f;
        channel_tooltip = tr("Channel 0 is 900 MHz, channel 250 is 920 MHz, and the channel spacing is 80 KHz.");
        break;
    case OPLinkSettings::RFBAND_868MHZ:
        frequency_base  = 860.0f;
        frequency_step  = FREQUENCY_STEP * 2.0f;
        channel_tooltip = tr("Channel 0 is 860 MHz, channel 250 is 880 MHz, and the channel spacing is 80 KHz.");
        break;
    case OPLinkSettings::RFBAND_433MHZ:
        frequency_base  = 430.0f;
        frequency_step  = FREQUENCY_STEP;
        channel_tooltip = tr("Channel 0 is 430 MHz, channel 250 is 440 MHz, and the channel spacing is 40 KHz.");
        break;
    }

    // Update frequency display according to the RF band change
    updateFrequencyDisplay();
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

    updateFrequencyDisplay();
}

void ConfigOPLinkWidget::updateFrequencyDisplay()
{
    // Calculate and Display frequency in MHz
    float minFrequency = frequency_base + (m_oplink->MinimumChannel->value() * frequency_step);
    float maxFrequency = frequency_base + (m_oplink->MaximumChannel->value() * frequency_step);

    m_oplink->MinFreq->setText("(" + QString::number(minFrequency, 'f', 3) + " MHz)");
    m_oplink->MaxFreq->setText("(" + QString::number(maxFrequency, 'f', 3) + " MHz)");

    m_oplink->MinimumChannel->setToolTip(channel_tooltip);
    m_oplink->MaximumChannel->setToolTip(channel_tooltip);
}

void ConfigOPLinkWidget::mainPortChanged()
{
    switch (getComboboxSelectedOption(m_oplink->MainPort)) {
    case OPLinkSettings::MAINPORT_PPM:
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_PPM)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_DISABLED);
        }
    case OPLinkSettings::MAINPORT_PWM:
    case OPLinkSettings::MAINPORT_DISABLED:
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_MAIN)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        m_oplink->MainComSpeed->setEnabled(false);
        break;
    case OPLinkSettings::MAINPORT_TELEMETRY:
        if (isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL);
        }
    case OPLinkSettings::MAINPORT_SERIAL:
        m_oplink->MainComSpeed->setEnabled(true);
        break;
    default:
        break;
    }
    updateSettings();
}

void ConfigOPLinkWidget::flexiPortChanged()
{
    switch (getComboboxSelectedOption(m_oplink->FlexiPort)) {
    case OPLinkSettings::FLEXIPORT_PPM:
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_PPM)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_DISABLED);
        }
    case OPLinkSettings::FLEXIPORT_PWM:
    case OPLinkSettings::FLEXIPORT_DISABLED:
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_FLEXI)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        m_oplink->FlexiComSpeed->setEnabled(false);
        break;
    case OPLinkSettings::FLEXIPORT_TELEMETRY:
        if (isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL);
        }
    case OPLinkSettings::FLEXIPORT_SERIAL:
        m_oplink->FlexiComSpeed->setEnabled(true);
        break;
    default:
        break;
    }
    updateSettings();
}

void ConfigOPLinkWidget::radioPriStreamChanged()
{
    switch (getComboboxSelectedOption(m_oplink->RadioPriStream)) {
    case OPLinkSettings::RADIOPRISTREAM_DISABLED:
        break;
    case OPLinkSettings::RADIOPRISTREAM_HID:
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_HID)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOPRISTREAM_MAIN:
        if (!isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_MAIN)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOPRISTREAM_FLEXI:
        if (!isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_FLEXI)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOPRISTREAM_VCP:
        if (!isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_VCP)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        break;
    }
    updateSettings();
}

void ConfigOPLinkWidget::radioAuxStreamChanged()
{
    switch (getComboboxSelectedOption(m_oplink->RadioAuxStream)) {
    case OPLinkSettings::RADIOAUXSTREAM_DISABLED:
        break;
    case OPLinkSettings::RADIOAUXSTREAM_HID:
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_HID)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOAUXSTREAM_MAIN:
        if (!isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_MAIN)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOAUXSTREAM_FLEXI:
        if (!isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_FLEXI)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        break;
    case OPLinkSettings::RADIOAUXSTREAM_VCP:
        if (!isComboboxOptionSelected(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED)) {
            setComboboxSelectedOption(m_oplink->VCPBridge, OPLinkSettings::VCPBRIDGE_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_VCP)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        break;
    }
    updateSettings();
}

void ConfigOPLinkWidget::vcpBridgeChanged()
{
    switch (getComboboxSelectedOption(m_oplink->VCPBridge)) {
    case OPLinkSettings::VCPBRIDGE_DISABLED:
        break;
    case OPLinkSettings::VCPBRIDGE_MAIN:
        if (!isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->MainPort, OPLinkSettings::MAINPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_VCP) ||
            isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_VCP) ||
            isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_MAIN)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        break;
    case OPLinkSettings::VCPBRIDGE_FLEXI:
        if (!isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_TELEMETRY) &&
            !isComboboxOptionSelected(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL)) {
            setComboboxSelectedOption(m_oplink->FlexiPort, OPLinkSettings::FLEXIPORT_SERIAL);
        }
        if (isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_VCP) ||
            isComboboxOptionSelected(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioPriStream, OPLinkSettings::RADIOPRISTREAM_DISABLED);
        }
        if (isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_VCP) ||
            isComboboxOptionSelected(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_FLEXI)) {
            setComboboxSelectedOption(m_oplink->RadioAuxStream, OPLinkSettings::RADIOAUXSTREAM_DISABLED);
        }
        break;
    }
    updateSettings();
}

void ConfigOPLinkWidget::unbind()
{
    // Clear the coordinator ID
    m_oplink->CoordID->clear();

    // Clear the OpenLRS settings when needed
    if (isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPENLRS)) {
        QStringList openLRS_settings;
        openLRS_settings << "Version" << "SerialBaudrate" << "ModemParams" << "Flags" \
                         << "RFFrequency" << "RFPower" << "RFChannelSpacing" << "HopChannel";

        for (int i = 0; i < openLRS_settings.size(); ++i) {
            oplinkSettingsObj->getField(openLRS_settings[i])->clear();
        }
    }
}

void ConfigOPLinkWidget::clearDeviceID()
{
    // Clear the OPLM device ID
    m_oplink->CustomDeviceID->clear();
}
