/*
 ******************************************************************************
 *
 * @file       VtolVelocityController.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Velocity roam controller for vtols
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
#include <alarms.h>
#include <CoordinateConversions.h>
#include <sin_lookup.h>
#include <pathdesired.h>
#include <paths.h>
#include "plans.h"
#include <sanitycheck.h>

#include <vtolpathfollowersettings.h>
#include <flightmodesettings.h>
#include <pathstatus.h>
#include <positionstate.h>
#include <velocitystate.h>
#include <velocitydesired.h>
#include <stabilizationdesired.h>
#include <attitudestate.h>
#include <flightstatus.h>
#include <manualcontrolcommand.h>
#include <systemsettings.h>
#include <stabilizationbank.h>
#include <stabilizationdesired.h>
#include <pathsummary.h>
}

// C++ includes
#include "vtolvelocitycontroller.h"
#include "pidcontrolne.h"

// Private constants

// pointer to a singleton instance
VtolVelocityController *VtolVelocityController::p_inst = 0;

VtolVelocityController::VtolVelocityController()
    : vtolPathFollowerSettings(0), mActive(false)
{}

// Called when mode first engaged
void VtolVelocityController::Activate(void)
{
    if (!mActive) {
        mActive = true;
        SettingsUpdated();
        controlNE.Activate();
    }
}

uint8_t VtolVelocityController::IsActive(void)
{
    return mActive;
}

uint8_t VtolVelocityController::Mode(void)
{
    return PATHDESIRED_MODE_VELOCITY;
}

void VtolVelocityController::ObjectiveUpdated(void)
{
    controlNE.UpdateVelocitySetpoint(pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_NORTH],
                                     pathDesired->ModeParameters[PATHDESIRED_MODEPARAMETER_VELOCITY_VELOCITYVECTOR_EAST]);
}

void VtolVelocityController::Deactivate(void)
{
    if (mActive) {
        mActive = false;
        controlNE.Deactivate();
    }
}

void VtolVelocityController::SettingsUpdated(void)
{
    const float dT = vtolPathFollowerSettings->UpdatePeriod / 1000.0f;

    controlNE.UpdateParameters(vtolPathFollowerSettings->VelocityRoamHorizontalVelPID.Kp,
                               vtolPathFollowerSettings->VelocityRoamHorizontalVelPID.Ki,
                               vtolPathFollowerSettings->VelocityRoamHorizontalVelPID.Kd,
                               vtolPathFollowerSettings->VelocityRoamHorizontalVelPID.Beta,
                               dT,
                               vtolPathFollowerSettings->HorizontalVelMax);


    controlNE.UpdatePositionalParameters(vtolPathFollowerSettings->HorizontalPosP);
    controlNE.UpdateCommandParameters(-vtolPathFollowerSettings->VelocityRoamMaxRollPitch, vtolPathFollowerSettings->VelocityRoamMaxRollPitch, vtolPathFollowerSettings->VelocityFeedforward);
}

int32_t VtolVelocityController::Initialize(VtolPathFollowerSettingsData *ptr_vtolPathFollowerSettings)
{
    PIOS_Assert(ptr_vtolPathFollowerSettings);

    vtolPathFollowerSettings = ptr_vtolPathFollowerSettings;
    return 0;
}


void VtolVelocityController::UpdateVelocityDesired()
{
    VelocityStateData velocityState;

    VelocityStateGet(&velocityState);
    VelocityDesiredData velocityDesired;

    controlNE.UpdateVelocityState(velocityState.North, velocityState.East);

    velocityDesired.Down  = 0.0f;
    float north, east;
    controlNE.GetVelocityDesired(&north, &east);
    velocityDesired.North = north;
    velocityDesired.East  = east;

    // update pathstatus
    pathStatus->error     = 0.0f;
    pathStatus->fractional_progress  = 0.0f;
    pathStatus->path_direction_north = velocityDesired.North;
    pathStatus->path_direction_east  = velocityDesired.East;
    pathStatus->path_direction_down  = velocityDesired.Down;

    pathStatus->correction_direction_north = velocityDesired.North - velocityState.North;
    pathStatus->correction_direction_east  = velocityDesired.East - velocityState.East;
    pathStatus->correction_direction_down  = 0.0f;

    VelocityDesiredSet(&velocityDesired);
}

int8_t VtolVelocityController::UpdateStabilizationDesired(__attribute__((unused)) bool yaw_attitude, __attribute__((unused)) float yaw_direction)
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

    float angle_radians = DEG2RAD(attitudeState.Yaw);
    float cos_angle     = cosf(angle_radians);
    float sine_angle    = sinf(angle_radians);
    float maxPitch = vtolPathFollowerSettings->VelocityRoamMaxRollPitch;
    stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Pitch = boundf(-northCommand * cos_angle - eastCommand * sine_angle, -maxPitch, maxPitch);
    stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
    stabDesired.Roll = boundf(-northCommand * sine_angle + eastCommand * cos_angle, -maxPitch, maxPitch);

    ManualControlCommandData manualControl;
    ManualControlCommandGet(&manualControl);

    stabDesired.StabilizationMode.Yaw = STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK;
    stabDesired.Yaw = stabSettings.MaximumRate.Yaw * manualControl.Yaw;

    // default thrust mode to altvario
    stabDesired.StabilizationMode.Thrust = STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO;
    stabDesired.Thrust = manualControl.Thrust;

    StabilizationDesiredSet(&stabDesired);

    return result;
}

void VtolVelocityController::UpdateAutoPilot()
{
    UpdateVelocityDesired();

    bool yaw_attitude = false;
    float yaw = 0.0f;

    int8_t result     = UpdateStabilizationDesired(yaw_attitude, yaw);
    if (result) {
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_OK);
    } else {
        pathStatus->Status = PATHSTATUS_STATUS_CRITICAL;
        AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE, SYSTEMALARMS_ALARM_WARNING);
    }

    PathStatusSet(pathStatus);
}
