/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup StabilizationModule Stabilization Module
 * @brief Stabilization PID loops in an airframe type independent manner
 * @note This object updates the @ref ActuatorDesired "Actuator Desired" based on the
 * PID loops on the @ref AttitudeDesired "Attitude Desired" and @ref AttitudeState "Attitude State"
 * @{
 *
 * @file       innerloop.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @brief      Attitude stabilization module.
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

#include <openpilot.h>
#include <pid.h>
#include <sin_lookup.h>
#include <callbackinfo.h>
#include <ratedesired.h>
#include <actuatordesired.h>
#include <gyrostate.h>
#include <airspeedstate.h>
#include <stabilizationstatus.h>
#include <flightstatus.h>
#include <manualcontrolcommand.h>
#include <stabilizationbank.h>
#include <stabilizationdesired.h>
#include <actuatordesired.h>

#include <stabilization.h>
#include <virtualflybar.h>
#include <cruisecontrol.h>
#include <sanitycheck.h>
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
#include <systemidentstate.h>
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

// Private constants

#define CALLBACK_PRIORITY   CALLBACK_PRIORITY_CRITICAL

#define UPDATE_EXPECTED     (1.0f / PIOS_SENSOR_RATE)
#define UPDATE_MIN          1.0e-6f
#define UPDATE_MAX          1.0f
#define UPDATE_ALPHA        1.0e-2f

#define SYSTEM_IDENT_PERIOD ((uint32_t)75)

#if defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
#define powapprox           fastpow
#define expapprox           fastexp
#else
#define powapprox           powf
#define expapprox           expf
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

// Private variables
static DelayedCallbackInfo *callbackHandle;
static float gyro_filtered[3] = { 0, 0, 0 };
static float axis_lock_accum[3] = { 0, 0, 0 };
static uint8_t previous_mode[AXES] = { 255, 255, 255, 255 };
static PiOSDeltatimeConfig timeval;
static float speedScaleFactor = 1.0f;
static bool frame_is_multirotor;
static bool measuredDterm_enabled;
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
static uint32_t systemIdentTimeVal = 0;
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

// Private functions
static void stabilizationInnerloopTask();
static void GyroStateUpdatedCb(__attribute__((unused)) UAVObjEvent *ev);
#ifdef REVOLUTION
static void AirSpeedUpdatedCb(__attribute__((unused)) UAVObjEvent *ev);
#endif

void stabilizationInnerloopInit()
{
    RateDesiredInitialize();
    ActuatorDesiredInitialize();
    GyroStateInitialize();
    StabilizationStatusInitialize();
    FlightStatusInitialize();
    ManualControlCommandInitialize();
    StabilizationDesiredInitialize();
    ActuatorDesiredInitialize();
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
    SystemIdentStateInitialize();
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
#ifdef REVOLUTION
    AirspeedStateInitialize();
    AirspeedStateConnectCallback(AirSpeedUpdatedCb);
#endif
    PIOS_DELTATIME_Init(&timeval, UPDATE_EXPECTED, UPDATE_MIN, UPDATE_MAX, UPDATE_ALPHA);

    callbackHandle = PIOS_CALLBACKSCHEDULER_Create(&stabilizationInnerloopTask, CALLBACK_PRIORITY, CBTASK_PRIORITY, CALLBACKINFO_RUNNING_STABILIZATION1, STACK_SIZE_BYTES);
    GyroStateConnectCallback(GyroStateUpdatedCb);

    // schedule dead calls every FAILSAFE_TIMEOUT_MS to have the watchdog cleared
    PIOS_CALLBACKSCHEDULER_Schedule(callbackHandle, FAILSAFE_TIMEOUT_MS, CALLBACK_UPDATEMODE_LATER);

    frame_is_multirotor   = (GetCurrentFrameType() == FRAME_TYPE_MULTIROTOR);
    measuredDterm_enabled = (stabSettings.settings.MeasureBasedDTerm == STABILIZATIONSETTINGS_MEASUREBASEDDTERM_TRUE);
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
    // Settings for system identification
    systemIdentTimeVal    = PIOS_DELAY_GetRaw();
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
}

static float get_pid_scale_source_value()
{
    float value;

    switch (stabSettings.stabBank.ThrustPIDScaleSource) {
    case STABILIZATIONBANK_THRUSTPIDSCALESOURCE_MANUALCONTROLTHROTTLE:
        ManualControlCommandThrottleGet(&value);
        break;
    case STABILIZATIONBANK_THRUSTPIDSCALESOURCE_STABILIZATIONDESIREDTHRUST:
        StabilizationDesiredThrustGet(&value);
        break;
    case STABILIZATIONBANK_THRUSTPIDSCALESOURCE_ACTUATORDESIREDTHRUST:
        ActuatorDesiredThrustGet(&value);
        break;
    default:
        ActuatorDesiredThrustGet(&value);
        break;
    }

    if (value < 0) {
        value = 0.0f;
    }

    return value;
}

typedef struct pid_curve_scaler {
    float  x;
    pointf points[5];
} pid_curve_scaler;

static float pid_curve_value(const pid_curve_scaler *scaler)
{
    float y = y_on_curve(scaler->x, scaler->points, sizeof(scaler->points) / sizeof(scaler->points[0]));

    return 1.0f + (IS_REAL(y) ? y : 0.0f);
}

static pid_scaler create_pid_scaler(int axis)
{
    pid_scaler scaler;

    // Always scaled with the this.
    scaler.p = scaler.i = scaler.d = speedScaleFactor;

    if (stabSettings.thrust_pid_scaling_enabled[axis][0]
        || stabSettings.thrust_pid_scaling_enabled[axis][1]
        || stabSettings.thrust_pid_scaling_enabled[axis][2]) {
        const pid_curve_scaler curve_scaler = {
            .x      = get_pid_scale_source_value(),
            .points = {
                { 0.00f, stabSettings.floatThrustPIDScaleCurve[0] },
                { 0.25f, stabSettings.floatThrustPIDScaleCurve[1] },
                { 0.50f, stabSettings.floatThrustPIDScaleCurve[2] },
                { 0.75f, stabSettings.floatThrustPIDScaleCurve[3] },
                { 1.00f, stabSettings.floatThrustPIDScaleCurve[4] }
            }
        };

        float curve_value = pid_curve_value(&curve_scaler);

        if (stabSettings.thrust_pid_scaling_enabled[axis][0]) {
            scaler.p *= curve_value;
        }
        if (stabSettings.thrust_pid_scaling_enabled[axis][1]) {
            scaler.i *= curve_value;
        }
        if (stabSettings.thrust_pid_scaling_enabled[axis][2]) {
            scaler.d *= curve_value;
        }
    }

    return scaler;
}

/**
 * WARNING! This callback executes with critical flight control priority every
 * time a gyroscope update happens do NOT put any time consuming calculations
 * in this loop unless they really have to execute with every gyro update
 */
static void stabilizationInnerloopTask()
{
    // watchdog and error handling
    {
#ifdef PIOS_INCLUDE_WDG
        PIOS_WDG_UpdateFlag(PIOS_WDG_STABILIZATION);
#endif
        bool warn  = false;
        bool error = false;
        bool crit  = false;
        // check if outer loop keeps executing
        if (stabSettings.monitor.rateupdates > -64) {
            stabSettings.monitor.rateupdates--;
        }
        if (stabSettings.monitor.rateupdates < -(2 * OUTERLOOP_SKIPCOUNT)) {
            // warning if rate loop skipped more than 2 execution
            warn = true;
        }
        if (stabSettings.monitor.rateupdates < -(4 * OUTERLOOP_SKIPCOUNT)) {
            // critical if rate loop skipped more than 4 executions
            crit = true;
        }
        // check if gyro keeps updating
        if (stabSettings.monitor.gyroupdates < 1) {
            // error if gyro didn't update at all!
            error = true;
        }
        if (stabSettings.monitor.gyroupdates > 1) {
            // warning if we missed a gyro update
            warn = true;
        }
        if (stabSettings.monitor.gyroupdates > 3) {
            // critical if we missed 3 gyro updates
            crit = true;
        }
        stabSettings.monitor.gyroupdates = 0;

        if (crit) {
            AlarmsSet(SYSTEMALARMS_ALARM_STABILIZATION, SYSTEMALARMS_ALARM_CRITICAL);
        } else if (error) {
            AlarmsSet(SYSTEMALARMS_ALARM_STABILIZATION, SYSTEMALARMS_ALARM_ERROR);
        } else if (warn) {
            AlarmsSet(SYSTEMALARMS_ALARM_STABILIZATION, SYSTEMALARMS_ALARM_WARNING);
        } else {
            AlarmsClear(SYSTEMALARMS_ALARM_STABILIZATION);
        }
    }

    RateDesiredData rateDesired;
    ActuatorDesiredData actuator;
    StabilizationStatusInnerLoopData enabled;
    FlightStatusControlChainData cchain;

    RateDesiredGet(&rateDesired);
    ActuatorDesiredGet(&actuator);
    StabilizationStatusInnerLoopGet(&enabled);
    FlightStatusControlChainGet(&cchain);
    float *rate = &rateDesired.Roll;
    float *actuatorDesiredAxis = &actuator.Roll;
    int t;
    float dT;
    bool multirotor = (GetCurrentFrameType() == FRAME_TYPE_MULTIROTOR); // check if frame is a multirotor
    dT = PIOS_DELTATIME_GetAverageSeconds(&timeval);

    StabilizationStatusOuterLoopData outerLoop;
    StabilizationStatusOuterLoopGet(&outerLoop);
    bool allowPiroComp = true;


    for (t = 0; t < AXES; t++) {
        bool reinit = (StabilizationStatusInnerLoopToArray(enabled)[t] != previous_mode[t]);
        previous_mode[t] = StabilizationStatusInnerLoopToArray(enabled)[t];

        if (t < STABILIZATIONSTATUS_INNERLOOP_THRUST) {
            if (reinit) {
                stabSettings.innerPids[t].iAccumulator = 0;
                if (frame_is_multirotor) {
                    // Multirotors should dump axis lock accumulators when unarmed or throttle is low.
                    // Fixed wing or ground vehicles can fly/drive with low throttle.
                    axis_lock_accum[t] = 0;
                }
            }
            // Any self leveling on roll or pitch must prevent pirouette compensation
            if (t < STABILIZATIONSTATUS_INNERLOOP_YAW && StabilizationStatusOuterLoopToArray(outerLoop)[t] != STABILIZATIONSTATUS_OUTERLOOP_DIRECT) {
                allowPiroComp = false;
            }
            switch (StabilizationStatusInnerLoopToArray(enabled)[t]) {
            case STABILIZATIONSTATUS_INNERLOOP_VIRTUALFLYBAR:
                stabilization_virtual_flybar(gyro_filtered[t], rate[t], &actuatorDesiredAxis[t], dT, reinit, t, &stabSettings.settings);
                break;
            case STABILIZATIONSTATUS_INNERLOOP_AXISLOCK:
                if (fabsf(rate[t]) > stabSettings.settings.MaxAxisLockRate) {
                    // While getting strong commands act like rate mode
                    axis_lock_accum[t] = 0;
                } else {
                    // For weaker commands or no command simply attitude lock (almost) on no gyro change
                    axis_lock_accum[t] += (rate[t] - gyro_filtered[t]) * dT;
                    axis_lock_accum[t]  = boundf(axis_lock_accum[t], -stabSettings.settings.MaxAxisLock, stabSettings.settings.MaxAxisLock);
                    rate[t] = axis_lock_accum[t] * stabSettings.settings.AxisLockKp;
                }
            // IMPORTANT: deliberately no "break;" here, execution continues with regular RATE control loop to avoid code duplication!
            // keep order as it is, RATE must follow!
            case STABILIZATIONSTATUS_INNERLOOP_RATE:
            {
                // limit rate to maximum configured limits (once here instead of 5 times in outer loop)
                rate[t] = boundf(rate[t],
                                 -StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t],
                                 StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t]
                                 );
                pid_scaler scaler = create_pid_scaler(t);
                actuatorDesiredAxis[t] = pid_apply_setpoint(&stabSettings.innerPids[t], &scaler, rate[t], gyro_filtered[t], dT, measuredDterm_enabled);
            }
            break;
            case STABILIZATIONSTATUS_INNERLOOP_ACRO:
            {
                float stickinput[3];
                stickinput[0] = boundf(rate[0] / stabSettings.stabBank.ManualRate.Roll, -1.0f, 1.0f);
                stickinput[1] = boundf(rate[1] / stabSettings.stabBank.ManualRate.Pitch, -1.0f, 1.0f);
                stickinput[2] = boundf(rate[2] / stabSettings.stabBank.ManualRate.Yaw, -1.0f, 1.0f);
                rate[t] = boundf(rate[t],
                                 -StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t],
                                 StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t]
                                 );

                pid_scaler ascaler = create_pid_scaler(t);
                ascaler.i *= boundf(1.0f - (1.5f * fabsf(stickinput[t])), 0.0f, 1.0f); // this prevents Integral from getting too high while controlled manually
                float arate  = pid_apply_setpoint(&stabSettings.innerPids[t], &ascaler, rate[t], gyro_filtered[t], dT, measuredDterm_enabled);
                float factor = fabsf(stickinput[t]) * stabSettings.acroInsanityFactors[t];
                actuatorDesiredAxis[t] = factor * stickinput[t] + (1.0f - factor) * arate;
            }
            break;

#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
            case STABILIZATIONSTATUS_INNERLOOP_SYSTEMIDENT:
            {
                static int8_t identIteration = 0;
                static float identOffsets[3] = { 0 };

                if (PIOS_DELAY_DiffuS(systemIdentTimeVal) / 1000.0f > SYSTEM_IDENT_PERIOD) {
                    const float SCALE_BIAS = 7.1f;
                    SystemIdentStateBetaData systemIdentBeta;

                    SystemIdentStateBetaGet(&systemIdentBeta);
                    systemIdentTimeVal = PIOS_DELAY_GetRaw();
                    identOffsets[0]    = 0.0f;
                    identOffsets[1]    = 0.0f;
                    identOffsets[2]    = 0.0f;
                    identIteration     = (identIteration + 1) & 7;
                    // why does yaw change twice a cycle and roll/pitch change only once?
                    uint8_t index = ((uint8_t[]) { '\2', '\0', '\2', '\0', '\2', '\1', '\2', '\1' }
                                     )[identIteration];
                    float scale   = expapprox(SCALE_BIAS - SystemIdentStateBetaToArray(systemIdentBeta)[index]);
                    // if roll or pitch limit to 25% of range
                    if (identIteration & 1) {
                        if (scale > 0.25f) {
                            scale = 0.25f;
                        }
                    }
                    // else it is yaw that can be a little more radical
                    else {
                        if (scale > 0.45f) {
                            scale = 0.45f;
                        }
                    }
                    if (identIteration & 2) {
                        scale = -scale;
                    }
                    identOffsets[index] = scale;
                    // this results in:
                    // when identIteration==0: identOffsets[2] = yaw_scale;
                    // when identIteration==1: identOffsets[0] = roll_scale;
                    // when identIteration==2: identOffsets[2] = -yaw_scale;
                    // when identIteration==3: identOffsets[0] = -roll_scale;
                    // when identIteration==4: identOffsets[2] = yaw_scale;
                    // when identIteration==5: identOffsets[1] = pitch_scale;
                    // when identIteration==6: identOffsets[2] = -yaw_scale;
                    // when identIteration==7: identOffsets[1] = -pitch_scale;
                    // each change has one axis with an offset
                    // and another axis coming back to zero from having an offset
                }

                rate[t] = boundf(rate[t],
                                 -StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t],
                                 StabilizationBankMaximumRateToArray(stabSettings.stabBank.MaximumRate)[t]
                                 );
                pid_scaler scaler = create_pid_scaler(t);
                actuatorDesiredAxis[t]  = pid_apply_setpoint(&stabSettings.innerPids[t], &scaler, rate[t], gyro_filtered[t], dT, measuredDterm_enabled);
                actuatorDesiredAxis[t] += identOffsets[t];
            }
            break;
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

            case STABILIZATIONSTATUS_INNERLOOP_DIRECT:
            default:
                actuatorDesiredAxis[t] = rate[t];
                break;
            }
        } else {
            switch (StabilizationStatusInnerLoopToArray(enabled)[t]) {
            case STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL:
                actuatorDesiredAxis[t] = cruisecontrol_apply_factor(rate[t]);
                break;
            case STABILIZATIONSTATUS_INNERLOOP_DIRECT:
            default:
                actuatorDesiredAxis[t] = rate[t];
                break;
            }
        }

        if (!multirotor) {
            // we only need to clamp the desired axis to a sane range if the frame is not a multirotor type
            // we don't want to do any clamping until after the motors are calculated and scaled.
            // need to figure out what to do with a tricopter tail servo.
            actuatorDesiredAxis[t] = boundf(actuatorDesiredAxis[t], -1.0f, 1.0f);
        }
    }

    actuator.UpdateTime = dT * 1000;

    if (cchain.Stabilization == FLIGHTSTATUS_CONTROLCHAIN_TRUE) {
        ActuatorDesiredSet(&actuator);
    } else {
        // Force all axes to reinitialize when engaged
        for (t = 0; t < AXES; t++) {
            previous_mode[t] = 255;
        }
    }

    if (allowPiroComp && stabSettings.stabBank.EnablePiroComp == STABILIZATIONBANK_ENABLEPIROCOMP_TRUE && stabSettings.innerPids[0].iLim > 1e-3f && stabSettings.innerPids[1].iLim > 1e-3f) {
        // attempted piro compensation - rotate pitch and yaw integrals (experimental)
        float angleYaw = DEG2RAD(gyro_filtered[2] * dT);
        float sinYaw   = sinf(angleYaw);
        float cosYaw   = cosf(angleYaw);
        float rollAcc  = stabSettings.innerPids[0].iAccumulator / stabSettings.innerPids[0].iLim;
        float pitchAcc = stabSettings.innerPids[1].iAccumulator / stabSettings.innerPids[1].iLim;
        stabSettings.innerPids[0].iAccumulator = stabSettings.innerPids[0].iLim * (cosYaw * rollAcc + sinYaw * pitchAcc);
        stabSettings.innerPids[1].iAccumulator = stabSettings.innerPids[1].iLim * (cosYaw * pitchAcc - sinYaw * rollAcc);
    }

    {
        FlightStatusArmedOptions armed;
        FlightStatusArmedGet(&armed);
        FlightStatusAlwaysStabilizeWhenArmedOptions alwaysStabilizeWhenArmed;
        FlightStatusAlwaysStabilizeWhenArmedGet(&alwaysStabilizeWhenArmed);

        float throttleDesired;
        ManualControlCommandThrottleGet(&throttleDesired);
        if (armed != FLIGHTSTATUS_ARMED_ARMED ||
            ((stabSettings.settings.LowThrottleZeroIntegral == STABILIZATIONSETTINGS_LOWTHROTTLEZEROINTEGRAL_TRUE) &&
             (throttleDesired < 0) &&
             (alwaysStabilizeWhenArmed != FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_TRUE))) {
            // Force all axes to reinitialize when engaged
            for (t = 0; t < AXES; t++) {
                previous_mode[t] = 255;
            }
        }
    }
    PIOS_CALLBACKSCHEDULER_Schedule(callbackHandle, FAILSAFE_TIMEOUT_MS, CALLBACK_UPDATEMODE_LATER);
}


static void GyroStateUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    GyroStateData gyroState;

    GyroStateGet(&gyroState);

    gyro_filtered[0] = gyro_filtered[0] * stabSettings.gyro_alpha + gyroState.x * (1 - stabSettings.gyro_alpha);
    gyro_filtered[1] = gyro_filtered[1] * stabSettings.gyro_alpha + gyroState.y * (1 - stabSettings.gyro_alpha);
    gyro_filtered[2] = gyro_filtered[2] * stabSettings.gyro_alpha + gyroState.z * (1 - stabSettings.gyro_alpha);

    PIOS_CALLBACKSCHEDULER_Dispatch(callbackHandle);
    stabSettings.monitor.gyroupdates++;
}

#ifdef REVOLUTION
static void AirSpeedUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    // Scale PID coefficients based on current airspeed estimation - needed for fixed wing planes
    AirspeedStateData airspeedState;

    AirspeedStateGet(&airspeedState);
    if (stabSettings.settings.ScaleToAirspeed < 0.1f || airspeedState.CalibratedAirspeed < 0.1f) {
        // feature has been turned off
        speedScaleFactor = 1.0f;
    } else {
        // scale the factor to be 1.0 at the specified airspeed (for example 10m/s) but scaled by 1/speed^2
        speedScaleFactor = boundf((stabSettings.settings.ScaleToAirspeed * stabSettings.settings.ScaleToAirspeed) / (airspeedState.CalibratedAirspeed * airspeedState.CalibratedAirspeed),
                                  stabSettings.settings.ScaleToAirspeedLimits.Min,
                                  stabSettings.settings.ScaleToAirspeedLimits.Max);
    }
}
#endif

/**
 * @}
 * @}
 */
