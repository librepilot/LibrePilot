/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup PathFollower CONTROL interface class
 * @brief PID controller for NE
 * @{
 *
 * @file       PIDControlNE.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Executes PID control loops for NE directions
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
extern "C" {
#include <openpilot.h>

#include <math.h>
#include <pid.h>
#include <CoordinateConversions.h>
#include <sin_lookup.h>
#include <pathdesired.h>
#include <paths.h>
#include "plans.h"
#include <pidstatus.h>
}
#include "pathfollowerfsm.h"
#include "pidcontrolne.h"

PIDControlNE::PIDControlNE()
    : deltaTime(0), mNECommand(0), mNeutral(0), mVelocityMax(0), mMinCommand(0), mMaxCommand(0), mVelocityFeedforward(0), mActive(false)
{
    memset((void *)PIDvel, 0, 2 * sizeof(pid2));
    memset((void *)PIDposH, 0, 2 * sizeof(pid));

    mVelocitySetpointTarget[0]  = 0.0f;
    mVelocitySetpointTarget[1]  = 0.0f;
    mVelocityState[0] = 0.0f;
    mVelocityState[1] = 0.0f;
    mPositionSetpointTarget[0]  = 0.0f;
    mPositionSetpointTarget[1]  = 0.0f;
    mPositionState[0] = 0.0f;
    mPositionState[1] = 0.0f;
    mVelocitySetpointCurrent[0] = 0.0f;
    mVelocitySetpointCurrent[1] = 0.0f;
}

PIDControlNE::~PIDControlNE() {}

void PIDControlNE::Deactivate()
{
    mActive = false;
}

void PIDControlNE::Activate()
{
    mActive = true;
    pid2_transfer(&PIDvel[0], 0.0f);
    pid2_transfer(&PIDvel[1], 0.0f);
    pid_zero(&PIDposH[0]);
    pid_zero(&PIDposH[1]);
}

void PIDControlNE::UpdateParameters(float kp, float ki, float kd, float beta, float dT, float velocityMax)
{
    float Td; // Derivative time constant
    float Ti; // Integral time constant
    float kt; // Feedback gain for integral windup avoidance
    float N = 10.0f; // N is the factor used to determine the
                     // time constant for derivative filtering
                     // Why 10? Maybe should be configurable?
    float Tf; // Low pass filter time constant for derivative filtering

    float u0 = 0.0f;

    // Define Td, handling zero kp term (for I or ID controller)
    if (kp < 1e-6f) {
        Td = 1e6f;
    } else {
        Td = kd / kp;
    }

    // Define Ti, Tt and kt, handling zero ki term (for P or PD controller)
    if (ki < 1e-6f) { // Avoid Ti being infinite
        kt = 0.0f;
    } else {
        Ti = kp / ki;
        kt = 1.0f / Ti;
    }

    // Define Tf, according to controller type
    if (kd < 1e-6f) {
        // PI Controller or P Controller
        Tf = 0;
    } else {
        Tf = Td / N;
    }

    if (beta > 1.0f) {
        beta = 1.0f;
    } else if (beta < 0.4f) {
        beta = 0.4f;
    }

    pid2_configure(&PIDvel[0], kp, ki, kd, Tf, kt, dT, beta, u0, 0.0f, 1.0f);
    pid2_configure(&PIDvel[1], kp, ki, kd, Tf, kt, dT, beta, u0, 0.0f, 1.0f);
    deltaTime    = dT;
    mVelocityMax = velocityMax;
}

void PIDControlNE::UpdatePositionalParameters(float kp)
{
    pid_configure(&PIDposH[0], kp, 0.0f, 0.0f, 0.0f);
    pid_configure(&PIDposH[1], kp, 0.0f, 0.0f, 0.0f);
}
void PIDControlNE::UpdatePositionSetpoint(float setpointNorth, float setpointEast)
{
    mPositionSetpointTarget[0] = setpointNorth;
    mPositionSetpointTarget[1] = setpointEast;
}
void PIDControlNE::UpdatePositionState(float pvNorth, float pvEast)
{
    mPositionState[0] = pvNorth;
    mPositionState[1] = pvEast;
}
// This is a pure position hold position control
void PIDControlNE::ControlPosition()
{
    // Current progress location relative to end
    float velNorth = 0.0f;
    float velEast  = 0.0f;

    velNorth = pid_apply(&PIDposH[0], mPositionSetpointTarget[0] - mPositionState[0], deltaTime);
    velEast  = pid_apply(&PIDposH[1], mPositionSetpointTarget[1] - mPositionState[1], deltaTime);
    UpdateVelocitySetpoint(velNorth, velEast);
}

void PIDControlNE::ControlPositionWithPath(struct path_status *progress)
{
    // Current progress location relative to end
    float velNorth = progress->path_vector[0];
    float velEast  = progress->path_vector[1];

    velNorth += pid_apply(&PIDposH[0], progress->correction_vector[0], deltaTime);
    velEast  += pid_apply(&PIDposH[1], progress->correction_vector[1], deltaTime);
    UpdateVelocitySetpoint(velNorth, velEast);
}

void PIDControlNE::UpdateVelocitySetpoint(float setpointNorth, float setpointEast)
{
    // scale velocity if it is above configured maximum
    // for braking, we can not help it if initial velocity was greater
    float velH = sqrtf(setpointNorth * setpointNorth + setpointEast * setpointEast);

    if (velH > mVelocityMax) {
        setpointNorth *= mVelocityMax / velH;
        setpointEast  *= mVelocityMax / velH;
    }

    mVelocitySetpointTarget[0] = setpointNorth;
    mVelocitySetpointTarget[1] = setpointEast;
}


void PIDControlNE::UpdateBrakeVelocity(float startingVelocity, float dT, float brakeRate, float currentVelocity, float *updatedVelocity)
{
    if (startingVelocity >= 0.0f) {
        *updatedVelocity = startingVelocity - dT * brakeRate;
        if (*updatedVelocity > currentVelocity) {
            *updatedVelocity = currentVelocity;
        }
        if (*updatedVelocity < 0.0f) {
            *updatedVelocity = 0.0f;
        }
    } else {
        *updatedVelocity = startingVelocity + dT * brakeRate;
        if (*updatedVelocity < currentVelocity) {
            *updatedVelocity = currentVelocity;
        }
        if (*updatedVelocity > 0.0f) {
            *updatedVelocity = 0.0f;
        }
    }
}

void PIDControlNE::UpdateVelocityStateWithBrake(float pvNorth, float pvEast, float path_time, float brakeRate)
{
    mVelocityState[0] = pvNorth;
    mVelocityState[1] = pvEast;

    float velocitySetpointDesired[2];

    UpdateBrakeVelocity(mVelocitySetpointTarget[0], path_time, brakeRate, pvNorth, &velocitySetpointDesired[0]);
    UpdateBrakeVelocity(mVelocitySetpointTarget[1], path_time, brakeRate, pvEast, &velocitySetpointDesired[1]);

    // If rate of change limits required, add here
    for (int iaxis = 0; iaxis < 2; iaxis++) {
        mVelocitySetpointCurrent[iaxis] = velocitySetpointDesired[iaxis];
    }
}

void PIDControlNE::UpdateVelocityState(float pvNorth, float pvEast)
{
    mVelocityState[0] = pvNorth;
    mVelocityState[1] = pvEast;

    // The FSM controls the actual descent velocity and introduces step changes as required
    float velocitySetpointDesired[2];
    velocitySetpointDesired[0] = mVelocitySetpointTarget[0];
    velocitySetpointDesired[1] = mVelocitySetpointTarget[1];

    // If rate of change limits required, add here
    for (int iaxis = 0; iaxis < 2; iaxis++) {
        mVelocitySetpointCurrent[iaxis] = velocitySetpointDesired[iaxis];
    }
}


void PIDControlNE::UpdateCommandParameters(float minCommand, float maxCommand, float velocityFeedforward)
{
    mMinCommand = minCommand;
    mMaxCommand = maxCommand;
    mVelocityFeedforward = velocityFeedforward;
}


void PIDControlNE::GetNECommand(float *northCommand, float *eastCommand)
{
    PIDvel[0].va  = mVelocitySetpointCurrent[0] * mVelocityFeedforward;
    *northCommand = pid2_apply(&(PIDvel[0]), mVelocitySetpointCurrent[0], mVelocityState[0], mMinCommand, mMaxCommand);
    PIDvel[1].va  = mVelocitySetpointCurrent[1] * mVelocityFeedforward;
    *eastCommand  = pid2_apply(&(PIDvel[1]), mVelocitySetpointCurrent[1], mVelocityState[1], mMinCommand, mMaxCommand);

    PIDStatusData pidStatus;
    pidStatus.setpoint = mVelocitySetpointCurrent[0];
    pidStatus.actual   = mVelocityState[0];
    pidStatus.error    = mVelocitySetpointCurrent[0] - mVelocityState[0];
    pidStatus.setpoint = mVelocitySetpointCurrent[0];
    pidStatus.ulow     = mMinCommand;
    pidStatus.uhigh    = mMaxCommand;
    pidStatus.command  = *northCommand;
    pidStatus.P = PIDvel[0].P;
    pidStatus.I = PIDvel[0].I;
    pidStatus.D = PIDvel[0].D;
    PIDStatusSet(&pidStatus);
}

void PIDControlNE::GetVelocityDesired(float *north, float *east)
{
    *north = mVelocitySetpointCurrent[0];
    *east  = mVelocitySetpointCurrent[1];
}
