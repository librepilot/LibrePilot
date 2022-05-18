/**
 ******************************************************************************
 *
 * @file       plan.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 *
 * @brief setups RTH/PH and other pathfollower/pathplanner status
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup LibrePilotLibraries LibrePilot Libraries Navigation
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

#include <plans.h>
#include <openpilot.h>
#include <attitudesettings.h>
#include <takeofflocation.h>
#include <pathdesired.h>
#include <positionstate.h>
#include <flightmodesettings.h>
#include <flightstatus.h>
#include <velocitystate.h>
#include <manualcontrolcommand.h>
#include <attitudestate.h>
#include <vtolpathfollowersettings.h>
#include <stabilizationbank.h>
#include <stabilizationdesired.h>
#include <sin_lookup.h>
#include <sanitycheck.h>
#include <statusvtolautotakeoff.h>

#define UPDATE_EXPECTED 0.02f
#define UPDATE_MIN      1.0e-6f
#define UPDATE_MAX      1.0f
#define UPDATE_ALPHA    1.0e-2f


static float applyExpo(float value, float expo);


static float applyExpo(float value, float expo)
{
    // note: fastPow makes a small error, therefore result needs to be bound
    float exp = boundf(fastPow(1.00695f, expo), 0.5f, 2.0f);

    // magic number scales expo
    // so that
    // expo=100 yields value**10
    // expo=0 yields value**1
    // expo=-100 yields value**(1/10)
    // (pow(2.0,1/100)~=1.00695)
    if (value > 0.0f) {
        return boundf(fastPow(value, exp), 0.0f, 1.0f);
    } else if (value < -0.0f) {
        return boundf(-fastPow(-value, exp), -1.0f, 0.0f);
    } else {
        return 0.0f;
    }
}


/**
 * @brief initialize UAVOs and structs used by this library
 */
void plan_initialize()
{
    PositionStateInitialize();
    PathDesiredInitialize();
    FlightStatusInitialize();
    AttitudeStateInitialize();
    ManualControlCommandInitialize();
    VelocityStateInitialize();
    StabilizationBankInitialize();
    StabilizationDesiredInitialize();
}

/**
 * @brief setup pathplanner/pathfollower for positionhold
 */
void plan_setup_positionHold()
{
    PositionStateData positionState;

    PositionStateGet(&positionState);
    PathDesiredData pathDesired;
    // re-initialise in setup stage
    memset(&pathDesired, 0, sizeof(PathDesiredData));

    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);

    pathDesired.End.North        = positionState.North;
    pathDesired.End.East         = positionState.East;
    pathDesired.End.Down         = positionState.Down;
    pathDesired.Start.North      = positionState.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
    pathDesired.Start.East       = positionState.East;
    pathDesired.Start.Down       = positionState.Down;
    pathDesired.StartingVelocity = 0.0f;
    pathDesired.EndingVelocity   = 0.0f;
    pathDesired.Mode = PATHDESIRED_MODE_GOTOENDPOINT;

    PathDesiredSet(&pathDesired);
}


/**
 * @brief setup pathplanner/pathfollower for return to base
 */
void plan_setup_returnToBase()
{
    // Simple Return To Base mode - keep altitude the same applying configured delta, fly to takeoff position
    float positionStateDown;

    PositionStateDownGet(&positionStateDown);

    PathDesiredData pathDesired;
    // re-initialise in setup stage
    memset(&pathDesired, 0, sizeof(PathDesiredData));

    TakeOffLocationData takeoffLocation;
    TakeOffLocationGet(&takeoffLocation);

    // TODO: right now VTOLPF does fly straight to destination altitude.
    // For a safer RTB destination altitude will be the higher between takeofflocation and current position (corrected with safety margin)

    float destDown;
    float destVelocity;
    FlightModeSettingsReturnToBaseAltitudeOffsetGet(&destDown);
    FlightModeSettingsReturnToBaseVelocityGet(&destVelocity);
    destDown = MIN(positionStateDown, takeoffLocation.Down) - destDown;
    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);

    pathDesired.End.North        = takeoffLocation.North;
    pathDesired.End.East         = takeoffLocation.East;
    pathDesired.End.Down         = destDown;

    pathDesired.Start.North      = takeoffLocation.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
    pathDesired.Start.East       = takeoffLocation.East;
    pathDesired.Start.Down       = destDown;

    pathDesired.StartingVelocity = destVelocity;
    pathDesired.EndingVelocity   = destVelocity;

    FlightModeSettingsReturnToBaseNextCommandOptions ReturnToBaseNextCommand;
    FlightModeSettingsReturnToBaseNextCommandGet(&ReturnToBaseNextCommand);
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_GOTOENDPOINT_NEXTCOMMAND] = (float)ReturnToBaseNextCommand;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_GOTOENDPOINT_UNUSED1]     = 0.0f;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_GOTOENDPOINT_UNUSED2]     = 0.0f;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_GOTOENDPOINT_UNUSED3]     = 0.0f;
    pathDesired.Mode = PATHDESIRED_MODE_GOTOENDPOINT;

    PathDesiredSet(&pathDesired);
}

void plan_setup_AutoTakeoff()
{
    PathDesiredData pathDesired;

    memset(&pathDesired, 0, sizeof(PathDesiredData));
    PositionStateData positionState;

    PositionStateGet(&positionState);
    float autotakeoff_height;

    FlightModeSettingsAutoTakeOffHeightGet(&autotakeoff_height);
    autotakeoff_height      = fabsf(autotakeoff_height);

    pathDesired.Start.North = positionState.North;
    pathDesired.Start.East  = positionState.East;
    pathDesired.Start.Down  = positionState.Down;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_NORTH] = 0.0f;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_EAST]  = 0.0f;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_DOWN]  = 0.0f;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_AUTOTAKEOFF_CONTROLSTATE] = 0.0f;

    pathDesired.End.North = positionState.North;
    pathDesired.End.East  = positionState.East;
    pathDesired.End.Down  = positionState.Down - autotakeoff_height;

    pathDesired.StartingVelocity = 0.0f;
    pathDesired.EndingVelocity   = 0.0f;
    pathDesired.Mode = PATHDESIRED_MODE_AUTOTAKEOFF;
    PathDesiredSet(&pathDesired);
}

static void plan_setup_land_helper(PathDesiredData *pathDesired)
{
    PositionStateData positionState;

    PositionStateGet(&positionState);
    float velocity_down;

    FlightModeSettingsLandingVelocityGet(&velocity_down);

    pathDesired->Start.North = positionState.North;
    pathDesired->Start.East  = positionState.East;
    pathDesired->Start.Down  = positionState.Down;
    pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_VELOCITYVECTOR_NORTH] = 0.0f;
    pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_VELOCITYVECTOR_EAST]  = 0.0f;
    pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_VELOCITYVECTOR_DOWN]  = velocity_down;

    pathDesired->End.North = positionState.North;
    pathDesired->End.East  = positionState.East;
    pathDesired->End.Down  = positionState.Down;

    pathDesired->StartingVelocity = 0.0f;
    pathDesired->EndingVelocity   = 0.0f;
    pathDesired->Mode = PATHDESIRED_MODE_LAND;
    pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_OPTIONS] = (float)PATHDESIRED_MODEPARAMETER_LAND_OPTION_HORIZONTAL_PH;
}

void plan_setup_land()
{
    PathDesiredData pathDesired;

    // re-initialise in setup stage
    memset(&pathDesired, 0, sizeof(PathDesiredData));

    plan_setup_land_helper(&pathDesired);
    PathDesiredSet(&pathDesired);
}

static void plan_setup_land_from_velocityroam()
{
    plan_setup_land();
    FlightStatusAssistedControlStateOptions assistedControlFlightMode;
    assistedControlFlightMode = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_HOLD;
    FlightStatusAssistedControlStateSet(&assistedControlFlightMode);
}

/**
 * @brief positionvario functionality
 */
static bool vario_hold    = true;
static float hold_position[3];
static float vario_control_lowpass[3];
static float vario_course = 0.0f;

static void plan_setup_PositionVario()
{
    vario_hold = true;
    vario_control_lowpass[0] = 0.0f;
    vario_control_lowpass[1] = 0.0f;
    vario_control_lowpass[2] = 0.0f;
    AttitudeStateYawGet(&vario_course);
    plan_setup_positionHold();
}

void plan_setup_CourseLock()
{
    plan_setup_PositionVario();
}

void plan_setup_PositionRoam()
{
    plan_setup_PositionVario();
}

void plan_setup_VelocityRoam()
{
    vario_control_lowpass[0] = 0.0f;
    vario_control_lowpass[1] = 0.0f;
    vario_control_lowpass[2] = 0.0f;
    AttitudeStateYawGet(&vario_course);
}

void plan_setup_HomeLeash()
{
    plan_setup_PositionVario();
}

void plan_setup_AbsolutePosition()
{
    plan_setup_PositionVario();
}


#define DEADBAND 0.1f
static bool normalizeDeadband(float controlVector[4])
{
    bool moving = false;

    // roll, pitch, yaw between -1 and +1
    // thrust between 0 and 1 mapped to -1 to +1
    controlVector[3] = (2.0f * controlVector[3]) - 1.0f;
    int t;

    for (t = 0; t < 4; t++) {
        if (controlVector[t] < -DEADBAND) {
            moving = true;
            controlVector[t] += DEADBAND;
        } else if (controlVector[t] > DEADBAND) {
            moving = true;
            controlVector[t] -= DEADBAND;
        } else {
            controlVector[t] = 0.0f;
        }
        // deadband has been cut out, scale value back to [-1,+1]
        controlVector[t] *= (1.0f / (1.0f - DEADBAND));
        controlVector[t]  = boundf(controlVector[t], -1.0f, 1.0f);
    }

    return moving;
}

typedef enum { COURSE, FPV, LOS, NSEW } vario_type;

static void getVector(float controlVector[4], vario_type type)
{
    FlightModeSettingsPositionHoldOffsetData offset;

    FlightModeSettingsPositionHoldOffsetGet(&offset);

    // scale controlVector[3] (thrust) by vertical/horizontal to have vertical plane less sensitive
    controlVector[3] *= offset.Vertical / offset.Horizontal;

    float length = sqrtf(controlVector[0] * controlVector[0] + controlVector[1] * controlVector[1] + controlVector[3] * controlVector[3]);

    if (length <= 1e-9f) {
        length = 1.0f; // should never happen as getVector is not called if control within deadband
    }
    {
        float direction[3] = {
            controlVector[1] / length, // pitch is north
            controlVector[0] / length, // roll is east
            controlVector[3] / length // thrust is down
        };
        controlVector[0] = direction[0];
        controlVector[1] = direction[1];
        controlVector[2] = direction[2];
    }
    controlVector[3] = length * offset.Horizontal;

    // rotate north and east - rotation angle based on type
    float angle;
    switch (type) {
    case COURSE:
        angle = vario_course;
        break;
    case NSEW:
        angle = 0.0f;
        // NSEW no rotation takes place
        break;
    case FPV:
        // local rotation, using current yaw
        AttitudeStateYawGet(&angle);
        break;
    case LOS:
        // determine location based on vector from takeoff to current location
    {
        PositionStateData positionState;
        PositionStateGet(&positionState);
        TakeOffLocationData takeoffLocation;
        TakeOffLocationGet(&takeoffLocation);
        angle = RAD2DEG(atan2f(positionState.East - takeoffLocation.East, positionState.North - takeoffLocation.North));
    }
    break;
    }
    // rotate horizontally by angle
    {
        float rotated[2] = {
            controlVector[0] * cos_lookup_deg(angle) - controlVector[1] * sin_lookup_deg(angle),
            controlVector[0] * sin_lookup_deg(angle) + controlVector[1] * cos_lookup_deg(angle)
        };
        controlVector[0] = rotated[0];
        controlVector[1] = rotated[1];
    }
}


static void plan_run_PositionVario(vario_type type)
{
    float controlVector[4];
    float alpha;
    PathDesiredData pathDesired;

    // Reuse the existing pathdesired object as setup in the setup to avoid
    // updating values already set.
    PathDesiredGet(&pathDesired);

    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);


    ManualControlCommandRollGet(&controlVector[0]);
    ManualControlCommandPitchGet(&controlVector[1]);
    ManualControlCommandYawGet(&controlVector[2]);
    ManualControlCommandThrustGet(&controlVector[3]);


    FlightModeSettingsVarioControlLowPassAlphaGet(&alpha);
    vario_control_lowpass[0] = alpha * vario_control_lowpass[0] + (1.0f - alpha) * controlVector[0];
    vario_control_lowpass[1] = alpha * vario_control_lowpass[1] + (1.0f - alpha) * controlVector[1];
    vario_control_lowpass[2] = alpha * vario_control_lowpass[2] + (1.0f - alpha) * controlVector[2];
    controlVector[0] = vario_control_lowpass[0];
    controlVector[1] = vario_control_lowpass[1];
    controlVector[2] = vario_control_lowpass[2];

    // check if movement is desired
    if (normalizeDeadband(controlVector) == false) {
        // no movement desired, re-enter positionHold at current start-position
        if (!vario_hold) {
            vario_hold = true;

            // new hold position is the position that was previously the start position
            pathDesired.End.North   = hold_position[0];
            pathDesired.End.East    = hold_position[1];
            pathDesired.End.Down    = hold_position[2];
            // while the new start position has the same offset as in position hold
            pathDesired.Start.North = pathDesired.End.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
            pathDesired.Start.East  = pathDesired.End.East;
            pathDesired.Start.Down  = pathDesired.End.Down;

            // set mode explicitly

            PathDesiredSet(&pathDesired);
        }
    } else {
        PositionStateData positionState;
        PositionStateGet(&positionState);

        // flip pitch to have pitch down (away) point north
        controlVector[1] = -controlVector[1];
        getVector(controlVector, type);

        // layout of control Vector : unitVector in movement direction {0,1,2} vector length {3} velocity {4}
        if (vario_hold) {
            // start position is the position that was previously the hold position
            vario_hold = false;
            hold_position[0] = pathDesired.End.North;
            hold_position[1] = pathDesired.End.East;
            hold_position[2] = pathDesired.End.Down;
        } else {
            // start position is advanced according to movement - in the direction of ControlVector only
            // projection using scalar product
            float kp = (positionState.North - hold_position[0]) * controlVector[0]
                       + (positionState.East - hold_position[1]) * controlVector[1]
                       + (positionState.Down - hold_position[2]) * -controlVector[2];
            if (kp > 0.0f) {
                hold_position[0] += kp * controlVector[0];
                hold_position[1] += kp * controlVector[1];
                hold_position[2] += kp * -controlVector[2];
            }
        }
        // new destination position is advanced based on controlVector
        pathDesired.End.North   = hold_position[0] + controlVector[0] * controlVector[3];
        pathDesired.End.East    = hold_position[1] + controlVector[1] * controlVector[3];
        pathDesired.End.Down    = hold_position[2] - controlVector[2] * controlVector[3];
        // the new start position has the same offset as in position hold
        pathDesired.Start.North = pathDesired.End.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
        pathDesired.Start.East  = pathDesired.End.East;
        pathDesired.Start.Down  = pathDesired.End.Down;
        PathDesiredSet(&pathDesired);
    }
}

void plan_run_VelocityRoam()
{
    // float alpha;
    PathDesiredData pathDesired;

    // velocity roam code completely sets pathdesired object. it was not set in setup phase
    memset(&pathDesired, 0, sizeof(PathDesiredData));
    FlightStatusAssistedControlStateOptions assistedControlFlightMode;
    FlightStatusFlightModeOptions flightMode;

    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);
    FlightStatusAssistedControlStateGet(&assistedControlFlightMode);
    FlightStatusFlightModeGet(&flightMode);
    StabilizationBankData stabSettings;
    StabilizationBankGet(&stabSettings);

    ManualControlCommandData cmd;
    ManualControlCommandGet(&cmd);

    cmd.Roll  = applyExpo(cmd.Roll, stabSettings.StickExpo.Roll);
    cmd.Pitch = applyExpo(cmd.Pitch, stabSettings.StickExpo.Pitch);
    cmd.Yaw   = applyExpo(cmd.Yaw, stabSettings.StickExpo.Yaw);

    bool flagRollPitchHasInput = (fabsf(cmd.Roll) > 0.0f || fabsf(cmd.Pitch) > 0.0f);

    if (!flagRollPitchHasInput) {
        // no movement desired, re-enter positionHold at current start-position
        if (assistedControlFlightMode == FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY) {
            // initiate braking and change assisted control flight mode to braking
            if (flightMode == FLIGHTSTATUS_FLIGHTMODE_LAND) {
                // avoid brake then hold sequence to continue descent.
                plan_setup_land_from_velocityroam();
            } else {
                plan_setup_assistedcontrol();
            }
        }
        // otherwise nothing to do in braking/hold modes
    } else {
        PositionStateData positionState;
        PositionStateGet(&positionState);

        // Revert assist control state to primary, which in this case implies
        // we are in roaming state (a GPS vector assisted velocity roam)
        assistedControlFlightMode = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_PRIMARY;

        // Calculate desired velocity in each direction
        float angle;
        AttitudeStateYawGet(&angle);
        angle = DEG2RAD(angle);
        float cos_angle  = cosf(angle);
        float sine_angle = sinf(angle);
        float rotated[2] = {
            -cmd.Pitch * cos_angle - cmd.Roll * sine_angle,
            -cmd.Pitch * sine_angle + cmd.Roll * cos_angle
        };
        // flip pitch to have pitch down (away) point north
        float horizontalVelMax;
        float verticalVelMax;
        VtolPathFollowerSettingsHorizontalVelMaxGet(&horizontalVelMax);
        VtolPathFollowerSettingsVerticalVelMaxGet(&verticalVelMax);
        float velocity_north = rotated[0] * horizontalVelMax;
        float velocity_east  = rotated[1] * horizontalVelMax;
        float velocity_down  = 0.0f;

        if (flightMode == FLIGHTSTATUS_FLIGHTMODE_LAND) {
            FlightModeSettingsLandingVelocityGet(&velocity_down);
        }

        float velocity = velocity_north * velocity_north + velocity_east * velocity_east;
        velocity = sqrtf(velocity);

        // if one stick input (pitch or roll) should we use fly by vector? set arbitrary distance of say 20m after which we
        // expect new stick input
        // if two stick input pilot is fighting wind manually and we use fly by velocity
        // in reality setting velocity desired to zero will fight wind anyway.

        pathDesired.Start.North = positionState.North;
        pathDesired.Start.East  = positionState.East;
        pathDesired.Start.Down  = positionState.Down;
        pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_NORTH] = velocity_north;
        pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_EAST]  = velocity_east;
        pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_DOWN]  = velocity_down;

        pathDesired.End.North = positionState.North;
        pathDesired.End.East  = positionState.East;
        pathDesired.End.Down  = positionState.Down;

        pathDesired.StartingVelocity = velocity;
        pathDesired.EndingVelocity   = velocity;
        pathDesired.Mode = PATHDESIRED_MODE_VELOCITY;
        if (flightMode == FLIGHTSTATUS_FLIGHTMODE_LAND) {
            pathDesired.Mode = PATHDESIRED_MODE_LAND;
            pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_OPTIONS] = (float)PATHDESIRED_MODEPARAMETER_LAND_OPTION_NONE;
        } else {
            pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_UNUSED] = 0.0f;
        }
        PathDesiredSet(&pathDesired);
        FlightStatusAssistedControlStateSet(&assistedControlFlightMode);
    }
}

void plan_run_CourseLock()
{
    plan_run_PositionVario(COURSE);
}

void plan_run_PositionRoam()
{
    plan_run_PositionVario(FPV);
}

void plan_run_HomeLeash()
{
    plan_run_PositionVario(LOS);
}

void plan_run_AbsolutePosition()
{
    plan_run_PositionVario(NSEW);
}


/**
 * @brief setup pathplanner/pathfollower for AutoCruise
 */
static PiOSDeltatimeConfig actimeval;
void plan_setup_AutoCruise()
{
    PositionStateData positionState;

    PositionStateGet(&positionState);

    PathDesiredData pathDesired;
    // setup needs to reinitialise the pathdesired object
    memset(&pathDesired, 0, sizeof(PathDesiredData));

    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);

    // initialization is flight in direction of the nose.
    // the velocity is not relevant, as it will be reset by the run function even during first call
    float angle;
    AttitudeStateYawGet(&angle);
    float vector[2] = {
        cos_lookup_deg(angle),
        sin_lookup_deg(angle)
    };
    hold_position[0]             = positionState.North;
    hold_position[1]             = positionState.East;
    hold_position[2]             = positionState.Down;
    pathDesired.End.North        = hold_position[0] + vector[0];
    pathDesired.End.East         = hold_position[1] + vector[1];
    pathDesired.End.Down         = hold_position[2];
    // start position has the same offset as in position hold
    pathDesired.Start.North      = pathDesired.End.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
    pathDesired.Start.East       = pathDesired.End.East;
    pathDesired.Start.Down       = pathDesired.End.Down;
    pathDesired.StartingVelocity = 0.0f;
    pathDesired.EndingVelocity   = 0.0f;
    pathDesired.Mode             = PATHDESIRED_MODE_GOTOENDPOINT;

    PathDesiredSet(&pathDesired);

    // re-iniztializing deltatime is valid and also good practice here since
    // getAverageSeconds() has not been called/updated in a long time if we were in a different flightmode.
    PIOS_DELTATIME_Init(&actimeval, UPDATE_EXPECTED, UPDATE_MIN, UPDATE_MAX, UPDATE_ALPHA);
}

/**
 * @brief execute autocruise
 */
void plan_run_AutoCruise()
{
    PositionStateData positionState;

    PositionStateGet(&positionState);
    PathDesiredData pathDesired;
    // re-use pathdesired that was setup correctly in setup stage.
    PathDesiredGet(&pathDesired);

    FlightModeSettingsPositionHoldOffsetData offset;
    FlightModeSettingsPositionHoldOffsetGet(&offset);

    float controlVector[4];
    ManualControlCommandRollGet(&controlVector[0]);
    ManualControlCommandPitchGet(&controlVector[1]);
    ManualControlCommandYawGet(&controlVector[2]);
    controlVector[3] = 0.5f; // dummy, thrust is normalized separately
    normalizeDeadband(controlVector); // return value ignored
    ManualControlCommandThrustGet(&controlVector[3]); // no deadband as we are using thrust for velocity
    controlVector[3] = boundf(controlVector[3], 1e-6f, 1.0f); // bound to above zero, to prevent loss of vector direction

    // normalize old desired movement vector
    float vector[3] = { pathDesired.End.North - hold_position[0],
                        pathDesired.End.East - hold_position[1],
                        pathDesired.End.Down - hold_position[2] };
    float length    = sqrtf(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
    if (length < 1e-9f) {
        length = 1.0f; // should not happen since initialized properly in setup()
    }
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;

    // start position is advanced according to actual movement - in the direction of desired vector only
    // projection using scalar product
    float kp = (positionState.North - hold_position[0]) * vector[0]
               + (positionState.East - hold_position[1]) * vector[1]
               + (positionState.Down - hold_position[2]) * vector[2];
    if (kp > 0.0f) {
        hold_position[0] += kp * vector[0];
        hold_position[1] += kp * vector[1];
        hold_position[2] += kp * vector[2];
    }

    // new angle is equal to old angle plus offset depending on yaw input and time
    // (controlVector is normalized with a deadband, change is zero within deadband)
    float angle = RAD2DEG(atan2f(vector[1], vector[0]));
    float dT    = PIOS_DELTATIME_GetAverageSeconds(&actimeval);
    angle    += 10.0f * controlVector[2] * dT; // TODO magic value could eventually end up in a to be created settings

    // resulting movement vector is scaled by velocity demand in controlvector[3] [0.0-1.0]
    vector[0] = cosf(DEG2RAD(angle)) * offset.Horizontal * controlVector[3];
    vector[1] = sinf(DEG2RAD(angle)) * offset.Horizontal * controlVector[3];
    vector[2] = -controlVector[1] * offset.Vertical * controlVector[3];

    pathDesired.End.North   = hold_position[0] + vector[0];
    pathDesired.End.East    = hold_position[1] + vector[1];
    pathDesired.End.Down    = hold_position[2] + vector[2];
    // start position has the same offset as in position hold
    pathDesired.Start.North = pathDesired.End.North + offset.Horizontal; // in FlyEndPoint the direction of this vector does not matter
    pathDesired.Start.East  = pathDesired.End.East;
    pathDesired.Start.Down  = pathDesired.End.Down;
    PathDesiredSet(&pathDesired);
}

/**
 * @brief setup pathplanner/pathfollower for braking in positionroam
 *        timeout_occurred = false: Attempt to enter flyvector for braking
 *        timeout_occurred = true: Revert to position hold
 */
#define ASSISTEDCONTROL_BRAKERATE_MINIMUM  0.2f   // m/s2
#define ASSISTEDCONTROL_TIMETOSTOP_MINIMUM 0.2f // seconds
#define ASSISTEDCONTROL_TIMETOSTOP_MAXIMUM 9.0f // seconds
#define ASSISTEDCONTROL_DELAY_TO_BRAKE     1.0f      // seconds
#define ASSISTEDCONTROL_TIMEOUT_MULTIPLIER 4.0f // actual deceleration rate can be 50% of desired...timeouts need to cater for this
void plan_setup_assistedcontrol()
{
    PositionStateData positionState;

    PositionStateGet(&positionState);
    PathDesiredData pathDesired;
    // setup function, avoid carry over from previous mode
    memset(&pathDesired, 0, sizeof(PathDesiredData));

    FlightStatusAssistedControlStateOptions assistedControlFlightMode;

    VelocityStateData velocityState;
    VelocityStateGet(&velocityState);
    float brakeRate;
    VtolPathFollowerSettingsBrakeRateGet(&brakeRate);
    if (brakeRate < ASSISTEDCONTROL_BRAKERATE_MINIMUM) {
        brakeRate = ASSISTEDCONTROL_BRAKERATE_MINIMUM; // set a minimum to avoid a divide by zero potential below
    }
    // Calculate the velocity
    float velocity = velocityState.North * velocityState.North + velocityState.East * velocityState.East + velocityState.Down * velocityState.Down;
    velocity = sqrtf(velocity);

    // Calculate the desired time to zero velocity.
    float time_to_stopped = ASSISTEDCONTROL_DELAY_TO_BRAKE; // we allow at least 0.5 seconds to rotate to a brake angle.
    time_to_stopped += velocity / brakeRate;

    // Sanity check the brake rate by ensuring that the time to stop is within a range.
    if (time_to_stopped < ASSISTEDCONTROL_TIMETOSTOP_MINIMUM) {
        time_to_stopped = ASSISTEDCONTROL_TIMETOSTOP_MINIMUM;
    } else if (time_to_stopped > ASSISTEDCONTROL_TIMETOSTOP_MAXIMUM) {
        time_to_stopped = ASSISTEDCONTROL_TIMETOSTOP_MAXIMUM;
    }

    // calculate the distance we will travel
    float north_delta = velocityState.North * ASSISTEDCONTROL_DELAY_TO_BRAKE; // we allow at least 0.5s to rotate to brake angle
    north_delta += (time_to_stopped - ASSISTEDCONTROL_DELAY_TO_BRAKE) * 0.5f * velocityState.North; // area under the linear deceleration plot
    float east_delta  = velocityState.East * ASSISTEDCONTROL_DELAY_TO_BRAKE; // we allow at least 0.5s to rotate to brake angle
    east_delta  += (time_to_stopped - ASSISTEDCONTROL_DELAY_TO_BRAKE) * 0.5f * velocityState.East; // area under the linear deceleration plot
    float down_delta  = velocityState.Down * ASSISTEDCONTROL_DELAY_TO_BRAKE;
    down_delta  += (time_to_stopped - ASSISTEDCONTROL_DELAY_TO_BRAKE) * 0.5f * velocityState.Down; // area under the linear deceleration plot
    float net_delta   = east_delta * east_delta + north_delta * north_delta + down_delta * down_delta;
    net_delta    = sqrtf(net_delta);

    pathDesired.Start.North = positionState.North;
    pathDesired.Start.East  = positionState.East;
    pathDesired.Start.Down  = positionState.Down;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_BRAKE_STARTVELOCITYVECTOR_NORTH] = velocityState.North;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_BRAKE_STARTVELOCITYVECTOR_EAST]  = velocityState.East;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_BRAKE_STARTVELOCITYVECTOR_DOWN]  = velocityState.Down;

    pathDesired.End.North = positionState.North + north_delta;
    pathDesired.End.East  = positionState.East + east_delta;
    pathDesired.End.Down  = positionState.Down + down_delta;

    pathDesired.StartingVelocity = velocity;
    pathDesired.EndingVelocity   = 0.0f;
    pathDesired.Mode = PATHDESIRED_MODE_BRAKE;
    pathDesired.ModeParameters[PATHDESIRED_MODEPARAMETER_BRAKE_TIMEOUT] = time_to_stopped * ASSISTEDCONTROL_TIMEOUT_MULTIPLIER;
    assistedControlFlightMode    = FLIGHTSTATUS_ASSISTEDCONTROLSTATE_BRAKE;
    FlightStatusAssistedControlStateSet(&assistedControlFlightMode);
    PathDesiredSet(&pathDesired);
}
