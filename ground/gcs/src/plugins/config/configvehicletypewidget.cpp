/**
 ******************************************************************************
 *
 * @file       configvehicletypewidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             E. Lafargue, K. Sebesta & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Airframe configuration panel
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
#include "configvehicletypewidget.h"

#include "ui_airframe.h"

#include "configgadgetfactory.h"
#include "uavobjectmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "systemsettings.h"
#include "actuatorsettings.h"

#include "cfg_vehicletypes/configccpmwidget.h"
#include "cfg_vehicletypes/configfixedwingwidget.h"
#include "cfg_vehicletypes/configgroundvehiclewidget.h"
#include "cfg_vehicletypes/configmultirotorwidget.h"
#include "cfg_vehicletypes/configcustomwidget.h"

#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QTextEdit>
#include <math.h>

/**
   Static function to get currently assigned channelDescriptions
   for all known vehicle types;  instantiates the appropriate object
   then asks it to supply channel descs
 */
QStringList ConfigVehicleTypeWidget::getChannelDescriptions()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objMngr = pm->getObject<UAVObjectManager>();

    Q_ASSERT(objMngr);

    // get an instance of systemsettings
    SystemSettings *systemSettings = SystemSettings::GetInstance(objMngr);
    Q_ASSERT(systemSettings);
    SystemSettings::DataFields systemSettingsData = systemSettings->getData();

    QStringList channelDesc;
    switch (systemSettingsData.AirframeType) {
    case SystemSettings::AIRFRAMETYPE_FIXEDWING:
        channelDesc = ConfigFixedWingWidget::getChannelDescriptions();
        break;
    case SystemSettings::AIRFRAMETYPE_FIXEDWINGELEVON:
        channelDesc = ConfigFixedWingWidget::getChannelDescriptions();
        break;
    case SystemSettings::AIRFRAMETYPE_FIXEDWINGVTAIL:
        channelDesc = ConfigFixedWingWidget::getChannelDescriptions();
        break;
    case SystemSettings::AIRFRAMETYPE_HELICP:
        // helicp
        channelDesc = ConfigCcpmWidget::getChannelDescriptions();
        break;
    case SystemSettings::AIRFRAMETYPE_VTOL:
    case SystemSettings::AIRFRAMETYPE_TRI:
    case SystemSettings::AIRFRAMETYPE_QUADX:
    case SystemSettings::AIRFRAMETYPE_QUADP:
    case SystemSettings::AIRFRAMETYPE_OCTOV:
    case SystemSettings::AIRFRAMETYPE_OCTOCOAXX:
    case SystemSettings::AIRFRAMETYPE_OCTOCOAXP:
    case SystemSettings::AIRFRAMETYPE_OCTO:
    case SystemSettings::AIRFRAMETYPE_OCTOX:
    case SystemSettings::AIRFRAMETYPE_HEXAX:
    case SystemSettings::AIRFRAMETYPE_HEXACOAX:
    case SystemSettings::AIRFRAMETYPE_HEXA:
    case SystemSettings::AIRFRAMETYPE_HEXAH:
        // multirotor
        channelDesc = ConfigMultiRotorWidget::getChannelDescriptions();
        break;
    case SystemSettings::AIRFRAMETYPE_GROUNDVEHICLECAR:
    case SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIAL:
    case SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEMOTORCYCLE:
    case SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEBOAT:
    case SystemSettings::AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIALBOAT:
        // ground
        channelDesc = ConfigGroundVehicleWidget::getChannelDescriptions();
        break;
    default:
        channelDesc = ConfigCustomWidget::getChannelDescriptions();
        break;
    }

    return channelDesc;
}

ConfigVehicleTypeWidget::ConfigVehicleTypeWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_aircraft = new Ui_AircraftWidget();
    m_aircraft->setupUi(this);

    // must be done before auto binding !
    setWikiURL("Vehicle+Configuration");

    addAutoBindings();

    disableMouseWheelEvents();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ConfigGadgetFactory *configGadgetFactory = pm->getObject<ConfigGadgetFactory>();
    connect(m_aircraft->vehicleSetupWizardButton, SIGNAL(clicked()), configGadgetFactory, SIGNAL(onOpenVehicleConfigurationWizard()));

    SystemSettings *syssettings = SystemSettings::GetInstance(getObjectManager());
    Q_ASSERT(syssettings);
    m_aircraft->nameEdit->setMaxLength(syssettings->VEHICLENAME_NUMELEM);

    addUAVObject("SystemSettings");
    addUAVObject("MixerSettings");
    addUAVObject("ActuatorSettings");

    addWidget(m_aircraft->nameEdit);

    // The order of the tabs is important since they correspond with the AirframeCategory enum
    m_aircraft->aircraftType->addTab(tr("Multirotor"));
    m_aircraft->aircraftType->addTab(tr("Fixed Wing"));
    m_aircraft->aircraftType->addTab(tr("Helicopter"));
    m_aircraft->aircraftType->addTab(tr("Ground"));
    m_aircraft->aircraftType->addTab(tr("Custom"));

    // Connect aircraft type selection dropbox to callback function
    connect(m_aircraft->aircraftType, SIGNAL(currentChanged(int)), this, SLOT(frameTypeChanged(int)));
}

/**
   Destructor
 */
ConfigVehicleTypeWidget::~ConfigVehicleTypeWidget()
{
    // Do nothing
}

void ConfigVehicleTypeWidget::frameTypeChanged(int index)
{
    VehicleConfig *vehicleConfig = getVehicleConfigWidget(index);

    if (vehicleConfig) {
        // switch tab
        m_aircraft->airframesWidget->setCurrentWidget(vehicleConfig);

        // enable controls
        enableControls(isConnected());

        // flag vehicle config as dirty (frame type was changed...)
        setDirty(true);
    }
}

void ConfigVehicleTypeWidget::enableControls(bool enable)
{
    ConfigTaskWidget::enableControls(enable);

    // forward call to selected vehicle config
    VehicleConfig *vehicleConfig = (VehicleConfig *)m_aircraft->airframesWidget->currentWidget();

    if (vehicleConfig) {
        vehicleConfig->enableControls(enable);
    }
}

/**
   Refreshes the current value of the SystemSettings which holds the aircraft type.
   Note: no widgets are bound so the default behavior of ConfigTaskWidget will not do much.
   Almost everything is handled here to the exception of one case (see ConfigCustomWidget...)
 */
void ConfigVehicleTypeWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    if (obj) {
        // single object was updated, skip...
        return;
    }

    // update current vehicle tab
    QString ft   = frameType();
    int category = frameCategory(ft);
    m_aircraft->aircraftType->setCurrentIndex(category);
    VehicleConfig *vehicleConfig = getVehicleConfigWidget(category);
    if (vehicleConfig) {
        vehicleConfig->setupUI(ft);
        vehicleConfig->refreshWidgetsValuesImpl(obj);
    }

    // update custom tab from others frame settings (to debug/learn hard coded mixers)
    if (ft != "Custom") {
        int customCategory = frameCategory("Custom");
        VehicleConfig *vehicleConfig = getVehicleConfigWidget(customCategory);
        if (vehicleConfig) {
            vehicleConfig->setupUI("Custom");
            vehicleConfig->refreshWidgetsValuesImpl(obj);
        }
    }

    // update vehicle name
    QString name = vehicleName();
    m_aircraft->nameEdit->setText(name);
}

/**
   Sends the config to the board (airframe type)

   We do all the tasks common to all airframes, or family of airframes, and
   we call additional methods for specific frames, so that we do not have a code
   that is too heavy.
 */
void ConfigVehicleTypeWidget::updateObjectsFromWidgetsImpl()
{
    VehicleConfig *vehicleConfig = (VehicleConfig *)m_aircraft->airframesWidget->currentWidget();

    // frame type defaults to Custom
    QString ft = "Custom";

    if (vehicleConfig) {
        vehicleConfig->updateObjectsFromWidgetsImpl();
        ft = vehicleConfig->getFrameType();
    }

    // save frame type
    setFrameType(ft);

    // save vehicle name
    QString name = m_aircraft->nameEdit->text();
    setVehicleName(name);

    // update custom tab from others frame settings (to debug/learn hard coded mixers)
    if (ft != "Custom") {
        int customCategory = frameCategory("Custom");
        VehicleConfig *vehicleConfig = getVehicleConfigWidget(customCategory);
        if (vehicleConfig) {
            vehicleConfig->setupUI("Custom");
            vehicleConfig->refreshWidgetsValuesImpl(NULL);
        }
    }
}

QString ConfigVehicleTypeWidget::frameType()
{
    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));

    Q_ASSERT(system);

    UAVObjectField *field = system->getField("AirframeType");
    Q_ASSERT(field);

    return field->getValue().toString();
}

void ConfigVehicleTypeWidget::setFrameType(QString frameType)
{
    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));

    Q_ASSERT(system);

    UAVObjectField *field = system->getField("AirframeType");
    Q_ASSERT(field);

    if (field) {
        field->setValue(frameType);
    }
}

QString ConfigVehicleTypeWidget::vehicleName()
{
    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));

    Q_ASSERT(system);

    UAVObjectField *field = system->getField("VehicleName");
    Q_ASSERT(field);

    QString name;
    for (uint i = 0; i < field->getNumElements(); ++i) {
        QVariant value = field->getValue(i);
        if (value == 0) {
            break;
        }
        name.append(value.toChar());
    }
    return name;
}

void ConfigVehicleTypeWidget::setVehicleName(QString name)
{
    UAVDataObject *system = dynamic_cast<UAVDataObject *>(getObjectManager()->getObject("SystemSettings"));

    Q_ASSERT(system);

    UAVObjectField *field = system->getField("VehicleName");
    Q_ASSERT(field);

    for (uint i = 0; i < field->getNumElements(); ++i) {
        if (i < (uint)name.length()) {
            field->setValue(name.at(i).toLatin1(), i);
        } else {
            field->setValue(0, i);
        }
    }
}

int ConfigVehicleTypeWidget::frameCategory(QString frameType)
{
    if (frameType == "FixedWing" || frameType == "Aileron" || frameType == "FixedWingElevon"
        || frameType == "Elevon" || frameType == "FixedWingVtail" || frameType == "Vtail") {
        return ConfigVehicleTypeWidget::FIXED_WING;
    } else if (frameType == "Tri" || frameType == "Tricopter Y" || frameType == "QuadX" || frameType == "Quad X"
               || frameType == "QuadP" || frameType == "Quad +" || frameType == "Hexa" || frameType == "Hexacopter"
               || frameType == "HexaX" || frameType == "Hexacopter X" || frameType == "HexaCoax"
               || frameType == "HexaH" || frameType == "Hexacopter H" || frameType == "Hexacopter Y6"
               || frameType == "Octo" || frameType == "Octocopter"
               || frameType == "OctoX" || frameType == "Octocopter X"
               || frameType == "OctoV" || frameType == "Octocopter V"
               || frameType == "OctoCoaxP" || frameType == "Octo Coax +"
               || frameType == "OctoCoaxX" || frameType == "Octo Coax X") {
        return ConfigVehicleTypeWidget::MULTIROTOR;
    } else if (frameType == "HeliCP") {
        return ConfigVehicleTypeWidget::HELICOPTER;
    } else if (frameType == "GroundVehicleCar" || frameType == "Car (Turnable)"
               || frameType == "GroundVehicleDifferential" || frameType == "Tank (Differential)"
               || frameType == "GroundVehicleMotorcycle" || frameType == "Motorcycle"
               || frameType == "GroundVehicleBoat" || frameType == "Boat (Turnable)"
               || frameType == "GroundVehicleDifferentialBoat" || frameType == "Boat (Differential)") {
        return ConfigVehicleTypeWidget::GROUND;
    } else {
        return ConfigVehicleTypeWidget::CUSTOM;
    }
}

VehicleConfig *ConfigVehicleTypeWidget::getVehicleConfigWidget(int frameCategory)
{
    VehicleConfig *vehicleConfig;

    if (!m_vehicleIndexMap.contains(frameCategory)) {
        // create config widget
        vehicleConfig = createVehicleConfigWidget(frameCategory);
    }
    int index = m_vehicleIndexMap.value(frameCategory);
    vehicleConfig = (VehicleConfig *)m_aircraft->airframesWidget->widget(index);
    return vehicleConfig;
}

VehicleConfig *ConfigVehicleTypeWidget::createVehicleConfigWidget(int frameCategory)
{
    VehicleConfig *vehicleConfig;

    switch (frameCategory) {
    case ConfigVehicleTypeWidget::FIXED_WING:
        vehicleConfig = new ConfigFixedWingWidget();
        break;
    case ConfigVehicleTypeWidget::MULTIROTOR:
        vehicleConfig = new ConfigMultiRotorWidget();
        break;
    case ConfigVehicleTypeWidget::HELICOPTER:
        vehicleConfig = new ConfigCcpmWidget();
        break;
    case ConfigVehicleTypeWidget::GROUND:
        vehicleConfig = new ConfigGroundVehicleWidget();
        break;
    case ConfigVehicleTypeWidget::CUSTOM:
        vehicleConfig = new ConfigCustomWidget();
        break;
    default:
        vehicleConfig = NULL;
        break;
    }
    if (vehicleConfig) {
        // bind config widget "field" to this ConfigTaskWidget
        // this is necessary to get "dirty" state management
        vehicleConfig->registerWidgets(*this);

        // add config widget to UI
        int index = m_aircraft->airframesWidget->insertWidget(m_aircraft->airframesWidget->count(), vehicleConfig);
        m_vehicleIndexMap[frameCategory] = index;
    }
    return vehicleConfig;
}
