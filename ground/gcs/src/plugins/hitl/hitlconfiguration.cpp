/**
 ******************************************************************************
 *
 * @file       hitlconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin
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

#include "hitlconfiguration.h"

// Default settings values
#define DFLT_SIMULATOR_ID           ""
#define DFLT_BIN_PATH               ""
#define DFLT_DATA_PATH              ""
#define DFLT_START_SIM              false
#define DFLT_addNoise               false
#define DFLT_ADD_NOISE              false
#define DFLT_HOST_ADDRESS           "127.0.0.1"
#define DFLT_REMOTE_ADDRESS         "127.0.0.1"
#define DFLT_OUT_PORT               0
#define DFLT_IN_PORT                0
#define DFLT_LATITUDE               ""
#define DFLT_LONGITUDE              ""

#define DFLT_ATT_RAW_ENABLED        false
#define DFLT_ATT_RAW_RATE           20

#define DFLT_ATT_STATE_ENABLED      true
#define DFLT_ATT_ACT_HW             false
#define DFLT_ATT_ACT_SIM            true
#define DFLT_ATT_ACT_CALC           false

#define DFLT_BARO_SENSOR_ENABLED    false
#define DFLT_BARO_ALT_RATE          0

#define DFLT_GPS_POSITION_ENABLED   false
#define DFLT_GPS_POS_RATE           100

#define DFLT_GROUND_TRUTH_ENABLED   false
#define DFLT_GROUND_TRUTH_RATE      100

#define DFLT_INPUT_COMMAND          false
#define DFLT_GCS_RECEIVER_ENABLED   false
#define DFLT_MANUAL_CONTROL_ENABLED false
#define DFLT_MIN_INPUT_PERIOD       100

#define DFLT_AIRSPEED_STATE_ENABLED false
#define DFLT_AIRSPEED_STATE_RATE    100

HITLConfiguration::HITLConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    simSettings.simulatorId   = settings.value("simulatorId", DFLT_SIMULATOR_ID).toString();
    simSettings.binPath       = settings.value("binPath", DFLT_BIN_PATH).toString();
    simSettings.dataPath      = settings.value("dataPath", DFLT_DATA_PATH).toString();

    simSettings.hostAddress   = settings.value("hostAddress", DFLT_HOST_ADDRESS).toString();
    simSettings.remoteAddress = settings.value("remoteAddress", DFLT_REMOTE_ADDRESS).toString();
    simSettings.outPort       = settings.value("outPort", DFLT_OUT_PORT).toInt();
    simSettings.inPort = settings.value("inPort", DFLT_IN_PORT).toInt();

    simSettings.latitude      = settings.value("latitude", DFLT_LATITUDE).toString();
    simSettings.longitude     = settings.value("longitude", DFLT_LONGITUDE).toString();
    simSettings.startSim      = settings.value("startSim", DFLT_START_SIM).toBool();
    simSettings.addNoise      = settings.value("noiseCheckBox", DFLT_ADD_NOISE).toBool();

    simSettings.gcsReceiverEnabled   = settings.value("gcsReceiverEnabled", DFLT_GCS_RECEIVER_ENABLED).toBool();
    simSettings.manualControlEnabled = settings.value("manualControlEnabled", DFLT_MANUAL_CONTROL_ENABLED).toBool();

    simSettings.attRawEnabled        = settings.value("attRawEnabled", DFLT_ATT_RAW_ENABLED).toBool();
    simSettings.attRawRate           = settings.value("attRawRate", DFLT_ATT_RAW_RATE).toInt();

    simSettings.attStateEnabled      = settings.value("attStateEnabled", DFLT_ATT_STATE_ENABLED).toBool();
    simSettings.attActHW = settings.value("attActHW", DFLT_ATT_ACT_HW).toBool();
    simSettings.attActSim = settings.value("attActSim", DFLT_ATT_ACT_SIM).toBool();
    simSettings.attActCalc           = settings.value("attActCalc", DFLT_ATT_ACT_CALC).toBool();

    simSettings.baroSensorEnabled    = settings.value("baroSensorEnabled", DFLT_BARO_SENSOR_ENABLED).toBool();
    simSettings.baroAltRate          = settings.value("baroAltRate", DFLT_BARO_ALT_RATE).toInt();

    simSettings.gpsPositionEnabled   = settings.value("gpsPositionEnabled", DFLT_GPS_POSITION_ENABLED).toBool();
    simSettings.gpsPosRate           = settings.value("gpsPosRate", DFLT_GPS_POS_RATE).toInt();

    simSettings.groundTruthEnabled   = settings.value("groundTruthEnabled", DFLT_GROUND_TRUTH_ENABLED).toBool();
    simSettings.groundTruthRate      = settings.value("groundTruthRate", DFLT_GROUND_TRUTH_RATE).toInt();

    simSettings.inputCommand         = settings.value("inputCommand", DFLT_INPUT_COMMAND).toBool();
    simSettings.minOutputPeriod      = settings.value("minOutputPeriod", DFLT_MIN_INPUT_PERIOD).toInt();

    simSettings.airspeedStateEnabled = settings.value("airspeedStateEnabled", DFLT_AIRSPEED_STATE_ENABLED).toBool();
    simSettings.airspeedStateRate    = settings.value("airspeedStateRate", DFLT_AIRSPEED_STATE_RATE).toInt();
}

HITLConfiguration::HITLConfiguration(const HITLConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    simSettings = obj.simSettings;
}

IUAVGadgetConfiguration *HITLConfiguration::clone() const
{
    return new HITLConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void HITLConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("simulatorId", simSettings.simulatorId);
    settings.setValue("binPath", simSettings.binPath);
    settings.setValue("dataPath", simSettings.dataPath);

    settings.setValue("hostAddress", simSettings.hostAddress);
    settings.setValue("remoteAddress", simSettings.remoteAddress);
    settings.setValue("outPort", simSettings.outPort);
    settings.setValue("inPort", simSettings.inPort);

    settings.setValue("latitude", simSettings.latitude);
    settings.setValue("longitude", simSettings.longitude);
    settings.setValue("addNoise", simSettings.addNoise);
    settings.setValue("startSim", simSettings.startSim);

    settings.setValue("gcsReceiverEnabled", simSettings.gcsReceiverEnabled);
    settings.setValue("manualControlEnabled", simSettings.manualControlEnabled);

    settings.setValue("attRawEnabled", simSettings.attRawEnabled);
    settings.setValue("attRawRate", simSettings.attRawRate);
    settings.setValue("attStateEnabled", simSettings.attStateEnabled);
    settings.setValue("attActHW", simSettings.attActHW);
    settings.setValue("attActSim", simSettings.attActSim);
    settings.setValue("attActCalc", simSettings.attActCalc);
    settings.setValue("baroSensorEnabled", simSettings.baroSensorEnabled);
    settings.setValue("baroAltRate", simSettings.baroAltRate);
    settings.setValue("gpsPositionEnabled", simSettings.gpsPositionEnabled);
    settings.setValue("gpsPosRate", simSettings.gpsPosRate);
    settings.setValue("groundTruthEnabled", simSettings.groundTruthEnabled);
    settings.setValue("groundTruthRate", simSettings.groundTruthRate);
    settings.setValue("inputCommand", simSettings.inputCommand);
    settings.setValue("minOutputPeriod", simSettings.minOutputPeriod);

    settings.setValue("airspeedStateEnabled", simSettings.airspeedStateEnabled);
    settings.setValue("airspeedStateRate", simSettings.airspeedStateRate);
}
