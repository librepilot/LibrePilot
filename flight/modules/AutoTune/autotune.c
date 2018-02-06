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
 * @file       AutoTune/autotune.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 *             Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Automatic PID tuning module.
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
#include <pios.h>
#include <flightstatus.h>
#include <manualcontrolcommand.h>
#include <manualcontrolsettings.h>
#include <flightmodesettings.h>
#include <gyrostate.h>
#include <actuatordesired.h>
#include <stabilizationdesired.h>
#include <stabilizationsettings.h>
#include <systemidentsettings.h>
#include <systemidentstate.h>
#include <pios_board_info.h>
#include <systemsettings.h>
#include <taskinfo.h>
#include <stabilization.h>
#include <hwsettings.h>
#include <stabilizationsettingsbank1.h>
#include <stabilizationsettingsbank2.h>
#include <stabilizationsettingsbank3.h>
#include <accessorydesired.h>

#if defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
#define powapprox fastpow
#define expapprox fastexp
#else
#define powapprox powf
#define expapprox expf
#endif /* defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */


// Private constants
#undef  STACK_SIZE_BYTES
// Pull Request version tested on Sparky2. 292 bytes of stack left when configured with 1340
// Beware that Nano needs 156 bytes more stack than Sparky2
#define STACK_SIZE_BYTES          1340
#define TASK_PRIORITY             (tskIDLE_PRIORITY + 1)

#define AF_NUMX                   13
#define AF_NUMP                   43

#if !defined(AT_QUEUE_NUMELEM)
#define AT_QUEUE_NUMELEM          18
#endif

#define TASK_STARTUP_DELAY_MS     250                                           /* delay task startup this much, waiting on accessory valid */
#define NOT_AT_MODE_DELAY_MS      50                                            /* delay this many ms if not in autotune mode */
#define NOT_AT_MODE_RATE          (1000.0f / NOT_AT_MODE_DELAY_MS)              /* this many loops per second if not in autotune mode */
#define SMOOTH_QUICK_FLUSH_DELAY  0.5f                                          /* wait this long after last change to flush to permanent storage */
#define SMOOTH_QUICK_FLUSH_TICKS  (SMOOTH_QUICK_FLUSH_DELAY * NOT_AT_MODE_RATE) /* this many ticks after last change to flush to permanent storage */

#define MAX_PTS_PER_CYCLE         4    /* max gyro updates to process per loop see YIELD_MS and consider gyro rate */
#define INIT_TIME_DELAY_MS        100  /* delay to allow stab bank, etc. to be populated after flight mode switch change detection */
#define SYSTEMIDENT_TIME_DELAY_MS 2000 /* delay before starting systemident (shaking) flight mode */
#define INIT_TIME_DELAY2_MS       2500 /* delay before starting to capture data */
#define YIELD_MS                  2    /* delay this long between processing sessions see MAX_PTS_PER_CYCLE and consider gyro rate */

// CheckSettings() returned error bits
#define TAU_NAN                   1
#define BETA_NAN                  2
#define ROLL_BETA_LOW             4
#define PITCH_BETA_LOW            8
#define YAW_BETA_LOW              16
#define TAU_TOO_LONG              32
#define TAU_TOO_SHORT             64
#define CPU_TOO_SLOW              128

#define FMS_TOGGLE_STEP_DISABLED  0.0f

// Private types
enum AUTOTUNE_STATE { AT_INIT, AT_INIT_DELAY, AT_INIT_DELAY2, AT_START, AT_RUN, AT_FINISHED, AT_WAITING };
struct at_queued_data {
    float    y[3];                       /* Gyro measurements */
    float    u[3];                       /* Actuator desired */
    float    throttle;                   /* Throttle desired */
    uint32_t gyroStateCallbackTimestamp; /* PIOS_DELAY_GetRaw() time of GyroState callback */
    uint32_t sensorReadTimestamp; /* PIOS_DELAY_GetRaw() time of sensor read */
};


// Private variables
static SystemIdentSettingsData systemIdentSettings;
// save memory because metadata is only briefly accessed, when normal data struct is not being used
// unnamed union issues a warning
static union {
    SystemIdentStateData systemIdentState;
    UAVObjMetadata systemIdentStateMetaData;
} u;
static StabilizationBankManualRateData manualRate;
static xTaskHandle taskHandle;
static xQueueHandle atQueue;
static float gX[AF_NUMX] = { 0 };
static float gP[AF_NUMP] = { 0 };
static float gyroReadTimeAverage;
static float gyroReadTimeAverageAlpha;
static float gyroReadTimeAverageAlphaAlpha;
static float alpha;
static float smoothQuickValue;
static float flightModeSwitchToggleStepValue;
static volatile uint32_t atPointsSpilled;
static uint32_t throttleAccumulator;
static uint8_t rollMax, pitchMax;
static int8_t accessoryToUse;
static bool moduleEnabled;


// Private functions
static void AtNewGyroData(UAVObjEvent *ev);
static bool AutoTuneFoundInFMS();
static void AutoTuneTask(void *parameters);
static void AfInit(float X[AF_NUMX], float P[AF_NUMP]);
static void AfPredict(float X[AF_NUMX], float P[AF_NUMP], const float u_in[3], const float gyro[3], const float dT_s, const float t_in);
static bool CheckFlightModeSwitchForPidRequest(uint8_t flightMode);
static uint8_t CheckSettings();
static uint8_t CheckSettingsRaw();
static void ComputeStabilizationAndSetPidsFromDampAndNoise(float damp, float noise);
static void FlightModeSettingsUpdatedCb(UAVObjEvent *ev);
static void InitSystemIdent(bool loadDefaults);
static void UpdateSmoothQuickSource(uint8_t smoothQuickSource, bool loadDefaults);
static void ProportionPidsSmoothToQuick();
static void UpdateSystemIdentState(const float *X, const float *noise, float dT_s, uint32_t predicts, uint32_t spills, float hover_throttle);
static void UpdateStabilizationDesired(bool doingIdent);


/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutoTuneInitialize(void)
{
    HwSettingsOptionalModulesData optionalModules;

    HwSettingsOptionalModulesGet(&optionalModules);

#if defined(MODULE_AUTOTUNE_BUILTIN)
    moduleEnabled = true;
    optionalModules.AutoTune = HWSETTINGS_OPTIONALMODULES_ENABLED;
    HwSettingsOptionalModulesSet(&optionalModules);
#else
    if (optionalModules.AutoTune == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        // even though the AutoTune module is automatically enabled
        // (below, when the flight mode switch is configured to use autotune)
        // there are use cases where the user may even want it enabled without being on the FMS
        // that allows PIDs to be adjusted in flight
        moduleEnabled = true;
    } else {
        // if the user did not manually enable the autotune module
        // do it for them if they have autotune on their flight mode switch
        moduleEnabled = AutoTuneFoundInFMS();
    }
#endif /* defined(MODULE_AUTOTUNE_BUILTIN) */

    if (moduleEnabled) {
        AccessoryDesiredInitialize();
        ActuatorDesiredInitialize();
        FlightStatusInitialize();
        GyroStateInitialize();
        ManualControlCommandInitialize();
        StabilizationBankInitialize();
        SystemIdentStateInitialize();

        atQueue = xQueueCreate(AT_QUEUE_NUMELEM, sizeof(struct at_queued_data));
        if (!atQueue) {
            moduleEnabled = false;
        }
    }
    if (!moduleEnabled) {
        // only need to watch for enabling AutoTune in FMS if AutoTune module is _not_ running
        FlightModeSettingsConnectCallback(FlightModeSettingsUpdatedCb);
    }

    return 0;
}


/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutoTuneStart(void)
{
    // Start main task if it is enabled
    if (moduleEnabled) {
        GyroStateConnectCallback(AtNewGyroData);
        xTaskCreate(AutoTuneTask, "AutoTune", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_AUTOTUNE, taskHandle);
    }
    return 0;
}


MODULE_INITCALL(AutoTuneInitialize, AutoTuneStart);


/**
 * Module thread, should not return.
 */
static void AutoTuneTask(__attribute__((unused)) void *parameters)
{
    float noise[3]            = { 0 };
    float dT_s                = 0.0f;
    uint32_t lastUpdateTime   = 0; // initialization is only for compiler warning
    uint32_t lastTime         = 0;
    uint32_t measureTime      = 0;
    uint32_t updateCounter    = 0;
    enum AUTOTUNE_STATE state = AT_INIT;
    uint8_t currentSmoothQuickSource = 0;
    bool saveSiNeeded         = false;
    bool savePidNeeded        = false;

    // wait for the accessory values to stabilize
    // otherwise they come up as zero, then change to their real value
    // and that causes the PIDs to be re-exported (if smoothquick is active), which the user may not want
    vTaskDelay(TASK_STARTUP_DELAY_MS / portTICK_RATE_MS);

    // get max attitude / max rate
    // for use in generating Attitude mode commands from this module
    // note that the values could change when they change flight mode (and the associated bank)
    StabilizationBankRollMaxGet(&rollMax);
    StabilizationBankPitchMaxGet(&pitchMax);
    StabilizationBankManualRateGet(&manualRate);
    // correctly set accessoryToUse and flightModeSwitchTogglePosition
    // based on what is in SystemIdent
    // so that the user can use the PID smooth->quick slider in flights following the autotune flight
    InitSystemIdent(false);
    smoothQuickValue = systemIdentSettings.SmoothQuickValue;

    while (1) {
        uint32_t diffTime;
        bool doingIdent = false;
        bool canSleep   = true;
        FlightStatusData flightStatus;
        FlightStatusGet(&flightStatus);

        if (flightStatus.Armed == FLIGHTSTATUS_ARMED_DISARMED) {
            if (saveSiNeeded) {
                saveSiNeeded = false;
                // Save SystemIdentSettings to permanent settings
                UAVObjSave(SystemIdentSettingsHandle(), 0);
            }
            if (savePidNeeded) {
                savePidNeeded = false;
                // Save PIDs to permanent settings
                switch (systemIdentSettings.DestinationPidBank) {
                case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK1:
                    UAVObjSave(StabilizationSettingsBank1Handle(), 0);
                    break;
                case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK2:
                    UAVObjSave(StabilizationSettingsBank2Handle(), 0);
                    break;
                case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK3:
                    UAVObjSave(StabilizationSettingsBank3Handle(), 0);
                    break;
                }
            }
        }

        // if using flight mode switch "quick toggle 3x" to "try smooth -> quick PIDs" is enabled
        // and user toggled into and back out of AutoTune three times in the last two seconds
        // and the autotune data gathering is complete
        // and the autotune data gathered is good
        // note: CheckFlightModeSwitchForPidRequest(mode) only returns true if current mode is not autotune
        if (flightModeSwitchToggleStepValue > FMS_TOGGLE_STEP_DISABLED && CheckFlightModeSwitchForPidRequest(flightStatus.FlightMode)
            && systemIdentSettings.Complete && !CheckSettings()) {
            if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
                // if user toggled while armed set PID's to next in sequence
                // if you assume that smoothest is -1 and quickest is +1
                // this corresponds to 0,+.50,+1.00,-1.00,-.50 (for 5 position toggle)
                smoothQuickValue += flightModeSwitchToggleStepValue;
                if (smoothQuickValue > 1.001f) {
                    smoothQuickValue = -1.0f;
                }
                // Assume the value is 0
                if (fabsf(smoothQuickValue) < 0.001f) {
                    smoothQuickValue = 0.0f;
                }
            } else {
                // if they did the 3x FMS toggle while disarmed, set PID's back to the middle of smoothquick
                smoothQuickValue = 0.0f;
            }
            // calculate PIDs based on new smoothQuickValue and save to the PID bank
            ProportionPidsSmoothToQuick();
            // save new PIDs permanently when / if disarmed
            savePidNeeded = true;
            // we also save the new knob/toggle value for startup next time
            // this keeps the PIDs in sync with the toggle position
            saveSiNeeded  = true;
        }

        // Check if the SmoothQuickSource changed,
        // allow config changes without reboot or reinit
        uint8_t smoothQuickSource;
        SystemIdentSettingsSmoothQuickSourceGet(&smoothQuickSource);
        if (smoothQuickSource != currentSmoothQuickSource) {
            UpdateSmoothQuickSource(smoothQuickSource, true);
            currentSmoothQuickSource = smoothQuickSource;
        }

        //////////////////////////////////////////////////////////////////////////////////////
        // if configured to use a slider for smooth-quick and the autotune module is running
        // (note that the module can be automatically or manually enabled)
        // then the smooth-quick slider is always active (when not actually in autotune mode)
        //
        // when the slider is active it will immediately change the PIDs
        // and it will schedule the PIDs to be written to permanent storage
        //
        // if the FC is disarmed, the perm write will happen on next loop
        // but if the FC is armed, the perm write will only occur when the FC goes disarmed
        //////////////////////////////////////////////////////////////////////////////////////

        // we don't want it saving to permanent storage many times
        // while the user is moving the knob once, so wait till the knob stops moving
        static uint8_t savePidDelay;
        // any time we are not in AutoTune mode:
        // - the user may be using the accessory0-3 knob/slider to request PID changes
        // - the state machine needs to be reset
        // - the local version of Attitude mode gets skipped
        if (flightStatus.FlightMode != FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE) {
            // if accessory0-3 is configured as a PID changing slider/knob over the smooth to quick range
            // and FC is not currently running autotune
            // and accessory0-3 changed by at least 1/85 of full range (2)
            // (don't bother checking to see if the requested accessory# is configured properly
            // if it isn't, the value will be 0 which is the center of [-1,1] anyway)
            if (accessoryToUse != -1 && systemIdentSettings.Complete && !CheckSettings()) {
                AccessoryDesiredData accessoryValue;
                AccessoryDesiredInstGet(accessoryToUse, &accessoryValue);
                // if the accessory changed more than 2 percent of total range (~20µs)
                // the smoothQuickValue will be changed
                if (fabsf(smoothQuickValue - accessoryValue.AccessoryVal) > 0.02f) {
                    smoothQuickValue = accessoryValue.AccessoryVal;
                    // calculate PIDs based on new smoothQuickValue and save to the PID bank
                    ProportionPidsSmoothToQuick();
                    // this schedules the first possible write of the PIDs to occur a fraction of a second or so from now
                    // and changes the scheduled time if it is already scheduled
                    savePidDelay = SMOOTH_QUICK_FLUSH_TICKS;
                } else if (savePidDelay && --savePidDelay == 0) {
                    // this flags that the PIDs can be written to permanent storage right now
                    // but they will only be written when the FC is disarmed
                    // so this means immediate (after NOT_AT_MODE_DELAY_MS) or wait till FC is disarmed
                    savePidNeeded = true;
                    // we also save the new knob/toggle value for startup next time
                    // this avoids rewriting the PIDs at each startup
                    // because knob is unknown / not where it is expected / looks like knob moved
                    saveSiNeeded = true;
                }
            } else {
                savePidDelay = 0;
            }
            state = AT_INIT;
            vTaskDelay(NOT_AT_MODE_DELAY_MS / portTICK_RATE_MS);
            continue;
        } else {
            savePidDelay = 0;
        }

        switch (state) {
        case AT_INIT:
            // beware that control comes here every time the user toggles the flight mode switch into AutoTune
            // and it isn't appropriate to reset the main state here
            // init must wait until after a delay has passed:
            // - to make sure they intended to stay in this mode
            // - to wait for the stab bank to get populated with the new bank info
            // This is a race.  It is possible that flightStatus.FlightMode has been changed,
            // but the stab bank hasn't been changed yet.
            state = AT_INIT_DELAY;
            lastUpdateTime = xTaskGetTickCount();
            break;

        case AT_INIT_DELAY:
            diffTime = xTaskGetTickCount() - lastUpdateTime;
            // after a small delay, get the stab bank values and SystemIdentSettings in case they changed
            // this is a very small delay (100ms), so "quick 3x fms toggle" gets in here
            if (diffTime > INIT_TIME_DELAY_MS) {
                // do these here so the user has at most a 1/10th second
                // with controls that use the previous bank's rates
                StabilizationBankRollMaxGet(&rollMax);
                StabilizationBankPitchMaxGet(&pitchMax);
                StabilizationBankManualRateGet(&manualRate);
                // load SystemIdentSettings so that they can change it
                // and do smooth-quick on changed values
                InitSystemIdent(false);
                // wait for FC to arm in case they are doing this without a flight mode switch
                // that causes the 2+ second delay that follows to happen after arming
                // which gives them a chance to take off before the shakes start
                // the FC must be armed and if we check here it also allows switchless setup to use autotune
                if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
                    state = AT_INIT_DELAY2;
                    lastUpdateTime = xTaskGetTickCount();
                }
            }
            break;

        case AT_INIT_DELAY2:
            // delay for 2 seconds before actually starting the SystemIdent flight mode and AutoTune.
            // that allows the user to get his fingers on the sticks
            // and avoids starting the AutoTune if the user is toggling the flight mode switch
            // to select other PIDs on the "simulated Smooth Quick slider".
            // or simply "passing through" this flight mode to get to another flight mode
            diffTime = xTaskGetTickCount() - lastUpdateTime;
            // after 2 seconds start systemident flight mode
            if (diffTime > SYSTEMIDENT_TIME_DELAY_MS) {
                // load default tune and clean up any NANs from previous tune
                InitSystemIdent(true);
                AfInit(gX, gP);
                // and write it out to the UAVO so innerloop can see the default values
                UpdateSystemIdentState(gX, NULL, 0.0f, 0, 0, 0.0f);
                // before starting SystemIdent stabilization mode
                doingIdent = true;
                state = AT_START;
            }
            break;

        case AT_START:
            diffTime   = xTaskGetTickCount() - lastUpdateTime;
            doingIdent = true;
            // after an additional short delay, start capturing data
            if (diffTime > INIT_TIME_DELAY2_MS) {
                // Reset save status
                // save SI data even if partial or bad, aids in diagnostics
                saveSiNeeded  = true;
                // don't save PIDs until data gathering is complete
                // and the complete data has been sanity checked
                savePidNeeded = false;
                // get the tuning duration in case the user just changed it
                measureTime   = (uint32_t)systemIdentSettings.TuningDuration * (uint32_t)1000;
                // init the "previous packet timestamp"
                lastTime = PIOS_DELAY_GetRaw();
                /* Drain the queue of all current data */
                xQueueReset(atQueue);
                /* And reset the point spill counter */
                updateCounter       = 0;
                atPointsSpilled     = 0;
                throttleAccumulator = 0;
                alpha = 0.0f;
                state = AT_RUN;
                lastUpdateTime      = xTaskGetTickCount();
            }
            break;

        case AT_RUN:
            diffTime   = xTaskGetTickCount() - lastUpdateTime;
            doingIdent = true;
            canSleep   = false;
            // 4 gyro samples per cycle
            // 2ms cycle time
            // that is 500 gyro samples per second if it sleeps each time
            // actually less than 500 because it cycle time is processing time + 2ms
            for (int i = 0; i < MAX_PTS_PER_CYCLE; i++) {
                struct at_queued_data pt;
                /* Grab an autotune point */
                if (xQueueReceive(atQueue, &pt, 0) != pdTRUE) {
                    /* We've drained the buffer fully */
                    canSleep = true;
                    break;
                }
                /* calculate time between successive points */
                dT_s = PIOS_DELAY_DiffuS2(lastTime, pt.gyroStateCallbackTimestamp) * 1.0e-6f;
                /* This is for the first point, but
                * also if we have extended drops */
                if (dT_s > 5.0f / PIOS_SENSOR_RATE) {
                    dT_s = 5.0f / PIOS_SENSOR_RATE;
                }
                lastTime = pt.gyroStateCallbackTimestamp;
                // original algorithm handles time from GyroStateGet() to detected motion
                // this algorithm also includes the time from raw gyro read to GyroStateGet()
                gyroReadTimeAverage = gyroReadTimeAverage * alpha
                                      + PIOS_DELAY_DiffuS2(pt.sensorReadTimestamp, pt.gyroStateCallbackTimestamp) * 1.0e-6f * (1.0f - alpha);
                alpha = alpha * gyroReadTimeAverageAlphaAlpha + gyroReadTimeAverageAlpha * (1.0f - gyroReadTimeAverageAlphaAlpha);
                AfPredict(gX, gP, pt.u, pt.y, dT_s, pt.throttle);
                for (int j = 0; j < 3; ++j) {
                    const float NOISE_ALPHA = 0.9997f; // 10 second time constant at 300 Hz
                    noise[j] = NOISE_ALPHA * noise[j] + (1 - NOISE_ALPHA) * (pt.y[j] - gX[j]) * (pt.y[j] - gX[j]);
                }
                // This will work up to 8kHz with an 89% throttle position before overflow
                throttleAccumulator += 10000 * pt.throttle;
                // Update uavo every 256 cycles to avoid
                // telemetry spam
                if (((++updateCounter) & 0xff) == 0) {
                    float hoverThrottle = ((float)(throttleAccumulator / updateCounter)) / 10000.0f;
                    UpdateSystemIdentState(gX, noise, dT_s, updateCounter, atPointsSpilled, hoverThrottle);
                }
            }
            if (diffTime > measureTime) { // Move on to next state
                // permanent flag that AT is complete and PIDs can be calculated
                state = AT_FINISHED;
            }
            break;

        case AT_FINISHED:
            // update with info from the last few data points
            if ((updateCounter & 0xff) != 0) {
                float hoverThrottle = ((float)(throttleAccumulator / updateCounter)) / 10000.0f;
                UpdateSystemIdentState(gX, noise, dT_s, updateCounter, atPointsSpilled, hoverThrottle);
            }
            // data is automatically considered bad if FC was disarmed at the time AT completed
            if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
                // always calculate and save PIDs if disabling sanity checks
                if (!CheckSettings()) {
                    ProportionPidsSmoothToQuick();
                    savePidNeeded = true;
                    // mark these results as good in the log settings so they can be viewed in playback
                    u.systemIdentState.Complete = true;
                    SystemIdentStateCompleteSet(&u.systemIdentState.Complete);
                    // mark these results as good in the permanent settings so they can be used next flight too
                    // this is written to the UAVO below, outside of the ARMED and CheckSettings() checks
                    systemIdentSettings.Complete = true;
                }
                // always raise an alarm if sanity checks failed
                // even if disabling sanity checks
                // that way user can still see that they failed
                uint8_t failureBits = CheckSettingsRaw();
                if (failureBits) {
                    // raise a warning that includes failureBits to indicate what failed
                    ExtendedAlarmsSet(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION, SYSTEMALARMS_ALARM_WARNING,
                                      SYSTEMALARMS_EXTENDEDALARMSTATUS_AUTOTUNE, failureBits);
                }
            }
            // need to save UAVO after .Complete gets potentially set
            // SystemIdentSettings needs the whole UAVO saved so it is saved outside the previous checks
            SystemIdentSettingsSet(&systemIdentSettings);
            state = AT_WAITING;
            break;

        case AT_WAITING:
        default:
            // after tuning, wait here till user switches to another flight mode
            // or disarms
            break;
        }

        // fly in Attitude mode or in SystemIdent mode
        UpdateStabilizationDesired(doingIdent);

        if (canSleep) {
            vTaskDelay(YIELD_MS / portTICK_RATE_MS);
        }
    }
}


// FlightModeSettings callback
// determine if autotune is enabled in the flight mode switch
static bool AutoTuneFoundInFMS()
{
    bool found = false;
    FlightModeSettingsFlightModePositionOptions fms[FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_NUMELEM];
    uint8_t num_flightMode;

    FlightModeSettingsFlightModePositionGet(fms);
    ManualControlSettingsFlightModeNumberGet(&num_flightMode);
    for (uint8_t i = 0; i < num_flightMode; ++i) {
        if (fms[i] == FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_AUTOTUNE) {
            found = true;
            break;
        }
    }
    return found;
}


// gyro sensor callback
// get gyro data and actuatordesired into a packet
// and put it in the queue for later processing
static void AtNewGyroData(UAVObjEvent *ev)
{
    static struct at_queued_data q_item;
    static bool last_sample_unpushed = false;
    GyroStateData gyro;
    ActuatorDesiredData actuators;
    uint32_t timestamp;

    if (!ev || !ev->obj || ev->instId != 0 || ev->event != EV_UPDATED) {
        return;
    }

    // object will at times change asynchronously so must copy data here, with locking
    // and do it as soon as possible
    timestamp = PIOS_DELAY_GetRaw();
    GyroStateGet(&gyro);
    ActuatorDesiredGet(&actuators);

    if (last_sample_unpushed) {
        /* Last time we were unable to queue up the gyro data.
         * Try again, last chance! */
        if (xQueueSend(atQueue, &q_item, 0) != pdTRUE) {
            atPointsSpilled++;
        }
    }

    q_item.gyroStateCallbackTimestamp = timestamp;
    q_item.y[0]     = q_item.y[0] * stabSettings.gyro_alpha + gyro.x * (1 - stabSettings.gyro_alpha);
    q_item.y[1]     = q_item.y[1] * stabSettings.gyro_alpha + gyro.y * (1 - stabSettings.gyro_alpha);
    q_item.y[2]     = q_item.y[2] * stabSettings.gyro_alpha + gyro.z * (1 - stabSettings.gyro_alpha);
    q_item.u[0]     = actuators.Roll;
    q_item.u[1]     = actuators.Pitch;
    q_item.u[2]     = actuators.Yaw;
    q_item.throttle = actuators.Thrust;
    q_item.sensorReadTimestamp = gyro.SensorReadTimestamp;

    if (xQueueSend(atQueue, &q_item, 0) != pdTRUE) {
        last_sample_unpushed = true;
    } else {
        last_sample_unpushed = false;
    }
}


// this callback is only enabled if the AutoTune module is not running
// if it sees that AutoTune was added to the FMS it issues BOOT and ? alarms
static void FlightModeSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    if (AutoTuneFoundInFMS()) {
        ExtendedAlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL, SYSTEMALARMS_EXTENDEDALARMSTATUS_REBOOTREQUIRED, 0);
    }
}


// check for the user quickly toggling the flight mode switch
// into and out of AutoTune, 3 times
// that is a signal that the user wants to try the next PID settings
// on the scale from smooth to quick
// when it exceeds the quickest setting, it starts back at the smoothest setting
static bool CheckFlightModeSwitchForPidRequest(uint8_t flightMode)
{
    static uint32_t lastUpdateTime;
    static uint8_t flightModePrev;
    static uint8_t counter;
    uint32_t updateTime;

    // only count transitions into and out of autotune
    if ((flightModePrev == FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE) ^ (flightMode == FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE)) {
        flightModePrev = flightMode;
        updateTime     = xTaskGetTickCount();
        // if it has been over 2 seconds, reset the counter
        if (updateTime - lastUpdateTime > 2000) {
            counter = 0;
        }
        // if the counter is reset, start a new time period
        if (counter == 0) {
            lastUpdateTime = updateTime;
        }
        // if flight mode has toggled into autotune 3 times but is currently not autotune
        if (++counter >= 5 && flightMode != FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE) {
            counter = 0;
            return true;
        }
    }

    return false;
}


// read SystemIdent uavos, update the local structures
// and set some flags based on the values
// it is used two ways:
// - on startup it reads settings so the user can reuse an old tune with smooth-quick
// - at tune time, it inits the state to default values of uavo xml file, in preparation for tuning
static void InitSystemIdent(bool loadDefaults)
{
    SystemIdentSettingsGet(&systemIdentSettings);
    if (loadDefaults) {
        // get these 10.0 10.0 7.0 -4.0 from default values of SystemIdent (.Beta and .Tau)
        // so that if they are changed there (mainly for future code changes), they will be changed here too
        // save metadata from being changed by the following SetDefaults()
        SystemIdentStateGetMetadata(&u.systemIdentStateMetaData);
        SystemIdentStateSetDefaults(SystemIdentStateHandle(), 0);
        SystemIdentStateSetMetadata(&u.systemIdentStateMetaData);
        SystemIdentStateGet(&u.systemIdentState);
        // Tau, GyroReadTimeAverage, Beta, and the Complete flag get default values
        // in preparation for running AutoTune
        systemIdentSettings.Tau = u.systemIdentState.Tau;
        systemIdentSettings.GyroReadTimeAverage = u.systemIdentState.GyroReadTimeAverage;
        memcpy(&systemIdentSettings.Beta, &u.systemIdentState.Beta, sizeof(SystemIdentSettingsBetaData));
        systemIdentSettings.Complete = u.systemIdentState.Complete;
    } else {
        // Tau, GyroReadTimeAverage, Beta, and the Complete flag get stored values
        // so the user can fly another battery to select and test PIDs with the slider/knob
        u.systemIdentState.Tau = systemIdentSettings.Tau;
        u.systemIdentState.GyroReadTimeAverage = systemIdentSettings.GyroReadTimeAverage;
        memcpy(&u.systemIdentState.Beta, &systemIdentSettings.Beta, sizeof(SystemIdentStateBetaData));
        u.systemIdentState.Complete = systemIdentSettings.Complete;
    }
    SystemIdentStateSet(&u.systemIdentState);

    // (1.0f / PIOS_SENSOR_RATE) is gyro period
    // the -1/10 makes it converge nicely, the other values make it converge the same way if the configuration is changed
    // gyroReadTimeAverageAlphaAlpha is 0.9996 when the tuning duration is the default of 60 seconds
    gyroReadTimeAverageAlphaAlpha = expapprox(-1.0f / PIOS_SENSOR_RATE / ((float)systemIdentSettings.TuningDuration / 10.0f));
    if (!IS_REAL(gyroReadTimeAverageAlphaAlpha)) {
        gyroReadTimeAverageAlphaAlpha = expapprox(-1.0f / 500.0f / (60 / 10)); // basically 0.9996
    }
    // 0.99999988f is as close to 1.0f as possible to make final average as smooth as possible
    gyroReadTimeAverageAlpha = 0.99999988f;
    gyroReadTimeAverage = u.systemIdentState.GyroReadTimeAverage;

    UpdateSmoothQuickSource(systemIdentSettings.SmoothQuickSource, loadDefaults);
}


// Update SmoothQuickSource to be used
static void UpdateSmoothQuickSource(uint8_t smoothQuickSource, bool loadDefaults)
{
    // disable PID changing with accessory0-3 and flight mode switch toggle
    accessoryToUse = -1;
    flightModeSwitchToggleStepValue = FMS_TOGGLE_STEP_DISABLED;

    switch (smoothQuickSource) {
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_ACCESSORY0:
        accessoryToUse = 0;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_ACCESSORY1:
        accessoryToUse = 1;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_ACCESSORY2:
        accessoryToUse = 2;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_ACCESSORY3:
        accessoryToUse = 3;
        break;
    // enable PID changing with flight mode switch
    // -1 to +1 give a range = 2, define step value for desired positions: 3, 5, 7
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_FMSTOGGLE3POS:
        flightModeSwitchToggleStepValue = 1.0f;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_FMSTOGGLE5POS:
        flightModeSwitchToggleStepValue = 0.5f;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_FMSTOGGLE7POS:
        flightModeSwitchToggleStepValue = 0.33f;
        break;
    case SYSTEMIDENTSETTINGS_SMOOTHQUICKSOURCE_DISABLED:
    default:
        accessoryToUse = -1;
        flightModeSwitchToggleStepValue = FMS_TOGGLE_STEP_DISABLED;
        break;
    }
    // don't allow init of current toggle position in the middle of 3x fms toggle
    if (loadDefaults && (flightModeSwitchToggleStepValue > FMS_TOGGLE_STEP_DISABLED)) {
        // set toggle to middle of range
        smoothQuickValue = 0.0f;
    }
}


// update the gain and delay with current calculated value
// these are stored in the settings for use with next battery
// and also in the state for logging purposes
static void UpdateSystemIdentState(const float *X, const float *noise,
                                   float dT_s, uint32_t predicts, uint32_t spills, float hover_throttle)
{
    u.systemIdentState.Beta.Roll  = X[6];
    u.systemIdentState.Beta.Pitch = X[7];
    u.systemIdentState.Beta.Yaw   = X[8];
    u.systemIdentState.Bias.Roll  = X[10];
    u.systemIdentState.Bias.Pitch = X[11];
    u.systemIdentState.Bias.Yaw   = X[12];
    u.systemIdentState.Tau = X[9];
    if (noise) {
        u.systemIdentState.Noise.Roll  = noise[0];
        u.systemIdentState.Noise.Pitch = noise[1];
        u.systemIdentState.Noise.Yaw   = noise[2];
    }
    u.systemIdentState.Period = dT_s * 1000.0f;
    u.systemIdentState.NumAfPredicts = predicts;
    u.systemIdentState.NumSpilledPts = spills;
    u.systemIdentState.HoverThrottle = hover_throttle;
    u.systemIdentState.GyroReadTimeAverage = gyroReadTimeAverage;

    // 'settings' tau, beta, and GyroReadTimeAverage have same value as 'state' versions
    // the state version produces a GCS log
    // the settings version is remembered after power off/on
    systemIdentSettings.Tau = u.systemIdentState.Tau;
    memcpy(&systemIdentSettings.Beta, &u.systemIdentState.Beta, sizeof(SystemIdentSettingsBetaData));
    systemIdentSettings.GyroReadTimeAverage = u.systemIdentState.GyroReadTimeAverage;
    systemIdentSettings.SmoothQuickValue    = smoothQuickValue;

    SystemIdentStateSet(&u.systemIdentState);
}


// when running AutoTune mode, this bypasses manualcontrol.c / stabilizedhandler.c
// to control whether the multicopter should be in Attitude mode vs. SystemIdent mode
static void UpdateStabilizationDesired(bool doingIdent)
{
    StabilizationDesiredData stabDesired;
    ManualControlCommandData manualControlCommand;

    ManualControlCommandGet(&manualControlCommand);

    stabDesired.Roll   = manualControlCommand.Roll * rollMax;
    stabDesired.Pitch  = manualControlCommand.Pitch * pitchMax;
    stabDesired.Yaw    = manualControlCommand.Yaw * manualRate.Yaw;
    stabDesired.Thrust = manualControlCommand.Thrust;

    if (doingIdent) {
        stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
        stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
        stabDesired.StabilizationMode.Yaw   = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
    } else {
        stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.StabilizationMode.Yaw   = STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK;
    }

    if (systemIdentSettings.ThrustControl == SYSTEMIDENTSETTINGS_THRUSTCONTROL_ALTITUDEVARIO) {
        stabDesired.StabilizationMode.Thrust = STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO;
    } else {
        stabDesired.StabilizationMode.Thrust = STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL;
    }

    StabilizationDesiredSet(&stabDesired);
}


// check the completed autotune state (mainly gain and delay)
// to see if it is reasonable
// return a bit mask of errors detected
static uint8_t CheckSettingsRaw()
{
    uint8_t retVal = 0;

    // inverting the comparisons then negating the bool result should catch the nans but it doesn't
    // so explictly check for nans
    if (!IS_REAL(expapprox(u.systemIdentState.Tau))) {
        retVal |= TAU_NAN;
    }
    if (!IS_REAL(expapprox(u.systemIdentState.Beta.Roll))) {
        retVal |= BETA_NAN;
    }
    if (!IS_REAL(expapprox(u.systemIdentState.Beta.Pitch))) {
        retVal |= BETA_NAN;
    }
    if (!IS_REAL(expapprox(u.systemIdentState.Beta.Yaw))) {
        retVal |= BETA_NAN;
    }

    // Check the axis gains
    // Extreme values: Your roll or pitch gain was lower than expected. This will result in large PID values.
    if (u.systemIdentState.Beta.Roll < 6) {
        retVal |= ROLL_BETA_LOW;
    }
    if (u.systemIdentState.Beta.Pitch < 6) {
        retVal |= PITCH_BETA_LOW;
    }
    // yaw gain is no longer checked, because the yaw options only include:
    // - not calculating yaw
    // - limiting yaw gain between two sane values (default)
    // - ignoring errors and accepting the calculated yaw

    // Check the response speed
    // Extreme values: Your estimated response speed (tau) is slower than normal. This will result in large PID values.
    if (expapprox(u.systemIdentState.Tau) > 0.1f) {
        retVal |= TAU_TOO_LONG;
    }
    // Extreme values: Your estimated response speed (tau) is faster than normal. This will result in large PID values.
    else if (expapprox(u.systemIdentState.Tau) < 0.008f) {
        retVal |= TAU_TOO_SHORT;
    }

    // Sanity check: CPU is too slow compared to gyro rate
    if (gyroReadTimeAverage > (1.0f / PIOS_SENSOR_RATE)) {
        retVal |= CPU_TOO_SLOW;
    }

    return retVal;
}


// check the completed autotune state (mainly gain and delay)
// to see if it is reasonable
// override bad yaw values if configured that way
// return a bit mask of errors detected
static uint8_t CheckSettings()
{
    uint8_t retVal = CheckSettingsRaw();

    if (systemIdentSettings.DisableSanityChecks) {
        retVal = 0;
    }
    return retVal;
}


// given Tau"+"GyroReadTimeAverage(delay) and Beta(gain) from the tune (and user selection of smooth to quick) calculate the PIDs
// this code came from dRonin GCS and has been converted from double precision math to single precision
static void ComputeStabilizationAndSetPidsFromDampAndNoise(float dampRate, float noiseRate)
{
    _Static_assert(sizeof(StabilizationSettingsBank1Data) == sizeof(StabilizationBankData), "sizeof(StabilizationSettingsBank1Data) != sizeof(StabilizationBankData)");
    StabilizationBankData volatile stabSettingsBank;
    switch (systemIdentSettings.DestinationPidBank) {
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK1:
        StabilizationSettingsBank1Get((void *)&stabSettingsBank);
        break;
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK2:
        StabilizationSettingsBank2Get((void *)&stabSettingsBank);
        break;
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK3:
        StabilizationSettingsBank3Get((void *)&stabSettingsBank);
        break;
    }

    // These three parameters define the desired response properties
    // - rate scale in the fraction of the natural speed of the system
    // to strive for.
    // - damp is the amount of damping in the system. higher values
    // make oscillations less likely
    // - ghf is the amount of high frequency gain and limits the influence
    // of noise
    const float ghf  = noiseRate / 1000.0f;
    const float damp = dampRate / 100.0f;

    float tau = expapprox(u.systemIdentState.Tau) + systemIdentSettings.GyroReadTimeAverage;
    float exp_beta_roll_times_ghf  = expapprox(u.systemIdentState.Beta.Roll) * ghf;
    float exp_beta_pitch_times_ghf = expapprox(u.systemIdentState.Beta.Pitch) * ghf;

    float wn    = 1.0f / tau;
    float tau_d = 0.0f;
    for (int i = 0; i < 30; i++) {
        float tau_d_roll  = (2.0f * damp * tau * wn - 1.0f) / (4.0f * tau * damp * damp * wn * wn - 2.0f * damp * wn - tau * wn * wn + exp_beta_roll_times_ghf);
        float tau_d_pitch = (2.0f * damp * tau * wn - 1.0f) / (4.0f * tau * damp * damp * wn * wn - 2.0f * damp * wn - tau * wn * wn + exp_beta_pitch_times_ghf);
        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn    = (tau + tau_d) / (tau * tau_d) / (2.0f * damp + 2.0f);
    }

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const float a = ((tau + tau_d) / tau / tau_d - 2.0f * damp * wn) / 20.0f;
    const float b = ((tau + tau_d) / tau / tau_d - 2.0f * damp * wn - a);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const float zeta_o = 1.3f;
    float kp_o = 1.0f / 4.0f / (zeta_o * zeta_o) / (1.0f / wn);

    // Except, if this is very high, we may be slew rate limited and pick
    // up oscillation that way.  Fix it with very soft clamping.
    // (dRonin) MaximumRate defaults to 350, 6.5 corresponds to where we begin
    // clamping rate ourselves.  ESCs, etc, it depends upon gains
    // and any pre-emphasis they do.   Still give ourselves partial credit
    // for inner loop bandwidth.

    // In dRonin, MaximumRate defaults to 350 and they begin clamping at outer Kp 6.5
    // To avoid oscillation, find the minimum rate, calculate the ratio of that to 350,
    // and scale (linearly) with that.  Skip yaw.  There is no outer yaw in the GUI.
    const uint16_t minRate = MIN(stabSettingsBank.MaximumRate.Roll, stabSettingsBank.MaximumRate.Pitch);
    const float kp_o_clamp = systemIdentSettings.OuterLoopKpSoftClamp * ((float)minRate / 350.0f);
    if (kp_o > kp_o_clamp) {
        kp_o = kp_o_clamp - sqrtf(kp_o_clamp) + sqrtf(kp_o);
    }
    kp_o *= 0.95f; // Pick up some margin.
    // Add a zero at 1/15th the innermost bandwidth.
    const float ki_o = 0.75f * kp_o / (2.0f * M_PI_F * tau * 15.0f);

    float kpMax = 0.0f;
    float betaMinLn  = 1000.0f;
    StabilizationBankRollRatePIDData volatile *rollPitchPid = NULL; // satisfy compiler warning only

    for (int i = 0; i < ((systemIdentSettings.CalculateYaw != SYSTEMIDENTSETTINGS_CALCULATEYAW_FALSE) ? 3 : 2); i++) {
        float betaLn = SystemIdentStateBetaToArray(u.systemIdentState.Beta)[i];
        float beta   = expapprox(betaLn);
        float ki;
        float kp;
        float kd;

        switch (i) {
        case 0: // Roll
        case 1: // Pitch
            ki = a * b * wn * wn * tau * tau_d / beta;
            kp = tau * tau_d * ((a + b) * wn * wn + 2.0f * a * b * damp * wn) / beta - ki * tau_d;
            kd = (tau * tau_d * (a * b + wn * wn + (a + b) * 2.0f * damp * wn) - 1.0f) / beta - kp * tau_d;
            if (betaMinLn > betaLn) {
                betaMinLn = betaLn;
                // RollRatePID PitchRatePID YawRatePID
                // form an array of structures
                // point to one
                // this pointer arithmetic no longer works as expected in a gcc 64 bit test program
                // rollPitchPid = &(&stabSettingsBank.RollRatePID)[i];
                if (i == 0) {
                    rollPitchPid = &stabSettingsBank.RollRatePID;
                } else {
                    rollPitchPid = (StabilizationBankRollRatePIDData *)&stabSettingsBank.PitchRatePID;
                }
            }
            break;
        case 2: // Yaw
            // yaw uses a mixture of yaw and the slowest axis (pitch) for it's beta and thus PID calculation
            // calculate the ratio to use when converting from the slowest axis (pitch) to the yaw axis
            // as (e^(betaMinLn-betaYawLn))^0.6
            // which is (e^betaMinLn / e^betaYawLn)^0.6
            // which is (betaMin / betaYaw)^0.6
            // which is betaMin^0.6 / betaYaw^0.6
            // now given that kp for each axis can be written as kpaxis = xp / betaaxis
            // for xp that is constant across all axes
            // then kpmin (probably kppitch) was xp / betamin (probably betapitch)
            // which we multiply by betaMin^0.6 / betaYaw^0.6 to get the new Yaw kp
            // so the new kpyaw is (xp / betaMin) * (betaMin^0.6 / betaYaw^0.6)
            // which is (xp / betaMin) * (betaMin^0.6 / betaYaw^0.6)
            // which is (xp * betaMin^0.6) / (betaMin * betaYaw^0.6)
            // which is xp / (betaMin * betaYaw^0.6 / betaMin^0.6)
            // which is xp / (betaMin^0.4 * betaYaw^0.6)
            // hence the new effective betaYaw for Yaw P is (betaMin^0.4)*(betaYaw^0.6)
            beta = expapprox(0.6f * (betaMinLn - u.systemIdentState.Beta.Yaw));
            // this casting assumes that RollRatePID is the same as PitchRatePID
            kp   = rollPitchPid->Kp * beta;
            ki   = 0.8f * rollPitchPid->Ki * beta;
            kd   = 0.8f * rollPitchPid->Kd * beta;
            break;
        }

        if (i < 2) {
            if (kpMax < kp) {
                kpMax = kp;
            }
        } else {
            // use the ratio with the largest roll/pitch kp to limit yaw kp to a reasonable value
            // use largest roll/pitch kp because it is the axis most slowed by rotational inertia
            // and yaw is also slowed maximally by rotational inertia
            // note that kp, ki, kd are all proportional in beta
            // so reducing them all proportionally is the same as changing beta
            float min = 0.0f;
            float max = 0.0f;
            switch (systemIdentSettings.CalculateYaw) {
            case SYSTEMIDENTSETTINGS_CALCULATEYAW_TRUELIMITTORATIO:
                max = kpMax * systemIdentSettings.YawToRollPitchPIDRatioMax;
                min = kpMax * systemIdentSettings.YawToRollPitchPIDRatioMin;
                break;
            case SYSTEMIDENTSETTINGS_CALCULATEYAW_TRUEIGNORELIMIT:
            default:
                max = 1000.0f;
                min = 0.0f;
                break;
            }

            float ratio = 1.0f;
            if (min > 0.0f && kp < min) {
                ratio = kp / min;
            } else if (max > 0.0f && kp > max) {
                ratio = kp / max;
            }
            kp /= ratio;
            ki /= ratio;
            kd /= ratio;
        }

        // reduce kd if so configured
        // both of the quads tested for d term oscillation exhibit some degree of it with the stock autotune PIDs
        // if may be that adjusting stabSettingsBank.DerivativeCutoff would have a similar affect
        // reducing kd requires that kp and ki be reduced to avoid ringing
        // the amount to reduce kp and ki is taken from ZN tuning
        // specifically kp is parameterized based on the ratio between kp(PID) and kp(PI) as the D factor varies from 1 to 0
        // https://en.wikipedia.org/wiki/PID_controller
        // Kp        Ki        Kd
        // -----------------------------------
        // P    0.50*Ku      -         -
        // PI   0.45*Ku  1.2*Kp/Tu     -
        // PID  0.60*Ku  2.0*Kp/Tu  Kp*Tu/8
        //
        // so  Kp is multiplied by (.45/.60) if Kd is reduced to 0
        // and Ki is multiplied by (1.2/2.0) if Kd is reduced to 0
        #define KP_REDUCTION (.45f / .60f)
        #define KI_REDUCTION (1.2f / 2.0f)

        // this link gives some additional ratios that are different
        // the reduced overshoot ratios are invalid for this purpose
        // https://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
        // Kp       Ki    Kd
        // ------------------------------------------------
        // P                     0.50*Ku     -     -
        // PI                    0.45*Ku  Tu/1.2   -
        // PD                    0.80*Ku     -    Tu/8
        // classic PID           0.60*Ku  Tu/2.0  Tu/8       #define KP_REDUCTION (.45f/.60f)  #define KI_REDUCTION (1.2f/2.0f)
        // Pessen Integral Rule  0.70*Ku  Tu/2.5  3.0*Tu/20  #define KP_REDUCTION (.45f/.70f)  #define KI_REDUCTION (1.2f/2.5f)
        // some overshoot        0.33*Ku  Tu/2.0  Tu/3       #define KP_REDUCTION (.45f/.33f)  #define KI_REDUCTION (1.2f/2.0f)
        // no overshoot          0.20*Ku  Tu/2.0  Tu/3       #define KP_REDUCTION (.45f/.20f)  #define KI_REDUCTION (1.2f/2.0f)

        // reduce roll and pitch, but not yaw
        // yaw PID is entirely based on roll or pitch PIDs which have already been reduced
        if (i < 2) {
            kp  = kp * KP_REDUCTION + kp * systemIdentSettings.DerivativeFactor * (1.0f - KP_REDUCTION);
            ki  = ki * KI_REDUCTION + ki * systemIdentSettings.DerivativeFactor * (1.0f - KI_REDUCTION);
            kd *= systemIdentSettings.DerivativeFactor;
        }

        switch (i) {
        case 0: // Roll
            stabSettingsBank.RollRatePID.Kp = kp;
            stabSettingsBank.RollRatePID.Ki = ki;
            stabSettingsBank.RollRatePID.Kd = kd;
            stabSettingsBank.RollPI.Kp = kp_o;
            stabSettingsBank.RollPI.Ki = ki_o;
            break;
        case 1: // Pitch
            stabSettingsBank.PitchRatePID.Kp = kp;
            stabSettingsBank.PitchRatePID.Ki = ki;
            stabSettingsBank.PitchRatePID.Kd = kd;
            stabSettingsBank.PitchPI.Kp    = kp_o;
            stabSettingsBank.PitchPI.Ki    = ki_o;
            break;
        case 2: // Yaw
            stabSettingsBank.YawRatePID.Kp = kp;
            stabSettingsBank.YawRatePID.Ki = ki;
            stabSettingsBank.YawRatePID.Kd = kd;
#if 0
            // if we ever choose to use these
            // (e.g. mag yaw attitude)
            // here they are
            stabSettingsBank.YawPI.Kp = kp_o;
            stabSettingsBank.YawPI.Ki = ki_o;
#endif
            break;
        }
    }

    // Librepilot might do something more with this some time
    // stabSettingsBank.DerivativeCutoff = 1.0f / (2.0f*M_PI_F*tau_d);
    // SystemIdentSettingsDerivativeCutoffSet(&systemIdentSettings.DerivativeCutoff);
    // then something to schedule saving this permanently to flash when disarmed

    // Save PIDs to UAVO RAM (not permanently yet)
    switch (systemIdentSettings.DestinationPidBank) {
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK1:
        StabilizationSettingsBank1Set((void *)&stabSettingsBank);
        break;
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK2:
        StabilizationSettingsBank2Set((void *)&stabSettingsBank);
        break;
    case SYSTEMIDENTSETTINGS_DESTINATIONPIDBANK_BANK3:
        StabilizationSettingsBank3Set((void *)&stabSettingsBank);
        break;
    }
}


// scale the damp and the noise to generate PIDs according to how a slider or other user specified ratio is set
//
// when val is half way between min and max, it generates the default PIDs
// when val is min, it generates the smoothest configured PIDs
// when val is max, it generates the quickest configured PIDs
//
// when val is between min and (min+max)/2, it scales val over the range [min, (min+max)/2] to generate PIDs between smoothest and default
// when val is between (min+max)/2 and max, it scales val over the range [(min+max)/2, max] to generate PIDs between default and quickest
//
// this is done piecewise because we are not guaranteed that default-min == max-default
// but we are given that [smoothDamp,smoothNoise] [defaultDamp,defaultNoise] [quickDamp,quickNoise] are all good parameterizations
// this code guarantees that we will get those exact parameterizations at (val =) min, (max+min)/2, and max
static void ProportionPidsSmoothToQuick()
{
    float ratio, damp, noise;
    float min = -1.0f;
    float val = smoothQuickValue;
    float max = 1.0f;

    // translate from range [min, max] to range [0, max-min]
    // that takes care of min < 0 case too
    val  -= min;
    max  -= min;
    ratio = val / max;

    if (ratio <= 0.5f) {
        // scale ratio in [0,0.5] to produce PIDs in [smoothest,default]
        ratio *= 2.0f;
        damp   = (systemIdentSettings.DampMax * (1.0f - ratio)) + (systemIdentSettings.DampRate * ratio);
        noise  = (systemIdentSettings.NoiseMin * (1.0f - ratio)) + (systemIdentSettings.NoiseRate * ratio);
    } else {
        // scale ratio in [0.5,1.0] to produce PIDs in [default,quickest]
        ratio = (ratio - 0.5f) * 2.0f;
        damp  = (systemIdentSettings.DampRate * (1.0f - ratio)) + (systemIdentSettings.DampMin * ratio);
        noise = (systemIdentSettings.NoiseRate * (1.0f - ratio)) + (systemIdentSettings.NoiseMax * ratio);
    }

    ComputeStabilizationAndSetPidsFromDampAndNoise(damp, noise);
    // save it to the system, but not yet written to flash
    SystemIdentSettingsSmoothQuickValueSet(&smoothQuickValue);
}


/**
 * Prediction step for EKF on control inputs to quad that
 * learns the system properties
 * @param X the current state estimate which is updated in place
 * @param P the current covariance matrix, updated in place
 * @param[in] the current control inputs (roll, pitch, yaw)
 * @param[in] the gyro measurements
 */
__attribute__((always_inline)) static inline void AfPredict(float X[AF_NUMX], float P[AF_NUMP], const float u_in[3], const float gyro[3], const float dT_s, const float t_in)
{
    const float Ts   = dT_s;
    const float Tsq  = Ts * Ts;
    const float Tsq3 = Tsq * Ts;
    const float Tsq4 = Tsq * Tsq;

    // for convenience and clarity code below uses the named versions of
    // the state variables
    float w1 = X[0]; // roll rate estimate
    float w2 = X[1]; // pitch rate estimate
    float w3 = X[2]; // yaw rate estimate
    float u1 = X[3]; // scaled roll torque
    float u2 = X[4]; // scaled pitch torque
    float u3 = X[5]; // scaled yaw torque
    const float e_b1   = expapprox(X[6]);   // roll torque scale
    const float b1     = X[6];
    const float e_b2   = expapprox(X[7]);   // pitch torque scale
    const float b2     = X[7];
    const float e_b3   = expapprox(X[8]);   // yaw torque scale
    const float b3     = X[8];
    const float e_tau  = expapprox(X[9]); // time response of the motors
    const float tau    = X[9];
    const float bias1  = X[10];       // bias in the roll torque
    const float bias2  = X[11];       // bias in the pitch torque
    const float bias3  = X[12];       // bias in the yaw torque

    // inputs to the system (roll, pitch, yaw)
    const float u1_in  = 4 * t_in * u_in[0];
    const float u2_in  = 4 * t_in * u_in[1];
    const float u3_in  = 4 * t_in * u_in[2];

    // measurements from gyro
    const float gyro_x = gyro[0];
    const float gyro_y = gyro[1];
    const float gyro_z = gyro[2];

    // update named variables because we want to use predicted
    // values below
    w1 = X[0] = w1 - Ts * bias1 * e_b1 + Ts * u1 * e_b1;
    w2 = X[1] = w2 - Ts * bias2 * e_b2 + Ts * u2 * e_b2;
    w3 = X[2] = w3 - Ts * bias3 * e_b3 + Ts * u3 * e_b3;
    u1 = X[3] = (Ts * u1_in) / (Ts + e_tau) + (u1 * e_tau) / (Ts + e_tau);
    u2 = X[4] = (Ts * u2_in) / (Ts + e_tau) + (u2 * e_tau) / (Ts + e_tau);
    u3 = X[5] = (Ts * u3_in) / (Ts + e_tau) + (u3 * e_tau) / (Ts + e_tau);
    // X[6] to X[12] unchanged

    /**** filter parameters ****/
    const float q_w        = 1e-3f;
    const float q_ud       = 1e-3f;
    const float q_B        = 1e-6f;
    const float q_tau      = 1e-6f;
    const float q_bias     = 1e-19f;
    const float s_a        = 150.0f; // expected gyro measurment noise

    const float Q[AF_NUMX] = { q_w, q_w, q_w, q_ud, q_ud, q_ud, q_B, q_B, q_B, q_tau, q_bias, q_bias, q_bias };

    float D[AF_NUMP];
    for (uint32_t i = 0; i < AF_NUMP; i++) {
        D[i] = P[i];
    }

    const float e_tau2    = e_tau * e_tau;
    const float e_tau3    = e_tau * e_tau2;
    const float e_tau4    = e_tau2 * e_tau2;
    const float Ts_e_tau2 = (Ts + e_tau) * (Ts + e_tau);
    const float Ts_e_tau4 = Ts_e_tau2 * Ts_e_tau2;

    // covariance propagation - D is stored copy of covariance
    P[0] = D[0] + Q[0] + 2 * Ts * e_b1 * (D[3] - D[28] - D[9] * bias1 + D[9] * u1)
           + Tsq * (e_b1 * e_b1) * (D[4] - 2 * D[29] + D[32] - 2 * D[10] * bias1 + 2 * D[30] * bias1 + 2 * D[10] * u1 - 2 * D[30] * u1
                                    + D[11] * (bias1 * bias1) + D[11] * (u1 * u1) - 2 * D[11] * bias1 * u1);
    P[1] = D[1] + Q[1] + 2 * Ts * e_b2 * (D[5] - D[33] - D[12] * bias2 + D[12] * u2)
           + Tsq * (e_b2 * e_b2) * (D[6] - 2 * D[34] + D[37] - 2 * D[13] * bias2 + 2 * D[35] * bias2 + 2 * D[13] * u2 - 2 * D[35] * u2
                                    + D[14] * (bias2 * bias2) + D[14] * (u2 * u2) - 2 * D[14] * bias2 * u2);
    P[2] = D[2] + Q[2] + 2 * Ts * e_b3 * (D[7] - D[38] - D[15] * bias3 + D[15] * u3)
           + Tsq * (e_b3 * e_b3) * (D[8] - 2 * D[39] + D[42] - 2 * D[16] * bias3 + 2 * D[40] * bias3 + 2 * D[16] * u3 - 2 * D[40] * u3
                                    + D[17] * (bias3 * bias3) + D[17] * (u3 * u3) - 2 * D[17] * bias3 * u3);
    P[3] = (D[3] * (e_tau2 + Ts * e_tau) + Ts * e_b1 * e_tau2 * (D[4] - D[29]) + Tsq * e_b1 * e_tau * (D[4] - D[29])
            + D[18] * Ts * e_tau * (u1 - u1_in) + D[10] * e_b1 * (u1 * (Ts * e_tau2 + Tsq * e_tau) - bias1 * (Ts * e_tau2 + Tsq * e_tau))
            + D[21] * Tsq * e_b1 * e_tau * (u1 - u1_in) + D[31] * Tsq * e_b1 * e_tau * (u1_in - u1)
            + D[24] * Tsq * e_b1 * e_tau * (u1 * (u1 - bias1) + u1_in * (bias1 - u1))) / Ts_e_tau2;
    P[4] = (Q[3] * Tsq4 + e_tau4 * (D[4] + Q[3]) + 2 * Ts * e_tau3 * (D[4] + 2 * Q[3]) + 4 * Q[3] * Tsq3 * e_tau
            + Tsq * e_tau2 * (D[4] + 6 * Q[3] + u1 * (D[27] * u1 + 2 * D[21]) + u1_in * (D[27] * u1_in - 2 * D[21]))
            + 2 * D[21] * Ts * e_tau3 * (u1 - u1_in) - 2 * D[27] * Tsq * u1 * u1_in * e_tau2) / Ts_e_tau4;
    P[5] = (D[5] * (e_tau2 + Ts * e_tau) + Ts * e_b2 * e_tau2 * (D[6] - D[34])
            + Tsq * e_b2 * e_tau * (D[6] - D[34]) + D[19] * Ts * e_tau * (u2 - u2_in)
            + D[13] * e_b2 * (u2 * (Ts * e_tau2 + Tsq * e_tau) - bias2 * (Ts * e_tau2 + Tsq * e_tau))
            + D[22] * Tsq * e_b2 * e_tau * (u2 - u2_in) + D[36] * Tsq * e_b2 * e_tau * (u2_in - u2)
            + D[25] * Tsq * e_b2 * e_tau * (u2 * (u2 - bias2) + u2_in * (bias2 - u2))) / Ts_e_tau2;
    P[6] = (Q[4] * Tsq4 + e_tau4 * (D[6] + Q[4]) + 2 * Ts * e_tau3 * (D[6] + 2 * Q[4]) + 4 * Q[4] * Tsq3 * e_tau
            + Tsq * e_tau2 * (D[6] + 6 * Q[4] + u2 * (D[27] * u2 + 2 * D[22]) + u2_in * (D[27] * u2_in - 2 * D[22]))
            + 2 * D[22] * Ts * e_tau3 * (u2 - u2_in) - 2 * D[27] * Tsq * u2 * u2_in * e_tau2) / Ts_e_tau4;
    P[7] = (D[7] * (e_tau2 + Ts * e_tau) + Ts * e_b3 * e_tau2 * (D[8] - D[39])
            + Tsq * e_b3 * e_tau * (D[8] - D[39]) + D[20] * Ts * e_tau * (u3 - u3_in)
            + D[16] * e_b3 * (u3 * (Ts * e_tau2 + Tsq * e_tau) - bias3 * (Ts * e_tau2 + Tsq * e_tau))
            + D[23] * Tsq * e_b3 * e_tau * (u3 - u3_in) + D[41] * Tsq * e_b3 * e_tau * (u3_in - u3)
            + D[26] * Tsq * e_b3 * e_tau * (u3 * (u3 - bias3) + u3_in * (bias3 - u3))) / Ts_e_tau2;
    P[8]  = (Q[5] * Tsq4 + e_tau4 * (D[8] + Q[5]) + 2 * Ts * e_tau3 * (D[8] + 2 * Q[5]) + 4 * Q[5] * Tsq3 * e_tau
             + Tsq * e_tau2 * (D[8] + 6 * Q[5] + u3 * (D[27] * u3 + 2 * D[23]) + u3_in * (D[27] * u3_in - 2 * D[23]))
             + 2 * D[23] * Ts * e_tau3 * (u3 - u3_in) - 2 * D[27] * Tsq * u3 * u3_in * e_tau2) / Ts_e_tau4;
    P[9]  = D[9] - Ts * e_b1 * (D[30] - D[10] + D[11] * (bias1 - u1));
    P[10] = (D[10] * (Ts + e_tau) + D[24] * Ts * (u1 - u1_in)) * (e_tau / Ts_e_tau2);
    P[11] = D[11] + Q[6];
    P[12] = D[12] - Ts * e_b2 * (D[35] - D[13] + D[14] * (bias2 - u2));
    P[13] = (D[13] * (Ts + e_tau) + D[25] * Ts * (u2 - u2_in)) * (e_tau / Ts_e_tau2);
    P[14] = D[14] + Q[7];
    P[15] = D[15] - Ts * e_b3 * (D[40] - D[16] + D[17] * (bias3 - u3));
    P[16] = (D[16] * (Ts + e_tau) + D[26] * Ts * (u3 - u3_in)) * (e_tau / Ts_e_tau2);
    P[17] = D[17] + Q[8];
    P[18] = D[18] - Ts * e_b1 * (D[31] - D[21] + D[24] * (bias1 - u1));
    P[19] = D[19] - Ts * e_b2 * (D[36] - D[22] + D[25] * (bias2 - u2));
    P[20] = D[20] - Ts * e_b3 * (D[41] - D[23] + D[26] * (bias3 - u3));
    P[21] = (D[21] * (Ts + e_tau) + D[27] * Ts * (u1 - u1_in)) * (e_tau / Ts_e_tau2);
    P[22] = (D[22] * (Ts + e_tau) + D[27] * Ts * (u2 - u2_in)) * (e_tau / Ts_e_tau2);
    P[23] = (D[23] * (Ts + e_tau) + D[27] * Ts * (u3 - u3_in)) * (e_tau / Ts_e_tau2);
    P[24] = D[24];
    P[25] = D[25];
    P[26] = D[26];
    P[27] = D[27] + Q[9];
    P[28] = D[28] - Ts * e_b1 * (D[32] - D[29] + D[30] * (bias1 - u1));
    P[29] = (D[29] * (Ts + e_tau) + D[31] * Ts * (u1 - u1_in)) * (e_tau / Ts_e_tau2);
    P[30] = D[30];
    P[31] = D[31];
    P[32] = D[32] + Q[10];
    P[33] = D[33] - Ts * e_b2 * (D[37] - D[34] + D[35] * (bias2 - u2));
    P[34] = (D[34] * (Ts + e_tau) + D[36] * Ts * (u2 - u2_in)) * (e_tau / Ts_e_tau2);
    P[35] = D[35];
    P[36] = D[36];
    P[37] = D[37] + Q[11];
    P[38] = D[38] - Ts * e_b3 * (D[42] - D[39] + D[40] * (bias3 - u3));
    P[39] = (D[39] * (Ts + e_tau) + D[41] * Ts * (u3 - u3_in)) * (e_tau / Ts_e_tau2);
    P[40] = D[40];
    P[41] = D[41];
    P[42] = D[42] + Q[12];

    /********* this is the update part of the equation ***********/
    float S[3] = { P[0] + s_a, P[1] + s_a, P[2] + s_a };
    X[0]  = w1 + P[0] * ((gyro_x - w1) / S[0]);
    X[1]  = w2 + P[1] * ((gyro_y - w2) / S[1]);
    X[2]  = w3 + P[2] * ((gyro_z - w3) / S[2]);
    X[3]  = u1 + P[3] * ((gyro_x - w1) / S[0]);
    X[4]  = u2 + P[5] * ((gyro_y - w2) / S[1]);
    X[5]  = u3 + P[7] * ((gyro_z - w3) / S[2]);
    X[6]  = b1 + P[9] * ((gyro_x - w1) / S[0]);
    X[7]  = b2 + P[12] * ((gyro_y - w2) / S[1]);
    X[8]  = b3 + P[15] * ((gyro_z - w3) / S[2]);
    X[9]  = tau + P[18] * ((gyro_x - w1) / S[0]) + P[19] * ((gyro_y - w2) / S[1]) + P[20] * ((gyro_z - w3) / S[2]);
    X[10] = bias1 + P[28] * ((gyro_x - w1) / S[0]);
    X[11] = bias2 + P[33] * ((gyro_y - w2) / S[1]);
    X[12] = bias3 + P[38] * ((gyro_z - w3) / S[2]);

    // update the duplicate cache
    for (uint32_t i = 0; i < AF_NUMP; i++) {
        D[i] = P[i];
    }

    // This is an approximation that removes some cross axis uncertainty but
    // substantially reduces the number of calculations
    P[0]  = -D[0] * (D[0] / S[0] - 1);
    P[1]  = -D[1] * (D[1] / S[1] - 1);
    P[2]  = -D[2] * (D[2] / S[2] - 1);
    P[3]  = -D[3] * (D[0] / S[0] - 1);
    P[4]  = D[4] - D[3] * (D[3] / S[0]);
    P[5]  = -D[5] * (D[1] / S[1] - 1);
    P[6]  = D[6] - D[5] * (D[5] / S[1]);
    P[7]  = -D[7] * (D[2] / S[2] - 1);
    P[8]  = D[8] - D[7] * (D[7] / S[2]);
    P[9]  = -D[9] * (D[0] / S[0] - 1);
    P[10] = D[10] - D[3] * (D[9] / S[0]);
    P[11] = D[11] - D[9] * (D[9] / S[0]);
    P[12] = -D[12] * (D[1] / S[1] - 1);
    P[13] = D[13] - D[5] * (D[12] / S[1]);
    P[14] = D[14] - D[12] * (D[12] / S[1]);
    P[15] = -D[15] * (D[2] / S[2] - 1);
    P[16] = D[16] - D[7] * (D[15] / S[2]);
    P[17] = D[17] - D[15] * (D[15] / S[2]);
    P[18] = -D[18] * (D[0] / S[0] - 1);
    P[19] = -D[19] * (D[1] / S[1] - 1);
    P[20] = -D[20] * (D[2] / S[2] - 1);
    P[21] = D[21] - D[3] * (D[18] / S[0]);
    P[22] = D[22] - D[5] * (D[19] / S[1]);
    P[23] = D[23] - D[7] * (D[20] / S[2]);
    P[24] = D[24] - D[9] * (D[18] / S[0]);
    P[25] = D[25] - D[12] * (D[19] / S[1]);
    P[26] = D[26] - D[15] * (D[20] / S[2]);
    P[27] = D[27] - D[18] * (D[18] / S[0]) - D[19] * (D[19] / S[1]) - D[20] * (D[20] / S[2]);
    P[28] = -D[28] * (D[0] / S[0] - 1);
    P[29] = D[29] - D[3] * (D[28] / S[0]);
    P[30] = D[30] - D[9] * (D[28] / S[0]);
    P[31] = D[31] - D[18] * (D[28] / S[0]);
    P[32] = D[32] - D[28] * (D[28] / S[0]);
    P[33] = -D[33] * (D[1] / S[1] - 1);
    P[34] = D[34] - D[5] * (D[33] / S[1]);
    P[35] = D[35] - D[12] * (D[33] / S[1]);
    P[36] = D[36] - D[19] * (D[33] / S[1]);
    P[37] = D[37] - D[33] * (D[33] / S[1]);
    P[38] = -D[38] * (D[2] / S[2] - 1);
    P[39] = D[39] - D[7] * (D[38] / S[2]);
    P[40] = D[40] - D[15] * (D[38] / S[2]);
    P[41] = D[41] - D[20] * (D[38] / S[2]);
    P[42] = D[42] - D[38] * (D[38] / S[2]);

    // apply limits to some of the state variables
    if (X[9] > -1.5f) {
        X[9] = -1.5f;
    } else if (X[9] < -5.5f) { /* 4ms */
        X[9] = -5.5f;
    }
    if (X[10] > 0.5f) {
        X[10] = 0.5f;
    } else if (X[10] < -0.5f) {
        X[10] = -0.5f;
    }
    if (X[11] > 0.5f) {
        X[11] = 0.5f;
    } else if (X[11] < -0.5f) {
        X[11] = -0.5f;
    }
    if (X[12] > 0.5f) {
        X[12] = 0.5f;
    } else if (X[12] < -0.5f) {
        X[12] = -0.5f;
    }
}


/**
 * Initialize the state variable and covariance matrix
 * for the system identification EKF
 */
static void AfInit(float X[AF_NUMX], float P[AF_NUMP])
{
    static const float qInit[AF_NUMX] = {
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        0.05f, 0.05f, 0.005f,
        0.05f,
        0.05f, 0.05f, 0.05f
    };

    // X[0] = X[1] = X[2] = 0.0f;    // assume no rotation
    // X[3] = X[4] = X[5] = 0.0f;    // and no net torque
    // X[6] = X[7]        = 10.0f;   // roll and pitch medium amount of strength
    // X[8]               = 7.0f;    // yaw strength
    // X[9] = -4.0f;                 // and 50 (18?) ms time scale
    // X[10] = X[11] = X[12] = 0.0f; // zero bias

    memset(X, 0, AF_NUMX * sizeof(X[0]));
    // get these 10.0 10.0 7.0 -4.0 from default values of SystemIdent (.Beta and .Tau)
    // so that if they are changed there (mainly for future code changes), they will be changed here too
    memcpy(&X[6], &u.systemIdentState.Beta, sizeof(u.systemIdentState.Beta));
    X[9] = u.systemIdentState.Tau;

    // P initialization
    memset(P, 0, AF_NUMP * sizeof(P[0]));
    P[0]  = qInit[0];
    P[1]  = qInit[1];
    P[2]  = qInit[2];
    P[4]  = qInit[3];
    P[6]  = qInit[4];
    P[8]  = qInit[5];
    P[11] = qInit[6];
    P[14] = qInit[7];
    P[17] = qInit[8];
    P[27] = qInit[9];
    P[32] = qInit[10];
    P[37] = qInit[11];
    P[42] = qInit[12];
}

/**
 * @}
 * @}
 */
