/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ManualControl
 * @brief Interpretes the control input in ManualControlCommand
 * @{
 *
 * @file       armhandler.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
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
#include <manualcontrolcommand.h>
#include <accessorydesired.h>
#include <flightstatus.h>
#include <flightmodesettings.h>
#include <stabilizationdesired.h>
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
#include <statusvtolland.h>
#endif

// Private constants
#define ARMED_THRESHOLD_SWITCH 0.20f
#define ARMED_THRESHOLD_STICK  0.80f
#define GROUND_LOW_THROTTLE    0.01f

// Private types
typedef enum {
    ARM_STATE_DISARMED,
    ARM_STATE_ARMING_MANUAL,
    ARM_STATE_ARMED,
    ARM_STATE_DISARMING_MANUAL,
    ARM_STATE_DISARMING_TIMEOUT
} ArmState_t;

// Private variables

// Private functions
static void setArmedIfChanged(uint8_t val);
static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time);
static bool okToArm(void);
static bool forcedDisArm(void);


/**
 * @brief Handler to interprete Command inputs in regards to arming/disarming
 * @input: ManualControlCommand, AccessoryDesired
 * @output: FlightStatus.Arming
 */
void armHandler(bool newinit, FrameType_t frameType)
{
    static ArmState_t armState;

    if (newinit) {
        AccessoryDesiredInitialize();
        setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);
        armState = ARM_STATE_DISARMED;
    }

    portTickType sysTime = xTaskGetTickCount();

    FlightModeSettingsData settings;
    FlightModeSettingsGet(&settings);
    ManualControlCommandData cmd;
    ManualControlCommandGet(&cmd);
    AccessoryDesiredData acc;

    bool lowThrottle = cmd.Throttle < 0;

    if (frameType == FRAME_TYPE_GROUND) {
        // Deadbanding applied in receiver.c typically at 2% but we don't assume its enabled.
        lowThrottle = fabsf(cmd.Throttle) < GROUND_LOW_THROTTLE;
    }

    bool armSwitch = false;

    switch (settings.Arming) {
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY0:
        AccessoryDesiredInstGet(0, &acc);
        armSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY1:
        AccessoryDesiredInstGet(1, &acc);
        armSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY2:
        AccessoryDesiredInstGet(2, &acc);
        armSwitch = true;
        break;
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY3:
        AccessoryDesiredInstGet(3, &acc);
        armSwitch = true;
        break;
    default:
        break;
    }

    // immediate disarm on switch
    if (armSwitch && acc.AccessoryVal <= -ARMED_THRESHOLD_SWITCH) {
        lowThrottle = true;
    }

    if (forcedDisArm()) {
        // PathPlanner forces explicit disarming due to error condition (crash, impact, fire, ...)
        armState = ARM_STATE_DISARMED;
        setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);
        return;
    }

    if (settings.Arming == FLIGHTMODESETTINGS_ARMING_ALWAYSDISARMED) {
        // In this configuration we always disarm
        setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);
        return;
    }


    // The throttle is not low, in case we where arming or disarming, abort
    if (!lowThrottle) {
        switch (armState) {
        case ARM_STATE_DISARMING_MANUAL:
        case ARM_STATE_DISARMING_TIMEOUT:
            armState = ARM_STATE_ARMED;
            break;
        case ARM_STATE_ARMING_MANUAL:
            armState = ARM_STATE_DISARMED;
            break;
        default:
            // Nothing needs to be done in the other states
            break;
        }
        return;
    }

    // The rest of these cases throttle is low
    if (settings.Arming == FLIGHTMODESETTINGS_ARMING_ALWAYSARMED) {
        // In this configuration, we go into armed state as soon as the throttle is low, never disarm
        setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMED);
        return;
    }

    // When the configuration is not "Always armed" and no "Always disarmed",
    // the state will not be changed when the throttle is not low
    static portTickType armedDisarmStart;
    float armingInputLevel = 0;

    // Calc channel see assumptions7
    switch (settings.Arming) {
    case FLIGHTMODESETTINGS_ARMING_ROLLLEFT:
        armingInputLevel = 1.0f * cmd.Roll;
        break;
    case FLIGHTMODESETTINGS_ARMING_ROLLRIGHT:
        armingInputLevel = -1.0f * cmd.Roll;
        break;
    case FLIGHTMODESETTINGS_ARMING_PITCHFORWARD:
        armingInputLevel = 1.0f * cmd.Pitch;
        break;
    case FLIGHTMODESETTINGS_ARMING_PITCHAFT:
        armingInputLevel = -1.0f * cmd.Pitch;
        break;
    case FLIGHTMODESETTINGS_ARMING_YAWLEFT:
        armingInputLevel = 1.0f * cmd.Yaw;
        break;
    case FLIGHTMODESETTINGS_ARMING_YAWRIGHT:
        armingInputLevel = -1.0f * cmd.Yaw;
        break;
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY0:
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY1:
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY2:
    case FLIGHTMODESETTINGS_ARMING_ACCESSORY3:
        armingInputLevel = -1.0f * acc.AccessoryVal;
        break;
    default:
        break;
    }

    bool manualArm    = false;
    bool manualDisarm = false;

    static FlightModeSettingsArmingOptions previousArmingSettings = -1;
    static float previousArmingInputLevel = 0.0f;

    if (previousArmingSettings != settings.Arming) {
        previousArmingSettings   = settings.Arming;
        previousArmingInputLevel = 0.0f;
    }

    // ignore previous arming input level if not transitioning from fully ARMED/DISARMED states.
    if ((armState != ARM_STATE_DISARMED) && (armState != ARM_STATE_ARMED)) {
        previousArmingInputLevel = 0.0f;
    }

    float armedThreshold = armSwitch ? ARMED_THRESHOLD_SWITCH : ARMED_THRESHOLD_STICK;

    if ((armingInputLevel <= -armedThreshold) && (previousArmingInputLevel > -armedThreshold)) {
        manualArm = true;
    } else if ((armingInputLevel >= +armedThreshold) && (previousArmingInputLevel < +armedThreshold)) {
        manualDisarm = true;
    }

    previousArmingInputLevel = armingInputLevel;

    switch (armState) {
    case ARM_STATE_DISARMED:
        setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);

        // only allow arming if it's OK too
        if (manualArm && okToArm()) {
            armedDisarmStart = sysTime;
            armState = ARM_STATE_ARMING_MANUAL;
        }
        break;

    case ARM_STATE_ARMING_MANUAL:
        setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMING);

        if (manualArm && (timeDifferenceMs(armedDisarmStart, sysTime) > settings.ArmingSequenceTime)) {
            armState = ARM_STATE_ARMED;
        } else if (!manualArm) {
            armState = ARM_STATE_DISARMED;
        }
        break;

    case ARM_STATE_ARMED:
        // When we get here, the throttle is low,
        // we go immediately to disarming due to timeout, also when the disarming mechanism is not enabled
        armedDisarmStart = sysTime;
        armState = ARM_STATE_DISARMING_TIMEOUT;
        setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMED);
        break;

    case ARM_STATE_DISARMING_TIMEOUT:
    {
        // we should never reach the disarming timeout if the pathfollower is engaged - reset timeout
        FlightStatusControlChainData cc;
        FlightStatusControlChainGet(&cc);
        if (cc.PathFollower == FLIGHTSTATUS_CONTROLCHAIN_TRUE) {
            armedDisarmStart = sysTime;
        }
    }

        // We get here when armed while throttle low, even when the arming timeout is not enabled
        if ((settings.ArmedTimeout != 0) && (timeDifferenceMs(armedDisarmStart, sysTime) > settings.ArmedTimeout)) {
            armState = ARM_STATE_DISARMED;
        }

        // Switch to disarming due to manual control when needed
        if (manualDisarm) {
            armedDisarmStart = sysTime;
            armState = ARM_STATE_DISARMING_MANUAL;
        }
        break;

    case ARM_STATE_DISARMING_MANUAL:
        // arming switch disarms immediately,
        if (manualDisarm && (timeDifferenceMs(armedDisarmStart, sysTime) > settings.DisarmingSequenceTime)) {
            armState = ARM_STATE_DISARMED;
        } else if (!manualDisarm) {
            armState = ARM_STATE_ARMED;
        }
        break;
    } // End Switch
}

static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time)
{
    return (end_time - start_time) * portTICK_RATE_MS;
}

/**
 * @brief Determine if the aircraft is safe to arm
 * @returns True if safe to arm, false otherwise
 */
static bool okToArm(void)
{
    // update checks
    configuration_check();

    // read alarms
    SystemAlarmsData alarms;

    SystemAlarmsGet(&alarms);

    // Check each alarm
    for (int i = 0; i < SYSTEMALARMS_ALARM_NUMELEM; i++) {
        if (SystemAlarmsAlarmToArray(alarms.Alarm)[i] >= SYSTEMALARMS_ALARM_CRITICAL) { // found an alarm thats set
            if (i == SYSTEMALARMS_ALARM_GPS || i == SYSTEMALARMS_ALARM_TELEMETRY) {
                continue;
            }

            return false;
        }
    }

    StabilizationDesiredStabilizationModeData stabDesired;

    FlightStatusData flightStatus;
    FlightStatusGet(&flightStatus);
    switch (flightStatus.FlightMode) {
    case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED4:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED5:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED6:
        // Prevent arming if unsafe due to the current Thrust Mode
        StabilizationDesiredStabilizationModeGet(&stabDesired);
        if (stabDesired.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEHOLD ||
            stabDesired.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO) {
            return false;
        }

        // avoid assisted control with auto throttle.  As it sits waiting to launch,
        // it will move to hold, and auto thrust will auto launch otherwise!
        if (flightStatus.FlightModeAssist == FLIGHTSTATUS_FLIGHTMODEASSIST_GPSASSIST) {
            return false;
        }

        // Avoid arming while AlwaysStabilizeWhenArmed is active
        if (flightStatus.AlwaysStabilizeWhenArmed == FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_TRUE) {
            return false;
        }

        return true;

        break;
    case FLIGHTSTATUS_FLIGHTMODE_LAND:
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
        return false;

    case FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF:
    case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
        // Avoid arming while AlwaysStabilizeWhenArmed is active
        if (flightStatus.AlwaysStabilizeWhenArmed == FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_TRUE) {
            return false;
        }
        return true;

    default:
        return false;

        break;
    }
}
/**
 * @brief Determine if the aircraft is forced to disarm by an explicit alarm
 * @returns True if safe to arm, false otherwise
 */
static bool forcedDisArm(void)
{
    // read alarms
    SystemAlarmsAlarmData alarms;

    SystemAlarmsAlarmGet(&alarms);

    if (alarms.Guidance == SYSTEMALARMS_ALARM_CRITICAL) {
        return true;
    }
    if (alarms.Receiver == SYSTEMALARMS_ALARM_CRITICAL) {
        return true;
    }

    return false;
}

/**
 * @brief Update the flightStatus object only if value changed.  Reduces callbacks
 * @param[in] val The new value
 */
static void setArmedIfChanged(uint8_t val)
{
    FlightStatusData flightStatus;

    FlightStatusGet(&flightStatus);

    if (flightStatus.Armed != val) {
        flightStatus.Armed = val;
        FlightStatusSet(&flightStatus);
    }
}

/**
 * @}
 * @}
 */
