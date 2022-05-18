/*
 ******************************************************************************
 *
 * @file       vtollandcontroller.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Vtol landing controller loop
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup LibrePilot LibrePilotModules Modules PathFollower Navigation
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
#include <alarms.h>
#include <CoordinateConversions.h>
#include <sin_lookup.h>
#include <pathdesired.h>
#include <paths.h>
#include "plans.h"
#include <sanitycheck.h>

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
#include <pathsummary.h>
}

// C++ includes
#include "vtollandcontroller.h"
#include "pathfollowerfsm.h"
#include "vtollandfsm.h"
#include "pidcontroldown.h"

// Private constants

// pointer to a singleton instance
VtolLandController *VtolLandController::p_inst = 0;

VtolLandController::VtolLandController()
    : fsm(NULL), vtolPathFollowerSettings(NULL), mActive(false)
{}

// Called when mode first engaged
void VtolLandController::Activate(void)
{
    if (!mActive) {
        mActive   = true;
        mOverride = true;
        SettingsUpdated();
        fsm->Activate();
        controlDown.Activate();
        controlNE.Activate();
    }
}

uint8_t VtolLandController::IsActive(void)
{
    return mActive;
}

uint8_t VtolLandController::Mode(void)
{
    return PATHDESIRED_MODE_LAND;
}

// Objective updated in pathdesired, e.g. same flight mode but new target velocity
void VtolLandController::ObjectiveUpdated(void)
{
    if (mOverride) {
        // override pathDesired from PathPLanner with current position,
        // as we deliberately don' not care about the location of the waypoints on the map
        float velocity_down;
        PositionStateData positionState;
        PositionStateGet(&positionState);
        FlightModeSettingsLandingVelocityGet(&velocity_down);
        controlDown.UpdateVelocitySetpoint(velocity_down);
        controlNE.UpdateVelocitySetpoint(0.0f, 0.0f);
        controlNE.UpdatePositionSetpoint(positionState.North, positionState.East);
        mOverride = false; // further updates always come from ManualControl and will control horizontal position
    } else {
        // Set the objective's target velocity
        controlDown.UpdateVelocitySetpoint(pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_DOWN]);
        controlNE.UpdateVelocitySetpoint(pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_NORTH],
                                         pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_EAST]);
        controlNE.UpdatePositionSetpoint(pathDesired->End.North, pathDesired->End.East);
    }
}
void VtolLandController::Deactivate(void)
{
    if (mActive) {
        mActive = false;
        fsm->Inactive();
        controlDown.Deactivate();
        controlNE.Deactivate();
    }
}


void VtolLandController::SettingsUpdated(void)
{
    const float dT = vtolPathFollowerSettings->UpdatePeriod / 1000.0f;

    controlNE.UpdateParameters(vtolPathFollowerSettings->HorizontalVelPID.Kp,
                               vtolPathFollowerSettings->HorizontalVelPID.Ki,
                               vtolPathFollowerSettings->HorizontalVelPID.Kd,
                               vtolPathFollowerSettings->HorizontalVelPID.Beta,
                               dT,
                               vtolPathFollowerSettings->HorizontalVelMax);


    controlNE.UpdatePositionalParameters(vtolPathFollowerSettings->HorizontalPosP);
    controlNE.UpdateCommandParameters(-vtolPathFollowerSettings->MaxRollPitch, vtolPathFollowerSettings->MaxRollPitch, vtolPathFollowerSettings->VelocityFeedforward);

    controlDown.UpdateParameters(vtolPathFollowerSettings->LandVerticalVelPID.Kp,
                                 vtolPathFollowerSettings->LandVerticalVelPID.Ki,
                                 vtolPathFollowerSettings->LandVerticalVelPID.Kd,
                                 vtolPathFollowerSettings->LandVerticalVelPID.Beta,
                                 dT,
                                 vtolPathFollowerSettings->VerticalVelMax);

    // The following is not currently used in the landing control.
    controlDown.UpdatePositionalParameters(vtolPathFollowerSettings->VerticalPosP);

    VtolSelfTuningStatsData vtolSelfTuningStats;
    VtolSelfTuningStatsGet(&vtolSelfTuningStats);
    controlDown.UpdateNeutralThrust(vtolSelfTuningStats.NeutralThrustOffset + vtolPathFollowerSettings->ThrustLimits.Neutral);
    // initialise limits on thrust but note the FSM can override.
    controlDown.SetThrustLimits(vtolPathFollowerSettings->ThrustLimits.Min, vtolPathFollowerSettings->ThrustLimits.Max);
    fsm->SettingsUpdated();
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t VtolLandController::Initialize(VtolPathFollowerSettingsData *ptr_vtolPathFollowerSettings)
{
    PIOS_Assert(ptr_vtolPathFollowerSettings);
    if (fsm == 0) {
        fsm = (PathFollowerFSM *)VtolLandFSM::instance();
        VtolLandFSM::instance()->Initialize(ptr_vtolPathFollowerSettings, pathDesired, flightStatus);
        vtolPathFollowerSettings = ptr_vtolPathFollowerSettings;
        controlDown.Initialize(fsm);
    }

    return 0;
}


void VtolLandController::UpdateVelocityDesired()
{
    VelocityStateData velocityState;

    VelocityStateGet(&velocityState);
    VelocityDesiredData velocityDesired;

    controlDown.UpdateVelocityState(velocityState.Down);
    controlNE.UpdateVelocityState(velocityState.North, velocityState.East);

    // Implement optional horizontal position hold.
    if ((((uint8_t)pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_LAND_OPTIONS]) == PATHDESIRED_MODEPARAMETER_LAND_OPTION_HORIZONTAL_PH) ||
        (flightStatus->ControlChain.PathPlanner == FLIGHTSTATUS_CONTROLCHAIN_TRUE)) {
        // landing flight mode has stored original horizontal position in pathdesired
        PositionStateData positionState;
        PositionStateGet(&positionState);
        controlNE.UpdatePositionState(positionState.North, positionState.East);
        controlNE.ControlPosition();
    }

    velocityDesired.Down  = controlDown.GetVelocityDesired();
    float north, east;
    controlNE.GetVelocityDesired(&north, &east);
    velocityDesired.North = north;
    velocityDesired.East  = east;

    // update pathstatus
    pathStatus->error     = 0.0f;
    pathStatus->fractional_progress = 0.0f;
    if (fsm->GetCurrentState() == PFFSM_STATE_DISARMED) {
        pathStatus->fractional_progress = 1.0f;
    }
    pathStatus->path_direction_north = velocityDesired.North;
    pathStatus->path_direction_east  = velocityDesired.East;
    pathStatus->path_direction_down  = velocityDesired.Down;

    pathStatus->correction_direction_north = velocityDesired.North - velocityState.North;
    pathStatus->correction_direction_east  = velocityDesired.East - velocityState.East;
    pathStatus->correction_direction_down  = velocityDesired.Down - velocityState.Down;


    VelocityDesiredSet(&velocityDesired);
}

int8_t VtolLandController::UpdateStabilizationDesired(bool yaw_attitude, float yaw_direction)
{
    uint8_t result = 1;
    StabilizationDesiredData stabDesired;
    AttitudeStateData attitudeState;
    StabilizationBankData stabSettings;
    float northCommand;
    float eastCommand;

    StabilizationDesiredGet(&stabDesired);
    AttitudeStateGet(&attitudeState);
    StabilizationBankGet(&stabSettings);

    controlNE.GetNECommand(&northCommand, &eastCommand);
    stabDesired.Thrust = controlDown.GetDownCommand();

    float angle_radians = DEG2RAD(attitudeState.Yaw);
    float cos_angle     = cosf(angle_radians);
    float sine_angle    = sinf(angle_radians);
    float maxPitch = vtolPathFollowerSettings->MaxRollPitch;
    stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Pitch = boundf(-northCommand * cos_angle - eastCommand * sine_angle, -maxPitch, maxPitch);
    stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Roll = boundf(-northCommand * sine_angle + eastCommand * cos_angle, -maxPitch, maxPitch);

    ManualControlCommandData manualControl;
    ManualControlCommandGet(&manualControl);

    if (yaw_attitude) {
        stabDesired.StabilizationMode.Yaw = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.Yaw = yaw_direction;
    } else {
        stabDesired.StabilizationMode.Yaw = STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK;
        stabDesired.Yaw = stabSettings.MaximumRate.Yaw * manualControl.Yaw;
    }

    // default thrust mode to cruise control
    stabDesired.StabilizationMode.Thrust = STABILIZATIONDESIRED_STABILIZATIONMODE_CRUISECONTROL;

    fsm->ConstrainStabiDesired(&stabDesired); // excludes thrust
    StabilizationDesiredSet(&stabDesired);

    return result;
}

void VtolLandController::UpdateAutoPilot()
{
    fsm->Update();

    UpdateVelocityDesired();

    // yaw behaviour is configurable in vtolpathfollower, select yaw control algorithm
    bool yaw_attitude = false;
    float yaw = 0.0f;

    fsm->GetYaw(yaw_attitude, yaw);

    int8_t result = UpdateStabilizationDesired(yaw_attitude, yaw);
    if (result) {
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_OK);
    } else {
        pathStatus->Status = PATHSTATUS_STATUS_CRITICAL;
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_WARNING);
    }

    PathStatusSet(pathStatus);
}
