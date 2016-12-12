/**
 ******************************************************************************
 *
 * @file       configcamerastabilizationwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011-2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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

/*
 * IMPORTANT: This module is meant to be a reference implementation which
 * demostrates the use of ConfigTaskWidget API for widgets which are not directly
 * related to UAVObjects. It contains a lot of comments including some commented
 * out code samples. Please DO NOT COPY/PASTE them into any other module based
 * on this.
 */

#include "configcamerastabilizationwidget.h"

#include "ui_camerastabilization.h"

#include "camerastabsettings.h"
#include "hwsettings.h"
#include "mixersettings.h"
#include "actuatorcommand.h"

ConfigCameraStabilizationWidget::ConfigCameraStabilizationWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    ui = new Ui_CameraStabilizationWidget();
    ui->setupUi(this);

    // must be done before auto binding !
    setWikiURL("Camera+Stabilisation+Configuration");

    addAutoBindings();

    disableMouseWheelEvents();

    // These widgets don't have direct relation to UAVObjects
    // and need special processing
    QComboBox *outputs[]  = { ui->rollChannel, ui->pitchChannel, ui->yawChannel, };
    const int NUM_OUTPUTS = sizeof(outputs) / sizeof(outputs[0]);

    // Populate widgets with channel numbers
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i]->clear();
        outputs[i]->addItem("None");
        for (quint32 j = 0; j < ActuatorCommand::CHANNEL_NUMELEM; j++) {
            outputs[i]->addItem(QString("Channel %1").arg(j + 1));
        }
    }

    // Add some widgets to track their UI dirty state and handle smartsave
    addWidget(ui->enableCameraStabilization);
    addWidget(ui->rollChannel);
    addWidget(ui->pitchChannel);
    addWidget(ui->yawChannel);

    // Add some UAVObjects to monitor their changes in addition to autoloaded ones.
    // Note also that we want to reload some UAVObjects by "Reload" button and have
    // to pass corresponding reload group numbers (defined also in objrelation property)
    // to the montitor. We don't reload HwSettings (module enable state) but reload
    // output channels.
    QList<int> reloadGroups;
    reloadGroups << 1;
    addUAVObject("HwSettings");
    addUAVObject("MixerSettings", &reloadGroups);

    // To set special widgets to defaults when requested
    connect(this, SIGNAL(defaultRequested(int)), this, SLOT(defaultRequestedSlot(int)));
}

ConfigCameraStabilizationWidget::~ConfigCameraStabilizationWidget()
{
    // Do nothing
}

/*
 * This overridden function refreshes widgets which have no direct relation
 * to any of UAVObjects. It saves their dirty state first because update comes
 * from UAVObjects, and then restores it.
 */
void ConfigCameraStabilizationWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    // Set module enable checkbox from OptionalModules UAVObject item.
    // It needs special processing because ConfigTaskWidget uses TRUE/FALSE
    // for QCheckBox, but OptionalModules uses Enabled/Disabled enum values.
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    ui->enableCameraStabilization->setChecked(
        hwSettings->getOptionalModules(HwSettings::OPTIONALMODULES_CAMERASTAB) == HwSettings::OPTIONALMODULES_ENABLED);

    // Load mixer outputs which are mapped to camera controls
    MixerSettings *mixerSettings = MixerSettings::GetInstance(getObjectManager());
    MixerSettings::DataFields mixerSettingsData = mixerSettings->getData();

    // TODO: Need to reformat object so types are an
    // array themselves.  This gets really awkward
    quint8 *mixerTypes[] = {
        &mixerSettingsData.Mixer1Type,
        &mixerSettingsData.Mixer2Type,
        &mixerSettingsData.Mixer3Type,
        &mixerSettingsData.Mixer4Type,
        &mixerSettingsData.Mixer5Type,
        &mixerSettingsData.Mixer6Type,
        &mixerSettingsData.Mixer7Type,
        &mixerSettingsData.Mixer8Type,
        &mixerSettingsData.Mixer9Type,
        &mixerSettingsData.Mixer10Type,
    };
    const int NUM_MIXERS = sizeof(mixerTypes) / sizeof(mixerTypes[0]);

    QComboBox *outputs[] = {
        ui->rollChannel,
        ui->pitchChannel,
        ui->yawChannel
    };
    const int NUM_OUTPUTS = sizeof(outputs) / sizeof(outputs[0]);

    for (int i = 0; i < NUM_OUTPUTS; i++) {
        // Default to none if not found.
        // Then search for any mixer channels set to this
        outputs[i]->setCurrentIndex(0);
        for (int j = 0; j < NUM_MIXERS; j++) {
            if (*mixerTypes[j] == (MixerSettings::MIXER1TYPE_CAMERAROLLORSERVO1 + i) &&
                outputs[i]->currentIndex() != (j + 1)) {
                outputs[i]->setCurrentIndex(j + 1);
            }
        }
    }
}

/*
 * This overridden function updates UAVObjects which have no direct relation
 * to any of widgets.
 */
void ConfigCameraStabilizationWidget::updateObjectsFromWidgetsImpl()
{
    // Save state of the module enable checkbox first.
    // Do not use setData() member on whole object, if possible, since it triggers unnecessary UAVObect update.
    quint8 enableModule    = ui->enableCameraStabilization->isChecked() ?
                             HwSettings::OPTIONALMODULES_ENABLED : HwSettings::OPTIONALMODULES_DISABLED;
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    hwSettings->setOptionalModules(HwSettings::OPTIONALMODULES_CAMERASTAB, enableModule);

    // Update mixer channels which were mapped to camera outputs in case they are
    // not used for other function yet
    MixerSettings *mixerSettings = MixerSettings::GetInstance(getObjectManager());
    MixerSettings::DataFields mixerSettingsData = mixerSettings->getData();

    // TODO: Need to reformat object so types are an
    // array themselves.  This gets really awkward
    quint8 *mixerTypes[] = {
        &mixerSettingsData.Mixer1Type,
        &mixerSettingsData.Mixer2Type,
        &mixerSettingsData.Mixer3Type,
        &mixerSettingsData.Mixer4Type,
        &mixerSettingsData.Mixer5Type,
        &mixerSettingsData.Mixer6Type,
        &mixerSettingsData.Mixer7Type,
        &mixerSettingsData.Mixer8Type,
        &mixerSettingsData.Mixer9Type,
        &mixerSettingsData.Mixer10Type,
    };
    const int NUM_MIXERS = sizeof(mixerTypes) / sizeof(mixerTypes[0]);

    QComboBox *outputs[] = {
        ui->rollChannel,
        ui->pitchChannel,
        ui->yawChannel
    };
    const int NUM_OUTPUTS = sizeof(outputs) / sizeof(outputs[0]);

    ui->message->setText("");
    bool widgetUpdated;
    do {
        widgetUpdated = false;

        for (int i = 0; i < NUM_OUTPUTS; i++) {
            // Channel 1 is second entry, so becomes zero
            int mixerNum = outputs[i]->currentIndex() - 1;

            if ((mixerNum >= 0) && // Short circuit in case of none
                (*mixerTypes[mixerNum] != MixerSettings::MIXER1TYPE_DISABLED) &&
                (*mixerTypes[mixerNum] != MixerSettings::MIXER1TYPE_CAMERAROLLORSERVO1 + i)) {
                // If the mixer channel already mapped to something, it should not be
                // used for camera output, we reset it to none
                outputs[i]->setCurrentIndex(0);
                ui->message->setText("One of the channels is already assigned, reverted to none");

                // Loop again or we may have inconsistent widget and UAVObject
                widgetUpdated = true;
            } else {
                // Make sure no other channels have this output set
                for (int j = 0; j < NUM_MIXERS; j++) {
                    if (*mixerTypes[j] == (MixerSettings::MIXER1TYPE_CAMERAROLLORSERVO1 + i)) {
                        *mixerTypes[j] = MixerSettings::MIXER1TYPE_DISABLED;
                    }
                }

                // If this channel is assigned to one of the outputs that is not disabled
                // set it
                if ((mixerNum >= 0) && (mixerNum < NUM_MIXERS)) {
                    *mixerTypes[mixerNum] = MixerSettings::MIXER1TYPE_CAMERAROLLORSERVO1 + i;
                }
            }
        }
    } while (widgetUpdated);

    // FIXME: Should not use setData() to prevent double updates.
    // It should be refactored after the reformatting of MixerSettings UAVObject.
    mixerSettings->setData(mixerSettingsData);
}

/*
 * This slot function is called when "Default" button is clicked.
 * All special widgets which belong to the group passed should be set here.
 */
void ConfigCameraStabilizationWidget::defaultRequestedSlot(int group)
{
    Q_UNUSED(group);

    // For outputs we set them all to none, so don't use any UAVObject to get defaults
    QComboBox *outputs[] = {
        ui->rollChannel,
        ui->pitchChannel,
        ui->yawChannel,
    };
    const int NUM_OUTPUTS = sizeof(outputs) / sizeof(outputs[0]);

    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i]->setCurrentIndex(0);
    }
}
