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
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
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

#include "openpilot.h"
#include "pios.h"
//#include "physical_constants.h"
#include "flightstatus.h"
//#include "modulesettings.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
//#include "gyros.h"
#include "gyrosensor.h"
#include "actuatordesired.h"
#include "stabilizationdesired.h"
#include "stabilizationsettings.h"
#include "systemident.h"
#include <pios_board_info.h>
//#include "pios_thread.h"
#include "systemsettings.h"
#include "taskinfo.h"

#include "stabilization.h"
#include "hwsettings.h"
#include "stabilizationsettingsbank1.h"
#include "stabilizationsettingsbank2.h"
#include "stabilizationsettingsbank3.h"

//#include "circqueue.h"
//#include "misc_math.h"

#define PIOS_malloc pios_malloc

#if defined(PIOS_EXCLUDE_ADVANCED_FEATURES)
#define powapprox fastpow
#define expapprox fastexp
#else
#define powapprox powf
#define expapprox expf
#endif /* defined(PIOS_EXCLUDE_ADVANCED_FEATURES) */

/**
 ******************************************************************************
 * @file       circqueue.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @brief Public header for 1 reader, 1 writer circular queue
 *****************************************************************************/

typedef struct circ_queue *circ_queue_t;

circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem);

void *circ_queue_cur_write_pos(circ_queue_t q);

int circ_queue_advance_write(circ_queue_t q);

void *circ_queue_read_pos(circ_queue_t q);

void circ_queue_read_completed(circ_queue_t q);

/**
 ******************************************************************************
 * @file       circqueue.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @brief Implements a 1 reader, 1 writer nonblocking circular queue
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

//#include <circqueue.h>

struct circ_queue {
    uint16_t elem_size;    /**< Element size in octets */
    uint16_t num_elem;    /**< Number of elements in circqueue (capacity+1) */
    volatile uint16_t write_head;    /**< Element position writer is at */
    volatile uint16_t read_tail;    /**< Element position reader is at */

    /* head == tail: empty.
     * head == tail-1: full.
     */

    /* This is declared as a uint32_t for alignment reasons. */
    uint32_t contents[];        /**< Contents of the circular queue */
};

/** Allocate a new circular queue.
 * @param[in] elem_size The size of each element, as obtained from sizeof().
 * @param[in] num_elem The number of elements in the queue.  The capacity is
 * one less than this (it may not be completely filled).
 * @returns The handle to the circular queue.
 */
circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem) {
    PIOS_Assert(elem_size > 0);
    PIOS_Assert(num_elem > 2);

    uint32_t size = elem_size * num_elem;

    /* PIOS_malloc_no_dma may not be safe for some later uses.. hmmm */
    struct circ_queue *ret = PIOS_malloc(sizeof(*ret) + size);

    memset(ret, 0, sizeof(*ret) + size);

    ret->elem_size = elem_size;
    ret->num_elem = num_elem;

    return ret;
}

/** Get a pointer to the current queue write position.
 * This position is unavailable to any present readers and may be filled in
 * with the desired data without respect to any synchronization.
 *
 * @param[in] q Handle to circular queue.
 * @returns The position for new data to be written to (of size elem_size).
 */
void *circ_queue_cur_write_pos(circ_queue_t q) {
    void *contents = q->contents;

    return contents + q->write_head * q->elem_size;
}

static inline uint16_t next_pos(uint16_t num_pos, uint16_t current_pos) {
    PIOS_Assert(current_pos < num_pos);

    current_pos++;
    /* Also save on uint16_t wrap */

    if (current_pos >= num_pos) {
        current_pos = 0;
    }

    return current_pos;
}

/** Makes the current block of data available to readers and advances write pos.
 * This may fail if the queue contain num_elems -1 elements, in which case the
 * advance may be retried in the future.  In this case, data already written to
 * write_pos is preserved and the advance may be retried (or overwritten with
 * new data).
 *
 * @param[in] q Handle to circular queue.
 * @returns 0 if the write succeeded, nonzero on error.
 */
int circ_queue_advance_write(circ_queue_t q) {
    uint16_t new_write_head = next_pos(q->num_elem, q->write_head);

    /* the head is not allowed to advance to meet the tail */
    if (new_write_head == q->read_tail) {
        return -1;    /* Full */

        /* Caller can either let the data go away, or try again to
         * advance later */
    }

    q->write_head = new_write_head;
    return 0;
}

/** Returns a block of data to the reader.
 * The block is "claimed" until released with circ_queue_read_completed.
 * No new data is available until that call is made (instead the same
 * block-in-progress will be returned).
 *
 * @param[in] q Handle to circular queue.
 * @returns pointer to the data, or NULL if the queue is empty.
 */
void *circ_queue_read_pos(circ_queue_t q) {
    uint16_t read_tail = q->read_tail;
    void *contents = q->contents;

    if (q->write_head == read_tail) {
        /* There is nothing new to read.  */
        return NULL;
    }

    return contents + q->read_tail * q->elem_size;
}

/** Releases a block of read data obtained by circ_queue_read_pos.
 * Behavior is undefined if circ_queue_read_pos did not previously return
 * a block of data.
 *
 * @param[in] q Handle to the circular queue.
 */
void circ_queue_read_completed(circ_queue_t q) {
    /* Avoid multiple accesses to a volatile */
    uint16_t read_tail = q->read_tail;

    /* If this is being called, the queue had better not be empty--
     * we're supposed to finish consuming this element after a prior call
     * to circ_queue_read_pos.
     */
    PIOS_Assert(read_tail != q->write_head);

    q->read_tail = next_pos(q->num_elem, read_tail);
}

// Private constants
#undef  STACK_SIZE_BYTES
// Nano locks up it seems in UAVObjSav() with 1340
// why did it lock up? 1540 now works (after a long initial delay) with 360 bytes left
#define STACK_SIZE_BYTES 1340
//#define TASK_PRIORITY PIOS_THREAD_PRIO_NORMAL
#define TASK_PRIORITY              (tskIDLE_PRIORITY + 1)

#define AF_NUMX 13
#define AF_NUMP 43

// Private types		<access gcs="readwrite" flight="readwrite"/>
enum AUTOTUNE_STATE { AT_INIT, AT_START, AT_RUN, AT_FINISHED, AT_WAITING };

struct at_queued_data {
    float y[3];        /* Gyro measurements */
    float u[3];        /* Actuator desired */
    float throttle;    /* Throttle desired */

    uint32_t raw_time;    /* From PIOS_DELAY_GetRaw() */
};

// Private variables
//static struct pios_thread *taskHandle;
static xTaskHandle taskHandle;
static bool module_enabled;
static circ_queue_t at_queue;
static volatile uint32_t at_points_spilled;
static uint32_t throttle_accumulator;
static uint8_t rollMax, pitchMax;
static StabilizationBankManualRateData manualRate;
static SystemSettingsAirframeTypeOptions airframe_type;
static float gX[AF_NUMX] = {0};
static float gP[AF_NUMP] = {0};
SystemIdentData systemIdentData;

// Private functions
static void AutotuneTask(void *parameters);
static void af_predict(float X[AF_NUMX], float P[AF_NUMP], const float u_in[3], const float gyro[3], const float dT_s, const float t_in);
static void af_init(float X[AF_NUMX], float P[AF_NUMP]);
static uint8_t checkSettings();
static void ComputeStabilizationAndSetPids();

#ifndef AT_QUEUE_NUMELEM
#define AT_QUEUE_NUMELEM 18
#endif

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutotuneInitialize(void)
{
    // Create a queue, connect to manual control command and flightstatus
#ifdef MODULE_AutoTune_BUILTIN
    module_enabled = true;
#else
    HwSettingsOptionalModulesData optionalModules;
    HwSettingsOptionalModulesGet(&optionalModules);
    if (optionalModules.AutoTune == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        module_enabled = true;
    } else {
        module_enabled = false;
    }
#endif

    if (module_enabled) {
        SystemIdentInitialize();
        at_queue = circ_queue_new(sizeof(struct at_queued_data),
                AT_QUEUE_NUMELEM);
        if (!at_queue) {
            module_enabled = false;
        }
    }

    return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AutotuneStart(void)
{
    // Start main task if it is enabled
    if (module_enabled) {
        //taskHandle = PIOS_Thread_Create(AutotuneTask, "Autotune", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
        //TaskMonitorAdd(TASKINFO_RUNNING_AUTOTUNE, taskHandle);
        //PIOS_WDG_RegisterFlag(PIOS_WDG_AUTOTUNE);
        xTaskCreate(AutotuneTask, "AutoTune", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_AUTOTUNE, taskHandle);
    }
    return 0;
}

MODULE_INITCALL(AutotuneInitialize, AutotuneStart);

#if 0
static void at_new_gyro_data(UAVObjEvent * ev, void *ctx, void *obj, int len) {
    (void) ev; (void) ctx;
#else
static void at_new_gyro_data(UAVObjEvent * ev) {
#endif
#if 0
typedef struct {
    UAVObjHandle    obj;
    uint16_t        instId;
    UAVObjEventType event;
    bool lowPriority; /* if true prevents raising warnings */
} UAVObjEvent;
#endif

    static bool last_sample_unpushed = 0;
    GyroSensorData gyro;
    ActuatorDesiredData actuators;

    if (!ev || !ev->obj || ev->instId!=0 || ev->event!=EV_UPDATED) {
        return;
    }

    // object can and possibly will at times change asynchronously so must copy data here, with locking
    // and do it as soon as possible
    GyroSensorGet(&gyro);
    ActuatorDesiredGet(&actuators);

//    GyroSensorData *g = ev->obj;

//    PIOS_Assert(len == sizeof(*g));

    if (last_sample_unpushed) {
        /* Last time we were unable to advance the write pointer.
         * Try again, last chance! */
        if (circ_queue_advance_write(at_queue)) {
            at_points_spilled++;
        }
    }

    struct at_queued_data *q_item = circ_queue_cur_write_pos(at_queue);

    q_item->raw_time = PIOS_DELAY_GetRaw();

    q_item->y[0] = gyro.x;
    q_item->y[1] = gyro.y;
    q_item->y[2] = gyro.z;

    q_item->u[0] = actuators.Roll;
    q_item->u[1] = actuators.Pitch;
    q_item->u[2] = actuators.Yaw;

    q_item->throttle = actuators.Thrust;

    if (circ_queue_advance_write(at_queue) != 0) {
        last_sample_unpushed = true;
    } else {
        last_sample_unpushed = false;
    }
}

static void UpdateSystemIdent(const float *X, const float *noise,
    float dT_s, uint32_t predicts, uint32_t spills, float hover_throttle) {
    systemIdentData.Beta.Roll    = X[6];
    systemIdentData.Beta.Pitch   = X[7];
    systemIdentData.Beta.Yaw     = X[8];
    systemIdentData.Bias.Roll    = X[10];
    systemIdentData.Bias.Pitch   = X[11];
    systemIdentData.Bias.Yaw     = X[12];
    systemIdentData.Tau          = X[9];

    if (noise) {
        systemIdentData.Noise.Roll  = noise[0];
        systemIdentData.Noise.Pitch = noise[1];
        systemIdentData.Noise.Yaw   = noise[2];
    }
    systemIdentData.Period = dT_s * 1000.0f;

    systemIdentData.NumAfPredicts = predicts;
    systemIdentData.NumSpilledPts = spills;

    systemIdentData.HoverThrottle = hover_throttle;

    SystemIdentSet(&systemIdentData);
}

static void UpdateStabilizationDesired(bool doingIdent) {
    StabilizationDesiredData stabDesired;
    StabilizationDesiredGet(&stabDesired);

    ManualControlCommandData manual_control_command;
    ManualControlCommandGet(&manual_control_command);

    stabDesired.Roll = manual_control_command.Roll * rollMax;
    stabDesired.Pitch = manual_control_command.Pitch * pitchMax;
    stabDesired.Yaw = manual_control_command.Yaw * manualRate.Yaw;

    if (doingIdent) {
        stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
        stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
        stabDesired.StabilizationMode.Yaw   = STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT;
    } else {
        stabDesired.StabilizationMode.Roll  = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.StabilizationMode.Pitch = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
        stabDesired.StabilizationMode.Yaw   = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
    }

    stabDesired.Thrust = (airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP) ? manual_control_command.Collective : manual_control_command.Throttle;
// is this a race
// control feels very sluggish too
    StabilizationDesiredSet(&stabDesired);
}


static uint8_t checkSettings()
{
    uint8_t retVal = 0;

    // Check the axis gains
    // Extreme values: Your roll or pitch gain was lower than expected. This will result in large PID values.
    if (systemIdentData.Beta.Roll < 6) {
        retVal |= 1;
    }
    if (systemIdentData.Beta.Pitch < 6) {
        retVal |= 2;
    }

    // Check the response speed
    // Extreme values: Your estimated response speed (tau) is slower than normal. This will result in large PID values.
    if (expf(systemIdentData.Tau) > 0.1f) {
        retVal |= 4;
    }
    // Extreme values: Your estimated response speed (tau) is faster than normal. This will result in large PID values.
    else if (expf(systemIdentData.Tau) < 0.008f) {
        retVal |= 8;
    }

    return retVal;
}


#if 0
void ComputeStabilization()
{
    StabilizationSettingsBank1Data stabSettingsBank;

    switch (systemIdentData.DestinationPidBank) {
    case 1:
        StabilizationSettingsBank1Get((void *)&stabSettingsBank);
        break;
    case 2:
        StabilizationSettingsBank2Get((void *)&stabSettingsBank);
        break;
    case 3:
        StabilizationSettingsBank3Get((void *)&stabSettingsBank);
        break;
    }

    // These three parameters define the desired response properties
    // - rate scale in the fraction of the natural speed of the system
    //   to strive for.
    // - damp is the amount of damping in the system. higher values
    //   make oscillations less likely
    // - ghf is the amount of high frequency gain and limits the influence
    //   of noise
    const double ghf = systemIdentData.RateNoise / 1000.0f;
    const double damp = systemIdentData.RateDamp / 100.0f;

    double tau = exp(systemIdentData.Tau);
    double beta_roll = systemIdentData.Beta.Roll;
    double beta_pitch = systemIdentData.Beta.Pitch;

    double wn = 1.0f/tau;
    double tau_d = 0.0f;
    for (int i = 0; i < 30; i++) {
        double tau_d_roll = (2.0f*damp*tau*wn - 1.0f)/(4.0f*tau*damp*damp*wn*wn - 2.0f*damp*wn - tau*wn*wn + exp(beta_roll)*ghf);
        double tau_d_pitch = (2.0f*damp*tau*wn - 1.0f)/(4.0f*tau*damp*damp*wn*wn - 2.0f*damp*wn - tau*wn*wn + exp(beta_pitch)*ghf);

        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn = (tau + tau_d) / (tau*tau_d) / (2.0f * damp + 2.0f);
    }

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau+tau_d) / tau / tau_d - 2.0f * damp * wn) / 20.0f;
    const double b = ((tau+tau_d) / tau / tau_d - 2.0f * damp * wn - a);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3;
    const double kp_o = 1.0f / 4.0f / (zeta_o * zeta_o) / (1.0f/wn);

    // For now just run over roll and pitch
    int axes = ((systemIdentData.CalculateYaw) : 3 : 2);
    for (int i = 0; i < axes; i++) {
        double beta = exp(SystemIdentBetaToArray(systemIdentData.Beta)[i]);

        double ki = a * b * wn * wn * tau * tau_d / beta;
        double kp = tau * tau_d * ((a+b)*wn*wn + 2.0f*a*b*damp*wn) / beta - ki*tau_d;
        double kd = (tau * tau_d * (a*b + wn*wn + (a+b)*2.0f*damp*wn) - 1.0f) / beta - kp * tau_d;

        switch(i) {
        case 0: // Roll
            stabSettingsBank.RollRatePID.Kp = kp;
            stabSettingsBank.RollRatePID.Ki = ki;
            stabSettingsBank.RollRatePID.Kd = kd;
            stabSettingsBank.RollPI.Kp = kp_o;
            stabSettingsBank.RollPI.Ki = 0;
            break;
        case 1: // Pitch
            stabSettingsBank.PitchRatePID.Kp = kp;
            stabSettingsBank.PitchRatePID.Ki = ki;
            stabSettingsBank.PitchRatePID.Kd = kd;
            stabSettingsBank.PitchPI.Kp = kp_o;
            stabSettingsBank.PitchPI.Ki = 0;
            break;
        case 2: // optional Yaw
            stabSettingsBank.YawRatePID.Kp = kp;
            stabSettingsBank.YawRatePID.Ki = ki;
            stabSettingsBank.YawRatePID.Kd = kd;
            stabSettingsBank.YawPI.Kp = kp_o;
            stabSettingsBank.YawPI.Ki = 0;
            break;
        }
    }
    //stabSettingsBank.DerivativeCutoff = 1.0f / (2.0f*M_PI*tau_d);
}
#else
static void ComputeStabilizationAndSetPids()
{
    StabilizationSettingsBank1Data stabSettingsBank;

    switch (systemIdentData.DestinationPidBank) {
    case 1:
        StabilizationSettingsBank1Get((void *)&stabSettingsBank);
        break;
    case 2:
        StabilizationSettingsBank2Get((void *)&stabSettingsBank);
        break;
    case 3:
        StabilizationSettingsBank3Get((void *)&stabSettingsBank);
        break;
    }

    // These three parameters define the desired response properties
    // - rate scale in the fraction of the natural speed of the system
    //   to strive for.
    // - damp is the amount of damping in the system. higher values
    //   make oscillations less likely
    // - ghf is the amount of high frequency gain and limits the influence
    //   of noise
    const double ghf = systemIdentData.RateNoise / 1000.0d;
    const double damp = systemIdentData.RateDamp / 100.0d;

    double tau = exp(systemIdentData.Tau);
    double beta_roll = systemIdentData.Beta.Roll;
    double beta_pitch = systemIdentData.Beta.Pitch;

    double wn = 1.0d/tau;
    double tau_d = 0.0d;
    for (int i = 0; i < 30; i++) {
        double tau_d_roll = (2.0d*damp*tau*wn - 1.0d)/(4.0d*tau*damp*damp*wn*wn - 2.0d*damp*wn - tau*wn*wn + exp(beta_roll)*ghf);
        double tau_d_pitch = (2.0d*damp*tau*wn - 1.0d)/(4.0d*tau*damp*damp*wn*wn - 2.0d*damp*wn - tau*wn*wn + exp(beta_pitch)*ghf);

        // Select the slowest filter property
        tau_d = (tau_d_roll > tau_d_pitch) ? tau_d_roll : tau_d_pitch;
        wn = (tau + tau_d) / (tau*tau_d) / (2.0d * damp + 2.0d);
    }

    // Set the real pole position. The first pole is quite slow, which
    // prevents the integral being too snappy and driving too much
    // overshoot.
    const double a = ((tau+tau_d) / tau / tau_d - 2.0d * damp * wn) / 20.0d;
    const double b = ((tau+tau_d) / tau / tau_d - 2.0d * damp * wn - a);

    // Calculate the gain for the outer loop by approximating the
    // inner loop as a single order lpf. Set the outer loop to be
    // critically damped;
    const double zeta_o = 1.3d;
    const double kp_o = 1.0d / 4.0d / (zeta_o * zeta_o) / (1.0d/wn);

    // For now just run over roll and pitch
    int axes = ((systemIdentData.CalculateYaw) ? 3 : 2);
    for (int i = 0; i < axes; i++) {
        double beta = exp(SystemIdentBetaToArray(systemIdentData.Beta)[i]);

        double ki = a * b * wn * wn * tau * tau_d / beta;
        double kp = tau * tau_d * ((a+b)*wn*wn + 2.0d*a*b*damp*wn) / beta - ki*tau_d;
        double kd = (tau * tau_d * (a*b + wn*wn + (a+b)*2.0d*damp*wn) - 1.0d) / beta - kp * tau_d;

        switch(i) {
        case 0: // Roll
            stabSettingsBank.RollRatePID.Kp = kp;
            stabSettingsBank.RollRatePID.Ki = ki;
            stabSettingsBank.RollRatePID.Kd = kd;
            stabSettingsBank.RollPI.Kp = kp_o;
            stabSettingsBank.RollPI.Ki = 0;
            break;
        case 1: // Pitch
            stabSettingsBank.PitchRatePID.Kp = kp;
            stabSettingsBank.PitchRatePID.Ki = ki;
            stabSettingsBank.PitchRatePID.Kd = kd;
            stabSettingsBank.PitchPI.Kp = kp_o;
            stabSettingsBank.PitchPI.Ki = 0;
            break;
        case 2: // optional Yaw
            stabSettingsBank.YawRatePID.Kp = kp;
            stabSettingsBank.YawRatePID.Ki = ki;
            stabSettingsBank.YawRatePID.Kd = kd;
            stabSettingsBank.YawPI.Kp = kp_o;
            stabSettingsBank.YawPI.Ki = 0;
            break;
        }
    }

    //stabSettingsBank.DerivativeCutoff = 1.0d / (2.0d*M_PI*tau_d);

    switch (systemIdentData.DestinationPidBank) {
    case 1:
        StabilizationSettingsBank1Set((void *)&stabSettingsBank);
        UAVObjSave(StabilizationSettingsBank1Handle(), 0);
        break;
    case 2:
        StabilizationSettingsBank2Set((void *)&stabSettingsBank);
        UAVObjSave(StabilizationSettingsBank2Handle(), 0);
        break;
    case 3:
        StabilizationSettingsBank3Set((void *)&stabSettingsBank);
        UAVObjSave(StabilizationSettingsBank3Handle(), 0);
        break;
    }
}
#endif


#define MAX_PTS_PER_CYCLE 4

/**
 * Module thread, should not return.
 */
static void AutotuneTask(__attribute__((unused)) void *parameters)
{
    enum AUTOTUNE_STATE state = AT_INIT;

    uint32_t last_update_time = xTaskGetTickCount();

    float noise[3] = {0};

// is this needed?
    af_init(gX,gP);

    uint32_t last_time = 0.0f;
    const uint32_t YIELD_MS = 2;

// should this be put in Init()?
    GyroSensorConnectCallback(at_new_gyro_data);

    bool save_needed = false;

    while(1) {
//PIOS_WDG_UpdateFlag(PIOS_WDG_AUTOTUNE);

        uint32_t diff_time;

        const uint32_t PREPARE_TIME = 2000;
        const uint32_t MEASURE_TIME = 60000;

        static uint32_t update_counter = 0;

        bool doing_ident = false;
        bool can_sleep = true;

        FlightStatusData flightStatus;
        FlightStatusGet(&flightStatus);

//this seems to lock up on Nano
// maybe it was only the first time when allocating new flash block?
        if (save_needed) {
            if (flightStatus.Armed == FLIGHTSTATUS_ARMED_DISARMED) {
                uint8_t failureBits;
                // Save the settings locally.
                UAVObjSave(SystemIdentHandle(), 0);
                failureBits = checkSettings();
                if (!failureBits) {
                    ComputeStabilizationAndSetPids();
                } else {
                    // raise a warning
                    ExtendedAlarmsSet(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION, SYSTEMALARMS_ALARM_WARNING,
                                      SYSTEMALARMS_EXTENDEDALARMSTATUS_AUTOTUNE, failureBits);
                }
                state = AT_INIT;
                save_needed = false;
            }
        }
// can't restart till after you save that's OK I guess
// but you should be able to stop in mid tune and restart from beginning
// maybe reset state in that fn that gets called on mode change

        // Only allow this module to run when autotuning
        if (flightStatus.FlightMode != FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE) {
            state = AT_INIT;
            vTaskDelay(50 / portTICK_RATE_MS);
            continue;
        }

        switch(state) {
            case AT_INIT:

                // moved from UpdateStabilizationDesired()
                StabilizationBankRollMaxGet(&rollMax);
                StabilizationBankPitchMaxGet(&pitchMax);
                StabilizationBankManualRateGet(&manualRate);
                SystemSettingsAirframeTypeGet(&airframe_type);

                // Reset save status; only save if this tune
                // completes.
                save_needed = false;

                last_update_time = xTaskGetTickCount();

                // Only start when armed and flying
                if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
#if 1
                    SystemIdentGet(&systemIdentData);
                    // these are values that could be changed by the user
                    // save them through the following xSetDefaults() call
                    uint8_t rateDamp = systemIdentData.RateDamp;
                    uint8_t rateNoise = systemIdentData.RateNoise;
                    bool calcYaw = systemIdentData.CalculateYaw;
                    uint8_t destBank = systemIdentData.DestinationPidBank;

                    // get these 10.0 10.0 7.0 -4.0 from default values of SystemIdent (.Beta and .Tau)
                    // so that if they are changed there (mainly for future code changes), they will be changed here too
                    SystemIdentSetDefaults(SystemIdentHandle(), 0);
                    SystemIdentGet(&systemIdentData);

                    // restore the user changeable values
                    systemIdentData.RateDamp = rateDamp;
                    systemIdentData.RateNoise = rateNoise;
                    systemIdentData.CalculateYaw = calcYaw;
                    systemIdentData.DestinationPidBank = destBank;
#endif
// remove this one and let the other one init it
// should wait on the other one if that is the case
                    af_init(gX, gP);
                    UpdateSystemIdent(gX, NULL, 0.0f, 0, 0, 0.0f);
                    state = AT_START;
                }

                break;

            case AT_START:

                diff_time = xTaskGetTickCount() - last_update_time;

                // Spend the first block of time in normal rate mode to get stabilized
                if (diff_time > PREPARE_TIME) {
                    last_time = PIOS_DELAY_GetRaw();

                    /* Drain the queue of all current data */
                    while (circ_queue_read_pos(at_queue)) {
                        circ_queue_read_completed(at_queue);
                    }

                    /* And reset the point spill counter */

                    update_counter = 0;
                    at_points_spilled = 0;

                    throttle_accumulator = 0;

                    state = AT_RUN;
                    last_update_time = xTaskGetTickCount();
                }


                break;

            case AT_RUN:

                diff_time = xTaskGetTickCount() - last_update_time;

                doing_ident = true;
                can_sleep = false;

                for (int i=0; i<MAX_PTS_PER_CYCLE; i++) {
                    struct at_queued_data *pt;

                    /* Grab an autotune point */
                    pt = circ_queue_read_pos(at_queue);

                    if (!pt) {
                        /* We've drained the buffer
                         * fully.  Yay! */
                        can_sleep = true;
                        break;
                    }

                    /* calculate time between successive
                     * points */

                    float dT_s = PIOS_DELAY_DiffuS2(last_time,
                            pt->raw_time) * 1.0e-6f;

                    /* This is for the first point, but
                     * also if we have extended drops */
                    if (dT_s > 0.010f) {
                        dT_s = 0.010f;
                    }

                    last_time = pt->raw_time;

                    af_predict(gX, gP, pt->u, pt->y, dT_s, pt->throttle);

                    for (int j=0; j<3; ++j) {
                        const float NOISE_ALPHA = 0.9997f;  // 10 second time constant at 300 Hz
                        noise[j] = NOISE_ALPHA * noise[j] + (1-NOISE_ALPHA) * (pt->y[j] - gX[j]) * (pt->y[j] - gX[j]);
                    }

                    //This will work up to 8kHz with an 89% throttle position before overflow
                    throttle_accumulator += 10000 * pt->throttle;

                    // Update uavo every 256 cycles to avoid
                    // telemetry spam
                    if (!((update_counter++) & 0xff)) {
                        float hover_throttle = ((float)(throttle_accumulator/update_counter))/10000.0f;
                        UpdateSystemIdent(gX, noise, dT_s, update_counter, at_points_spilled, hover_throttle);
                    }

                    /* Free the buffer containing an AT point */
                    circ_queue_read_completed(at_queue);
                }

                if (diff_time > MEASURE_TIME) { // Move on to next state
                    state = AT_FINISHED;
                    last_update_time = xTaskGetTickCount();
                }

                break;

            case AT_FINISHED: ;

                // Wait until disarmed and landed before saving the settings

                float hover_throttle = ((float)(throttle_accumulator/update_counter))/10000.0f;
                UpdateSystemIdent(gX, noise, 0, update_counter, at_points_spilled, hover_throttle);

                save_needed = true;
                state = AT_WAITING;

                break;

            case AT_WAITING:
            default:
                // Set an alarm or some shit like that
                break;
        }

        // Update based on manual controls
        UpdateStabilizationDesired(doing_ident);

        if (can_sleep) {
            vTaskDelay(YIELD_MS / portTICK_RATE_MS);
        }
    }
}

/**
 * Prediction step for EKF on control inputs to quad that
 * learns the system properties
 * @param X the current state estimate which is updated in place
 * @param P the current covariance matrix, updated in place
 * @param[in] the current control inputs (roll, pitch, yaw)
 * @param[in] the gyro measurements
 */
__attribute__((always_inline)) static inline void af_predict(float X[AF_NUMX], float P[AF_NUMP], const float u_in[3], const float gyro[3], const float dT_s, const float t_in)
{
    const float Ts = dT_s;
    const float Tsq = Ts * Ts;
    const float Tsq3 = Tsq * Ts;
    const float Tsq4 = Tsq * Tsq;

    // for convenience and clarity code below uses the named versions of
    // the state variables
    float w1 = X[0];           // roll rate estimate
    float w2 = X[1];           // pitch rate estimate
    float w3 = X[2];           // yaw rate estimate
    float u1 = X[3];           // scaled roll torque
    float u2 = X[4];           // scaled pitch torque
    float u3 = X[5];           // scaled yaw torque
    const float e_b1 = expapprox(X[6]);   // roll torque scale
    const float b1 = X[6];
    const float e_b2 = expapprox(X[7]);   // pitch torque scale
    const float b2 = X[7];
    const float e_b3 = expapprox(X[8]);   // yaw torque scale
    const float b3 = X[8];
    const float e_tau = expapprox(X[9]); // time response of the motors
    const float tau = X[9];
    const float bias1 = X[10];        // bias in the roll torque
    const float bias2 = X[11];       // bias in the pitch torque
    const float bias3 = X[12];       // bias in the yaw torque

    // inputs to the system (roll, pitch, yaw)
    const float u1_in = 4*t_in*u_in[0];
    const float u2_in = 4*t_in*u_in[1];
    const float u3_in = 4*t_in*u_in[2];

    // measurements from gyro
    const float gyro_x = gyro[0];
    const float gyro_y = gyro[1];
    const float gyro_z = gyro[2];

    // update named variables because we want to use predicted
    // values below
    w1 = X[0] = w1 - Ts*bias1*e_b1 + Ts*u1*e_b1;
    w2 = X[1] = w2 - Ts*bias2*e_b2 + Ts*u2*e_b2;
    w3 = X[2] = w3 - Ts*bias3*e_b3 + Ts*u3*e_b3;
    u1 = X[3] = (Ts*u1_in)/(Ts + e_tau) + (u1*e_tau)/(Ts + e_tau);
    u2 = X[4] = (Ts*u2_in)/(Ts + e_tau) + (u2*e_tau)/(Ts + e_tau);
    u3 = X[5] = (Ts*u3_in)/(Ts + e_tau) + (u3*e_tau)/(Ts + e_tau);
    // X[6] to X[12] unchanged

    /**** filter parameters ****/
    const float q_w = 1e-3f;
    const float q_ud = 1e-3f;
    const float q_B = 1e-6f;
    const float q_tau = 1e-6f;
    const float q_bias = 1e-19f;
    const float s_a = 150.0f; // expected gyro measurment noise

    const float Q[AF_NUMX] = {q_w, q_w, q_w, q_ud, q_ud, q_ud, q_B, q_B, q_B, q_tau, q_bias, q_bias, q_bias};

    float D[AF_NUMP];
    for (uint32_t i = 0; i < AF_NUMP; i++)
        D[i] = P[i];

    const float e_tau2    = e_tau * e_tau;
    const float e_tau3    = e_tau * e_tau2;
    const float e_tau4    = e_tau2 * e_tau2;
    const float Ts_e_tau2 = (Ts + e_tau) * (Ts + e_tau);
    const float Ts_e_tau4 = Ts_e_tau2 * Ts_e_tau2;

    // covariance propagation - D is stored copy of covariance
    P[0] = D[0] + Q[0] + 2*Ts*e_b1*(D[3] - D[28] - D[9]*bias1 + D[9]*u1) + Tsq*(e_b1*e_b1)*(D[4] - 2*D[29] + D[32] - 2*D[10]*bias1 + 2*D[30]*bias1 + 2*D[10]*u1 - 2*D[30]*u1 + D[11]*(bias1*bias1) + D[11]*(u1*u1) - 2*D[11]*bias1*u1);
    P[1] = D[1] + Q[1] + 2*Ts*e_b2*(D[5] - D[33] - D[12]*bias2 + D[12]*u2) + Tsq*(e_b2*e_b2)*(D[6] - 2*D[34] + D[37] - 2*D[13]*bias2 + 2*D[35]*bias2 + 2*D[13]*u2 - 2*D[35]*u2 + D[14]*(bias2*bias2) + D[14]*(u2*u2) - 2*D[14]*bias2*u2);
    P[2] = D[2] + Q[2] + 2*Ts*e_b3*(D[7] - D[38] - D[15]*bias3 + D[15]*u3) + Tsq*(e_b3*e_b3)*(D[8] - 2*D[39] + D[42] - 2*D[16]*bias3 + 2*D[40]*bias3 + 2*D[16]*u3 - 2*D[40]*u3 + D[17]*(bias3*bias3) + D[17]*(u3*u3) - 2*D[17]*bias3*u3);
    P[3] = (D[3]*(e_tau2 + Ts*e_tau) + Ts*e_b1*e_tau2*(D[4] - D[29]) + Tsq*e_b1*e_tau*(D[4] - D[29]) + D[18]*Ts*e_tau*(u1 - u1_in) + D[10]*e_b1*(u1*(Ts*e_tau2 + Tsq*e_tau) - bias1*(Ts*e_tau2 + Tsq*e_tau)) + D[21]*Tsq*e_b1*e_tau*(u1 - u1_in) + D[31]*Tsq*e_b1*e_tau*(u1_in - u1) + D[24]*Tsq*e_b1*e_tau*(u1*(u1 - bias1) + u1_in*(bias1 - u1)))/Ts_e_tau2;
    P[4] = (Q[3]*Tsq4 + e_tau4*(D[4] + Q[3]) + 2*Ts*e_tau3*(D[4] + 2*Q[3]) + 4*Q[3]*Tsq3*e_tau + Tsq*e_tau2*(D[4] + 6*Q[3] + u1*(D[27]*u1 + 2*D[21]) + u1_in*(D[27]*u1_in - 2*D[21])) + 2*D[21]*Ts*e_tau3*(u1 - u1_in) - 2*D[27]*Tsq*u1*u1_in*e_tau2)/Ts_e_tau4;
    P[5] = (D[5]*(e_tau2 + Ts*e_tau) + Ts*e_b2*e_tau2*(D[6] - D[34]) + Tsq*e_b2*e_tau*(D[6] - D[34]) + D[19]*Ts*e_tau*(u2 - u2_in) + D[13]*e_b2*(u2*(Ts*e_tau2 + Tsq*e_tau) - bias2*(Ts*e_tau2 + Tsq*e_tau)) + D[22]*Tsq*e_b2*e_tau*(u2 - u2_in) + D[36]*Tsq*e_b2*e_tau*(u2_in - u2) + D[25]*Tsq*e_b2*e_tau*(u2*(u2 - bias2) + u2_in*(bias2 - u2)))/Ts_e_tau2;
    P[6] = (Q[4]*Tsq4 + e_tau4*(D[6] + Q[4]) + 2*Ts*e_tau3*(D[6] + 2*Q[4]) + 4*Q[4]*Tsq3*e_tau + Tsq*e_tau2*(D[6] + 6*Q[4] + u2*(D[27]*u2 + 2*D[22]) + u2_in*(D[27]*u2_in - 2*D[22])) + 2*D[22]*Ts*e_tau3*(u2 - u2_in) - 2*D[27]*Tsq*u2*u2_in*e_tau2)/Ts_e_tau4;
    P[7] = (D[7]*(e_tau2 + Ts*e_tau) + Ts*e_b3*e_tau2*(D[8] - D[39]) + Tsq*e_b3*e_tau*(D[8] - D[39]) + D[20]*Ts*e_tau*(u3 - u3_in) + D[16]*e_b3*(u3*(Ts*e_tau2 + Tsq*e_tau) - bias3*(Ts*e_tau2 + Tsq*e_tau)) + D[23]*Tsq*e_b3*e_tau*(u3 - u3_in) + D[41]*Tsq*e_b3*e_tau*(u3_in - u3) + D[26]*Tsq*e_b3*e_tau*(u3*(u3 - bias3) + u3_in*(bias3 - u3)))/Ts_e_tau2;
    P[8] = (Q[5]*Tsq4 + e_tau4*(D[8] + Q[5]) + 2*Ts*e_tau3*(D[8] + 2*Q[5]) + 4*Q[5]*Tsq3*e_tau + Tsq*e_tau2*(D[8] + 6*Q[5] + u3*(D[27]*u3 + 2*D[23]) + u3_in*(D[27]*u3_in - 2*D[23])) + 2*D[23]*Ts*e_tau3*(u3 - u3_in) - 2*D[27]*Tsq*u3*u3_in*e_tau2)/Ts_e_tau4;
    P[9] = D[9] - Ts*e_b1*(D[30] - D[10] + D[11]*(bias1 - u1));
    P[10] = (D[10]*(Ts + e_tau) + D[24]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
    P[11] = D[11] + Q[6];
    P[12] = D[12] - Ts*e_b2*(D[35] - D[13] + D[14]*(bias2 - u2));
    P[13] = (D[13]*(Ts + e_tau) + D[25]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
    P[14] = D[14] + Q[7];
    P[15] = D[15] - Ts*e_b3*(D[40] - D[16] + D[17]*(bias3 - u3));
    P[16] = (D[16]*(Ts + e_tau) + D[26]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
    P[17] = D[17] + Q[8];
    P[18] = D[18] - Ts*e_b1*(D[31] - D[21] + D[24]*(bias1 - u1));
    P[19] = D[19] - Ts*e_b2*(D[36] - D[22] + D[25]*(bias2 - u2));
    P[20] = D[20] - Ts*e_b3*(D[41] - D[23] + D[26]*(bias3 - u3));
    P[21] = (D[21]*(Ts + e_tau) + D[27]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
    P[22] = (D[22]*(Ts + e_tau) + D[27]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
    P[23] = (D[23]*(Ts + e_tau) + D[27]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
    P[24] = D[24];
    P[25] = D[25];
    P[26] = D[26];
    P[27] = D[27] + Q[9];
    P[28] = D[28] - Ts*e_b1*(D[32] - D[29] + D[30]*(bias1 - u1));
    P[29] = (D[29]*(Ts + e_tau) + D[31]*Ts*(u1 - u1_in))*(e_tau/Ts_e_tau2);
    P[30] = D[30];
    P[31] = D[31];
    P[32] = D[32] + Q[10];
    P[33] = D[33] - Ts*e_b2*(D[37] - D[34] + D[35]*(bias2 - u2));
    P[34] = (D[34]*(Ts + e_tau) + D[36]*Ts*(u2 - u2_in))*(e_tau/Ts_e_tau2);
    P[35] = D[35];
    P[36] = D[36];
    P[37] = D[37] + Q[11];
    P[38] = D[38] - Ts*e_b3*(D[42] - D[39] + D[40]*(bias3 - u3));
    P[39] = (D[39]*(Ts + e_tau) + D[41]*Ts*(u3 - u3_in))*(e_tau/Ts_e_tau2);
    P[40] = D[40];
    P[41] = D[41];
    P[42] = D[42] + Q[12];


    /********* this is the update part of the equation ***********/

    float S[3] = {P[0] + s_a, P[1] + s_a, P[2] + s_a};

    X[0] = w1 + P[0]*((gyro_x - w1)/S[0]);
    X[1] = w2 + P[1]*((gyro_y - w2)/S[1]);
    X[2] = w3 + P[2]*((gyro_z - w3)/S[2]);
    X[3] = u1 + P[3]*((gyro_x - w1)/S[0]);
    X[4] = u2 + P[5]*((gyro_y - w2)/S[1]);
    X[5] = u3 + P[7]*((gyro_z - w3)/S[2]);
    X[6] = b1 + P[9]*((gyro_x - w1)/S[0]);
    X[7] = b2 + P[12]*((gyro_y - w2)/S[1]);
    X[8] = b3 + P[15]*((gyro_z - w3)/S[2]);
    X[9] = tau + P[18]*((gyro_x - w1)/S[0]) + P[19]*((gyro_y - w2)/S[1]) + P[20]*((gyro_z - w3)/S[2]);
    X[10] = bias1 + P[28]*((gyro_x - w1)/S[0]);
    X[11] = bias2 + P[33]*((gyro_y - w2)/S[1]);
    X[12] = bias3 + P[38]*((gyro_z - w3)/S[2]);

    // update the duplicate cache
    for (uint32_t i = 0; i < AF_NUMP; i++)
        D[i] = P[i];

    // This is an approximation that removes some cross axis uncertainty but
    // substantially reduces the number of calculations
    P[0] = -D[0]*(D[0]/S[0] - 1);
    P[1] = -D[1]*(D[1]/S[1] - 1);
    P[2] = -D[2]*(D[2]/S[2] - 1);
    P[3] = -D[3]*(D[0]/S[0] - 1);
    P[4] = D[4] - D[3]*D[3]/S[0];
    P[5] = -D[5]*(D[1]/S[1] - 1);
    P[6] = D[6] - D[5]*D[5]/S[1];
    P[7] = -D[7]*(D[2]/S[2] - 1);
    P[8] = D[8] - D[7]*D[7]/S[2];
    P[9] = -D[9]*(D[0]/S[0] - 1);
    P[10] = D[10] - D[3]*(D[9]/S[0]);
    P[11] = D[11] - D[9]*(D[9]/S[0]);
    P[12] = -D[12]*(D[1]/S[1] - 1);
    P[13] = D[13] - D[5]*(D[12]/S[1]);
    P[14] = D[14] - D[12]*(D[12]/S[1]);
    P[15] = -D[15]*(D[2]/S[2] - 1);
    P[16] = D[16] - D[7]*(D[15]/S[2]);
    P[17] = D[17] - D[15]*(D[15]/S[2]);
    P[18] = -D[18]*(D[0]/S[0] - 1);
    P[19] = -D[19]*(D[1]/S[1] - 1);
    P[20] = -D[20]*(D[2]/S[2] - 1);
    P[21] = D[21] - D[3]*(D[18]/S[0]);
    P[22] = D[22] - D[5]*(D[19]/S[1]);
    P[23] = D[23] - D[7]*(D[20]/S[2]);
    P[24] = D[24] - D[9]*(D[18]/S[0]);
    P[25] = D[25] - D[12]*(D[19]/S[1]);
    P[26] = D[26] - D[15]*(D[20]/S[2]);
    P[27] = D[27] - D[18]*(D[18]/S[0]) - D[19]*(D[19]/S[1]) - D[20]*(D[20]/S[2]);
    P[28] = -D[28]*(D[0]/S[0] - 1);
    P[29] = D[29] - D[3]*(D[28]/S[0]);
    P[30] = D[30] - D[9]*(D[28]/S[0]);
    P[31] = D[31] - D[18]*(D[28]/S[0]);
    P[32] = D[32] - D[28]*(D[28]/S[0]);
    P[33] = -D[33]*(D[1]/S[1] - 1);
    P[34] = D[34] - D[5]*(D[33]/S[1]);
    P[35] = D[35] - D[12]*(D[33]/S[1]);
    P[36] = D[36] - D[19]*(D[33]/S[1]);
    P[37] = D[37] - D[33]*(D[33]/S[1]);
    P[38] = -D[38]*(D[2]/S[2] - 1);
    P[39] = D[39] - D[7]*(D[38]/S[2]);
    P[40] = D[40] - D[15]*(D[38]/S[2]);
    P[41] = D[41] - D[20]*(D[38]/S[2]);
    P[42] = D[42] - D[38]*(D[38]/S[2]);

    // apply limits to some of the state variables
    if (X[9] > -1.5f)
        X[9] = -1.5f;
    else if (X[9] < -5.5f)        /* 4ms */
        X[9] = -5.5f;
    if (X[10] > 0.5f)
        X[10] = 0.5f;
    else if (X[10] < -0.5f)
        X[10] = -0.5f;
    if (X[11] > 0.5f)
        X[11] = 0.5f;
    else if (X[11] < -0.5f)
        X[11] = -0.5f;
    if (X[12] > 0.5f)
        X[12] = 0.5f;
    else if (X[12] < -0.5f)
        X[12] = -0.5f;
}

/**
 * Initialize the state variable and covariance matrix
 * for the system identification EKF
 */
static void af_init(float X[AF_NUMX], float P[AF_NUMP])
{
    static const float q_init[AF_NUMX] = {
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.05f, 0.05f, 0.005f,
        0.05f,
        0.05f, 0.05f, 0.05f
    };

    // X[0] = X[1] = X[2] = 0.0f;    // assume no rotation
    // X[3] = X[4] = X[5] = 0.0f;    // and no net torque
    // X[6] = X[7]        = 10.0f;   // medium amount of strength
    // X[8]               = 7.0f;    // yaw
    // X[9] = -4.0f;                 // and 50 (18?) ms time scale
    // X[10] = X[11] = X[12] = 0.0f; // zero bias

// lets not set SystemIdent to defaults at all
// that way the user can run it a second time, with initial values from the first run
#if 0
    // these are values that could be changed by the user
    // save them through the following xSetDefaults() call
    uint8_t damp = systemIdentData.RateDamp;
    uint8_t noise = systemIdentData.RateNoise;
    bool yaw = systemIdentData.CalculateYaw;
    uint8_t bank = systemIdentData.DestinationPidBank;

    // get these 10.0 10.0 7.0 -4.0 from default values of SystemIdent (.Beta and .Tau)
    // so that if they are changed there (mainly for future code changes), they will be changed here too
    memset(X, 0, AF_NUMX*sizeof(X[0]));
    SystemIdentSetDefaults(SystemIdentHandle(), 0);
    SystemIdentBetaArrayGet(&X[6]);
    SystemIdentTauGet(&X[9]);

    // restore the user changeable values
    systemIdentData.RateDamp = damp;
    systemIdentData.RateNoise = noise;
    systemIdentData.CalculateYaw = yaw;
    systemIdentData.DestinationPidBank = bank;
#else
    // get these 10.0 10.0 7.0 -4.0 from default values of SystemIdent (.Beta and .Tau)
    // so that if they are changed there (mainly for future code changes), they will be changed here too
    memset(X, 0, AF_NUMX*sizeof(X[0]));
    //SystemIdentSetDefaults(SystemIdentHandle(), 0);
    //SystemIdentBetaArrayGet(&X[6]);
    memcpy(&X[6], &systemIdentData.Beta, sizeof(systemIdentData.Beta));
    //SystemIdentTauGet(&X[9]);
    X[9] = systemIdentData.Tau;
#endif

    // P initialization
    // Could zero this like: *P = *((float [AF_NUMP]){});
    memset(P, 0, AF_NUMP*sizeof(P[0]));
    P[0] = q_init[0];
    P[1] = q_init[1];
    P[2] = q_init[2];
    P[4] = q_init[3];
    P[6] = q_init[4];
    P[8] = q_init[5];
    P[11] = q_init[6];
    P[14] = q_init[7];
    P[17] = q_init[8];
    P[27] = q_init[9];
    P[32] = q_init[10];
    P[37] = q_init[11];
    P[42] = q_init[12];
}

/**
 * @}
 * @}
 */
