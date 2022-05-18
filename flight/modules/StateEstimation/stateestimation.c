/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup State Estimation
 * @brief Acquires sensor data and computes state estimate
 * @{
 *
 * @file       stateestimation.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      Module to handle all comms to the AHRS on a periodic basis.
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

#include "inc/stateestimation.h"

#include <callbackinfo.h>

#include <gyrosensor.h>
#include <accelsensor.h>
#include <magsensor.h>
#include <barosensor.h>
#include <airspeedsensor.h>
#include <gpspositionsensor.h>
#include <gpsvelocitysensor.h>
#include <homelocation.h>
#include <auxmagsensor.h>
#include <auxmagsettings.h>

#include <gyrostate.h>
#include <accelstate.h>
#include <magstate.h>
#include <airspeedstate.h>
#include <attitudestate.h>
#include <positionstate.h>
#include <velocitystate.h>

#include "revosettings.h"
#include "flightstatus.h"

#include "CoordinateConversions.h"

// Private constants
#define STACK_SIZE_BYTES        256
#define CALLBACK_PRIORITY       CALLBACK_PRIORITY_REGULAR
#define TASK_PRIORITY           CALLBACK_TASK_FLIGHTCONTROL
#define TIMEOUT_MS              10

// Private filter init const
#define FILTER_INIT_FORCE       -1
#define FILTER_INIT_IF_POSSIBLE -2

// local macros, ONLY to be used in the middle of StateEstimationCb in section RUNSTATE_LOAD after the update of states updated!
#define FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(sensorname, shortname, a1, a2, a3) \
    if (IS_SET(states.updated, SENSORUPDATES_##shortname)) { \
        sensorname##Data s; \
        sensorname##Get(&s); \
        if (IS_REAL(s.a1) && IS_REAL(s.a2) && IS_REAL(s.a3)) { \
            states.shortname[0] = s.a1; \
            states.shortname[1] = s.a2; \
            states.shortname[2] = s.a3; \
        } \
        else { \
            UNSET_MASK(states.updated, SENSORUPDATES_##shortname); \
        } \
    }

#define FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_1_DIMENSION_WITH_CUSTOM_EXTRA_CHECK(sensorname, shortname, a1, EXTRACHECK) \
    if (IS_SET(states.updated, SENSORUPDATES_##shortname)) { \
        sensorname##Data s; \
        sensorname##Get(&s); \
        if (IS_REAL(s.a1) && EXTRACHECK) { \
            states.shortname[0] = s.a1; \
        } \
        else { \
            UNSET_MASK(states.updated, SENSORUPDATES_##shortname); \
        } \
    }

#define FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_2_DIMENSION_WITH_CUSTOM_EXTRA_CHECK(sensorname, shortname, a1, a2, EXTRACHECK) \
    if (IS_SET(states.updated, SENSORUPDATES_##shortname)) { \
        sensorname##Data s; \
        sensorname##Get(&s); \
        if (IS_REAL(s.a1) && IS_REAL(s.a2) && EXTRACHECK) { \
            states.shortname[0] = s.a1; \
            states.shortname[1] = s.a2; \
        } \
        else { \
            UNSET_MASK(states.updated, SENSORUPDATES_##shortname); \
        } \
    }

// local macros, ONLY to be used in the middle of StateEstimationCb in section RUNSTATE_SAVE before the check of alarms!
#define EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_3_DIMENSIONS(statename, shortname, a1, a2, a3) \
    if (IS_SET(states.updated, SENSORUPDATES_##shortname)) { \
        statename##Data s; \
        statename##Get(&s); \
        s.a1 = states.shortname[0]; \
        s.a2 = states.shortname[1]; \
        s.a3 = states.shortname[2]; \
        statename##Set(&s); \
    }

#define EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_2_DIMENSIONS(statename, shortname, a1, a2) \
    if (IS_SET(states.updated, SENSORUPDATES_##shortname)) { \
        statename##Data s; \
        statename##Get(&s); \
        s.a1 = states.shortname[0]; \
        s.a2 = states.shortname[1]; \
        statename##Set(&s); \
    }


// Private types
struct filterPipelineStruct;

typedef const struct filterPipelineStruct {
    const stateFilter *filter;
    const struct filterPipelineStruct *next;
} filterPipeline;

// Private variables
static DelayedCallbackInfo *stateEstimationCallback;

static volatile RevoSettingsData revoSettings;
static volatile sensorUpdates updatedSensors;
static volatile int32_t fusionAlgorithm  = -1;
static const filterPipeline *filterChain = NULL;

// different filters available to state estimation
static stateFilter magFilter;
static stateFilter baroFilter;
static stateFilter baroiFilter;
static stateFilter velocityFilter;
static stateFilter altitudeFilter;
static stateFilter airFilter;
static stateFilter stationaryFilter;
static stateFilter llaFilter;
static stateFilter cfFilter;
static stateFilter cfmFilter;
static stateFilter ekf13iFilter;
static stateFilter ekf13Filter;
static stateFilter ekf13iNavFilter;
static stateFilter ekf13NavFilter;

// this is a hack to provide a computational shortcut for faster gyro state progression
static float gyroRaw[3];
static float gyroDelta[3];

// preconfigured filter chains selectable via revoSettings.FusionAlgorithm

static const filterPipeline *acroQueue = &(filterPipeline) {
    .filter = &cfFilter,
    .next   = NULL,
};

static const filterPipeline *cfQueue = &(filterPipeline) {
    .filter = &airFilter,
    .next   = &(filterPipeline) {
        .filter = &baroiFilter,
        .next   = &(filterPipeline) {
            .filter = &altitudeFilter,
            .next   = &(filterPipeline) {
                .filter = &cfFilter,
                .next   = NULL,
            }
        }
    }
};
static const filterPipeline *cfmiQueue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &baroiFilter,
            .next   = &(filterPipeline) {
                .filter = &altitudeFilter,
                .next   = &(filterPipeline) {
                    .filter = &cfmFilter,
                    .next   = NULL,
                }
            }
        }
    }
};
static const filterPipeline *cfmQueue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &llaFilter,
            .next   = &(filterPipeline) {
                .filter = &baroFilter,
                .next   = &(filterPipeline) {
                    .filter = &altitudeFilter,
                    .next   = &(filterPipeline) {
                        .filter = &cfmFilter,
                        .next   = NULL,
                    }
                }
            }
        }
    }
};
static const filterPipeline *ekf13iQueue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &baroiFilter,
            .next   = &(filterPipeline) {
                .filter = &stationaryFilter,
                .next   = &(filterPipeline) {
                    .filter = &ekf13iFilter,
                    .next   = &(filterPipeline) {
                        .filter = &velocityFilter,
                        .next   = NULL,
                    }
                }
            }
        }
    }
};

static const filterPipeline *ekf13Queue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &llaFilter,
            .next   = &(filterPipeline) {
                .filter = &baroFilter,
                .next   = &(filterPipeline) {
                    .filter = &ekf13Filter,
                    .next   = &(filterPipeline) {
                        .filter = &velocityFilter,
                        .next   = NULL,
                    }
                }
            }
        }
    }
};

static const filterPipeline *ekf13NavCFAttQueue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &llaFilter,
            .next   = &(filterPipeline) {
                .filter = &baroFilter,
                .next   = &(filterPipeline) {
                    .filter = &ekf13NavFilter,
                    .next   = &(filterPipeline) {
                        .filter = &velocityFilter,
                        .next   = &(filterPipeline) {
                            .filter = &cfmFilter,
                            .next   = NULL,
                        }
                    }
                }
            }
        }
    }
};

static const filterPipeline *ekf13iNavCFAttQueue = &(filterPipeline) {
    .filter = &magFilter,
    .next   = &(filterPipeline) {
        .filter = &airFilter,
        .next   = &(filterPipeline) {
            .filter = &baroiFilter,
            .next   = &(filterPipeline) {
                .filter = &stationaryFilter,
                .next   = &(filterPipeline) {
                    .filter = &ekf13iNavFilter,
                    .next   = &(filterPipeline) {
                        .filter = &velocityFilter,
                        .next   = &(filterPipeline) {
                            .filter = &cfmFilter,
                            .next   = NULL,
                        }
                    }
                }
            }
        }
    }
};

// Private functions

static void settingsUpdatedCb(UAVObjEvent *objEv);
static void sensorUpdatedCb(UAVObjEvent *objEv);
static void criticalConfigUpdatedCb(UAVObjEvent *objEv);
static void StateEstimationCb(void);

static inline int32_t maxint32_t(int32_t a, int32_t b)
{
    if (a > b) {
        return a;
    }
    return b;
}

/**
 * Initialise the module.  Called before the start function
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t StateEstimationInitialize(void)
{
    GyroSensorInitialize();
    MagSensorInitialize();
    AuxMagSensorInitialize();
    BaroSensorInitialize();
    AirspeedSensorInitialize();
    GPSVelocitySensorInitialize();
    GPSPositionSensorInitialize();

    GyroStateInitialize();
    AccelStateInitialize();
    MagStateInitialize();
    AirspeedStateInitialize();
    PositionStateInitialize();
    VelocityStateInitialize();

    RevoSettingsConnectCallback(&settingsUpdatedCb);

    HomeLocationConnectCallback(&criticalConfigUpdatedCb);
    AuxMagSettingsConnectCallback(&criticalConfigUpdatedCb);

    GyroSensorConnectCallback(&sensorUpdatedCb);
    AccelSensorConnectCallback(&sensorUpdatedCb);
    MagSensorConnectCallback(&sensorUpdatedCb);
    BaroSensorConnectCallback(&sensorUpdatedCb);
    AirspeedSensorConnectCallback(&sensorUpdatedCb);
    AuxMagSensorConnectCallback(&sensorUpdatedCb);
    GPSVelocitySensorConnectCallback(&sensorUpdatedCb);
    GPSPositionSensorConnectCallback(&sensorUpdatedCb);

    uint32_t stack_required = STACK_SIZE_BYTES;
    // Initialize Filters
    stack_required = maxint32_t(stack_required, filterMagInitialize(&magFilter));
    stack_required = maxint32_t(stack_required, filterBaroiInitialize(&baroiFilter));
    stack_required = maxint32_t(stack_required, filterBaroInitialize(&baroFilter));
    stack_required = maxint32_t(stack_required, filterVelocityInitialize(&velocityFilter));
    stack_required = maxint32_t(stack_required, filterAltitudeInitialize(&altitudeFilter));
    stack_required = maxint32_t(stack_required, filterAirInitialize(&airFilter));
    stack_required = maxint32_t(stack_required, filterStationaryInitialize(&stationaryFilter));
    stack_required = maxint32_t(stack_required, filterLLAInitialize(&llaFilter));
    stack_required = maxint32_t(stack_required, filterCFInitialize(&cfFilter));
    stack_required = maxint32_t(stack_required, filterCFMInitialize(&cfmFilter));
    stack_required = maxint32_t(stack_required, filterEKF13iInitialize(&ekf13iFilter));
    stack_required = maxint32_t(stack_required, filterEKF13Initialize(&ekf13Filter));
    stack_required = maxint32_t(stack_required, filterEKF13NavOnlyInitialize(&ekf13NavFilter));
    stack_required = maxint32_t(stack_required, filterEKF13iNavOnlyInitialize(&ekf13iNavFilter));

    stateEstimationCallback = PIOS_CALLBACKSCHEDULER_Create(&StateEstimationCb, CALLBACK_PRIORITY, TASK_PRIORITY, CALLBACKINFO_RUNNING_STATEESTIMATION, stack_required);

    return 0;
}

/**
 * Start the task.  Expects all objects to be initialized by this point.
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t StateEstimationStart(void)
{
    RevoSettingsConnectCallback(&settingsUpdatedCb);

    // Force settings update to make sure rotation loaded
    settingsUpdatedCb(NULL);

    return 0;
}

MODULE_INITCALL(StateEstimationInitialize, StateEstimationStart);


/**
 * Module callback
 */
static void StateEstimationCb(void)
{
    static filterResult alarm        = FILTERRESULT_OK;
    static filterResult lastAlarm    = FILTERRESULT_UNINITIALISED;
    static bool lastNavStatus        = false;
    static uint16_t alarmcounter     = 0;
    static uint16_t navstatuscounter = 0;
    static const filterPipeline *current;
    static stateEstimation states;
    static uint32_t last_time;
    static uint16_t bootDelay = 64;

    // after system startup, first few sensor readings might be messed up, delay until everything has settled
    if (bootDelay) {
        bootDelay--;
        PIOS_CALLBACKSCHEDULER_Schedule(stateEstimationCallback, TIMEOUT_MS, CALLBACK_UPDATEMODE_SOONER);
        return;
    }

    alarm = FILTERRESULT_OK;

    // set alarm to warning if called through timeout
    if (updatedSensors == 0) {
        if (PIOS_DELAY_DiffuS(last_time) > 1000 * TIMEOUT_MS) {
            alarm = FILTERRESULT_WARNING;
        }
    } else {
        last_time = PIOS_DELAY_GetRaw();
    }
    FlightStatusArmedOptions fsarmed;
    FlightStatusArmedGet(&fsarmed);
    states.armed = fsarmed != FLIGHTSTATUS_ARMED_DISARMED;

    // check if a new filter chain should be initialized
    if (fusionAlgorithm != revoSettings.FusionAlgorithm) {
        if (fsarmed == FLIGHTSTATUS_ARMED_DISARMED || fusionAlgorithm == FILTER_INIT_FORCE) {
            const filterPipeline *newFilterChain;
            switch ((RevoSettingsFusionAlgorithmOptions)revoSettings.FusionAlgorithm) {
            case REVOSETTINGS_FUSIONALGORITHM_ACRONOSENSORS:
                newFilterChain = acroQueue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_BASICCOMPLEMENTARY:
                newFilterChain = cfQueue;
                // reinit Mag alarm
                AlarmsSet(SYSTEMALARMS_ALARM_MAGNETOMETER, SYSTEMALARMS_ALARM_UNINITIALISED);
                break;
            case REVOSETTINGS_FUSIONALGORITHM_COMPLEMENTARYMAG:
                newFilterChain = cfmiQueue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_COMPLEMENTARYMAGGPSOUTDOOR:
                newFilterChain = cfmQueue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_INS13INDOOR:
                newFilterChain = ekf13iQueue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_GPSNAVIGATIONINS13:
                newFilterChain = ekf13Queue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_GPSNAVIGATIONINS13CF:
                newFilterChain = ekf13NavCFAttQueue;
                break;
            case REVOSETTINGS_FUSIONALGORITHM_TESTINGINSINDOORCF:
                newFilterChain = ekf13iNavCFAttQueue;
                break;
            default:
                newFilterChain = NULL;
            }
            // initialize filters in chain
            current = newFilterChain;
            bool error = 0;
            states.debugNavYaw = 0;
            states.navOk = false;
            states.navUsed     = false;
            while (current != NULL) {
                int32_t result = current->filter->init((stateFilter *)current->filter);
                if (result != 0) {
                    error = 1;
                    break;
                }
                current = current->next;
            }
            if (error) {
                AlarmsSet(SYSTEMALARMS_ALARM_ATTITUDE, SYSTEMALARMS_ALARM_ERROR);
                return;
            } else {
                // set new fusion algorithm
                filterChain     = newFilterChain;
                fusionAlgorithm = revoSettings.FusionAlgorithm;
            }
        }
    }

    // read updated sensor UAVObjects and set initial state
    states.updated = updatedSensors;
    updatedSensors = 0;

    // fetch sensors, check values, and load into state struct
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(GyroSensor, gyro, x, y, z);
    if (IS_SET(states.updated, SENSORUPDATES_gyro)) {
        gyroRaw[0] = states.gyro[0];
        gyroRaw[1] = states.gyro[1];
        gyroRaw[2] = states.gyro[2];
    }
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(AccelSensor, accel, x, y, z);
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(MagSensor, boardMag, x, y, z);
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(AuxMagSensor, auxMag, x, y, z);
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_3_DIMENSIONS(GPSVelocitySensor, vel, North, East, Down);
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_1_DIMENSION_WITH_CUSTOM_EXTRA_CHECK(BaroSensor, baro, Altitude, true);
    FETCH_SENSOR_FROM_UAVOBJECT_CHECK_AND_LOAD_TO_STATE_2_DIMENSION_WITH_CUSTOM_EXTRA_CHECK(AirspeedSensor, airspeed, CalibratedAirspeed, TrueAirspeed, s.SensorConnected == AIRSPEEDSENSOR_SENSORCONNECTED_TRUE);

    // GPS position data (LLA) is not fetched here since it does not contain floats. The filter must do all checks itself

    // at this point sensor state is stored in "states" with some rudimentary filtering applied

    // apply all filters in the current filter chain
    current = filterChain;

    // we are not done, re-dispatch self execution

    while (current) {
        filterResult result = current->filter->filter((stateFilter *)current->filter, &states);
        if (result > alarm) {
            alarm = result;
        }
        current = current->next;
    }

    // the final output of filters is saved in state variables
    // EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_3_DIMENSIONS(GyroState, gyro, x, y, z) // replaced by performance shortcut
    if (IS_SET(states.updated, SENSORUPDATES_gyro)) {
        gyroDelta[0] = states.gyro[0] - gyroRaw[0];
        gyroDelta[1] = states.gyro[1] - gyroRaw[1];
        gyroDelta[2] = states.gyro[2] - gyroRaw[2];
    }
    EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_3_DIMENSIONS(AccelState, accel, x, y, z);
    if (IS_SET(states.updated, SENSORUPDATES_mag)) {
        MagStateData s;

        MagStateGet(&s);
        s.x = states.mag[0];
        s.y = states.mag[1];
        s.z = states.mag[2];
        switch (states.magStatus) {
        case MAGSTATUS_OK:
            s.Source = MAGSTATE_SOURCE_ONBOARD;
            break;
        case MAGSTATUS_AUX:
            s.Source = MAGSTATE_SOURCE_AUX;
            break;
        default:
            s.Source = MAGSTATE_SOURCE_INVALID;
        }
        MagStateSet(&s);
    }

    EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_3_DIMENSIONS(PositionState, pos, North, East, Down);
    EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_3_DIMENSIONS(VelocityState, vel, North, East, Down);
    EXPORT_STATE_TO_UAVOBJECT_IF_UPDATED_2_DIMENSIONS(AirspeedState, airspeed, CalibratedAirspeed, TrueAirspeed);
    // attitude nees manual conversion from quaternion to euler
    if (IS_SET(states.updated, SENSORUPDATES_attitude)) { \
        AttitudeStateData s;
        AttitudeStateGet(&s);
        s.q1     = states.attitude[0];
        s.q2     = states.attitude[1];
        s.q3     = states.attitude[2];
        s.q4     = states.attitude[3];
        Quaternion2RPY(&s.q1, &s.Roll);
        s.NavYaw = states.debugNavYaw;
        AttitudeStateSet(&s);
    }
    // throttle alarms, raise alarm flags immediately
    // but require system to run for a while before decreasing
    // to prevent alarm flapping
    if (alarm >= lastAlarm) {
        lastAlarm    = alarm;
        alarmcounter = 0;
    } else {
        if (alarmcounter < 100) {
            alarmcounter++;
        } else {
            lastAlarm    = alarm;
            alarmcounter = 0;
        }
    }

    if (lastNavStatus < states.navOk) {
        lastNavStatus    = states.navOk;
        navstatuscounter = 0;
    } else {
        if (navstatuscounter < 100) {
            navstatuscounter++;
        } else {
            lastNavStatus    = states.navOk;
            navstatuscounter = 0;
        }
    }

    // clear alarms if everything is alright, then schedule callback execution after timeout
    if (lastAlarm == FILTERRESULT_WARNING) {
        AlarmsSet(SYSTEMALARMS_ALARM_ATTITUDE, SYSTEMALARMS_ALARM_WARNING);
    } else if (lastAlarm == FILTERRESULT_CRITICAL) {
        AlarmsSet(SYSTEMALARMS_ALARM_ATTITUDE, SYSTEMALARMS_ALARM_CRITICAL);
    } else if (lastAlarm >= FILTERRESULT_ERROR) {
        AlarmsSet(SYSTEMALARMS_ALARM_ATTITUDE, SYSTEMALARMS_ALARM_ERROR);
    } else {
        AlarmsClear(SYSTEMALARMS_ALARM_ATTITUDE);
    }

    if (!states.navUsed) {
        AlarmsSet(SYSTEMALARMS_ALARM_NAV, SYSTEMALARMS_ALARM_UNINITIALISED);
    } else {
        if (states.navOk) {
            AlarmsClear(SYSTEMALARMS_ALARM_NAV);
        } else {
            AlarmsSet(SYSTEMALARMS_ALARM_NAV, SYSTEMALARMS_ALARM_CRITICAL);
        }
    }

    if (updatedSensors) {
        PIOS_CALLBACKSCHEDULER_Dispatch(stateEstimationCallback);
    } else {
        PIOS_CALLBACKSCHEDULER_Schedule(stateEstimationCallback, TIMEOUT_MS, CALLBACK_UPDATEMODE_SOONER);
    }
}


/**
 * Callback for eventdispatcher when RevoSettings has been updated
 */
static void settingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    RevoSettingsGet((RevoSettingsData *)&revoSettings);
}

/**
 * Callback for eventdispatcher when HomeLocation or other critical configs (auxmagsettings, ...) has been updated
 */
static void criticalConfigUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    // Ask for a filter init (necessary for LLA filter)
    // Only possible if disarmed
    fusionAlgorithm = FILTER_INIT_IF_POSSIBLE;
}


/**
 * Callback for eventdispatcher when any sensor UAVObject has been updated
 * updates the list of "recently updated UAVObjects" and dispatches the state estimator callback
 */
static void sensorUpdatedCb(UAVObjEvent *ev)
{
    if (!ev) {
        return;
    }

    if (ev->obj == GyroSensorHandle()) {
        updatedSensors |= SENSORUPDATES_gyro;
        // shortcut - update GyroState right away
        GyroSensorData s;
        GyroStateData t;
        GyroSensorGet(&s);
        t.x = s.x + gyroDelta[0];
        t.y = s.y + gyroDelta[1];
        t.z = s.z + gyroDelta[2];
        t.SensorReadTimestamp = s.SensorReadTimestamp;
        GyroStateSet(&t);
    }

    if (ev->obj == AccelSensorHandle()) {
        updatedSensors |= SENSORUPDATES_accel;
    }

    if (ev->obj == MagSensorHandle()) {
        updatedSensors |= SENSORUPDATES_boardMag;
    }

    if (ev->obj == AuxMagSensorHandle()) {
        updatedSensors |= SENSORUPDATES_auxMag;
    }

    if (ev->obj == GPSPositionSensorHandle()) {
        updatedSensors |= SENSORUPDATES_lla;
    }

    if (ev->obj == GPSVelocitySensorHandle()) {
        updatedSensors |= SENSORUPDATES_vel;
    }

    if (ev->obj == BaroSensorHandle()) {
        updatedSensors |= SENSORUPDATES_baro;
    }

    if (ev->obj == AirspeedSensorHandle()) {
        updatedSensors |= SENSORUPDATES_airspeed;
    }

    PIOS_CALLBACKSCHEDULER_Dispatch(stateEstimationCallback);
}


/**
 * @}
 * @}
 */
