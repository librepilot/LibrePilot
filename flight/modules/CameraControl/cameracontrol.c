/**
 ******************************************************************************
 *
 * @file       cameracontrol.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @brief      camera control module. triggers cameras with multiple options
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <openpilot.h>
#include "inc/cameracontrol.h"
#include <CoordinateConversions.h>
#include <cameradesired.h>
#include <cameracontrolsettings.h>
#include <cameracontrolactivity.h>
#include <accessorydesired.h>
#include <attitudestate.h>
#include <callbackinfo.h>
#include <flightstatus.h>
#include <gpstime.h>
#include <homelocation.h>
#include <hwsettings.h>
#include <positionstate.h>
#include <velocitystate.h>

// Private variables

typedef enum {
    CAMERASTATUS_Idle = 0,
    CAMERASTATUS_Shot,
    CAMERASTATUS_Video
} CameraStatus;

static struct CameraControl_data {
    int32_t lastTriggerTimeRaw;
    float   lastTriggerNEDPosition[3];
    CameraControlSettingsData settings;
    CameraControlActivityData activity;
    DelayedCallbackInfo *callbackHandle;
    CameraStatus outputStatus;
    CameraStatus lastOutputStatus;
    CameraStatus manualInput;
    CameraStatus lastManualInput;
    bool autoTriggerEnabled;
    uint16_t     ImageId;
    float HomeECEF[3];
    float HomeRne[3][3];
} *ccd;

#define CALLBACK_PRIORITY   CALLBACK_PRIORITY_REGULAR
#define CBTASK_PRIORITY     CALLBACK_TASK_AUXILIARY
#define STACK_SIZE_BYTES    512
#define CALLBACK_STD_PERIOD 50
#define INPUT_DEADBAND      0.1f

static void CameraControlTask();
static void SettingsUpdateCb(__attribute__((unused)) UAVObjEvent *ev);
static void HomeLocationUpdateCb(__attribute__((unused)) UAVObjEvent *ev);
static void UpdateOutput();
static void PublishActivity();
static bool checkActivation();
static void FillActivityInfo();


/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CameraControlInitialize(void)
{
    ccd = 0;
    HwSettingsOptionalModulesData modules;
    HwSettingsOptionalModulesGet(&modules);
    if (modules.CameraControl == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        ccd = (struct CameraControl_data *)pios_malloc(sizeof(struct CameraControl_data));
        memset(ccd, 0, sizeof(struct CameraControl_data));
        ccd->callbackHandle = PIOS_CALLBACKSCHEDULER_Create(&CameraControlTask, CALLBACK_PRIORITY, CBTASK_PRIORITY, CALLBACKINFO_RUNNING_CAMERACONTROL, STACK_SIZE_BYTES);
        CameraControlActivityInitialize();
        CameraDesiredInitialize();
        CameraControlSettingsConnectCallback(SettingsUpdateCb);
        HomeLocationConnectCallback(HomeLocationUpdateCb);
        GPSTimeInitialize();
        PositionStateInitialize();
        AttitudeStateInitialize();
        AccessoryDesiredInitialize();
        FlightStatusInitialize();


        SettingsUpdateCb(NULL);
        HomeLocationUpdateCb(NULL);

        // init output:
        ccd->outputStatus = CAMERASTATUS_Idle;
        UpdateOutput();
    }
    return 0;
}

/* stub: module has no module thread */
int32_t CameraControlStart(void)
{
    if (!ccd) {
        return 0;
    }

    PIOS_CALLBACKSCHEDULER_Schedule(ccd->callbackHandle, CALLBACK_STD_PERIOD, CALLBACK_UPDATEMODE_LATER);
    ccd->lastTriggerTimeRaw = PIOS_DELAY_GetRaw();
    return 0;
}

MODULE_INITCALL(CameraControlInitialize, CameraControlStart);

static void CameraControlTask()
{
    bool trigger = false;
    PositionStateData pos;
    uint32_t timeSinceLastShot = PIOS_DELAY_DiffuS(ccd->lastTriggerTimeRaw);
    CameraStatus newStatus;

    if (checkActivation()) {
        if (ccd->manualInput != ccd->lastManualInput && ccd->manualInput != CAMERASTATUS_Idle) {
            // Manual override
            trigger   = true;
            newStatus = ccd->manualInput;
            ccd->activity.Reason = CAMERACONTROLACTIVITY_REASON_MANUAL;
        } else {
            // MinimumTimeInterval sets a hard limit on time between two consecutive shots, i.e. the minimum time between shots the camera can achieve
            if (ccd->autoTriggerEnabled &&
                timeSinceLastShot > (ccd->settings.MinimumTimeInterval * 1000 * 1000)) {
                // check trigger conditions
                if (ccd->settings.TimeInterval > 0) {
                    if (timeSinceLastShot > ccd->settings.TimeInterval * (1000 * 1000)) {
                        trigger   = true;
                        newStatus = CAMERASTATUS_Shot;
                        ccd->activity.Reason = CAMERACONTROLACTIVITY_REASON_AUTOTIME;
                    }
                }

                if (ccd->settings.SpaceInterval > 0) {
                    PositionStateGet(&pos);
                    float dn = pos.North - ccd->lastTriggerNEDPosition[0];
                    float de = pos.East - ccd->lastTriggerNEDPosition[1];
                    float distance = sqrtf((dn * dn) + (de * de));
                    ccd->activity.TriggerMillisecond = (int16_t)distance * 10.0f;
                    if (distance > ccd->settings.SpaceInterval) {
                        trigger   = true;
                        newStatus = CAMERASTATUS_Shot;
                        ccd->activity.Reason = CAMERACONTROLACTIVITY_REASON_AUTODISTANCE;
                    }
                }
            }
        }
    }
    if (trigger) {
        ccd->outputStatus = newStatus;
        ccd->ImageId++;
        ccd->lastTriggerTimeRaw = PIOS_DELAY_GetRaw();
        ccd->lastTriggerNEDPosition[0] = pos.North;
        ccd->lastTriggerNEDPosition[1] = pos.East;
        ccd->lastTriggerNEDPosition[2] = pos.Down;
    } else {
        ccd->outputStatus = CAMERASTATUS_Idle;
        ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_IDLE;
    }

    ccd->lastManualInput = ccd->manualInput;
    PublishActivity();
    UpdateOutput();
    PIOS_CALLBACKSCHEDULER_Schedule(ccd->callbackHandle, CALLBACK_STD_PERIOD, CALLBACK_UPDATEMODE_SOONER);
}

static void SettingsUpdateCb(__attribute__((unused)) UAVObjEvent *ev)
{
    CameraControlSettingsGet(&ccd->settings);
}

static bool checkActivation()
{
    if (ccd->settings.ManualTriggerInput != CAMERACONTROLSETTINGS_MANUALTRIGGERINPUT_NONE) {
        uint8_t accessory = ccd->settings.ManualTriggerInput - CAMERACONTROLSETTINGS_MANUALTRIGGERINPUT_ACCESSORY0;

        AccessoryDesiredData accessoryDesired;
        AccessoryDesiredInstGet(accessory, &accessoryDesired);

        if (fabsf(accessoryDesired.AccessoryVal - ccd->settings.InputValues.Shot) < INPUT_DEADBAND) {
            ccd->manualInput = CAMERASTATUS_Shot;
        } else if (fabsf(accessoryDesired.AccessoryVal - ccd->settings.InputValues.Video) < INPUT_DEADBAND) {
            ccd->manualInput = CAMERASTATUS_Video;
        } else {
            ccd->manualInput = CAMERASTATUS_Idle;
        }
    }

    switch (ccd->settings.AutoTriggerMode) {
    case CAMERACONTROLSETTINGS_AUTOTRIGGERMODE_DISABLED:
        ccd->autoTriggerEnabled = false;
        break;
    case CAMERACONTROLSETTINGS_AUTOTRIGGERMODE_WHENARMED:
    {
        FlightStatusArmedOptions armed;

        FlightStatusArmedGet(&armed);
        ccd->autoTriggerEnabled = (armed == FLIGHTSTATUS_ARMED_ARMED);
    }
    break;
    case CAMERACONTROLSETTINGS_AUTOTRIGGERMODE_ALWAYS:
        ccd->autoTriggerEnabled = true;
        break;
    case CAMERACONTROLSETTINGS_AUTOTRIGGERMODE_INPUT:
    {
        uint8_t accessory = ccd->settings.AutoTriggerInput - CAMERACONTROLSETTINGS_AUTOTRIGGERINPUT_ACCESSORY0;
        AccessoryDesiredData accessoryDesired;
        AccessoryDesiredInstGet(accessory, &accessoryDesired);

        ccd->autoTriggerEnabled = (accessoryDesired.AccessoryVal > INPUT_DEADBAND);
    }
    break;
    case CAMERACONTROLSETTINGS_AUTOTRIGGERMODE_MISSION:
    {
        FlightStatusFlightModeOptions flightmode;
        FlightStatusFlightModeGet(&flightmode);
        ccd->autoTriggerEnabled = (flightmode == FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER);
    }
    break;
    }
    return ccd->autoTriggerEnabled || (ccd->manualInput != CAMERASTATUS_Idle);
}

static void UpdateOutput()
{
    if (ccd->outputStatus != ccd->lastOutputStatus) {
        switch (ccd->outputStatus) {
        case CAMERASTATUS_Idle:
            if (CAMERASTATUS_Shot == ccd->lastOutputStatus) {
                if (PIOS_DELAY_DiffuS(ccd->lastTriggerTimeRaw) > ccd->settings.TriggerPulseWidth * 1000) {
                    CameraDesiredTriggerSet(&ccd->settings.OutputValues.Idle);
                } else {
                    // skip updating lastOutputStatus until TriggerPulseWidth elapsed
                    return;
                }
            }
            break;
        case CAMERASTATUS_Shot:
            CameraDesiredTriggerSet(&ccd->settings.OutputValues.Shot);
            break;
        case CAMERASTATUS_Video:
            CameraDesiredTriggerSet(&ccd->settings.OutputValues.Video);
            break;
        }
    }
    ccd->lastOutputStatus = ccd->outputStatus;
}

static void PublishActivity()
{
    if (ccd->outputStatus != ccd->lastOutputStatus) {
        switch (ccd->outputStatus) {
        case CAMERASTATUS_Idle:
            if (ccd->lastOutputStatus == CAMERASTATUS_Video) {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_STOPVIDEO;
            } else {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_IDLE;
            }
            break;
        case CAMERASTATUS_Shot:
            ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_TRIGGERPICTURE;
            break;
        case CAMERASTATUS_Video:
            if (ccd->lastOutputStatus != CAMERASTATUS_Video) {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_STARTVIDEO;
            } else {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_IDLE;
            }
            break;
        }
        if (ccd->activity.Activity != CAMERACONTROLACTIVITY_ACTIVITY_IDLE
            || ccd->lastOutputStatus != CAMERASTATUS_Shot) {
            FillActivityInfo();
            CameraControlActivitySet(&ccd->activity);
        }
    }
}

static void FillActivityInfo()
{
    CameraControlActivityData *activity = &ccd->activity;
    {
        PositionStateData position;
        PositionStateGet(&position);
        int32_t LLAi[3];
        const float pos[3] = {
            position.North,
            position.East,
            position.Down
        };
        Base2LLA(pos, ccd->HomeECEF, ccd->HomeRne, LLAi);

        activity->Latitude  = LLAi[0];
        activity->Longitude = LLAi[1];
        activity->Altitude  = ((float)LLAi[2]) * 1e-4f;
    }
    {
        GPSTimeData time;
        GPSTimeGet(&time);

        activity->TriggerYear   = time.Year;
        activity->TriggerMonth  = time.Month;
        activity->TriggerDay    = time.Day;
        activity->TriggerHour   = time.Hour;
        activity->TriggerMinute = time.Minute;
        activity->TriggerSecond = time.Second;
        activity->TriggerMillisecond = time.Millisecond;
    }

    activity->ImageId  = ccd->ImageId;
    activity->SystemTS = xTaskGetTickCount() * portTICK_RATE_MS;
    {
        AttitudeStateData attitude;
        AttitudeStateGet(&attitude);

        activity->Roll  = attitude.Roll;
        activity->Pitch = attitude.Pitch;
        activity->Yaw   = attitude.Yaw;
    }
}

static void HomeLocationUpdateCb(__attribute__((unused)) UAVObjEvent *ev)
{
    HomeLocationData home;

    HomeLocationGet(&home);

    int32_t LLAi[3] = {
        home.Latitude,
        home.Longitude,
        home.Altitude
    };
    LLA2ECEF(LLAi, ccd->HomeECEF);
    RneFromLLA(LLAi, ccd->HomeRne);
}
