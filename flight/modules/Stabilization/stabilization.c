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
 * @file       stabilization.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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
#include <manualcontrolcommand.h>
#include <flightmodesettings.h>
#include <stabilizationsettings.h>
#include <stabilizationdesired.h>
#include <stabilizationstatus.h>
#include <stabilizationbank.h>
#include <stabilizationsettingsbank1.h>
#include <stabilizationsettingsbank2.h>
#include <stabilizationsettingsbank3.h>
#include <ratedesired.h>
#include <sin_lookup.h>
#include <stabilization.h>
#include <innerloop.h>
#include <outerloop.h>
#include <altitudeloop.h>


// Public variables
StabilizationData stabSettings;

// Private variables
static int cur_flight_mode = -1;

// Private functions
static void SettingsUpdatedCb(UAVObjEvent *ev);
static void BankUpdatedCb(UAVObjEvent *ev);
static void SettingsBankUpdatedCb(UAVObjEvent *ev);
static void FlightModeSwitchUpdatedCb(UAVObjEvent *ev);
static void StabilizationDesiredUpdatedCb(UAVObjEvent *ev);

/**
 * Module initialization
 */
int32_t StabilizationStart()
{
    StabilizationSettingsConnectCallback(SettingsUpdatedCb);
    ManualControlCommandConnectCallback(FlightModeSwitchUpdatedCb);
    StabilizationBankConnectCallback(BankUpdatedCb);
    StabilizationSettingsBank1ConnectCallback(SettingsBankUpdatedCb);
    StabilizationSettingsBank2ConnectCallback(SettingsBankUpdatedCb);
    StabilizationSettingsBank3ConnectCallback(SettingsBankUpdatedCb);
    StabilizationDesiredConnectCallback(StabilizationDesiredUpdatedCb);
    SettingsUpdatedCb(StabilizationSettingsHandle());
    StabilizationDesiredUpdatedCb(StabilizationDesiredHandle());
    FlightModeSwitchUpdatedCb(ManualControlCommandHandle());
    BankUpdatedCb(StabilizationBankHandle());

#ifdef PIOS_INCLUDE_WDG
    PIOS_WDG_RegisterFlag(PIOS_WDG_STABILIZATION);
#endif
    return 0;
}

/**
 * Module initialization
 */
int32_t StabilizationInitialize()
{
    // Initialize variables
    StabilizationDesiredInitialize();
    StabilizationStatusInitialize();
    StabilizationBankInitialize();
    RateDesiredInitialize();
    ManualControlCommandInitialize(); // only used for PID bank selection based on flight mode switch
    sin_lookup_initalize();

    stabilizationOuterloopInit();
    stabilizationInnerloopInit();
#ifdef REVOLUTION
    stabilizationAltitudeloopInit();
#endif
    pid_zero(&stabSettings.outerPids[0]);
    pid_zero(&stabSettings.outerPids[1]);
    pid_zero(&stabSettings.outerPids[2]);
    pid_zero(&stabSettings.innerPids[0]);
    pid_zero(&stabSettings.innerPids[1]);
    pid_zero(&stabSettings.innerPids[2]);
    return 0;
}

MODULE_INITCALL(StabilizationInitialize, StabilizationStart);

static void StabilizationDesiredUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    StabilizationStatusData status;
    StabilizationDesiredStabilizationModeData mode;
    int t;

    StabilizationDesiredStabilizationModeGet(&mode);
    for (t = 0; t < AXES; t++) {
        switch (StabilizationDesiredStabilizationModeToArray(mode)[t]) {
        case STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_DIRECT;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_RATE:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_RATE;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_RATETRAINER:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECTWITHLIMITS;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_RATE;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT:
#if !defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
            // roll or pitch
            if (t <= 1) {
                StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_ATTITUDE;
            }
            // yaw
            else {
                StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            }
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_SYSTEMIDENT;
            break;
#else /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
            // no break, do not reorder this code
            // for low power FCs just fall through to Attitude mode
            // that means Yaw will be Attitude, but at least it is safe and creates no/minimal extra code
#endif /* !defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */
// do not reorder this code
        case STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_ATTITUDE;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_RATE;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_AXISLOCK;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_WEAKLEVELING;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_RATE;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_VIRTUALBAR:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_VIRTUALFLYBAR;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_ACRO:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_ACRO;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_RATTITUDE:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_RATTITUDE;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_RATE;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEHOLD:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_ALTITUDE;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_ALTITUDEVARIO;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL;
            break;
        case STABILIZATIONDESIRED_STABILIZATIONMODE_CRUISECONTROL:
            StabilizationStatusOuterLoopToArray(status.OuterLoop)[t] = STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
            StabilizationStatusInnerLoopToArray(status.InnerLoop)[t] = STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL;
            break;
        }
    }
    StabilizationStatusSet(&status);
}

static void FlightModeSwitchUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    uint8_t fm;

    ManualControlCommandFlightModeSwitchPositionGet(&fm);

    if (fm == cur_flight_mode) {
        return;
    }
    cur_flight_mode = fm;
    SettingsBankUpdatedCb(NULL);
}

static void SettingsBankUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    if (cur_flight_mode < 0 || cur_flight_mode >= FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_NUMELEM) {
        return;
    }
    if ((ev) && ((stabSettings.settings.FlightModeMap[cur_flight_mode] == 0 && ev->obj != StabilizationSettingsBank1Handle()) ||
                 (stabSettings.settings.FlightModeMap[cur_flight_mode] == 1 && ev->obj != StabilizationSettingsBank2Handle()) ||
                 (stabSettings.settings.FlightModeMap[cur_flight_mode] == 2 && ev->obj != StabilizationSettingsBank3Handle()) ||
                 stabSettings.settings.FlightModeMap[cur_flight_mode] > 2)) {
        return;
    }


    switch (stabSettings.settings.FlightModeMap[cur_flight_mode]) {
    case 0:
        StabilizationSettingsBank1Get((StabilizationSettingsBank1Data *)&stabSettings.stabBank);
        break;

    case 1:
        StabilizationSettingsBank2Get((StabilizationSettingsBank2Data *)&stabSettings.stabBank);
        break;

    case 2:
        StabilizationSettingsBank3Get((StabilizationSettingsBank3Data *)&stabSettings.stabBank);
        break;
    }
    StabilizationBankSet(&stabSettings.stabBank);
}

static bool use_tps_for_roll()
{
    uint8_t axes = stabSettings.stabBank.ThrustPIDScaleAxes;

    return axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLPITCHYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLPITCH ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLL;
}

static bool use_tps_for_pitch()
{
    uint8_t axes = stabSettings.stabBank.ThrustPIDScaleAxes;

    return axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLPITCHYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLPITCH ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_PITCHYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_PITCH;
}

static bool use_tps_for_yaw()
{
    uint8_t axes = stabSettings.stabBank.ThrustPIDScaleAxes;

    return axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLPITCHYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_ROLLYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_PITCHYAW ||
           axes == STABILIZATIONBANK_THRUSTPIDSCALEAXES_YAW;
}

static bool use_tps_for_p()
{
    uint8_t target = stabSettings.stabBank.ThrustPIDScaleTarget;

    return target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PID ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PI ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PD ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_P;
}

static bool use_tps_for_i()
{
    uint8_t target = stabSettings.stabBank.ThrustPIDScaleTarget;

    return target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PID ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PI ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_ID ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_I;
}

static bool use_tps_for_d()
{
    uint8_t target = stabSettings.stabBank.ThrustPIDScaleTarget;

    return target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PID ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_PD ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_ID ||
           target == STABILIZATIONBANK_THRUSTPIDSCALETARGET_D;
}

static void BankUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    StabilizationBankGet(&stabSettings.stabBank);

    // Set the roll rate PID constants
    pid_configure(&stabSettings.innerPids[0], stabSettings.stabBank.RollRatePID.Kp,
                  stabSettings.stabBank.RollRatePID.Ki,
                  stabSettings.stabBank.RollRatePID.Kd,
                  stabSettings.stabBank.RollRatePID.ILimit);

    // Set the pitch rate PID constants
    pid_configure(&stabSettings.innerPids[1], stabSettings.stabBank.PitchRatePID.Kp,
                  stabSettings.stabBank.PitchRatePID.Ki,
                  stabSettings.stabBank.PitchRatePID.Kd,
                  stabSettings.stabBank.PitchRatePID.ILimit);

    // Set the yaw rate PID constants
    pid_configure(&stabSettings.innerPids[2], stabSettings.stabBank.YawRatePID.Kp,
                  stabSettings.stabBank.YawRatePID.Ki,
                  stabSettings.stabBank.YawRatePID.Kd,
                  stabSettings.stabBank.YawRatePID.ILimit);

    // Set the roll attitude PI constants
    pid_configure(&stabSettings.outerPids[0], stabSettings.stabBank.RollPI.Kp,
                  stabSettings.stabBank.RollPI.Ki,
                  0,
                  stabSettings.stabBank.RollPI.ILimit);

    // Set the pitch attitude PI constants
    pid_configure(&stabSettings.outerPids[1], stabSettings.stabBank.PitchPI.Kp,
                  stabSettings.stabBank.PitchPI.Ki,
                  0,
                  stabSettings.stabBank.PitchPI.ILimit);

    // Set the yaw attitude PI constants
    pid_configure(&stabSettings.outerPids[2], stabSettings.stabBank.YawPI.Kp,
                  stabSettings.stabBank.YawPI.Ki,
                  0,
                  stabSettings.stabBank.YawPI.ILimit);

    bool tps_for_axis[3] = {
        use_tps_for_roll(),
        use_tps_for_pitch(),
        use_tps_for_yaw()
    };
    bool tps_for_pid[3] = {
        use_tps_for_p(),
        use_tps_for_i(),
        use_tps_for_d()
    };
    for (int axis = 0; axis < 3; axis++) {
        for (int pid = 0; pid < 3; pid++) {
            stabSettings.thrust_pid_scaling_enabled[axis][pid] = stabSettings.stabBank.EnableThrustPIDScaling
                                                                 && tps_for_axis[axis]
                                                                 && tps_for_pid[pid];
        }
    }

    for (int i = 0; i < STABILIZATIONSETTINGSBANK1_THRUSTPIDSCALECURVE_NUMELEM; i++) {
        stabSettings.floatThrustPIDScaleCurve[i] = (float)(stabSettings.stabBank.ThrustPIDScaleCurve[i]) * 0.01f;
    }

    stabSettings.acroInsanityFactors[0] = (float)(stabSettings.stabBank.AcroInsanityFactor.Roll) * 0.01f;
    stabSettings.acroInsanityFactors[1] = (float)(stabSettings.stabBank.AcroInsanityFactor.Pitch) * 0.01f;
    stabSettings.acroInsanityFactors[2] = (float)(stabSettings.stabBank.AcroInsanityFactor.Yaw) * 0.01f;

    // The dT has some jitter iteration to iteration that we don't want to
    // make thie result unpredictable.  Still, it's nicer to specify the constant
    // based on a time (in ms) rather than a fixed multiplier.  The error between
    // update rates on OP (~300 Hz) and CC (~475 Hz) is negligible for this
    // calculation
    const float fakeDt = 0.0025f;
    for (int t = 0; t < STABILIZATIONBANK_ATTITUDEFEEDFORWARD_NUMELEM; t++) {
        float tau = StabilizationBankAttitudeFeedForwardToArray(stabSettings.stabBank.AttitudeFeedForward)[t] * 0.1f;
        if (tau < 0.0001f) {
            stabSettings.feedForward_alpha[t] = 0.0f; // not trusting this to resolve to 0
        } else {
            stabSettings.feedForward_alpha[t] = expf(-fakeDt / tau);
        }
    }
}


static void SettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    // needs no mutex, as long as eventdispatcher and Stabilization are both TASK_PRIORITY_CRITICAL
    StabilizationSettingsGet(&stabSettings.settings);

    // Set up the derivative term
    pid_configure_derivative(stabSettings.settings.DerivativeCutoff, stabSettings.settings.DerivativeGamma);

    // The dT has some jitter iteration to iteration that we don't want to
    // make thie result unpredictable.  Still, it's nicer to specify the constant
    // based on a time (in ms) rather than a fixed multiplier.  The error between
    // update rates on OP (~300 Hz) and CC (~475 Hz) is negligible for this
    // calculation
    const float fakeDt = 0.0025f;
    if (stabSettings.settings.GyroTau < 0.0001f) {
        stabSettings.gyro_alpha = 0; // not trusting this to resolve to 0
    } else {
        stabSettings.gyro_alpha = expf(-fakeDt / stabSettings.settings.GyroTau);
    }

    // force flight mode update
    cur_flight_mode = -1;

    // Rattitude stick angle where the attitude to rate transition happens
    if (stabSettings.settings.RattitudeModeTransition < (uint8_t)10) {
        stabSettings.rattitude_mode_transition_stick_position = 10.0f / 100.0f;
    } else {
        stabSettings.rattitude_mode_transition_stick_position = (float)stabSettings.settings.RattitudeModeTransition / 100.0f;
    }

    stabSettings.cruiseControl.min_thrust = (float)stabSettings.settings.CruiseControlMinThrust / 100.0f;
    stabSettings.cruiseControl.max_thrust = (float)stabSettings.settings.CruiseControlMaxThrust / 100.0f;
    stabSettings.cruiseControl.thrust_difference = stabSettings.cruiseControl.max_thrust - stabSettings.cruiseControl.min_thrust;

    stabSettings.cruiseControl.power_trim = stabSettings.settings.CruiseControlPowerTrim / 100.0f;
    stabSettings.cruiseControl.half_power_delay = stabSettings.settings.CruiseControlPowerDelayComp / 2.0f;
    stabSettings.cruiseControl.max_power_factor_angle = RAD2DEG(acosf(1.0f / stabSettings.settings.CruiseControlMaxPowerFactor));
}

/**
 * @}
 * @}
 */
