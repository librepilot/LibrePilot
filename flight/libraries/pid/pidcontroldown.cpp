/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup PathFollower PID interface class
 * @brief PID loop for down control
 * @{
 *
 * @file       pidcontroldown.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Executes PID control for down direction
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
#include <stabilizationdesired.h>
#include <vtolselftuningstats.h>
}

#include "pidcontroldown.h"

#define NEUTRALTHRUST_PH_POSITIONAL_ERROR_LIMIT 0.5f
#define NEUTRALTHRUST_PH_VEL_DESIRED_LIMIT      0.2f
#define NEUTRALTHRUST_PH_VEL_STATE_LIMIT        0.2f
#define NEUTRALTHRUST_PH_VEL_ERROR_LIMIT        0.1f

#define NEUTRALTHRUST_START_DELAY               (2 * 20) // 2 seconds at rate of 20Hz (50ms update rate)
#define NEUTRALTHRUST_END_COUNT                 (NEUTRALTHRUST_START_DELAY + (4 * 20))  // 4 second sample


PIDControlDown::PIDControlDown()
    : deltaTime(0.0f), mVelocitySetpointTarget(0.0f), mVelocitySetpointCurrent(0.0f), mVelocityState(0.0f), mDownCommand(0.0f),
    mCallback(NULL), mNeutral(0.5f), mVelocityMax(1.0f), mPositionSetpointTarget(0.0f), mPositionState(0.0f),
    mMinThrust(0.1f), mMaxThrust(0.6f), mActive(false), mAllowNeutralThrustCalc(true)
{
    Deactivate();
}

PIDControlDown::~PIDControlDown() {}

void PIDControlDown::Initialize(PIDControlDownCallback *callback)
{
    mCallback = callback;
}

void PIDControlDown::SetThrustLimits(float min_thrust, float max_thrust)
{
    mMinThrust = min_thrust;
    mMaxThrust = max_thrust;
}

void PIDControlDown::Deactivate()
{
    mActive = false;
}

void PIDControlDown::Activate()
{
    float currentThrust;

    StabilizationDesiredThrustGet(&currentThrust);
    pid2_transfer(&PID, currentThrust);
    mActive = true;
}

void PIDControlDown::UpdateParameters(float kp, float ki, float kd, float beta, float dT, float velocityMax)
{
    float Td; // Derivative time constant
    float Ti; // Integral time constant
    float kt; // Feedback gain for integral windup avoidance
    float N = 10.0f; // N is the factor used to determine the
                     // time constant for derivative filtering
                     // Why 10? Maybe should be configurable?
    float Tf; // Low pass filter time constant for derivative filtering

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

    pid2_configure(&PID, kp, ki, kd, Tf, kt, dT, beta, mNeutral, mNeutral, -1.0f);
    deltaTime    = dT;
    mVelocityMax = velocityMax;
}


void PIDControlDown::UpdatePositionalParameters(float kp)
{
    pid_configure(&PIDpos, kp, 0.0f, 0.0f, 0.0f);
}
void PIDControlDown::UpdatePositionSetpoint(float setpointDown)
{
    mPositionSetpointTarget = setpointDown;
}
void PIDControlDown::UpdatePositionState(float pvDown)
{
    mPositionState = pvDown;
    setup_neutralThrustCalc();
}
// This is a pure position hold position control
void PIDControlDown::ControlPosition()
{
    // Current progress location relative to end
    float velDown = 0.0f;

    velDown = pid_apply(&PIDpos, mPositionSetpointTarget - mPositionState, deltaTime);
    UpdateVelocitySetpoint(velDown);

    run_neutralThrustCalc();
}


void PIDControlDown::ControlPositionWithPath(struct path_status *progress)
{
    // Current progress location relative to end
    float velDown = progress->path_vector[2];

    velDown += pid_apply(&PIDpos, progress->correction_vector[2], deltaTime);
    UpdateVelocitySetpoint(velDown);
}


void PIDControlDown::run_neutralThrustCalc(void)
{
    // if auto thrust and we have not run a correction calc check for PH and stability to then run an assessment
    // note that arming into this flight mode is not allowed, so assumption here is that
    // we have not landed.  If GPSAssist+Manual/Cruise control thrust mode is used, a user overriding the
    // altitude maintaining PID will have moved the throttle state to FLIGHTSTATUS_ASSISTEDTHROTTLESTATE_MANUAL.
    // In manualcontrol.c the state will stay in manual throttle until the throttle command exceeds the vtol thrust min,
    // avoiding auto-takeoffs.  Therefore no need to check that here.

    if (neutralThrustEst.have_correction != true) {
        // Make FSM specific
        bool stable = (fabsf(mPositionSetpointTarget - mPositionState) < NEUTRALTHRUST_PH_POSITIONAL_ERROR_LIMIT &&
                       fabsf(mVelocitySetpointCurrent) < NEUTRALTHRUST_PH_VEL_DESIRED_LIMIT &&
                       fabsf(mVelocityState) < NEUTRALTHRUST_PH_VEL_STATE_LIMIT &&
                       fabsf(mVelocitySetpointCurrent - mVelocityState) < NEUTRALTHRUST_PH_VEL_ERROR_LIMIT);

        if (stable) {
            if (neutralThrustEst.start_sampling) {
                neutralThrustEst.count++;


                // delay count for 2 seconds into hold allowing for stablisation
                if (neutralThrustEst.count > NEUTRALTHRUST_START_DELAY) {
                    neutralThrustEst.sum += PID.I;
                    if (PID.I < neutralThrustEst.min) {
                        neutralThrustEst.min = PID.I;
                    }
                    if (PID.I > neutralThrustEst.max) {
                        neutralThrustEst.max = PID.I;
                    }
                }

                if (neutralThrustEst.count >= NEUTRALTHRUST_END_COUNT) {
                    // 6 seconds have past
                    // lets take an average
                    neutralThrustEst.average    = neutralThrustEst.sum / (float)(NEUTRALTHRUST_END_COUNT - NEUTRALTHRUST_START_DELAY);
                    neutralThrustEst.correction = neutralThrustEst.average;

                    PID.I -= neutralThrustEst.average;

                    neutralThrustEst.start_sampling  = false;
                    neutralThrustEst.have_correction = true;

                    // Write a new adjustment value
                    // vtolSelfTuningStats.NeutralThrustOffset  was incremental adjusted above
                    VtolSelfTuningStatsData vtolSelfTuningStats;
                    VtolSelfTuningStatsGet(&vtolSelfTuningStats);
                    // add the average remaining i value to the
                    vtolSelfTuningStats.NeutralThrustOffset     += neutralThrustEst.correction;
                    vtolSelfTuningStats.NeutralThrustCorrection  = neutralThrustEst.correction; // the i term thrust correction value applied
                    vtolSelfTuningStats.NeutralThrustAccumulator = PID.I; // the actual iaccumulator value after correction
                    vtolSelfTuningStats.NeutralThrustRange = neutralThrustEst.max - neutralThrustEst.min;
                    VtolSelfTuningStatsSet(&vtolSelfTuningStats);
                }
            } else {
                // start a tick count
                neutralThrustEst.start_sampling = true;
                neutralThrustEst.count = 0;
                neutralThrustEst.sum   = 0.0f;
                neutralThrustEst.max   = 0.0f;
                neutralThrustEst.min   = 0.0f;
            }
        } else {
            // reset sampling as we did't get 6 continuous seconds
            neutralThrustEst.start_sampling = false;
        }
    } // else we already have a correction for this PH run
}


void PIDControlDown::setup_neutralThrustCalc(void)
{
    // reset neutral thrust assessment.
    // and do once for each position hold engagement
    neutralThrustEst.start_sampling  = false;
    neutralThrustEst.count = 0;
    neutralThrustEst.sum = 0.0f;
    neutralThrustEst.have_correction = false;
    neutralThrustEst.average    = 0.0f;
    neutralThrustEst.correction = 0.0f;
    neutralThrustEst.min = 0.0f;
    neutralThrustEst.max = 0.0f;
}


void PIDControlDown::UpdateNeutralThrust(float neutral)
{
    if (mActive) {
        // adjust neutral and achieve bumpless transfer
        PID.va = neutral;
        pid2_transfer(&PID, mDownCommand);
    }
    mNeutral = neutral;
}

void PIDControlDown::UpdateVelocitySetpoint(float setpoint)
{
    mVelocitySetpointTarget = setpoint;
    if (fabsf(mVelocitySetpointTarget) > mVelocityMax) {
        // maintain sign but set to max
        mVelocitySetpointTarget *= mVelocityMax / fabsf(mVelocitySetpointTarget);
    }
}

void PIDControlDown::RateLimit(float *spDesired, float *spCurrent, float rateLimit)
{
    float velocity_delta = *spDesired - *spCurrent;

    if (fabsf(velocity_delta) < 1e-6f) {
        *spCurrent = *spDesired;
        return;
    }

    // Calculate the rate of change
    float accelerationDesired = velocity_delta / deltaTime;

    if (fabsf(accelerationDesired) > rateLimit) {
        accelerationDesired *= rateLimit / accelerationDesired;
    }

    if (fabsf(accelerationDesired) < 0.1f) {
        *spCurrent = *spDesired;
    } else {
        *spCurrent += accelerationDesired * deltaTime;
    }
}

void PIDControlDown::UpdateBrakeVelocity(float startingVelocity, float dT, float brakeRate, float currentVelocity, float *updatedVelocity)
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

void PIDControlDown::UpdateVelocityStateWithBrake(float pvDown, float path_time, float brakeRate)
{
    mVelocityState = pvDown;

    float velocitySetpointDesired;

    UpdateBrakeVelocity(mVelocitySetpointTarget, path_time, brakeRate, pvDown, &velocitySetpointDesired);

    // Calculate the rate of change
    // RateLimit(velocitySetpointDesired[iaxis], mVelocitySetpointCurrent[iaxis], 2.0f );

    mVelocitySetpointCurrent = velocitySetpointDesired;
}


// Update velocity state called per dT. Also update current
// desired velocity
void PIDControlDown::UpdateVelocityState(float pv)
{
    mVelocityState = pv;

    if (mCallback) {
        // The FSM controls the actual descent velocity and introduces step changes as required
        float velocitySetpointDesired = mCallback->BoundVelocityDown(mVelocitySetpointTarget);
        // RateLimit(velocitySetpointDesired, mVelocitySetpointCurrent, 2.0f );
        mVelocitySetpointCurrent = velocitySetpointDesired;
    } else {
        mVelocitySetpointCurrent = mVelocitySetpointTarget;
    }
}

float PIDControlDown::GetVelocityDesired(void)
{
    return mVelocitySetpointCurrent;
}

float PIDControlDown::GetDownCommand(void)
{
    float ulow  = mMinThrust;
    float uhigh = mMaxThrust;

    if (mCallback) {
        mCallback->BoundThrust(ulow, uhigh);
    }
    float downCommand = pid2_apply(&PID, mVelocitySetpointCurrent, mVelocityState, ulow, uhigh);
    mDownCommand = downCommand;
    return mDownCommand;
}
