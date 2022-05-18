/*
 ******************************************************************************
 *
 * @file       vtolautotakeofffsm.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2018
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      This autotakeoff state machine is a helper state machine to the
 *             VtolAutoTakeoffController.
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

extern "C" {
#include <openpilot.h>

#include <math.h>
#include <pid.h>
#include <pathdesired.h>
#include <paths.h>
#include "plans.h"
#include <sanitycheck.h>

#include <homelocation.h>
#include <accelstate.h>
#include <vtolpathfollowersettings.h>
#include <flightstatus.h>
#include <flightmodesettings.h>
#include <pathstatus.h>
#include <positionstate.h>
#include <velocitystate.h>
#include <velocitydesired.h>
#include <stabilizationdesired.h>
#include <attitudestate.h>
#include <takeofflocation.h>
#include <manualcontrolcommand.h>
#include <systemsettings.h>
#include <stabilizationbank.h>
#include <stabilizationdesired.h>
#include <vtolselftuningstats.h>
#include <statusvtolautotakeoff.h>
#include <pathsummary.h>
}

// C++ includes
#include <vtolautotakeofffsm.h>


// Private constants
#define TIMER_COUNT_PER_SECOND      (1000 / vtolPathFollowerSettings->UpdatePeriod)
#define TIMEOUT_SLOWSTART           (2 * TIMER_COUNT_PER_SECOND)
#define TIMEOUT_THRUSTUP            (1 * TIMER_COUNT_PER_SECOND)
#define TIMEOUT_THRUSTDOWN          (5 * TIMER_COUNT_PER_SECOND)
#define AUTOTAKEOFF_SLOWDOWN_HEIGHT -5.0f

VtolAutoTakeoffFSM::PathFollowerFSM_AutoTakeoffStateHandler_T VtolAutoTakeoffFSM::sAutoTakeoffStateTable[AUTOTAKEOFF_STATE_SIZE] = {
    [AUTOTAKEOFF_STATE_INACTIVE]   = { .setup = &VtolAutoTakeoffFSM::setup_inactive,   .run = 0                                   },
    [AUTOTAKEOFF_STATE_CHECKSTATE] = { .setup = &VtolAutoTakeoffFSM::setup_checkstate, .run = 0                                   },
    [AUTOTAKEOFF_STATE_SLOWSTART]  = { .setup = &VtolAutoTakeoffFSM::setup_slowstart,  .run = &VtolAutoTakeoffFSM::run_slowstart  },
    [AUTOTAKEOFF_STATE_THRUSTUP]   = { .setup = &VtolAutoTakeoffFSM::setup_thrustup,   .run = &VtolAutoTakeoffFSM::run_thrustup   },
    [AUTOTAKEOFF_STATE_TAKEOFF]    = { .setup = &VtolAutoTakeoffFSM::setup_takeoff,    .run = &VtolAutoTakeoffFSM::run_takeoff    },
    [AUTOTAKEOFF_STATE_HOLD] =       { .setup = &VtolAutoTakeoffFSM::setup_hold,       .run = &VtolAutoTakeoffFSM::run_hold       },
    [AUTOTAKEOFF_STATE_THRUSTDOWN] = { .setup = &VtolAutoTakeoffFSM::setup_thrustdown, .run = &VtolAutoTakeoffFSM::run_thrustdown },
    [AUTOTAKEOFF_STATE_THRUSTOFF]  = { .setup = &VtolAutoTakeoffFSM::setup_thrustoff,  .run = &VtolAutoTakeoffFSM::run_thrustoff  },
    [AUTOTAKEOFF_STATE_DISARMED]   = { .setup = &VtolAutoTakeoffFSM::setup_disarmed,   .run = &VtolAutoTakeoffFSM::run_disarmed   }
};

// pointer to a singleton instance
VtolAutoTakeoffFSM *VtolAutoTakeoffFSM::p_inst = 0;


VtolAutoTakeoffFSM::VtolAutoTakeoffFSM()
    : mAutoTakeoffData(0), vtolPathFollowerSettings(0), pathDesired(0), flightStatus(0)
{}

// Private types

// Private functions
// Public API methods
/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t VtolAutoTakeoffFSM::Initialize(VtolPathFollowerSettingsData *ptr_vtolPathFollowerSettings,
                                       PathDesiredData *ptr_pathDesired,
                                       FlightStatusData *ptr_flightStatus)
{
    PIOS_Assert(ptr_vtolPathFollowerSettings);
    PIOS_Assert(ptr_pathDesired);
    PIOS_Assert(ptr_flightStatus);

    if (mAutoTakeoffData == 0) {
        mAutoTakeoffData = (VtolAutoTakeoffFSMData_T *)pios_malloc(sizeof(VtolAutoTakeoffFSMData_T));
        PIOS_Assert(mAutoTakeoffData);
    }
    memset(mAutoTakeoffData, 0, sizeof(VtolAutoTakeoffFSMData_T));
    vtolPathFollowerSettings = ptr_vtolPathFollowerSettings;
    pathDesired  = ptr_pathDesired;
    flightStatus = ptr_flightStatus;
    initFSM();

    return 0;
}

void VtolAutoTakeoffFSM::Inactive(void)
{
    memset(mAutoTakeoffData, 0, sizeof(VtolAutoTakeoffFSMData_T));
    initFSM();
}

// Initialise the FSM
void VtolAutoTakeoffFSM::initFSM(void)
{
    if (vtolPathFollowerSettings != 0) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
    } else {
        mAutoTakeoffData->currentState = STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE;
    }
}

void VtolAutoTakeoffFSM::Activate()
{
    memset(mAutoTakeoffData, 0, sizeof(VtolAutoTakeoffFSMData_T));
    mAutoTakeoffData->currentState   = STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE;
    mAutoTakeoffData->flLowAltitude  = true;
    mAutoTakeoffData->flAltitudeHold = false;
    mAutoTakeoffData->boundThrustMin = 0.0f;
    mAutoTakeoffData->boundThrustMax = 0.0f;
    mAutoTakeoffData->flZeroStabiHorizontal = true; // turn off positional controllers
    TakeOffLocationGet(&(mAutoTakeoffData->takeOffLocation));
    mAutoTakeoffData->fsmAutoTakeoffStatus.AltitudeAtState[STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE] = 0.0f;
    mAutoTakeoffData->fsmAutoTakeoffStatus.ControlState = STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_WAITFORARMED;
    assessAltitude();

    // Check if we are already flying.  This can happen in pathplanner mode
    // going into a second loop of the waypoints.
    StabilizationDesiredData stabDesired;
    StabilizationDesiredGet(&stabDesired);
    if (stabDesired.Thrust > vtolPathFollowerSettings->ThrustLimits.Min) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_HOLD, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        return;
    }

    // initially inactive and wait for a change in controlstate.
    setState(STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
}

PathFollowerFSMState_T VtolAutoTakeoffFSM::GetCurrentState(void)
{
    switch (mAutoTakeoffData->currentState) {
    case STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE:
        return PFFSM_STATE_INACTIVE;

        break;
    case STATUSVTOLAUTOTAKEOFF_STATE_DISARMED:
        return PFFSM_STATE_DISARMED;

        break;
    default:
        return PFFSM_STATE_ACTIVE;

        break;
    }
}

void VtolAutoTakeoffFSM::Update()
{
    runState();
    if (GetCurrentState() != PFFSM_STATE_INACTIVE) {
        runAlways();
    }
}

int32_t VtolAutoTakeoffFSM::runState(void)
{
    uint8_t flTimeout = false;

    mAutoTakeoffData->stateRunCount++;

    if (mAutoTakeoffData->stateTimeoutCount > 0 && mAutoTakeoffData->stateRunCount > mAutoTakeoffData->stateTimeoutCount) {
        flTimeout = true;
    }

    // If the current state has a static function, call it
    if (sAutoTakeoffStateTable[mAutoTakeoffData->currentState].run) {
        (this->*sAutoTakeoffStateTable[mAutoTakeoffData->currentState].run)(flTimeout);
    }
    return 0;
}

int32_t VtolAutoTakeoffFSM::runAlways(void)
{
    void assessAltitude(void);

    return 0;
}

// PathFollower implements the PID scheme and has a objective
// set by a PathDesired object.  Based on the mode, pathfollower
// uses FSM's as helper functions that manage state and event detection.
// PathFollower calls into FSM methods to alter its commands.

void VtolAutoTakeoffFSM::BoundThrust(float &ulow, float &uhigh)
{
    ulow  = mAutoTakeoffData->boundThrustMin;
    uhigh = mAutoTakeoffData->boundThrustMax;


    if (mAutoTakeoffData->flConstrainThrust) {
        uhigh = mAutoTakeoffData->thrustLimit;
    }
}

void VtolAutoTakeoffFSM::ConstrainStabiDesired(StabilizationDesiredData *stabDesired)
{
    if (mAutoTakeoffData->flZeroStabiHorizontal && stabDesired) {
        stabDesired->Pitch = 0.0f;
        stabDesired->Roll  = 0.0f;
        stabDesired->Yaw   = 0.0f;
    }
}

// Set the new state and perform setup for subsequent state run calls
// This is called by state run functions on event detection that drive
// state transitions.
void VtolAutoTakeoffFSM::setState(StatusVtolAutoTakeoffStateOptions newState, StatusVtolAutoTakeoffStateExitReasonOptions reason)
{
    mAutoTakeoffData->fsmAutoTakeoffStatus.StateExitReason[mAutoTakeoffData->currentState] = reason;

    if (mAutoTakeoffData->currentState == newState) {
        return;
    }
    mAutoTakeoffData->currentState = newState;

    if (newState != STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE) {
        float altitudeAboveTakeoff = assessAltitude();
        mAutoTakeoffData->fsmAutoTakeoffStatus.AltitudeAtState[newState] = altitudeAboveTakeoff;
    }

    // Restart state timer counter
    mAutoTakeoffData->stateRunCount     = 0;

    // Reset state timeout to disabled/zero
    mAutoTakeoffData->stateTimeoutCount = 0;

    if (sAutoTakeoffStateTable[mAutoTakeoffData->currentState].setup) {
        (this->*sAutoTakeoffStateTable[mAutoTakeoffData->currentState].setup)();
    }

    // Update StatusVtolAutoTakeoff UAVObject with new status
    updateVtolAutoTakeoffFSMStatus();
}


// Timeout utility function for use by state init implementations
void VtolAutoTakeoffFSM::setStateTimeout(int32_t count)
{
    mAutoTakeoffData->stateTimeoutCount = count;
}

// Update StatusVtolAutoTakeoff UAVObject
void VtolAutoTakeoffFSM::updateVtolAutoTakeoffFSMStatus()
{
    mAutoTakeoffData->fsmAutoTakeoffStatus.State = mAutoTakeoffData->currentState;
    if (mAutoTakeoffData->flLowAltitude) {
        mAutoTakeoffData->fsmAutoTakeoffStatus.AltitudeState = STATUSVTOLAUTOTAKEOFF_ALTITUDESTATE_LOW;
    } else {
        mAutoTakeoffData->fsmAutoTakeoffStatus.AltitudeState = STATUSVTOLAUTOTAKEOFF_ALTITUDESTATE_HIGH;
    }
    StatusVtolAutoTakeoffSet(&mAutoTakeoffData->fsmAutoTakeoffStatus);
}


float VtolAutoTakeoffFSM::assessAltitude(void)
{
    float positionDown;

    PositionStateDownGet(&positionDown);
    float takeOffDown = 0.0f;
    if (mAutoTakeoffData->takeOffLocation.Status == TAKEOFFLOCATION_STATUS_VALID) {
        takeOffDown = mAutoTakeoffData->takeOffLocation.Down;
    }
    float positionDownRelativeToTakeoff = positionDown - takeOffDown;
    if (positionDownRelativeToTakeoff < AUTOTAKEOFF_SLOWDOWN_HEIGHT) {
        mAutoTakeoffData->flLowAltitude = false;
    } else {
        mAutoTakeoffData->flLowAltitude = true;
    }
    // Return the altitude above takeoff, which is the negation of positionDownRelativeToTakeoff
    return -positionDownRelativeToTakeoff;
}

// Action the required state from plans.c
void VtolAutoTakeoffFSM::setControlState(StatusVtolAutoTakeoffControlStateOptions controlState)
{
    mAutoTakeoffData->fsmAutoTakeoffStatus.ControlState = controlState;

    switch (controlState) {
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_WAITFORARMED:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_WAITFORMIDTHROTTLE:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_REQUIREUNARMEDFIRST:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_INACTIVE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_INITIATE:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_CHECKSTATE, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_POSITIONHOLD:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_HOLD, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    case STATUSVTOLAUTOTAKEOFF_CONTROLSTATE_ABORT:
        setState(STATUSVTOLAUTOTAKEOFF_STATE_HOLD, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        break;
    }
}


// State: INACTIVE
void VtolAutoTakeoffFSM::setup_inactive(void)
{
    // Re-initialise local variables
    mAutoTakeoffData->flZeroStabiHorizontal = false;
    mAutoTakeoffData->flConstrainThrust     = false;
}

// State: CHECKSTATE
void VtolAutoTakeoffFSM::setup_checkstate(void)
{
    // Assumptions that do not need to be checked if flight mode is AUTOTAKEOFF
    // 1. Already armed
    // 2. Not in flight. This was checked in plans.c
    // 3. User has placed throttle position to more than 30% to allow autotakeoff

    // If pathplanner, we need additional checks
    // E.g. if inflight, this mode is just position hold
    StabilizationDesiredData stabDesired;

    StabilizationDesiredGet(&stabDesired);
    if (stabDesired.Thrust > vtolPathFollowerSettings->ThrustLimits.Min) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_HOLD, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
        return;
    }


    // Start from a enforced thrust off condition
    mAutoTakeoffData->flConstrainThrust = false;
    mAutoTakeoffData->boundThrustMin    = -0.1f;
    mAutoTakeoffData->boundThrustMax    = 0.0f;

    setState(STATUSVTOLAUTOTAKEOFF_STATE_SLOWSTART, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_TIMEOUT);
}

// STATE: SLOWSTART spools up motors to vtol min over 2 seconds for effect.
// PID loops may be cumulating I terms but that problem needs to be solved
#define SLOWSTART_INITIAL_THRUST 0.05f // assumed to be less than vtol min
void VtolAutoTakeoffFSM::setup_slowstart(void)
{
    setStateTimeout(TIMEOUT_SLOWSTART);
    mAutoTakeoffData->flZeroStabiHorizontal = true; // turn off positional controllers
    StabilizationDesiredData stabDesired;
    StabilizationDesiredGet(&stabDesired);
    float vtol_thrust_min = vtolPathFollowerSettings->ThrustLimits.Min;
    if (vtol_thrust_min < SLOWSTART_INITIAL_THRUST) {
        vtol_thrust_min = SLOWSTART_INITIAL_THRUST;
    }
    mAutoTakeoffData->sum1 = (vtol_thrust_min - SLOWSTART_INITIAL_THRUST) / (float)TIMEOUT_SLOWSTART;
    mAutoTakeoffData->sum2 = vtol_thrust_min;
    mAutoTakeoffData->boundThrustMin = SLOWSTART_INITIAL_THRUST;
    mAutoTakeoffData->boundThrustMax = SLOWSTART_INITIAL_THRUST;
    PositionStateData positionState;
    PositionStateGet(&positionState);
    mAutoTakeoffData->expectedAutoTakeoffPositionNorth = positionState.North;
    mAutoTakeoffData->expectedAutoTakeoffPositionEast  = positionState.East;
}

void VtolAutoTakeoffFSM::run_slowstart(__attribute__((unused)) uint8_t flTimeout)
{
    // increase thrust setpoint step by step
    if (mAutoTakeoffData->boundThrustMin < mAutoTakeoffData->sum2) {
        mAutoTakeoffData->boundThrustMin += mAutoTakeoffData->sum1;
    }
    mAutoTakeoffData->boundThrustMax += mAutoTakeoffData->sum1;
    if (mAutoTakeoffData->boundThrustMax > mAutoTakeoffData->sum2) {
        mAutoTakeoffData->boundThrustMax = mAutoTakeoffData->sum2;
    }

    if (flTimeout) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_THRUSTUP, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_TIMEOUT);
    }
}

// STATE: THRUSTUP spools up motors to vtol min over 1 second for effect.
// PID loops may be cumulating I terms but that problem needs to be solved
#define THRUSTUP_FINAL_THRUST_AS_RATIO_OF_VTOLMAX 0.8f
void VtolAutoTakeoffFSM::setup_thrustup(void)
{
    setStateTimeout(TIMEOUT_THRUSTUP);
    mAutoTakeoffData->flZeroStabiHorizontal = false;
    mAutoTakeoffData->sum2 = THRUSTUP_FINAL_THRUST_AS_RATIO_OF_VTOLMAX * vtolPathFollowerSettings->ThrustLimits.Max;
    mAutoTakeoffData->sum1 = (mAutoTakeoffData->sum2 - mAutoTakeoffData->boundThrustMax) / (float)TIMEOUT_THRUSTUP;
    mAutoTakeoffData->boundThrustMin = vtolPathFollowerSettings->ThrustLimits.Min;
}

void VtolAutoTakeoffFSM::run_thrustup(__attribute__((unused)) uint8_t flTimeout)
{
    // increase thrust setpoint step by step
    mAutoTakeoffData->boundThrustMax += mAutoTakeoffData->sum1;
    if (mAutoTakeoffData->boundThrustMax > mAutoTakeoffData->sum2) {
        mAutoTakeoffData->boundThrustMax = mAutoTakeoffData->sum2;
    }

    if (flTimeout) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_TAKEOFF, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_TIMEOUT);
    }
}


// STATE: TAKEOFF
void VtolAutoTakeoffFSM::setup_takeoff(void)
{
    mAutoTakeoffData->flZeroStabiHorizontal = false;
}
void VtolAutoTakeoffFSM::run_takeoff(__attribute__((unused)) uint8_t flTimeout)
{
    StabilizationDesiredData stabDesired;

    StabilizationDesiredGet(&stabDesired);
    if (stabDesired.Thrust < 0.0f) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_THRUSTOFF, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_ZEROTHRUST);
        return;
    }

    // detect broad sideways drift.
    PositionStateData positionState;
    PositionStateGet(&positionState);
    float north_error   = mAutoTakeoffData->expectedAutoTakeoffPositionNorth - positionState.North;
    float east_error    = mAutoTakeoffData->expectedAutoTakeoffPositionEast - positionState.East;
    float down_error    = pathDesired->End.Down - positionState.Down;
    float positionError = sqrtf(north_error * north_error + east_error * east_error);
    if (positionError > 3.0f) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_THRUSTDOWN, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_POSITIONERROR);
        return;
    }
    if (fabsf(down_error) < 0.5f) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_HOLD, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_ARRIVEDATALT);
        return;
    }
}

// STATE: HOLD
void VtolAutoTakeoffFSM::setup_hold(void)
{
    mAutoTakeoffData->flZeroStabiHorizontal = false;
    mAutoTakeoffData->flAltitudeHold = true;
}
void VtolAutoTakeoffFSM::run_hold(__attribute__((unused)) uint8_t flTimeout)
{}

uint8_t VtolAutoTakeoffFSM::PositionHoldState(void)
{
    return mAutoTakeoffData->flAltitudeHold;
}

// STATE: THRUSTDOWN
void VtolAutoTakeoffFSM::setup_thrustdown(void)
{
    setStateTimeout(TIMEOUT_THRUSTDOWN);
    mAutoTakeoffData->flZeroStabiHorizontal = true;
    mAutoTakeoffData->flConstrainThrust     = true;
    StabilizationDesiredData stabDesired;
    StabilizationDesiredGet(&stabDesired);
    mAutoTakeoffData->thrustLimit    = stabDesired.Thrust;
    mAutoTakeoffData->sum1 = stabDesired.Thrust / (float)TIMEOUT_THRUSTDOWN;
    mAutoTakeoffData->boundThrustMin = -0.1f;
    mAutoTakeoffData->boundThrustMax = vtolPathFollowerSettings->ThrustLimits.Neutral;
}

void VtolAutoTakeoffFSM::run_thrustdown(__attribute__((unused)) uint8_t flTimeout)
{
    // reduce thrust setpoint step by step
    mAutoTakeoffData->thrustLimit -= mAutoTakeoffData->sum1;

    StabilizationDesiredData stabDesired;
    StabilizationDesiredGet(&stabDesired);
    if (stabDesired.Thrust < 0.0f || mAutoTakeoffData->thrustLimit < 0.0f) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_THRUSTOFF, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_ZEROTHRUST);
    }

    if (flTimeout) {
        setState(STATUSVTOLAUTOTAKEOFF_STATE_THRUSTOFF, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_TIMEOUT);
    }
}

// STATE: THRUSTOFF
void VtolAutoTakeoffFSM::setup_thrustoff(void)
{
    mAutoTakeoffData->thrustLimit       = -1.0f;
    mAutoTakeoffData->flConstrainThrust = true;
    mAutoTakeoffData->boundThrustMin    = -0.1f;
    mAutoTakeoffData->boundThrustMax    = 0.0f;
}

void VtolAutoTakeoffFSM::run_thrustoff(__attribute__((unused)) uint8_t flTimeout)
{
    setState(STATUSVTOLAUTOTAKEOFF_STATE_DISARMED, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
}

// STATE: DISARMED
void VtolAutoTakeoffFSM::setup_disarmed(void)
{
    mAutoTakeoffData->thrustLimit = -1.0f;
    mAutoTakeoffData->flConstrainThrust     = true;
    mAutoTakeoffData->flZeroStabiHorizontal = true;
    mAutoTakeoffData->observationCount = 0;
    mAutoTakeoffData->boundThrustMin   = -0.1f;
    mAutoTakeoffData->boundThrustMax   = 0.0f;

    if (flightStatus->ControlChain.PathPlanner != FLIGHTSTATUS_CONTROLCHAIN_TRUE) {
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_CRITICAL);
    }
}

void VtolAutoTakeoffFSM::run_disarmed(__attribute__((unused)) uint8_t flTimeout)
{
    if (flightStatus->ControlChain.PathPlanner != FLIGHTSTATUS_CONTROLCHAIN_TRUE) {
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_CRITICAL);
    }
#ifdef DEBUG_GROUNDIMPACT
    if (mAutoTakeoffData->observationCount++ > 100) {
        setState(AUTOTAKEOFF_STATE_WTG_FOR_GROUNDEFFECT, STATUSVTOLAUTOTAKEOFF_STATEEXITREASON_NONE);
    }
#endif
}
