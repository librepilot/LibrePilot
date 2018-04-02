/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup State Estimation
 * @brief Acquires sensor data and computes state estimate
 * @{
 *
 * @file       filterekf.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      Extended Kalman Filter. Calculates complete system state except
 *             accelerometer drift.
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

#include <ekfconfiguration.h>
#include <ekfstatevariance.h>
#include <attitudestate.h>
#include <systemalarms.h>
#include <homelocation.h>

#include <insgps.h>
#include <CoordinateConversions.h>

// Private constants

#define STACK_REQUIRED 2048
#define DT_ALPHA       1e-3f
#define DT_MIN         1e-6f
#define DT_MAX         1.0f
#define DT_INIT        (1.0f / PIOS_SENSOR_RATE) // initialize with board sensor rate

#define IMPORT_SENSOR_IF_UPDATED(shortname, num) \
    if (IS_SET(state->updated, SENSORUPDATES_##shortname)) { \
        uint8_t t; \
        for (t = 0; t < num; t++) { \
            this->work.shortname[t] = state->shortname[t]; \
        } \
    }

// Private types
struct data {
    EKFConfigurationData ekfConfiguration;
    HomeLocationData     homeLocation;

    bool    usePos;

    int32_t init_stage;

    stateEstimation work;

    bool  inited;

    PiOSDeltatimeConfig dtconfig;
    bool  navOnly;
    float magLockAlpha;
};


// Private functions

static int32_t init(stateFilter *self);
static filterResult filter(stateFilter *self, stateEstimation *state);
static inline bool invalid_var(float data);

static int32_t globalInit(stateFilter *handle, bool usePos, bool navOnly);


static int32_t globalInit(stateFilter *handle, bool usePos, bool navOnly)
{
    handle->init      = &init;
    handle->filter    = &filter;
    handle->localdata = pios_malloc(sizeof(struct data));
    struct data *this = (struct data *)handle->localdata;
    this->usePos      = usePos;
    this->navOnly     = navOnly;
    EKFStateVarianceInitialize();
    return STACK_REQUIRED;
}

int32_t filterEKF13iInitialize(stateFilter *handle)
{
    return globalInit(handle, false, false);
}

int32_t filterEKF13Initialize(stateFilter *handle)
{
    return globalInit(handle, true, false);
}

int32_t filterEKF13iNavOnlyInitialize(stateFilter *handle)
{
    return globalInit(handle, false, true);
}

int32_t filterEKF13NavOnlyInitialize(stateFilter *handle)
{
    return globalInit(handle, true, true);
}

#ifdef FALSE
// XXX
// TODO: Until the 16 state EKF is implemented, run 13 state, so compilation runs through
// XXX
int32_t filterEKF16iInitialize(stateFilter *handle)
{
    return filterEKFi13Initialize(handle);
}
int32_t filterEKF16Initialize(stateFilter *handle)
{
    return filterEKF13Initialize(handle);
}
#endif

static int32_t init(stateFilter *self)
{
    struct data *this = (struct data *)self->localdata;

    this->inited       = false;
    this->init_stage   = 0;
    this->work.updated = 0;
    PIOS_DELTATIME_Init(&this->dtconfig, DT_INIT, DT_MIN, DT_MAX, DT_ALPHA);

    EKFConfigurationGet(&this->ekfConfiguration);
    int t;
    // plausibility check
    for (t = 0; t < EKFCONFIGURATION_P_NUMELEM; t++) {
        if (invalid_var(EKFConfigurationPToArray(this->ekfConfiguration.P)[t])) {
            return 2;
        }
    }
    for (t = 0; t < EKFCONFIGURATION_Q_NUMELEM; t++) {
        if (invalid_var(EKFConfigurationQToArray(this->ekfConfiguration.Q)[t])) {
            return 2;
        }
    }
    for (t = 0; t < EKFCONFIGURATION_R_NUMELEM; t++) {
        if (invalid_var(EKFConfigurationRToArray(this->ekfConfiguration.R)[t])) {
            return 2;
        }
    }
    HomeLocationGet(&this->homeLocation);
    // Don't require HomeLocation.Set to be true but at least require a mag configuration (allows easily
    // switching between indoor and outdoor mode with Set = false)
    if ((this->homeLocation.Be[0] * this->homeLocation.Be[0] + this->homeLocation.Be[1] * this->homeLocation.Be[1] + this->homeLocation.Be[2] * this->homeLocation.Be[2] < 1e-5f)) {
        return 2;
    }

    // calculate Angle between Down vector and homeLocation.Be
    float cross[3];
    float magnorm[3]    = { this->homeLocation.Be[0], this->homeLocation.Be[1], this->homeLocation.Be[2] };
    vector_normalizef(magnorm, 3);
    const float down[3] = { 0, 0, 1 };
    CrossProduct(down, magnorm, cross);
    // VectorMagnitude(cross) = sin(Alpha)
    // [0,0,1] dot magnorm = magnorm[2] = cos(Alpha)
    this->magLockAlpha = atan2f(VectorMagnitude(cross), magnorm[2]);

    return 0;
}

/**
 * Collect all required state variables, then run complementary filter
 */
static filterResult filter(stateFilter *self, stateEstimation *state)
{
    struct data *this    = (struct data *)self->localdata;

    const float zeros[3] = { 0.0f, 0.0f, 0.0f };

    // Perform the update
    float dT;
    uint16_t sensors = 0;

    INSSetArmed(state->armed);
    INSSetMagNorth(this->homeLocation.Be);
    state->navUsed      = (this->usePos || this->navOnly);
    this->work.updated |= state->updated;
    // check magnetometer alarm, discard any magnetometer readings if not OK
    // during initialization phase (but let them through afterwards)
    SystemAlarmsAlarmData alarms;
    SystemAlarmsAlarmGet(&alarms);
    if (alarms.Magnetometer != SYSTEMALARMS_ALARM_OK && !this->inited) {
        UNSET_MASK(state->updated, SENSORUPDATES_mag);
        UNSET_MASK(this->work.updated, SENSORUPDATES_mag);
    }

    // Get most recent data
    IMPORT_SENSOR_IF_UPDATED(gyro, 3);
    IMPORT_SENSOR_IF_UPDATED(accel, 3);
    IMPORT_SENSOR_IF_UPDATED(mag, 3);
    IMPORT_SENSOR_IF_UPDATED(baro, 1);
    IMPORT_SENSOR_IF_UPDATED(pos, 3);
    IMPORT_SENSOR_IF_UPDATED(vel, 3);
    IMPORT_SENSOR_IF_UPDATED(airspeed, 2);

    // check whether mandatory updates are present accels must have been supplied already,
    // and gyros must be supplied just now for a prediction step to take place
    // ("gyros last" rule for multi object synchronization)
    if (!(IS_SET(this->work.updated, SENSORUPDATES_accel) && IS_SET(state->updated, SENSORUPDATES_gyro))) {
        UNSET_MASK(state->updated, SENSORUPDATES_pos);
        UNSET_MASK(state->updated, SENSORUPDATES_vel);
        UNSET_MASK(state->updated, SENSORUPDATES_attitude);
        UNSET_MASK(state->updated, SENSORUPDATES_gyro);
        return FILTERRESULT_OK;
    }

    dT = PIOS_DELTATIME_GetAverageSeconds(&this->dtconfig);

    if (!this->inited && IS_SET(this->work.updated, SENSORUPDATES_mag) && IS_SET(this->work.updated, SENSORUPDATES_baro) && IS_SET(this->work.updated, SENSORUPDATES_pos)) {
        // Don't initialize until all sensors are read
        if (this->init_stage == 0) {
            // Reset the INS algorithm
            INSGPSInit();
            // variance is measured in mGaus, but internally the EKF works with a normalized  vector. Scale down by Be^2
            float Be2 = this->homeLocation.Be[0] * this->homeLocation.Be[0] + this->homeLocation.Be[1] * this->homeLocation.Be[1] + this->homeLocation.Be[2] * this->homeLocation.Be[2];
            INSSetMagVar((float[3]) { this->ekfConfiguration.R.MagX / Be2,
                                      this->ekfConfiguration.R.MagY / Be2,
                                      this->ekfConfiguration.R.MagZ / Be2 }
                         );
            INSSetAccelVar((float[3]) { this->ekfConfiguration.Q.AccelX,
                                        this->ekfConfiguration.Q.AccelY,
                                        this->ekfConfiguration.Q.AccelZ }
                           );
            INSSetGyroVar((float[3]) { this->ekfConfiguration.Q.GyroX,
                                       this->ekfConfiguration.Q.GyroY,
                                       this->ekfConfiguration.Q.GyroZ }
                          );
            INSSetGyroBiasVar((float[3]) { this->ekfConfiguration.Q.GyroDriftX,
                                           this->ekfConfiguration.Q.GyroDriftY,
                                           this->ekfConfiguration.Q.GyroDriftZ }
                              );
            INSSetBaroVar(this->ekfConfiguration.R.BaroZ);

            // Initialize the gyro bias
            float gyro_bias[3] = { 0.0f, 0.0f, 0.0f };
            INSSetGyroBias(gyro_bias);

            AttitudeStateData attitudeState;
            AttitudeStateGet(&attitudeState);

            // Set initial attitude. Use accels to determine roll and pitch, rotate magnetic measurement accordingly,
            // so pseudo "north" vector can be estimated even if the board is not level
            attitudeState.Roll = atan2f(-this->work.accel[1], -this->work.accel[2]);
            float zn  = cosf(attitudeState.Roll) * this->work.mag[2] + sinf(attitudeState.Roll) * this->work.mag[1];
            float yn  = cosf(attitudeState.Roll) * this->work.mag[1] - sinf(attitudeState.Roll) * this->work.mag[2];

            // rotate accels z vector according to roll
            float azn = cosf(attitudeState.Roll) * this->work.accel[2] + sinf(attitudeState.Roll) * this->work.accel[1];
            attitudeState.Pitch = atan2f(this->work.accel[0], -azn);

            float xn  = cosf(attitudeState.Pitch) * this->work.mag[0] + sinf(attitudeState.Pitch) * zn;

            attitudeState.Yaw = atan2f(-yn, xn);
            // TODO: This is still a hack
            // Put this in a proper generic function in CoordinateConversion.c
            // should take 4 vectors: g (0,0,-9.81), accels, Be (or 1,0,0 if no home loc) and magnetometers (or 1,0,0 if no mags)
            // should calculate the rotation in 3d space using proper cross product math
            // SUBTODO: formulate the math required

            attitudeState.Roll  = RAD2DEG(attitudeState.Roll);
            attitudeState.Pitch = RAD2DEG(attitudeState.Pitch);
            attitudeState.Yaw   = RAD2DEG(attitudeState.Yaw);

            RPY2Quaternion(&attitudeState.Roll, this->work.attitude);

            INSSetState(this->work.pos, (float *)zeros, this->work.attitude, (float *)zeros, (float *)zeros);

            INSResetP(EKFConfigurationPToArray(this->ekfConfiguration.P));
        } else {
            // Run prediction a bit before any corrections

            float gyros[3] = { DEG2RAD(this->work.gyro[0]), DEG2RAD(this->work.gyro[1]), DEG2RAD(this->work.gyro[2]) };
            INSStatePrediction(gyros, this->work.accel, dT);

            // Copy the attitude into the state
            // NOTE: updating gyr correctly is valid, because this code is reached only when SENSORUPDATES_gyro is already true
            if (!this->navOnly) {
                state->attitude[0] = Nav.q[0];
                state->attitude[1] = Nav.q[1];
                state->attitude[2] = Nav.q[2];
                state->attitude[3] = Nav.q[3];

                state->gyro[0]    -= RAD2DEG(Nav.gyro_bias[0]);
                state->gyro[1]    -= RAD2DEG(Nav.gyro_bias[1]);
                state->gyro[2]    -= RAD2DEG(Nav.gyro_bias[2]);
            }
            state->pos[0]   = Nav.Pos[0];
            state->pos[1]   = Nav.Pos[1];
            state->pos[2]   = Nav.Pos[2];
            state->vel[0]   = Nav.Vel[0];
            state->vel[1]   = Nav.Vel[1];
            state->vel[2]   = Nav.Vel[2];
            state->updated |= SENSORUPDATES_attitude | SENSORUPDATES_pos | SENSORUPDATES_vel;
        }

        this->init_stage++;
        if (this->init_stage > 10) {
            state->navOk = true;
            this->inited = true;
        }

        return FILTERRESULT_OK;
    }

    if (!this->inited) {
        return this->navOnly ? FILTERRESULT_OK : FILTERRESULT_CRITICAL;
    }

    float gyros[3] = { DEG2RAD(this->work.gyro[0]), DEG2RAD(this->work.gyro[1]), DEG2RAD(this->work.gyro[2]) };

    // Advance the state estimate
    INSStatePrediction(gyros, this->work.accel, dT);

    // Copy the attitude into the state
    // NOTE: updating gyr correctly is valid, because this code is reached only when SENSORUPDATES_gyro is already true
    if (!this->navOnly) {
        state->attitude[0] = Nav.q[0];
        state->attitude[1] = Nav.q[1];
        state->attitude[2] = Nav.q[2];
        state->attitude[3] = Nav.q[3];
        state->gyro[0]    -= RAD2DEG(Nav.gyro_bias[0]);
        state->gyro[1]    -= RAD2DEG(Nav.gyro_bias[1]);
        state->gyro[2]    -= RAD2DEG(Nav.gyro_bias[2]);
    }
    {
        float tmp[3];
        Quaternion2RPY(Nav.q, tmp);
        state->debugNavYaw = tmp[2];
    }
    state->pos[0]   = Nav.Pos[0];
    state->pos[1]   = Nav.Pos[1];
    state->pos[2]   = Nav.Pos[2];
    state->vel[0]   = Nav.Vel[0];
    state->vel[1]   = Nav.Vel[1];
    state->vel[2]   = Nav.Vel[2];
    state->updated |= SENSORUPDATES_attitude | SENSORUPDATES_pos | SENSORUPDATES_vel;

    // Advance the covariance estimate
    INSCovariancePrediction(dT);

    if (IS_SET(this->work.updated, SENSORUPDATES_mag)) {
        sensors |= MAG_SENSORS;
        if (this->ekfConfiguration.MapMagnetometerToHorizontalPlane == EKFCONFIGURATION_MAPMAGNETOMETERTOHORIZONTALPLANE_TRUE) {
            // Map Magnetometer vector to correspond to the Roll+Pitch of the current Attitude State Estimate (no conflicting gravity)
            // Idea: Alpha between Local Down and Mag is invariant of orientation, and identical to Alpha between [0,0,1] and HomeLocation.Be
            // which is measured in init()
            float R[3][3];

            // 1. rotate down vector into body frame
            Quaternion2R(Nav.q, R);
            float local_down[3];
            rot_mult(R, (float[3]) { 0, 0, 1 }, local_down);
            // 2. create a rotation vector that is perpendicular to rotated down vector, magnetic field vector and of size magLockAlpha
            float rotvec[3];
            CrossProduct(local_down, this->work.mag, rotvec);
            vector_normalizef(rotvec, 3);
            rotvec[0] *= -this->magLockAlpha;
            rotvec[1] *= -this->magLockAlpha;
            rotvec[2] *= -this->magLockAlpha;
            // 3. rotate artificial magnetometer reading from straight down to correct roll+pitch
            Rv2Rot(rotvec, R);
            float MagStrength = VectorMagnitude(this->homeLocation.Be);
            local_down[0] *= MagStrength;
            local_down[1] *= MagStrength;
            local_down[2] *= MagStrength;
            rot_mult(R, local_down, this->work.mag);
        }
        // From Eric: "exporting it in MagState was meant for debugging, but I think it makes a
        // lot of sense to have a "corrected" magnetometer reading available in the system."
        // TODO: Should move above calc to filtermag, updating from here cause trouble with the state->MagStatus (LP-534)
        // debug rotated mags
        // state->mag[0]   = this->work.mag[0];
        // state->mag[1]   = this->work.mag[1];
        // state->mag[2]   = this->work.mag[2];
        // state->updated |= SENSORUPDATES_mag;
    } // else {
      // mag state is delayed until EKF processed it, allows overriding/debugging magnetometer estimate
      // UNSET_MASK(state->updated, SENSORUPDATES_mag);
      // }

    if (IS_SET(this->work.updated, SENSORUPDATES_baro)) {
        sensors |= BARO_SENSOR;
    }

    if (!this->usePos) {
        // position and velocity variance used in indoor mode
        INSSetPosVelVar((float[3]) { this->ekfConfiguration.FakeR.FakeGPSPosIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSPosIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSPosIndoor },
                        (float[3]) { this->ekfConfiguration.FakeR.FakeGPSVelIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSVelIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSVelIndoor }
                        );
    } else {
        // position and velocity variance used in outdoor mode
        INSSetPosVelVar((float[3]) { this->ekfConfiguration.R.GPSPosNorth,
                                     this->ekfConfiguration.R.GPSPosEast,
                                     this->ekfConfiguration.R.GPSPosDown },
                        (float[3]) { this->ekfConfiguration.R.GPSVelNorth,
                                     this->ekfConfiguration.R.GPSVelEast,
                                     this->ekfConfiguration.R.GPSVelDown }
                        );
    }

    if (IS_SET(this->work.updated, SENSORUPDATES_pos)) {
        sensors |= POS_SENSORS;
    }

    if (IS_SET(this->work.updated, SENSORUPDATES_vel)) {
        sensors |= HORIZ_SENSORS | VERT_SENSORS;
    }

    if (IS_SET(this->work.updated, SENSORUPDATES_airspeed) && ((!IS_SET(this->work.updated, SENSORUPDATES_vel) && !IS_SET(this->work.updated, SENSORUPDATES_pos)) | !this->usePos)) {
        // HACK: feed airspeed into EKF as velocity, treat wind as 1e2 variance
        sensors |= HORIZ_SENSORS | VERT_SENSORS;
        INSSetPosVelVar((float[3]) { this->ekfConfiguration.FakeR.FakeGPSPosIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSPosIndoor,
                                     this->ekfConfiguration.FakeR.FakeGPSPosIndoor },
                        (float[3]) { this->ekfConfiguration.FakeR.FakeGPSVelAirspeed,
                                     this->ekfConfiguration.FakeR.FakeGPSVelAirspeed,
                                     this->ekfConfiguration.FakeR.FakeGPSVelAirspeed }
                        );
        // rotate airspeed vector into NED frame - airspeed is measured in X axis only
        float R[3][3];
        Quaternion2R(Nav.q, R);
        float vtas[3] = { this->work.airspeed[1], 0.0f, 0.0f };
        rot_mult(R, vtas, this->work.vel);
    }

    /*
     * TODO: Need to add a general sanity check for all the inputs to make sure their kosher
     * although probably should occur within INS itself
     */
    if (sensors) {
        INSCorrection(this->work.mag, this->work.pos, this->work.vel, this->work.baro[0], sensors);
    }

    EKFStateVarianceData vardata;
    EKFStateVarianceGet(&vardata);
    INSGetVariance(EKFStateVariancePToArray(vardata.P));
    EKFStateVarianceSet(&vardata);
    int t;
    for (t = 0; t < EKFSTATEVARIANCE_P_NUMELEM; t++) {
        if (!IS_REAL(EKFStateVariancePToArray(vardata.P)[t]) || EKFStateVariancePToArray(vardata.P)[t] <= 0.0f) {
            INSResetP(EKFConfigurationPToArray(this->ekfConfiguration.P));
            this->init_stage = -1;
            break;
        }
    }

    // all sensor data has been used, reset!
    this->work.updated = 0;

    if (this->init_stage < 0) {
        return this->navOnly ? FILTERRESULT_OK : FILTERRESULT_WARNING;
    } else {
        return FILTERRESULT_OK;
    }
}

// check for invalid variance values
static inline bool invalid_var(float data)
{
    if (isnan(data) || isinf(data)) {
        return true;
    }
    if (data < 1e-15f) { // var should not be close to zero. And not negative either.
        return true;
    }
    return false;
}

/**
 * @}
 * @}
 */
