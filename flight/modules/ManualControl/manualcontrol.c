/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ManualControlModule Manual Control Module
 * @brief Provide manual control or allow it alter flight mode.
 * @{
 *
 * Reads in the ManualControlCommand FlightMode setting from receiver then either
 * pass the settings straght to ActuatorDesired object (manual mode) or to
 * AttitudeDesired object (stabilized mode)
 *
 * @file       manualcontrol.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @brief      ManualControl module. Handles safety R/C link and flight mode.
 *
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

#include "inc/manualcontrol.h"
#include <sanitycheck.h>
#include <manualcontrolsettings.h>
#include <manualcontrolcommand.h>
#include <accessorydesired.h>
#include <vtolselftuningstats.h>
#include <flightmodesettings.h>
#include <flightstatus.h>
#include <systemsettings.h>
#include <stabilizationdesired.h>
#include <callbackinfo.h>
#include <stabilizationsettings.h>
#include <systemalarms.h>
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
#include <vtolpathfollowersettings.h>
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */

// Private constants
#if defined(PIOS_MANUAL_STACK_SIZE)
#define STACK_SIZE_BYTES                               PIOS_MANUAL_STACK_SIZE
#else
#define STACK_SIZE_BYTES                               1152
#endif

#define CALLBACK_PRIORITY                              CALLBACK_PRIORITY_REGULAR
#define CBTASK_PRIORITY                                CALLBACK_TASK_FLIGHTCONTROL

#define ASSISTEDCONTROL_NEUTRALTHROTTLERANGE_FACTOR    0.2f
#define ASSISTEDCONTROL_BRAKETHRUST_DEADBAND_FACTOR_LO 0.96f
#define ASSISTEDCONTROL_BRAKETHRUST_DEADBAND_FACTOR_HI 1.04f

#define ALWAYSTABILIZEACCESSORY_THRESHOLD              0.05f

// defined handlers

static const controlHandler handler_MANUAL = {
    .controlChain      = {
        .Stabilization = false,
        .PathFollower  = false,
        .PathPlanner   = false,
    },
    .handler           = &manualHandler,
};
static const controlHandler handler_STABILIZED = {
    .controlChain      = {
        .Stabilization = true,
        .PathFollower  = false,
        .PathPlanner   = false,
    },
    .handler           = &stabilizedHandler,
};


#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
static const controlHandler handler_PATHFOLLOWER = {
    .controlChain      = {
        .Stabilization = true,
        .PathFollower  = true,
        .PathPlanner   = false,
    },
    .handler           = &pathFollowerHandler,
};

static const controlHandler handler_PATHPLANNER = {
    .controlChain      = {
        .Stabilization = true,
        .PathFollower  = true,
        .PathPlanner   = true,
    },
    .handler           = &pathPlannerHandler,
};
static float thrustAtBrakeStart = 0.0f;
static float thrustLo = 0.0f;
static float thrustHi = 0.0f;

#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
// Private variables
static DelayedCallbackInfo *callbackHandle;
static FrameType_t frameType = FRAME_TYPE_MULTIROTOR;

// Private functions
static void configurationUpdatedCb(UAVObjEvent *ev);
static void commandUpdatedCb(UAVObjEvent *ev);
static void manualControlTask(void);
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
static uint8_t isAssistedFlightMode(uint8_t position, uint8_t flightMode, FlightModeSettingsData *modeSettings);
static void HandleBatteryFailsafe(uint8_t *position, FlightModeSettingsData *modeSettings);
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
static void SettingsUpdatedCb(UAVObjEvent *ev);
#define assumptions (assumptions1 && assumptions2 && assumptions3 && assumptions4 && assumptions5 && assumptions6 && assumptions7 && assumptions_flightmode)

/**
 * Module starting
 */
int32_t ManualControlStart()
{
    // Whenever the configuration changes, make sure it is safe to fly
    SystemSettingsConnectCallback(configurationUpdatedCb);
    ManualControlSettingsConnectCallback(configurationUpdatedCb);
    FlightModeSettingsConnectCallback(configurationUpdatedCb);
    ManualControlCommandConnectCallback(commandUpdatedCb);

    // Run this initially to make sure the configuration is checked
    configuration_check();

    // clear alarms
    AlarmsClear(SYSTEMALARMS_ALARM_MANUALCONTROL);

    SettingsUpdatedCb(NULL);

    // Make sure unarmed on power up
    armHandler(true, frameType);

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    takeOffLocationHandlerInit();
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */

    // Start main task
    PIOS_CALLBACKSCHEDULER_Dispatch(callbackHandle);

    return 0;
}

/**
 * Module initialization
 */
int32_t ManualControlInitialize()
{
    /* Check the assumptions about uavobject enum's are correct */
    PIOS_STATIC_ASSERT(assumptions);

    ManualControlCommandInitialize();
    FlightStatusInitialize();
    AccessoryDesiredInitialize();
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    SystemAlarmsInitialize();
    VtolSelfTuningStatsInitialize();
    VtolPathFollowerSettingsConnectCallback(&SettingsUpdatedCb);
    SystemSettingsConnectCallback(&SettingsUpdatedCb);
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
    callbackHandle = PIOS_CALLBACKSCHEDULER_Create(&manualControlTask, CALLBACK_PRIORITY, CBTASK_PRIORITY, CALLBACKINFO_RUNNING_MANUALCONTROL, STACK_SIZE_BYTES);

    return 0;
}
MODULE_INITCALL(ManualControlInitialize, ManualControlStart);

static void SettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    frameType = GetCurrentFrameType();
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    VtolPathFollowerSettingsTreatCustomCraftAsOptions TreatCustomCraftAs;
    VtolPathFollowerSettingsTreatCustomCraftAsGet(&TreatCustomCraftAs);


    if (frameType == FRAME_TYPE_CUSTOM) {
        switch (TreatCustomCraftAs) {
        case VTOLPATHFOLLOWERSETTINGS_TREATCUSTOMCRAFTAS_FIXEDWING:
            frameType = FRAME_TYPE_FIXED_WING;
            break;
        case VTOLPATHFOLLOWERSETTINGS_TREATCUSTOMCRAFTAS_VTOL:
            frameType = FRAME_TYPE_MULTIROTOR;
            break;
        case VTOLPATHFOLLOWERSETTINGS_TREATCUSTOMCRAFTAS_GROUND:
            frameType = FRAME_TYPE_GROUND;
            break;
        }
    }
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
}

/**
 * Module task
 */
static void manualControlTask(void)
{
    // Process Arming
    armHandler(false, frameType);
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    takeOffLocationHandler();
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
    // Process flight mode
    FlightStatusData flightStatus;

    FlightStatusGet(&flightStatus);
    ManualControlCommandData cmd;
    ManualControlCommandGet(&cmd);
    AccessoryDesiredData acc;
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    VtolPathFollowerSettingsThrustLimitsData thrustLimits;
    VtolPathFollowerSettingsThrustLimitsGet(&thrustLimits);
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */

    FlightModeSettingsData modeSettings;
    FlightModeSettingsGet(&modeSettings);

    static uint8_t lastPosition      = 0;
    uint8_t position = cmd.FlightModeSwitchPosition;
    uint8_t newMode = flightStatus.FlightMode;
    uint8_t newAlwaysStabilized      = flightStatus.AlwaysStabilizeWhenArmed;
    uint8_t newFlightModeAssist      = flightStatus.FlightModeAssist;
    uint8_t newAssistedControlState  = flightStatus.AssistedControlState;
    uint8_t newAssistedThrottleState = flightStatus.AssistedThrottleState;

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    HandleBatteryFailsafe(&position, &modeSettings);
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */

    if (position < FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_NUMELEM) {
        newMode = modeSettings.FlightModePosition[position];
    }

    // Ignore change to AutoTakeOff and keep last flight mode position
    // if vehicle is already armed and maybe in air...
    if ((newMode == FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF) && flightStatus.Armed) {
        newMode  = flightStatus.FlightMode;
        position = lastPosition;
    }
    // if a mode change occurs we default the assist mode and states here
    // to avoid having to add it to all of the below modes that are
    // otherwise unrelated
    if (newMode != flightStatus.FlightMode) {
        // set assist mode to none to avoid an assisted flight mode position
        // carrying over and impacting a newly selected non-assisted flight mode pos
        newFlightModeAssist      = FLIGHTSTATUS_FLIGHTMODEASSIST_NONE;
        // The following are equivalent to none effectively. Code should always
        // check the flightmodeassist state.
        newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
    }

    // Depending on the mode update the Stabilization or Actuator objects
    const controlHandler *handler = &handler_MANUAL;
    switch (newMode) {
    case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
        handler = &handler_MANUAL;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED4:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED5:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED6:
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
        handler = &handler_STABILIZED;

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
        newFlightModeAssist = isAssistedFlightMode(position, newMode, &modeSettings);

        if (newFlightModeAssist != flightStatus.FlightModeAssist) {
            // On change of assist mode reinitialise control state.  This is required
            // for the scenario where a flight position change reuses a flight mode
            // but adds assistedcontrol.
            newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
            newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
        }

        if (newFlightModeAssist) {
            // assess roll/pitch state
            bool flagRollPitchHasInput = (fabsf(cmd.Roll) > 0.0f || fabsf(cmd.Pitch) > 0.0f);

            // assess throttle state
            bool throttleNeutral = false;
            float neutralThrustOffset = 0.0f;
            VtolSelfTuningStatsNeutralThrustOffsetGet(&neutralThrustOffset);


            float throttleRangeDelta = (thrustLimits.Neutral + neutralThrustOffset) * ASSISTEDCONTROL_NEUTRALTHROTTLERANGE_FACTOR;
            float throttleNeutralLow = (thrustLimits.Neutral + neutralThrustOffset) - throttleRangeDelta;
            float throttleNeutralHi  = (thrustLimits.Neutral + neutralThrustOffset) + throttleRangeDelta;
            if (cmd.Thrust > throttleNeutralLow && cmd.Thrust < throttleNeutralHi) {
                throttleNeutral = true;
            }

            // determine default thrust mode for hold/brake states
            uint8_t pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
            if (newFlightModeAssist == FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST) {
                pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTO;
            }

            switch (newAssistedControlState) {
            case FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY:
                if (!flagRollPitchHasInput) {
                    // no stick input on roll/pitch so enter brake state
                    newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_BRAKE;
                    newAssistedThrottleState = pathfollowerthrustmode;
                    handler = &handler_PATHFOLLOWER;
                    // retain thrust cmd for later comparison with actual in braking
                    thrustAtBrakeStart = cmd.Thrust;

                    // calculate hi and low value of +-4% as a mini-deadband
                    // for use in auto-override in brake sequence
                    thrustLo = ASSISTEDCONTROL_BRAKETHRUST_DEADBAND_FACTOR_LO * thrustAtBrakeStart;
                    thrustHi = ASSISTEDCONTROL_BRAKETHRUST_DEADBAND_FACTOR_HI * thrustAtBrakeStart;

                    // The purpose for auto throttle assist is to go from a mid to high thrust range to a
                    // neutral vertical-holding/maintaining ~50% thrust range.  It is not designed/intended
                    // to go from near zero to 50%...we don't want an auto-takeoff feature here!
                    // Also for rapid decents a user might have a bit of forward stick and low throttle
                    // then stick-off for auto-braking...but if below the vtol min of 20% it will not
                    // kick in...the flyer needs to manually manage throttle to slow down decent,
                    // and the next time they put in a bit of stick, revert to primary, and then
                    // sticks-off it will brake and hold with auto-thrust
                    if (thrustAtBrakeStart < thrustLimits.Min) {
                        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                    }
                } else {
                    // stick input so stay in primary mode control state
                    // newAssistedControlState = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
                    newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                }
                break;

            case FLIGHTSTATUS_ASSISTEDCONTROLSTATE_BRAKE:
                if (flagRollPitchHasInput) {
                    // stick input during brake sequence allows immediate resumption of control
                    newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
                    newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                } else {
                    // no stick input, stay in brake mode
                    newAssistedThrottleState = pathfollowerthrustmode;
                    handler = &handler_PATHFOLLOWER;

                    // if auto thrust and user adjusts thrust outside of a deadband in which case revert to manual
                    if ((newAssistedThrottleState == FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTO) &&
                        (cmd.Thrust < thrustLo || cmd.Thrust > thrustHi)) {
                        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
                    }
                }
                break;

            case FLIGHTSTATUS_ASSISTEDCONTROLSTATE_HOLD:

                if (newFlightModeAssist == FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST_PRIMARYTHRUST ||
                    (newFlightModeAssist == FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST &&
                     newAssistedThrottleState == FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL)) {
                    // for manual or primary throttle states/modes, stick control immediately reverts to primary mode control
                    if (flagRollPitchHasInput) {
                        newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
                        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                    } else {
                        // otherwise stay in the hold state
                        // newAssistedControlState = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_HOLD;
                        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
                        handler = &handler_PATHFOLLOWER;
                    }
                } else if (newFlightModeAssist == FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST &&
                           newAssistedThrottleState != FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL) {
                    // ok auto thrust mode applies in hold unless overridden

                    if (flagRollPitchHasInput) {
                        // throttle is neutral approximately and stick input present so revert to primary mode control
                        newAssistedControlState  = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;
                        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                    } else {
                        // otherwise hold, autothrust, no stick input...we watch throttle for need to change mode
                        switch (newAssistedThrottleState) {
                        case FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTO:
                            // whilst in auto, watch for neutral range, from which we allow override
                            if (throttleNeutral) {
                                pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTOOVERRIDE;
                            } else { pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTO; }
                            break;
                        case FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTOOVERRIDE:
                            // previously user has set throttle to neutral range, apply a deadband and revert to manual
                            // if moving out of deadband.  This allows for landing in hold state.
                            if (!throttleNeutral) {
                                pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
                            } else { pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTOOVERRIDE; }
                            break;
                        case FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL:
                            pathfollowerthrustmode = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
                            break;
                        }

                        newAssistedThrottleState = pathfollowerthrustmode;
                        handler = &handler_PATHFOLLOWER;
                    }
                }
                break;
            }
        }
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
        break;
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES

    // During development the assistedcontrol implementation is optional and set
    // set in stabi settings.  Once if we decide to always have this on, it can
    // can be directly set here...i.e. set the flight mode assist as required.
    case FLIGHTSTATUS_FLIGHTMODE_VELOCITYROAM:
        newFlightModeAssist = FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST_PRIMARYTHRUST;
        newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
        handler = &handler_PATHFOLLOWER;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
    case FLIGHTSTATUS_FLIGHTMODE_LAND:
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF:
        newFlightModeAssist = isAssistedFlightMode(position, newMode, &modeSettings);
        if (newFlightModeAssist) {
            // Set the default thrust state
            switch (newFlightModeAssist) {
            case FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST_PRIMARYTHRUST:
                newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL;
                break;
            case FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST:
                newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_AUTO;
                break;
            case FLIGHTSTATUS_FLIGHTMODEASSIST_NONE:
            default:
                newAssistedThrottleState = FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL; // Effectively None
                break;
            }
        }
        handler = &handler_PATHFOLLOWER;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_COURSELOCK:
    case FLIGHTSTATUS_FLIGHTMODE_HOMELEASH:
    case FLIGHTSTATUS_FLIGHTMODE_ABSOLUTEPOSITION:
    case FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE:
    case FLIGHTSTATUS_FLIGHTMODE_POI:
    case FLIGHTSTATUS_FLIGHTMODE_AUTOCRUISE:
        handler = &handler_PATHFOLLOWER;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
        handler = &handler_PATHPLANNER;
        break;
#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */
        // There is no default, so if a flightmode is forgotten the compiler can throw a warning!
    }

    bool alwaysStabilizedSwitch = false;

    // Check for a AlwaysStabilizeWhenArmed accessory switch
    switch (modeSettings.AlwaysStabilizeWhenArmedSwitch) {
    case FLIGHTMODESETTINGS_ALWAYSSTABILIZEWHENARMEDSWITCH_ACCESSORY0:
        AccessoryDesiredInstGet(0, &acc);
        alwaysStabilizedSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ALWAYSSTABILIZEWHENARMEDSWITCH_ACCESSORY1:
        AccessoryDesiredInstGet(1, &acc);
        alwaysStabilizedSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ALWAYSSTABILIZEWHENARMEDSWITCH_ACCESSORY2:
        AccessoryDesiredInstGet(2, &acc);
        alwaysStabilizedSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ALWAYSSTABILIZEWHENARMEDSWITCH_ACCESSORY3:
        AccessoryDesiredInstGet(3, &acc);
        alwaysStabilizedSwitch = true;
        break;
    default:
        break;
    }

    if (alwaysStabilizedSwitch) {
        if (acc.AccessoryVal <= -ALWAYSTABILIZEACCESSORY_THRESHOLD) {
            newAlwaysStabilized = FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_FALSE;
        } else if ((acc.AccessoryVal >= ALWAYSTABILIZEACCESSORY_THRESHOLD) &&
                   (cmd.Throttle >= modeSettings.AlwaysStabilizeWhenArmedThrottleThreshold)) {
            newAlwaysStabilized = FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_TRUE;
        }
    } else {
        newAlwaysStabilized = FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_FALSE;
    }

    bool newinit = false;

    // FlightMode needs to be set correctly on first run (otherwise ControlChain is invalid)
    static bool firstRun = true;

    if (flightStatus.AlwaysStabilizeWhenArmed != newAlwaysStabilized ||
        flightStatus.FlightMode != newMode || firstRun ||
        newFlightModeAssist != flightStatus.FlightModeAssist ||
        newAssistedControlState != flightStatus.AssistedControlState ||
        flightStatus.AssistedThrottleState != newAssistedThrottleState) {
        firstRun = false;
        flightStatus.ControlChain             = handler->controlChain;
        flightStatus.FlightMode               = newMode;
        flightStatus.AlwaysStabilizeWhenArmed = newAlwaysStabilized;
        flightStatus.FlightModeAssist         = newFlightModeAssist;
        flightStatus.AssistedControlState     = newAssistedControlState;
        flightStatus.AssistedThrottleState    = newAssistedThrottleState;
        FlightStatusSet(&flightStatus);
        newinit = true;
        lastPosition = position;
    }
    if (handler->handler) {
        handler->handler(newinit);
    }
}

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
void HandleBatteryFailsafe(uint8_t *position, FlightModeSettingsData *modeSettings)
{
    // static uint8_t lastInputPosition = -1;
    typedef enum { BATTERYFAILSAFE_NONE = 0, BATTERYFAILSAFE_WARNING = 1, BATTERYFAILSAFE_CRITICAL = 2 } batteryfailsafemode_t;
    static batteryfailsafemode_t lastFailsafeStatus = BATTERYFAILSAFE_NONE;
    static bool failsafeOverridden;
    static uint8_t lastFlightPosition;
    static uint32_t changeTimestamp;
    SystemAlarmsAlarmData alarms;
    batteryfailsafemode_t failsafeStatus;
    FlightStatusArmedOptions armed;
    FlightStatusArmedGet(&armed);

    // reset the status and do not change anything when not armed
    if (armed != FLIGHTSTATUS_ARMED_ARMED) {
        lastFailsafeStatus = BATTERYFAILSAFE_NONE;
        failsafeOverridden = false;
        changeTimestamp    = PIOS_DELAY_GetRaw();
        lastFlightPosition = *position;
        return;
    }

    SystemAlarmsAlarmGet(&alarms);

    switch (alarms.Battery) {
    case SYSTEMALARMS_ALARM_WARNING:
        failsafeStatus = BATTERYFAILSAFE_WARNING;
        break;
    case SYSTEMALARMS_ALARM_CRITICAL:
        failsafeStatus = BATTERYFAILSAFE_CRITICAL;
        break;
    default:
        failsafeStatus = BATTERYFAILSAFE_NONE;
        break;
    }
    uint32_t debounceTimerms = PIOS_DELAY_DiffuS(changeTimestamp) / 1000;

    if (failsafeStatus == lastFailsafeStatus) {
        changeTimestamp = PIOS_DELAY_GetRaw();
    } else if ((debounceTimerms < modeSettings->BatteryFailsafeDebounceTimer) || failsafeStatus < lastFailsafeStatus) {
        // do not change within the "grace" period and do not "downgrade" the failsafe mode
        failsafeStatus = lastFailsafeStatus;
    } else {
        // a higher failsafe status was met and grace period elapsed. Trigger the new state
        lastFailsafeStatus = failsafeStatus;
        lastFlightPosition = *position;
        failsafeOverridden = false;
    }

    if ((failsafeStatus == BATTERYFAILSAFE_NONE) || failsafeOverridden) {
        return;
    }

    // failsafe has been triggered. Check for override
    if (lastFlightPosition != *position) {
        // flag the override and reset the grace period
        failsafeOverridden = true;
        changeTimestamp    = PIOS_DELAY_GetRaw();
        return;
    }

    switch (failsafeStatus) {
    case BATTERYFAILSAFE_CRITICAL:
        // if critical is not set, jump to the other case to use the warning setting.
        if (modeSettings->BatteryFailsafeSwitchPositions.Critical != -1) {
            *position = modeSettings->BatteryFailsafeSwitchPositions.Critical;
            break;
        }
    case BATTERYFAILSAFE_WARNING:
        if (modeSettings->BatteryFailsafeSwitchPositions.Warning != -1) {
            *position = modeSettings->BatteryFailsafeSwitchPositions.Warning;
        }
        break;
    default:
        break;
    }
}

#endif /* ifndef PIOS_EXCLUDE_ADVANCED_FEATURES */

/**
 * Called whenever a critical configuration component changes
 */
static void configurationUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    configuration_check();
}

/**
 * Called whenever a critical configuration component changes
 */
static void commandUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    PIOS_CALLBACKSCHEDULER_Dispatch(callbackHandle);
}


#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
/**
 * Check and set modes for gps assisted stablised flight modes
 */
static uint8_t isAssistedFlightMode(uint8_t position, uint8_t flightMode, FlightModeSettingsData *modeSettings)
{
    StabilizationSettingsFlightModeAssistMapOptions FlightModeAssistMap[STABILIZATIONSETTINGS_FLIGHTMODEASSISTMAP_NUMELEM];

    StabilizationSettingsFlightModeAssistMapGet(FlightModeAssistMap);
    if (flightMode == FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE
        || position >= STABILIZATIONSETTINGS_FLIGHTMODEASSISTMAP_NUMELEM) {
        return FLIGHTSTATUS_FLIGHTMODEASSIST_NONE;
    }

    switch (FlightModeAssistMap[position]) {
    case STABILIZATIONSETTINGS_FLIGHTMODEASSISTMAP_NONE:
        break;
    case STABILIZATIONSETTINGS_FLIGHTMODEASSISTMAP_GPSASSIST:
    {
        // default to cruise control.
        FlightModeSettingsStabilization1SettingsOptions thrustMode = FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_CRUISECONTROL;

        switch (flightMode) {
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
            thrustMode = modeSettings->Stabilization1Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
            thrustMode = modeSettings->Stabilization2Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
            thrustMode = modeSettings->Stabilization3Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED4:
            thrustMode = modeSettings->Stabilization4Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED5:
            thrustMode = modeSettings->Stabilization5Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_STABILIZED6:
            thrustMode = modeSettings->Stabilization6Settings.Thrust;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
        case FLIGHTSTATUS_FLIGHTMODE_VELOCITYROAM:
            // we hard code the "GPS Assisted" PostionHold/Roam to use alt-vario which
            // is a more appropriate throttle mode.  "GPSAssist" adds braking
            // and a better throttle management to the standard Position Hold.
            thrustMode = FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEVARIO;
            break;
        case FLIGHTSTATUS_FLIGHTMODE_LAND:
        case FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF:
            thrustMode = FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_CRUISECONTROL;
            break;

            // other modes will use cruisecontrol as default
        }


        switch (thrustMode) {
        case FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEHOLD:
        case FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_ALTITUDEVARIO:
            // this is only for use with stabi mods with althold/vario.
            return FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST_PRIMARYTHRUST;

        case FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_MANUAL:
        case FLIGHTMODESETTINGS_STABILIZATION1SETTINGS_CRUISECONTROL:
        default:
            // this is the default for non stabi modes also
            return FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST;
        }
    }
    break;
    }

    // return isAssistedFlag;
    return FLIGHTSTATUS_FLIGHTMODEASSIST_NONE;
}
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

/**
 * @}
 * @}
 */
