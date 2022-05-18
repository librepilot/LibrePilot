/**
 ******************************************************************************
 *
 * @file       notify.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Notify module, show events and status on external led.
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
#include <flightstatus.h>
#include <systemalarms.h>
#include <flightbatterystate.h>
#include <lednotification.h>
#include <optypes.h>
#include <pios_notify.h>
#include <FreeRTOS.h>
#include <task.h>
#include <eventdispatcher.h>
#include "inc/notify.h"
#include "inc/sequences.h"
#include <pios_mem.h>

#define SAMPLE_PERIOD_MS 250
// private types
typedef struct {
    uint32_t lastAlarmTime;
    uint8_t  lastAlarm;
} AlarmStatus_t;
// function declarations
static void updatedCb(UAVObjEvent *ev);
static void onTimerCb(UAVObjEvent *ev);
static void checkAlarm(uint8_t alarm, uint8_t *last_alarm, uint32_t *last_alm_time,
                       uint8_t warn_sequence, uint8_t critical_sequence, uint8_t error_sequence,
                       uint32_t timeBetweenNotifications);
static AlarmStatus_t *alarmStatus;
int32_t NotifyInitialize(void)
{
    if (PIOS_WS2811_DEVICE) {
        alarmStatus = (AlarmStatus_t *)pios_malloc(sizeof(AlarmStatus_t) * alarmsMapSize);
        for (uint8_t i = 0; i < alarmsMapSize; i++) {
            alarmStatus[i].lastAlarm     = SYSTEMALARMS_ALARM_OK;
            alarmStatus[i].lastAlarmTime = 0;
        }

        FlightStatusConnectCallback(&updatedCb);
        static UAVObjEvent ev;
        memset(&ev, 0, sizeof(UAVObjEvent));
        EventPeriodicCallbackCreate(&ev, onTimerCb, SAMPLE_PERIOD_MS / portTICK_RATE_MS);

        updatedCb(0);
    }
    return 0;
}
MODULE_INITCALL(NotifyInitialize, 0);


void updatedCb(UAVObjEvent *ev)
{
    if (!ev || ev->obj == FlightStatusHandle()) {
        static uint8_t last_armed = 0xff;
        static uint8_t last_flightmode = 0xff;
        uint8_t armed;
        uint8_t flightmode;
        FlightStatusArmedGet(&armed);
        FlightStatusFlightModeGet(&flightmode);
        if (last_armed != armed || (armed && flightmode != last_flightmode)) {
            if (armed) {
                PIOS_NOTIFICATION_Default_Ext_Led_Play(flightModeMap[flightmode], NOTIFY_PRIORITY_BACKGROUND);
            } else {
                PIOS_NOTIFICATION_Default_Ext_Led_Play(&notifications[NOTIFY_SEQUENCE_DISARMED], NOTIFY_PRIORITY_BACKGROUND);
            }
        }
        last_armed = armed;
        last_flightmode = flightmode;
    }
}

void onTimerCb(__attribute__((unused)) UAVObjEvent *ev)
{
    static SystemAlarmsAlarmData alarms;

    SystemAlarmsAlarmGet(&alarms);
    for (uint8_t i = 0; i < alarmsMapSize; i++) {
        uint8_t alarm = SystemAlarmsAlarmToArray(alarms)[alarmsMap[i].alarmIndex];
        checkAlarm(alarm,
                   &alarmStatus[i].lastAlarm,
                   &alarmStatus[i].lastAlarmTime,
                   alarmsMap[i].warnNotification,
                   alarmsMap[i].criticalNotification,
                   alarmsMap[i].errorNotification,
                   alarmsMap[i].timeBetweenNotifications);
    }
}

void checkAlarm(uint8_t alarm, uint8_t *last_alarm, uint32_t *last_alm_time, uint8_t warn_sequence, uint8_t critical_sequence, uint8_t error_sequence, uint32_t timeBetweenNotifications)
{
    if (alarm > SYSTEMALARMS_ALARM_OK) {
        uint32_t current_time = PIOS_DELAY_GetuS();
        if (*last_alarm < alarm || (*last_alm_time + timeBetweenNotifications * 1000) < current_time) {
            uint8_t sequence = (alarm == SYSTEMALARMS_ALARM_WARNING) ? warn_sequence :
                               ((alarm == SYSTEMALARMS_ALARM_CRITICAL) ? critical_sequence : error_sequence);
            bool updated     = true;
            if (sequence != NOTIFY_SEQUENCE_NULL) {
                updated = PIOS_NOTIFICATION_Default_Ext_Led_Play(
                    &notifications[sequence],
                    alarm == SYSTEMALARMS_ALARM_WARNING ? NOTIFY_PRIORITY_REGULAR : NOTIFY_PRIORITY_CRITICAL);
            }
            if (updated) {
                *last_alarm    = alarm;
                *last_alm_time = current_time;
            }
        }
        // workaround timer overflow
        if (*last_alm_time > current_time) {
            *last_alm_time = 0;
        }
    } else {
        *last_alarm = SYSTEMALARMS_ALARM_OK;
    }
}
