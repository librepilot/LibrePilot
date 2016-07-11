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
 * @brief The Configuration Gadget used to configure the OPLink and Revo modem
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

#include <coreplugin/generalsettings.h>
#include <uavobjectmanager.h>

#include <oplinksettings.h>
#include <oplinkstatus.h>

#include <QMessageBox>
#include <QDateTime>

// Channel range and Frequency display
static const int MAX_CHANNEL_NUM   = 250;
static const int MIN_CHANNEL_RANGE = 10;
static const float FIRST_FREQUENCY = 430.000;
static const float FREQUENCY_STEP  = 0.040;

ConfigOPLinkWidget::ConfigOPLinkWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_oplink = new Ui_OPLinkWidget();
    m_oplink->setupUi(this);

    // Connect to the OPLinkStatus object updates
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    oplinkStatusObj = dynamic_cast<UAVDataObject *>(objManager->getObject("OPLinkStatus"));
    Q_ASSERT(oplinkStatusObj);
    connect(oplinkStatusObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateStatus(UAVObject *)));

    // Connect to the OPLinkSettings object updates
    oplinkSettingsObj = dynamic_cast<OPLinkSettings *>(objManager->getObject("OPLinkSettings"));
    Q_ASSERT(oplinkSettingsObj);
    connect(oplinkSettingsObj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateSettings(UAVObject *)));

    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    if (!settings->useExpertMode()) {
        m_oplink->Apply->setVisible(false);
    }
    addApplySaveButtons(m_oplink->Apply, m_oplink->Save);

    addWidgetBinding("OPLinkSettings", "MainPort", m_oplink->MainPort);
    addWidgetBinding("OPLinkSettings", "FlexiPort", m_oplink->FlexiPort);
    addWidgetBinding("OPLinkSettings", "VCPPort", m_oplink->VCPPort);
    addWidgetBinding("OPLinkSettings", "MaxRFPower", m_oplink->MaxRFTxPower);
    addWidgetBinding("OPLinkSettings", "MinChannel", m_oplink->MinimumChannel);
    addWidgetBinding("OPLinkSettings", "MaxChannel", m_oplink->MaximumChannel);
    addWidgetBinding("OPLinkSettings", "CoordID", m_oplink->CoordID);
    addWidgetBinding("OPLinkSettings", "Protocol", m_oplink->Protocol);
    addWidgetBinding("OPLinkSettings", "LinkType", m_oplink->LinkType);
    addWidgetBinding("OPLinkSettings", "ComSpeed", m_oplink->ComSpeed);
    addWidgetBinding("OPLinkSettings", "CustomDeviceID", m_oplink->CustomDeviceID);

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

    // Connect the selection changed signals.
    connect(m_oplink->Protocol, SIGNAL(valueChanged(int)), this, SLOT(protocolChanged()));
    connect(m_oplink->LinkType, SIGNAL(valueChanged(int)), this, SLOT(linkTypeChanged()));
    connect(m_oplink->MinimumChannel, SIGNAL(valueChanged(int)), this, SLOT(minChannelChanged()));
    connect(m_oplink->MaximumChannel, SIGNAL(valueChanged(int)), this, SLOT(maxChannelChanged()));
    connect(m_oplink->CustomDeviceID, SIGNAL(editingFinished()), this, SLOT(updateCustomDeviceID()));

    // Connect the Unbind button
    connect(m_oplink->UnbindButton, SIGNAL(released()), this, SLOT(unbind()));

    m_oplink->CustomDeviceID->setInputMask("HHHHHHHH");
    m_oplink->CoordID->setInputMask("HHHHHHHH");

    m_oplink->MinimumChannel->setKeyboardTracking(false);
    m_oplink->MaximumChannel->setKeyboardTracking(false);

    m_oplink->MaximumChannel->setMaximum(MAX_CHANNEL_NUM);
    m_oplink->MinimumChannel->setMaximum(MAX_CHANNEL_NUM - MIN_CHANNEL_RANGE);

    // Request and update of the setting object.
    settingsUpdated = false;
    setWikiURL("OPLink+Configuration");
    autoLoadWidgets();
    disableMouseWheelEvents();
    updateEnableControls();
}

ConfigOPLinkWidget::~ConfigOPLinkWidget()
{}

/*!
   \brief Called by updates to @OPLinkStatus
 */
void ConfigOPLinkWidget::updateStatus(UAVObject *object)
{
    // Request and update of the setting object if we haven't received it yet.
    if (!settingsUpdated) {
        oplinkSettingsObj->requestUpdate();
    }

    // Update the link state
    UAVObjectField *linkField = object->getField("LinkState");
    m_oplink->LinkState->setText(linkField->getValue().toString());
    bool linkConnected = (linkField->getValue() == linkField->getOptions().at(OPLinkStatus::LINKSTATE_CONNECTED));

    m_oplink->PairSignalStrengthBar1->setValue(linkConnected ? m_oplink->RSSI->text().toInt() : -127);
    m_oplink->PairSignalStrengthLabel1->setText(QString("%1dB").arg(m_oplink->PairSignalStrengthBar1->value()));

    // Update the Description field
    // TODO use  UAVObjectUtilManager::descriptionToStructure()
    UAVObjectField *descField = object->getField("Description");
    if (descField->getValue(0) != QChar(255)) {
        /*
         * This looks like a binary with a description at the end:
         *   4 bytes: header: "OpFw".
         *   4 bytes: GIT commit tag (short version of SHA1).
         *   4 bytes: Unix timestamp of compile time.
         *   2 bytes: target platform. Should follow same rule as BOARD_TYPE and BOARD_REVISION in board define files.
         *  26 bytes: commit tag if it is there, otherwise branch name. '-dirty' may be added if needed. Zero-padded.
         *  20 bytes: SHA1 sum of the firmware.
         *  20 bytes: SHA1 sum of the uavo definitions.
         *  20 bytes: free for now.
         */
        char buf[OPLinkStatus::DESCRIPTION_NUMELEM];
        for (unsigned int i = 0; i < 26; ++i) {
            buf[i] = descField->getValue(i + 14).toChar().toLatin1();
        }
        buf[26] = '\0';
        QString descstr(buf);
        quint32 gitDate = descField->getValue(11).toChar().toLatin1() & 0xFF;
        for (int i = 1; i < 4; i++) {
            gitDate  = gitDate << 8;
            gitDate += descField->getValue(11 - i).toChar().toLatin1() & 0xFF;
        }
        QString date = QDateTime::fromTime_t(gitDate).toUTC().toString("yyyy-MM-dd HH:mm");
        m_oplink->FirmwareVersion->setText(descstr + " " + date);
    } else {
        m_oplink->FirmwareVersion->setText(tr("Unknown"));
    }

    // Update the serial number field
    UAVObjectField *serialField = object->getField("CPUSerial");
    char buf[OPLinkStatus::CPUSERIAL_NUMELEM * 2 + 1];
    for (unsigned int i = 0; i < OPLinkStatus::CPUSERIAL_NUMELEM; ++i) {
        unsigned char val = serialField->getValue(i).toUInt() >> 4;
        buf[i * 2]     = ((val < 10) ? '0' : '7') + val;
        val = serialField->getValue(i).toUInt() & 0xf;
        buf[i * 2 + 1] = ((val < 10) ? '0' : '7') + val;
    }
    buf[OPLinkStatus::CPUSERIAL_NUMELEM * 2] = '\0';
    m_oplink->SerialNumber->setText(buf);

    updateEnableControls();
}

/*!
   \brief Called by updates to @OPLinkSettings
 */
void ConfigOPLinkWidget::updateSettings(UAVObject *object)
{
    Q_UNUSED(object);

    if (!settingsUpdated) {
        settingsUpdated = true;
        // Enable components based on the board type connected.
        UAVObjectField *board_type_field = oplinkStatusObj->getField("BoardType");
        switch (board_type_field->getValue().toInt()) {
        case 0x09: // Revolution, DiscoveryF4Bare, RevoNano, RevoProto
        case 0x92: // Sparky2
            m_oplink->MainPort->setVisible(false);
            m_oplink->MainPortLabel->setVisible(false);
            m_oplink->FlexiPort->setVisible(false);
            m_oplink->FlexiPortLabel->setVisible(false);
            m_oplink->VCPPort->setVisible(false);
            m_oplink->VCPPortLabel->setVisible(false);
            m_oplink->LinkType->setEnabled(true);
            break;
        case 0x03: // OPLinkMini
            m_oplink->MainPort->setVisible(true);
            m_oplink->MainPortLabel->setVisible(true);
            m_oplink->FlexiPort->setVisible(true);
            m_oplink->FlexiPortLabel->setVisible(true);
            m_oplink->VCPPort->setVisible(true);
            m_oplink->VCPPortLabel->setVisible(true);
            m_oplink->LinkType->setEnabled(true);
            break;
        default:
            // This shouldn't happen.
            break;
        }
        updateEnableControls();
    }
}

void ConfigOPLinkWidget::updateEnableControls()
{
    enableControls(true);
    updateCustomDeviceID();
    updateCoordID();
    protocolChanged();
}

void ConfigOPLinkWidget::disconnected()
{
    if (settingsUpdated) {
        settingsUpdated = false;
    }
}

void ConfigOPLinkWidget::protocolChanged()
{
    bool is_enabled     = !isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_DISABLED);
    bool is_coordinator = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKCOORDINATOR);
    bool is_receiver    = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPLINKRECEIVER);
    bool is_openlrs     = isComboboxOptionSelected(m_oplink->Protocol, OPLinkSettings::PROTOCOL_OPENLRS);
    bool is_ppm_only    = isComboboxOptionSelected(m_oplink->LinkType, OPLinkSettings::LINKTYPE_CONTROL);
    bool is_oplm  = m_oplink->MainPort->isVisible();
    bool is_bound = (m_oplink->CoordID->text() != "");

    m_oplink->ComSpeed->setEnabled(!is_ppm_only && !is_openlrs && is_enabled);
    m_oplink->CoordID->setEnabled(is_receiver & is_enabled);
    m_oplink->UnbindButton->setEnabled(is_bound && !is_coordinator && is_enabled);
    m_oplink->CustomDeviceID->setEnabled(is_coordinator);
    m_oplink->MinimumChannel->setEnabled(is_receiver || is_coordinator);
    m_oplink->MaximumChannel->setEnabled(is_receiver || is_coordinator);
    m_oplink->MainPort->setEnabled(is_oplm);
    m_oplink->FlexiPort->setEnabled(is_oplm);
    m_oplink->VCPPort->setEnabled((is_receiver || is_coordinator) && is_oplm);
    m_oplink->LinkType->setEnabled(is_enabled && !is_openlrs);
    m_oplink->MaxRFTxPower->setEnabled(is_enabled && !is_openlrs);
}

void ConfigOPLinkWidget::linkTypeChanged()
{
    protocolChanged();
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

void ConfigOPLinkWidget::updateCoordID()
{
    bool coordinatorNotSet = (m_oplink->CoordID->text() == "0");

    if (settingsUpdated && coordinatorNotSet) {
        m_oplink->CoordID->clear();
    }
}

void ConfigOPLinkWidget::updateCustomDeviceID()
{
    bool customDeviceIDNotSet = (m_oplink->CustomDeviceID->text() == "0");

    if (settingsUpdated && customDeviceIDNotSet) {
        m_oplink->CustomDeviceID->clear();
        m_oplink->CustomDeviceID->setPlaceholderText("AutoGen");
    }
}

void ConfigOPLinkWidget::unbind()
{
    if (settingsUpdated) {
        // Clear the coordinator ID
        oplinkSettingsObj->getField("CoordID")->setValue(0);
        m_oplink->CoordID->setText("0");

        // Clear the OpenLRS settings
        oplinkSettingsObj->getField("Version")->setValue(0);
        oplinkSettingsObj->getField("SerialBaudrate")->setValue(0);
        oplinkSettingsObj->getField("RFFrequency")->setValue(0);
        oplinkSettingsObj->getField("RFPower")->setValue(0);
        oplinkSettingsObj->getField("RFChannelSpacing")->setValue(0);
        oplinkSettingsObj->getField("ModemParams")->setValue(0);
        oplinkSettingsObj->getField("Flags")->setValue(0);
    }
}

/**
   @}
   @}
 */
