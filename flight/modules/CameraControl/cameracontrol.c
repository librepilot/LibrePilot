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
#include <cameradesired.h>
#include <cameracontrolsettings.h>
#include <cameracontrolactivity.h>
#include <accessorydesired.h>
#include <attitudestate.h>
#include <callbackinfo.h>
#include <flightstatus.h>
#include <gpstime.h>
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
    bool autoTriggerEnabled;
} *ccd;

#define CALLBACK_PRIORITY   CALLBACK_PRIORITY_REGULAR
#define CBTASK_PRIORITY     CALLBACK_TASK_AUXILIARY
#define STACK_SIZE_BYTES    512
#define CALLBACK_STD_PERIOD 50
#define INPUT_DEADBAND      0.1f

static void CameraControlTask();
static void SettingsUpdateCb(__attribute__((unused)) UAVObjEvent *ev);
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
    HwSettingsInitialize();
    HwSettingsOptionalModulesData modules;
    HwSettingsOptionalModulesGet(&modules);
    if (modules.CameraControl == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        ccd = (struct CameraControl_data *)pios_malloc(sizeof(struct CameraControl_data));
        memset(ccd, 0, sizeof(struct CameraControl_data));
        ccd->callbackHandle = PIOS_CALLBACKSCHEDULER_Create(&CameraControlTask, CALLBACK_PRIORITY, CBTASK_PRIORITY, CALLBACKINFO_RUNNING_CAMERACONTROL, STACK_SIZE_BYTES);
        CameraControlActivityInitialize();
        CameraDesiredInitialize();
        CameraControlSettingsInitialize();
        CameraControlSettingsConnectCallback(SettingsUpdateCb);
        SettingsUpdateCb(NULL);

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
    if (checkActivation()) {
        // Manual override
        if (ccd->manualInput != CAMERASTATUS_Idle) {
            ccd->outputStatus    = ccd->manualInput;
            ccd->activity.Reason = CAMERACONTROLACTIVITY_REASON_MANUAL;
        } else {
            if (ccd->autoTriggerEnabled) {
                ccd->activity.Reason = CAMERACONTROLACTIVITY_REASON_AUTOTIME;
                ccd->outputStatus    = CAMERASTATUS_Shot;
            }
        }
    } else {
        ccd->outputStatus = CAMERASTATUS_Idle;
    }

    UpdateOutput();
    PublishActivity();
    ccd->lastOutputStatus = ccd->outputStatus;
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
            CameraDesiredTriggerSet(&ccd->settings.OutputValues.Idle);
            break;
        case CAMERASTATUS_Shot:
            CameraDesiredTriggerSet(&ccd->settings.OutputValues.Shot);
            break;
        case CAMERASTATUS_Video:
            CameraDesiredTriggerSet(&ccd->settings.OutputValues.Video);
            break;
        }
    }
}

static void PublishActivity()
{
    if (ccd->outputStatus != ccd->lastOutputStatus) {
        switch (ccd->outputStatus) {
        case CAMERASTATUS_Idle:
            if (ccd->lastOutputStatus == CAMERASTATUS_Video) {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_STOPVIDEO;
            } else {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_NONE;
            }
            break;
        case CAMERASTATUS_Shot:
            ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_TRIGGERPICTURE;
            break;
        case CAMERASTATUS_Video:
            if (ccd->lastOutputStatus != CAMERASTATUS_Video) {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_STARTVIDEO;
            } else {
                ccd->activity.Activity = CAMERACONTROLACTIVITY_ACTIVITY_NONE;
            }
            break;
        }
        if (ccd->activity.Activity != CAMERACONTROLACTIVITY_ACTIVITY_NONE) {
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

        activity->Latitude  = position.North;
        activity->Longitude = position.East;
        activity->Altitude  = -position.Down;
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
    }
        activity->SysTS = PIOS_DELAY_GetuS();
    {
        AttitudeStateData attitude;
        AttitudeStateGet(&attitude);

        activity->Roll  = attitude.Roll;
        activity->Pitch = attitude.Pitch;
        activity->Yaw   = attitude.Yaw;
    }
}
