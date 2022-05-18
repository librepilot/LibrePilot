/**
 ******************************************************************************
 * @addtogroup OpenPilot System OpenPilot System
 * @{
 * @addtogroup OpenPilot Libraries OpenPilot System Libraries
 * @{
 * @file       sanitycheck.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Utilities to validate a flight configuration
 * @see        The GNU Public License (GPL) Version 3
 *
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

#include <openpilot.h>
#include <pios_board_info.h>

// Private includes
#include "inc/sanitycheck.h"

// UAVOs
#include <manualcontrolsettings.h>
#include <flightmodesettings.h>
#include <systemsettings.h>
#include <stabilizationsettings.h>
#include <systemalarms.h>
#include <revosettings.h>
#include <positionstate.h>

// a number of useful macros
#define ADDSEVERITY(check)                                  severity = (severity != SYSTEMALARMS_ALARM_OK ? severity : ((check) ? SYSTEMALARMS_ALARM_OK : SYSTEMALARMS_ALARM_CRITICAL))
#define ADDEXTENDEDALARMSTATUS(error_code, error_substatus) if ((severity != SYSTEMALARMS_ALARM_OK) && (alarmstatus == SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE)) { alarmstatus = (error_code); alarmsubstatus = (error_substatus); }

// private types
typedef struct SANITYCHECK_CustomHookInstance {
    SANITYCHECK_CustomHook_function *hook;
    struct SANITYCHECK_CustomHookInstance *next;
    bool enabled;
} SANITYCHECK_CustomHookInstance;

// ! Check a stabilization mode switch position for safety
static bool check_stabilization_settings(int index, bool multirotor, bool coptercontrol, bool gpsassisted);

SANITYCHECK_CustomHookInstance *hooks = 0;

/**
 * Run a preflight check over the hardware configuration
 * and currently active modules
 */
int32_t configuration_check()
{
    int32_t severity = SYSTEMALARMS_ALARM_OK;
    SystemAlarmsExtendedAlarmStatusOptions alarmstatus = SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE;
    uint8_t alarmsubstatus = 0;
    // Get board type
    const struct pios_board_info *bdinfo = &pios_board_info_blob;
    bool coptercontrol     = bdinfo->board_type == 0x04;

    // Classify navigation capability
#ifdef REVOLUTION
    RevoSettingsFusionAlgorithmOptions revoFusion;
    RevoSettingsFusionAlgorithmGet(&revoFusion);
    bool navCapableFusion;
    switch (revoFusion) {
    case REVOSETTINGS_FUSIONALGORITHM_COMPLEMENTARYMAGGPSOUTDOOR:
    case REVOSETTINGS_FUSIONALGORITHM_GPSNAVIGATIONINS13:
    case REVOSETTINGS_FUSIONALGORITHM_GPSNAVIGATIONINS13CF:
        navCapableFusion = true;
        break;
    default:
        navCapableFusion = false;
        // check for hitl.  hitl allows to feed position and velocity state via
        // telemetry, this makes nav possible even with an unsuited algorithm
        if (PositionStateHandle()) {
            if (PositionStateReadOnly()) {
                navCapableFusion = true;
            }
        }
    }
#else /* ifdef REVOLUTION */
    const bool navCapableFusion = false;
#endif /* ifdef REVOLUTION */


    // Classify airframe type
    bool multirotor = (GetCurrentFrameType() == FRAME_TYPE_MULTIROTOR);


    // For each available flight mode position sanity check the available
    // modes
    uint8_t num_modes;
    FlightModeSettingsFlightModePositionOptions modes[FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_NUMELEM];
    StabilizationSettingsFlightModeAssistMapOptions FlightModeAssistMap[STABILIZATIONSETTINGS_FLIGHTMODEASSISTMAP_NUMELEM];
    ManualControlSettingsFlightModeNumberGet(&num_modes);
    StabilizationSettingsFlightModeAssistMapGet(FlightModeAssistMap);
    FlightModeSettingsFlightModePositionGet(modes);

    for (uint32_t i = 0; i < num_modes; i++) {
        uint8_t gps_assisted = FlightModeAssistMap[i];
        if (gps_assisted) {
            ADDSEVERITY(!coptercontrol);
            ADDSEVERITY(multirotor);
            ADDSEVERITY(navCapableFusion);
        }

        switch ((FlightModeSettingsFlightModePositionOptions)modes[i]) {
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_MANUAL:
            ADDSEVERITY(!gps_assisted);
            ADDSEVERITY(!multirotor);
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED1:
            ADDSEVERITY(check_stabilization_settings(1, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED2:
            ADDSEVERITY(check_stabilization_settings(2, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED3:
            ADDSEVERITY(check_stabilization_settings(3, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED4:
            ADDSEVERITY(check_stabilization_settings(4, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED5:
            ADDSEVERITY(check_stabilization_settings(5, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_STABILIZED6:
            ADDSEVERITY(check_stabilization_settings(6, multirotor, coptercontrol, gps_assisted));
            break;
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_PATHPLANNER:
        {
            ADDSEVERITY(!gps_assisted);
        }
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_POSITIONHOLD:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_VELOCITYROAM:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_LAND:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_AUTOTAKEOFF:
            ADDSEVERITY(!coptercontrol);
            ADDSEVERITY(navCapableFusion);
            break;

        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_COURSELOCK:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_HOMELEASH:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_ABSOLUTEPOSITION:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_POI:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_RETURNTOBASE:
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_AUTOCRUISE:
            ADDSEVERITY(!gps_assisted);
            ADDSEVERITY(!coptercontrol);
            ADDSEVERITY(navCapableFusion);
            break;
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
        case FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_AUTOTUNE:
            ADDSEVERITY(!gps_assisted);
            // it would be fun to try autotune on a fixed wing
            // but that should only be attempted by devs at first
            ADDSEVERITY(multirotor);
            break;
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
        default:
            // Uncovered modes are automatically an error
            ADDSEVERITY(false);
        }
        // mark the first encountered erroneous setting in status and substatus
        if ((severity != SYSTEMALARMS_ALARM_OK) && (alarmstatus == SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE)) {
            alarmstatus    = SYSTEMALARMS_EXTENDEDALARMSTATUS_FLIGHTMODE;
            alarmsubstatus = i;
        }
    }


    // Check throttle/collective channel range for valid configuration of input for critical control
    SystemSettingsThrustControlOptions thrustType;
    SystemSettingsThrustControlGet(&thrustType);
    ManualControlSettingsChannelMinData channelMin;
    ManualControlSettingsChannelMaxData channelMax;
    ManualControlSettingsChannelMinGet(&channelMin);
    ManualControlSettingsChannelMaxGet(&channelMax);
    switch (thrustType) {
    case SYSTEMSETTINGS_THRUSTCONTROL_THROTTLE:
        ADDSEVERITY(fabsf(channelMax.Throttle - channelMin.Throttle) > 300.0f);
        ADDEXTENDEDALARMSTATUS(SYSTEMALARMS_EXTENDEDALARMSTATUS_BADTHROTTLEORCOLLECTIVEINPUTRANGE, 0);
        break;
    case SYSTEMSETTINGS_THRUSTCONTROL_COLLECTIVE:
        ADDSEVERITY(fabsf(channelMax.Collective - channelMin.Collective) > 300.0f);
        ADDEXTENDEDALARMSTATUS(SYSTEMALARMS_EXTENDEDALARMSTATUS_BADTHROTTLEORCOLLECTIVEINPUTRANGE, 0);
        break;
    default:
        break;
    }

    // query sanity check hooks
    if (severity < SYSTEMALARMS_ALARM_CRITICAL) {
        SANITYCHECK_CustomHookInstance *instance = NULL;
        LL_FOREACH(hooks, instance) {
            if (instance->enabled) {
                alarmstatus = instance->hook();
                if (alarmstatus != SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE) {
                    severity = SYSTEMALARMS_ALARM_CRITICAL;
                    break;
                }
            }
        }
    }

    FlightModeSettingsDisableSanityChecksOptions checks_disabled;
    FlightModeSettingsDisableSanityChecksGet(&checks_disabled);
    if (checks_disabled == FLIGHTMODESETTINGS_DISABLESANITYCHECKS_TRUE) {
        severity = SYSTEMALARMS_ALARM_WARNING;
    }

    if (severity != SYSTEMALARMS_ALARM_OK) {
        ExtendedAlarmsSet(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION, severity, alarmstatus, alarmsubstatus);
    } else {
        AlarmsClear(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION);
    }

    return 0;
}

/**
 * Checks the stabilization settings for a particular mode and makes
 * sure it is appropriate for the airframe
 * @param[in] index Which stabilization mode to check
 * @returns true or false
 */
static bool check_stabilization_settings(int index, bool multirotor, bool coptercontrol, bool gpsassisted)
{
    uint8_t modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_NUMELEM];

    // Get the different axis modes for this switch position
    switch (index) {
    case 1:
        FlightModeSettingsStabilization1SettingsArrayGet((FlightModeSettingsStabilization1SettingsOptions *)modes);
        break;
    case 2:
        FlightModeSettingsStabilization2SettingsArrayGet((FlightModeSettingsStabilization2SettingsOptions *)modes);
        break;
    case 3:
        FlightModeSettingsStabilization3SettingsArrayGet((FlightModeSettingsStabilization3SettingsOptions *)modes);
        break;
    case 4:
        FlightModeSettingsStabilization4SettingsArrayGet((FlightModeSettingsStabilization4SettingsOptions *)modes);
        break;
    case 5:
        FlightModeSettingsStabilization5SettingsArrayGet((FlightModeSettingsStabilization5SettingsOptions *)modes);
        break;
    case 6:
        FlightModeSettingsStabilization6SettingsArrayGet((FlightModeSettingsStabilization6SettingsOptions *)modes);
        break;
    default:
        return false;
    }

    // For multirotors verify that roll/pitch/yaw are not set to "none"
    // (why not? might be fun to test ones reactions ;) if you dare, set your frame to "custom"!
    if (multirotor) {
        for (uint32_t i = 0; i < FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST; i++) {
            if (modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_MANUAL) {
                return false;
            }
        }
    }

    if (gpsassisted) {
        // For multirotors verify that roll/pitch are either attitude or rattitude
        for (uint32_t i = 0; i < FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_YAW; i++) {
            if (!(modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ATTITUDE ||
                  modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_RATTITUDE)) {
                return false;
            }
        }
    }


    // coptercontrol cannot do altitude holding
    if (coptercontrol) {
        if (modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEHOLD
            || modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEVARIO
            ) {
            return false;
        }
    }

    // check that thrust modes are only set to thrust axis
    for (uint32_t i = 0; i < FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST; i++) {
        if (modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEHOLD
            || modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEVARIO
            ) {
            return false;
        }
    }
    if (!(modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_MANUAL
          || modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEHOLD
          || modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEVARIO
          || modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_CRUISECONTROL
          )) {
        return false;
    }

    // if cruise control, ensure Acro+ is not set
    if (modes[FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_THRUST] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_CRUISECONTROL) {
        for (uint32_t i = 0; i < FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_YAW; i++) {
            // Do not allow Acro+, attitude estimation is not safe.
            if (modes[i] == FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ACRO) {
                return false;
            }
        }
    }

    // Warning: This assumes that certain conditions in the XML file are met.  That
    // FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_MANUAL has the same numeric value for each channel
    // and is the same for STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL
    // (this is checked at compile time by static constraint manualcontrol.h)


    return true;
}

FrameType_t GetCurrentFrameType()
{
    SystemSettingsAirframeTypeOptions airframe_type;

    SystemSettingsAirframeTypeGet(&airframe_type);
    switch ((SystemSettingsAirframeTypeOptions)airframe_type) {
    case SYSTEMSETTINGS_AIRFRAMETYPE_QUADX:
    case SYSTEMSETTINGS_AIRFRAMETYPE_QUADP:
    case SYSTEMSETTINGS_AIRFRAMETYPE_HEXA:
    case SYSTEMSETTINGS_AIRFRAMETYPE_OCTO:
    case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOX:
    case SYSTEMSETTINGS_AIRFRAMETYPE_HEXAX:
    case SYSTEMSETTINGS_AIRFRAMETYPE_HEXAH:
    case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOV:
    case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOCOAXP:
    case SYSTEMSETTINGS_AIRFRAMETYPE_HEXACOAX:
    case SYSTEMSETTINGS_AIRFRAMETYPE_TRI:
    case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOCOAXX:
        return FRAME_TYPE_MULTIROTOR;

    case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING:
    case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON:
    case SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL:
        return FRAME_TYPE_FIXED_WING;

    case SYSTEMSETTINGS_AIRFRAMETYPE_HELICP:
        return FRAME_TYPE_HELI;

    case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLECAR:
    case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIAL:
    case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEMOTORCYCLE:
    case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEBOAT:
    case SYSTEMSETTINGS_AIRFRAMETYPE_GROUNDVEHICLEDIFFERENTIALBOAT:
        return FRAME_TYPE_GROUND;

    case SYSTEMSETTINGS_AIRFRAMETYPE_VTOL:
    case SYSTEMSETTINGS_AIRFRAMETYPE_CUSTOM:
        return FRAME_TYPE_CUSTOM;
    }
    // anyway it should not reach here
    return FRAME_TYPE_CUSTOM;
}

void SANITYCHECK_AttachHook(SANITYCHECK_CustomHook_function *hook)
{
    PIOS_Assert(hook);
    SANITYCHECK_CustomHookInstance *instance = NULL;

    // Check whether there is an existing instance and enable it
    LL_FOREACH(hooks, instance) {
        if (instance->hook == hook) {
            instance->enabled = true;
            return;
        }
    }

    // No existing instance found, attach this new one
    instance = (SANITYCHECK_CustomHookInstance *)pios_malloc(sizeof(SANITYCHECK_CustomHookInstance));
    PIOS_Assert(instance);
    instance->hook    = hook;
    instance->next    = NULL;
    instance->enabled = true;
    LL_APPEND(hooks, instance);
}

void SANITYCHECK_DetachHook(SANITYCHECK_CustomHook_function *hook)
{
    if (!hooks) {
        return;
    }
    SANITYCHECK_CustomHookInstance *instance = NULL;
    LL_FOREACH(hooks, instance) {
        if (instance->hook == hook) {
            instance->enabled = false;
            return;
        }
    }
}
