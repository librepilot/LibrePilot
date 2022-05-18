/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GPSModule GPS Module
 * @brief Process GPS information
 * @{
 *
 * @file       GPS.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      GPS module, handles GPS and various streams
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

// ****************

#include <openpilot.h>

#include "gpspositionsensor.h"
#include "homelocation.h"
#include "gpstime.h"
#include "gpssatellites.h"
#include "gpsvelocitysensor.h"
#include "gpssettings.h"
#include "taskinfo.h"
#include "hwsettings.h"
#include "auxmagsensor.h"
#include "WorldMagModel.h"
#include "CoordinateConversions.h"
#include <pios_com.h>
#include <pios_board_io.h>

#include "GPS.h"
#include "NMEA.h"
#include "UBX.h"
#include "DJI.h"
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER) && !defined(PIOS_GPS_MINIMAL)
#include "inc/ubx_autoconfig.h"
#define FULL_UBX_PARSER
#endif
#include <auxmagsupport.h>

#include <pios_instrumentation_helper.h>
PERF_DEFINE_COUNTER(counterBytesIn);
PERF_DEFINE_COUNTER(counterRate);
PERF_DEFINE_COUNTER(counterParse);

#if defined(ANY_GPS_PARSER) || defined(ANY_FULL_GPS_PARSER) || defined(ANY_FULL_MAG_PARSER)
#error ANY_GPS_PARSER ANY_FULL_GPS_PARSER and ANY_FULL_MAG_PARSER should all be undefined at this point.
#endif

#if defined(PIOS_INCLUDE_GPS_NMEA_PARSER) || defined(PIOS_INCLUDE_GPS_UBX_PARSER) || defined(PIOS_INCLUDE_GPS_DJI_PARSER)
#define ANY_GPS_PARSER
#endif
#if defined(ANY_GPS_PARSER) && !defined(PIOS_GPS_MINIMAL)
#define ANY_FULL_GPS_PARSER
#endif
#if (defined(PIOS_INCLUDE_HMC5X83) || defined(PIOS_INCLUDE_GPS_UBX_PARSER) || defined(PIOS_INCLUDE_GPS_DJI_PARSER)) && !defined(PIOS_GPS_MINIMAL)
#define ANY_FULL_MAG_PARSER
#endif

// ****************
// Private functions

static void gpsTask(__attribute__((unused)) void *parameters);
static void updateHwSettings(__attribute__((unused)) UAVObjEvent *ev);

#if defined(PIOS_GPS_SETS_HOMELOCATION)
static void setHomeLocation(GPSPositionSensorData *gpsData);
static float GravityAccel(float latitude, float longitude, float altitude);
#endif

#if defined(ANY_FULL_MAG_PARSER)
void AuxMagSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev);
#endif
#if defined(ANY_FULL_GPS_PARSER)
void updateGpsSettings(__attribute__((unused)) UAVObjEvent *ev);
#endif

// ****************
// Private constants

// GPS timeout is greater than 1000ms so that a stock GPS configuration can be used without timeout errors
#define GPS_TIMEOUT_MS             1250

// delay from detecting HomeLocation.Set == False before setting new homelocation
// this prevent that a save with homelocation.Set = false triggered by gps ends saving
// the new location with Set = true.
#define GPS_HOMELOCATION_SET_DELAY 5000

// PIOS serial port receive buffer for GPS is set to 32 bytes for the minimal and 128 bytes for the full version.
// GPS_READ_BUFFER is defined a few lines below in this file.
//
// 57600 bps = 5760 bytes per second
//
// For 32 bytes buffer: this is a maximum of 5760/32 = 180 buffers per second
// that is 1/180 = 0.0056 seconds per packet
// We must never wait more than 5ms since packet was last drained or it may overflow
//
// For 128 bytes buffer: this is a maximum of 5760/128 = 45 buffers per second
// that is 1/45 = 0.022 seconds per packet
// We must never wait more than 22ms since packet was last drained or it may overflow

// There are two delays in play for the GPS task:
// - GPS_BLOCK_ON_NO_DATA_MS is the time to block while waiting to receive serial data from the GPS
// - GPS_LOOP_DELAY_MS is used for a context switch initiated by calling vTaskDelayUntil() to let other tasks run
//
// The delayMs time is not that critical. It should not be too high so that maintenance actions within this task
// are run often enough.
// GPS_LOOP_DELAY_MS on the other hand, should be less then 5.55 ms. A value set too high will cause data to be dropped.

#define GPS_LOOP_DELAY_MS        5
#define GPS_BLOCK_ON_NO_DATA_MS  20

#ifdef PIOS_GPS_SETS_HOMELOCATION
// Unfortunately need a good size stack for the WMM calculation
        #define STACK_SIZE_BYTES 1024
#else
#if defined(PIOS_GPS_MINIMAL)
        #define GPS_READ_BUFFER  32

#ifdef PIOS_INCLUDE_GPS_NMEA_PARSER
        #define STACK_SIZE_BYTES 580 // NMEA
#else // PIOS_INCLUDE_GPS_NMEA_PARSER
        #define STACK_SIZE_BYTES 440 // UBX
#endif // PIOS_INCLUDE_GPS_NMEA_PARSER
#else // PIOS_GPS_MINIMAL
        #define STACK_SIZE_BYTES 650
#endif // PIOS_GPS_MINIMAL
#endif // PIOS_GPS_SETS_HOMELOCATION

#ifndef GPS_READ_BUFFER
#define GPS_READ_BUFFER          128
#endif

#define TASK_PRIORITY            (tskIDLE_PRIORITY + 1)

// ****************
// Private variables

static GPSSettingsData gpsSettings;

#define gpsPort PIOS_COM_GPS
static bool gpsEnabled = false;

static xTaskHandle gpsTaskHandle;

static char *gps_rx_buffer;

static uint32_t timeOfLastUpdateMs;

#if defined(ANY_GPS_PARSER)
static struct GPS_RX_STATS gpsRxStats;
#endif

// ****************
/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */

int32_t GPSStart(void)
{
    if (gpsEnabled) {
        // Start gps task
        xTaskCreate(gpsTask, "GPS", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &gpsTaskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_GPS, gpsTaskHandle);
        return 0;
    }
    return -1;
}

/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */

int32_t GPSInitialize(void)
{
    HwSettingsOptionalModulesData optionalModules;

    HwSettingsOptionalModulesGet(&optionalModules);

#ifdef MODULE_GPS_BUILTIN
    gpsEnabled = true;
    optionalModules.GPS = HWSETTINGS_OPTIONALMODULES_ENABLED;
    HwSettingsOptionalModulesSet(&optionalModules);
#else
    if (optionalModules.GPS == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        gpsEnabled = true;
    } else {
        gpsEnabled = false;
    }
#endif

#if defined(REVOLUTION)
    // These objects MUST be initialized for Revolution
    // because the rest of the system expects to just
    // attach to their queues
    GPSPositionSensorInitialize();
    GPSVelocitySensorInitialize();
    GPSTimeInitialize();
    GPSSatellitesInitialize();
#if defined(ANY_FULL_MAG_PARSER)
    AuxMagSensorInitialize();
    GPSExtendedStatusInitialize();

    // Initialize mag parameters
    AuxMagSettingsUpdatedCb(NULL);
    AuxMagSettingsConnectCallback(AuxMagSettingsUpdatedCb);
#endif
    // updateHwSettings() uses gpsSettings
    GPSSettingsGet(&gpsSettings);
    // must updateHwSettings() before updateGpsSettings() so baud rate is set before GPS serial code starts running
    updateHwSettings(0);
#else /* if defined(REVOLUTION) */
    if (gpsEnabled) {
        GPSPositionSensorInitialize();
        GPSVelocitySensorInitialize();
#if !defined(PIOS_GPS_MINIMAL)
        GPSTimeInitialize();
        GPSSatellitesInitialize();
#endif
        // updateHwSettings() uses gpsSettings
        GPSSettingsGet(&gpsSettings);
        // must updateHwSettings() before updateGpsSettings() so baud rate is set before GPS serial code starts running
        updateHwSettings(0);
    }
#endif /* if defined(REVOLUTION) */

    if (gpsEnabled) {
#if defined(PIOS_GPS_MINIMAL)
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
        gps_rx_buffer = pios_malloc(sizeof(struct UBXPacket));
#elif defined(PIOS_INCLUDE_GPS_DJI_PARSER)
        gps_rx_buffer = pios_malloc(sizeof(struct DJIPacket));
#else
        gps_rx_buffer = NULL;
#endif
#else /* defined(PIOS_GPS_MINIMAL) */
#if defined(ANY_GPS_PARSER)
#if defined(PIOS_INCLUDE_GPS_NMEA_PARSER)
        size_t bufSize = NMEA_MAX_PACKET_LENGTH;
#else
        size_t bufSize = 0;
#endif
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
        if (bufSize < sizeof(struct UBXPacket)) {
            bufSize = sizeof(struct UBXPacket);
        }
#endif
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)
        if (bufSize < sizeof(struct DJIPacket)) {
            bufSize = sizeof(struct DJIPacket);
        }
#endif
        gps_rx_buffer = pios_malloc(bufSize);
#else /* defined(ANY_GPS_PARSER) */
        gps_rx_buffer = NULL;
#endif /* defined(ANY_GPS_PARSER) */
#endif /* defined(PIOS_GPS_MINIMAL) */
#if defined(ANY_GPS_PARSER)
        PIOS_Assert(gps_rx_buffer);
#endif
#if defined(ANY_FULL_GPS_PARSER)
        HwSettingsConnectCallback(updateHwSettings); // allow changing baud rate even after startup
        GPSSettingsConnectCallback(updateGpsSettings);
#endif
        return 0;
    }

    return -1;
}

MODULE_INITCALL(GPSInitialize, GPSStart);

// ****************
/**
 * Main gps task. It does not return.
 */

static void gpsTask(__attribute__((unused)) void *parameters)
{
    uint32_t timeNowMs = xTaskGetTickCount() * portTICK_RATE_MS;

#ifdef PIOS_GPS_SETS_HOMELOCATION
    portTickType homelocationSetDelay = 0;
#endif
    GPSPositionSensorData gpspositionsensor;

    timeOfLastUpdateMs = timeNowMs;
    GPSPositionSensorGet(&gpspositionsensor);
#if defined(ANY_FULL_GPS_PARSER)
    // this should be done in the task because it calls out to actually start the ubx GPS serial reads
    updateGpsSettings(0);
#endif

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    PERF_INIT_COUNTER(counterBytesIn, 0x97510001);
    PERF_INIT_COUNTER(counterRate, 0x97510002);
    PERF_INIT_COUNTER(counterParse, 0x97510003);
    uint8_t c[GPS_READ_BUFFER];

    // Loop forever
    while (1) {
        if (gpsPort) {
#if defined(FULL_UBX_PARSER)
            // do autoconfig stuff for UBX GPS's
            if (gpsSettings.DataProtocol == GPSSETTINGS_DATAPROTOCOL_UBX) {
                char *buffer   = 0;
                uint16_t count = 0;

                gps_ubx_autoconfig_run(&buffer, &count);
                // Something to send?
                if (count) {
                    // clear ack/nak
                    ubxLastAck.clsID = 0x00;
                    ubxLastAck.msgID = 0x00;
                    ubxLastNak.clsID = 0x00;
                    ubxLastNak.msgID = 0x00;

                    PIOS_COM_SendBuffer(gpsPort, (uint8_t *)buffer, count);
                }
            }

            // need to do this whether there is received data or not, or the status (e.g. gcs) will not always be correct
            int32_t ac_status = ubx_autoconfig_get_status();
            static uint8_t lastStatus = GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DISABLED
                                        + GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_RUNNING
                                        + GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DONE
                                        + GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_ERROR;
            gpspositionsensor.AutoConfigStatus =
                ac_status == UBX_AUTOCONFIG_STATUS_DISABLED ? GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DISABLED :
                ac_status == UBX_AUTOCONFIG_STATUS_DONE ? GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DONE :
                ac_status == UBX_AUTOCONFIG_STATUS_ERROR ? GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_ERROR :
                GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_RUNNING;
            if (gpspositionsensor.AutoConfigStatus != lastStatus) {
                GPSPositionSensorAutoConfigStatusSet(&gpspositionsensor.AutoConfigStatus);
                lastStatus = gpspositionsensor.AutoConfigStatus;
            }
#endif /* if defined(FULL_UBX_PARSER) */

            uint16_t cnt;
            int res;
            // This blocks the task until there is something in the buffer (or GPS_BLOCK_ON_NO_DATA_MS passes)
            cnt = PIOS_COM_ReceiveBuffer(gpsPort, c, GPS_READ_BUFFER, GPS_BLOCK_ON_NO_DATA_MS);
            res = PARSER_INCOMPLETE;
            if (cnt > 0) {
                PERF_TIMED_SECTION_START(counterParse);
                PERF_TRACK_VALUE(counterBytesIn, cnt);
                PERF_MEASURE_PERIOD(counterRate);
                switch (gpsSettings.DataProtocol) {
#if defined(PIOS_INCLUDE_GPS_NMEA_PARSER)
                case GPSSETTINGS_DATAPROTOCOL_NMEA:
                    res = parse_nmea_stream(c, cnt, gps_rx_buffer, &gpspositionsensor, &gpsRxStats);
                    break;
#endif
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
                case GPSSETTINGS_DATAPROTOCOL_UBX:
                    res = parse_ubx_stream(c, cnt, gps_rx_buffer, &gpspositionsensor, &gpsRxStats);
                    break;
#endif
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)
                case GPSSETTINGS_DATAPROTOCOL_DJI:
                    res = parse_dji_stream(c, cnt, gps_rx_buffer, &gpspositionsensor, &gpsRxStats);
                    break;
#endif
                default:
                    res = NO_PARSER; // this should not happen
                    break;
                }
                PERF_TIMED_SECTION_END(counterParse);

                if (res == PARSER_COMPLETE) {
                    timeNowMs = xTaskGetTickCount() * portTICK_RATE_MS;
                    timeOfLastUpdateMs = timeNowMs;
                }
            }

            // if there is a protocol error or communication error, or timeout error,
            // generally, if there is an error that is due to configuration or bad hardware, set status to NOGPS
            // poor GPS signal gets a different error/alarm (below)
            //
            // should this be expanded to include aux mag status as well? currently the aux mag
            // attached to a GPS protocol (OPV9 and DJI) still says OK after the GPS/mag goes down
            // (data cable unplugged or flight battery removed with USB still powering the FC)
            timeNowMs = xTaskGetTickCount() * portTICK_RATE_MS;
            if ((res == PARSER_ERROR) ||
                (timeNowMs - timeOfLastUpdateMs) >= GPS_TIMEOUT_MS ||
                (gpsSettings.DataProtocol == GPSSETTINGS_DATAPROTOCOL_UBX && gpspositionsensor.AutoConfigStatus == GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_ERROR)) {
                // we have not received any valid GPS sentences for a while.
                // either the GPS is not plugged in or a hardware problem or the GPS has locked up.
                GPSPositionSensorStatusOptions status = GPSPOSITIONSENSOR_STATUS_NOGPS;
                GPSPositionSensorStatusSet(&status);
                AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_ERROR);
            }
            // if we parsed at least one packet successfully
            // we aren't offline, but we still may not have a good enough fix to fly
            // or we might not even be receiving all necessary GPS packets (NMEA)
            else if (res == PARSER_COMPLETE) {
                // we appear to be receiving packets OK
                // criteria for GPS-OK taken from this post...
                // http://forums.openpilot.org/topic/1523-professors-insgps-in-svn/page__view__findpost__p__5220
                // NMEA doesn't verify that all necessary packet types for an update have been received
                //
                // if (the fix is good) {
                if ((gpspositionsensor.PDOP < gpsSettings.MaxPDOP) && (gpspositionsensor.Satellites >= gpsSettings.MinSatellites) &&
                    ((gpspositionsensor.Status == GPSPOSITIONSENSOR_STATUS_FIX3D) || (gpspositionsensor.Status == GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS)) &&
                    (gpspositionsensor.Latitude != 0 || gpspositionsensor.Longitude != 0)) {
                    AlarmsClear(SYSTEMALARMS_ALARM_GPS);
#ifdef PIOS_GPS_SETS_HOMELOCATION
                    HomeLocationData home;
                    HomeLocationGet(&home);

                    if (home.Set == HOMELOCATION_SET_FALSE) {
                        if (homelocationSetDelay == 0) {
                            homelocationSetDelay = xTaskGetTickCount();
                        }
                        if (xTaskGetTickCount() - homelocationSetDelay > GPS_HOMELOCATION_SET_DELAY) {
                            setHomeLocation(&gpspositionsensor);
                            homelocationSetDelay = 0;
                        }
                    } else {
                        homelocationSetDelay = 0;
                    }
#endif
                    // else if (we are at least getting what might be usable GPS data to finish a flight with) {
                } else if (((gpspositionsensor.Status == GPSPOSITIONSENSOR_STATUS_FIX3D) || (gpspositionsensor.Status == GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS)) &&
                           (gpspositionsensor.Latitude != 0 || gpspositionsensor.Longitude != 0)) {
                    AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_WARNING);
                    // else data is probably not good enough to fly
                } else {
                    AlarmsSet(SYSTEMALARMS_ALARM_GPS, SYSTEMALARMS_ALARM_CRITICAL);
                }
            }
        } // if (gpsPort)
        vTaskDelayUntil(&xLastWakeTime, GPS_LOOP_DELAY_MS / portTICK_RATE_MS);
    } // while (1)
}

#ifdef PIOS_GPS_SETS_HOMELOCATION
/*
 * Estimate the acceleration due to gravity for a particular location in LLA
 */
static float GravityAccel(float latitude, __attribute__((unused)) float longitude, float altitude)
{
    /* WGS84 gravity model.  The effect of gravity over latitude is strong
     * enough to change the estimated accelerometer bias in those apps. */
    float sinsq = sinf(latitude);

    sinsq *= sinsq;
    /* Likewise, over the altitude range of a high-altitude balloon, the effect
     * due to change in altitude can also affect the model. */
    return 9.7803267714f * (1.0f + 0.00193185138639f * sinsq) / sqrtf(1.0f - 0.00669437999013f * sinsq)
           - 3.086e-6f * altitude;
}

// ****************

static void setHomeLocation(GPSPositionSensorData *gpsData)
{
    HomeLocationData home;

    HomeLocationGet(&home);
    GPSTimeData gps;
    GPSTimeGet(&gps);

    if (gps.Year >= 2000) {
        /* Store LLA */
        home.Latitude  = gpsData->Latitude;
        home.Longitude = gpsData->Longitude;
        home.Altitude  = gpsData->Altitude + gpsData->GeoidSeparation;

        /* Compute home ECEF coordinates and the rotation matrix into NED
         * Note that floats are used here - they should give enough precision
         * for this application.*/

        float LLA[3] = { (home.Latitude) / 10e6f, (home.Longitude) / 10e6f, (home.Altitude) };

        /* Compute magnetic flux direction at home location */
        if (WMM_GetMagVector(LLA[0], LLA[1], LLA[2], gps.Month, gps.Day, gps.Year, &home.Be[0]) == 0) {
            /*Compute local acceleration due to gravity.  Vehicles that span a very large
             * range of altitude (say, weather balloons) may need to update this during the
             * flight. */
            home.g_e = GravityAccel(LLA[0], LLA[1], LLA[2]);
            home.Set = HOMELOCATION_SET_TRUE;
            HomeLocationSet(&home);
        }
    }
}
#endif /* ifdef PIOS_GPS_SETS_HOMELOCATION */


uint32_t hwsettings_gpsspeed_enum_to_baud(uint8_t baud)
{
    switch (baud) {
    case HWSETTINGS_GPSSPEED_2400:
        return 2400;

    case HWSETTINGS_GPSSPEED_4800:
        return 4800;

    default:
    case HWSETTINGS_GPSSPEED_9600:
        return 9600;

    case HWSETTINGS_GPSSPEED_19200:
        return 19200;

    case HWSETTINGS_GPSSPEED_38400:
        return 38400;

    case HWSETTINGS_GPSSPEED_57600:
        return 57600;

    case HWSETTINGS_GPSSPEED_115200:
        return 115200;

    case HWSETTINGS_GPSSPEED_230400:
        return 230400;
    }
}


// having already set the GPS's baud rate with a serial command, set the local Revo port baud rate
void gps_set_fc_baud_from_arg(uint8_t baud)
{
    static uint8_t previous_baud = 255;
    static uint8_t mutex; // = 0

    // do we need this?
    // can the code stand having two tasks/threads do an XyzSet() call at the same time?
    if (__sync_fetch_and_add(&mutex, 1) == 0) {
        // don't bother doing the baud change if it is actually the same
        // might drop less data
        if (previous_baud != baud) {
            previous_baud = baud;
            // Set Revo port hwsettings_baud
            PIOS_COM_ChangeBaud(PIOS_COM_GPS, hwsettings_gpsspeed_enum_to_baud(baud));
            GPSPositionSensorBaudRateSet(&baud);
        }
    }
    --mutex;
}


// set the FC port baud rate
// from HwSettings or override with 115200 if DJI
static void gps_set_fc_baud_from_settings()
{
    uint8_t speed;

    // Retrieve settings
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER) && !defined(PIOS_GPS_MINIMAL)
    if (gpsSettings.DataProtocol == GPSSETTINGS_DATAPROTOCOL_DJI) {
        speed = HWSETTINGS_GPSSPEED_115200;
    } else {
#endif
    HwSettingsGPSSpeedGet(&speed);
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER) && !defined(PIOS_GPS_MINIMAL)
}
#endif
    // set fc baud
    gps_set_fc_baud_from_arg(speed);
}


/**
 * HwSettings callback
 * Generally speaking, set the FC baud rate
 * and for UBX, if it is safe, start the auto config process
 * this allows the user to change the baud rate and see if it works without rebooting
 */

// must updateHwSettings() before updateGpsSettings() so baud rate is set before GPS serial code starts running
static void updateHwSettings(UAVObjEvent __attribute__((unused)) *ev)
{
#if defined(ANY_FULL_GPS_PARSER)
    static uint32_t previousGpsPort = 0xf0f0f0f0; // = doesn't match gpsport
#endif
    // if GPS (UBX or NMEA) is enabled at all
    if (gpsPort && gpsEnabled) {
// if we have ubx auto config then sometimes we don't set the baud rate
#if defined(FULL_UBX_PARSER)
        // just for UBX, because it has autoconfig
        // if in startup, or not configured to do ubx and ubx auto config
        //
        // on first use of this port (previousGpsPort != gpsPort) we must set the Revo port baud rate
        // after startup (previousGpsPort == gpsPort) we must set the Revo port baud rate only if autoconfig is disabled
        // always we must set the Revo port baud rate if not using UBX
        if (ev == NULL
            || previousGpsPort != gpsPort
            || gpsSettings.UbxAutoConfig == GPSSETTINGS_UBXAUTOCONFIG_DISABLED
            || gpsSettings.DataProtocol != GPSSETTINGS_DATAPROTOCOL_UBX)
#endif /* defined(FULL_UBX_PARSER) */
        {
            // always set the baud rate
            gps_set_fc_baud_from_settings();
#if defined(FULL_UBX_PARSER)
            // just for UBX, because it has subtypes UBX(6), UBX7 and UBX8
            // changing anything in HwSettings will make it re-verify the sensor type (including auto-baud if not completely disabled)
            // for auto baud disabled, the user can just try some baud rates and when the baud rate is correct, the sensor type becomes valid
            gps_ubx_reset_sensor_type();
#endif /* defined(FULL_UBX_PARSER) */
        }
#if defined(FULL_UBX_PARSER)
        else {
            // else:
            // - we are past startup
            // - we are running uxb protocol
            // - and some form of ubx auto config is enabled
            //
            // it will never do this during startup because ev == NULL during startup
            //
            // this is here so that runtime (not startup) user changes to HwSettings will restart auto-config
            gps_ubx_autoconfig_set(NULL);
        }
#endif /* defined(FULL_UBX_PARSER) */
    }
#if defined(ANY_FULL_GPS_PARSER)
    previousGpsPort = gpsPort;
#endif
}


#if defined(ANY_FULL_MAG_PARSER)
void AuxMagSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    auxmagsupport_reload_settings();
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
    op_gpsv9_load_mag_settings();
#endif
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)
    dji_load_mag_settings();
#endif
#if defined(PIOS_INCLUDE_HMC5X83)
    aux_hmc5x83_load_mag_settings();
#endif
}
#endif /* defined(ANY_FULL_MAG_PARSER) */


#if defined(ANY_FULL_GPS_PARSER)
void updateGpsSettings(__attribute__((unused)) UAVObjEvent *ev)
{
    ubx_autoconfig_settings_t newconfig;

    GPSSettingsGet(&gpsSettings);

#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)
    // each time there is a protocol change, set the baud rate
    // so that DJI can be forced to 115200, but changing to another protocol will change the baud rate to the user specified value
    // note that changes to HwSettings GPS baud rate are detected in the HwSettings callback,
    // but changing to/from DJI is effectively a baud rate change because DJI is forced to be 115200
    gps_set_fc_baud_from_settings(); // knows to force 115200 for DJI
#endif

    // it's OK that ubx auto config is inited and ready to go, when GPS is disabled or running another protocol
    // because ubx auto config never gets called
    // setting it up completely means that if we switch from the other protocol to UBX or disabled to enabled, that it will start normally
    newconfig.UbxAutoConfig = gpsSettings.UbxAutoConfig;
    newconfig.AssistNowAutonomous = gpsSettings.UbxAssistNowAutonomous;
    newconfig.navRate = gpsSettings.UbxRate;
    newconfig.dynamicModel  = gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_PORTABLE ? UBX_DYNMODEL_PORTABLE :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_STATIONARY ? UBX_DYNMODEL_STATIONARY :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_PEDESTRIAN ? UBX_DYNMODEL_PEDESTRIAN :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_AUTOMOTIVE ? UBX_DYNMODEL_AUTOMOTIVE :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_SEA ? UBX_DYNMODEL_SEA :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_AIRBORNE1G ? UBX_DYNMODEL_AIRBORNE1G :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_AIRBORNE2G ? UBX_DYNMODEL_AIRBORNE2G :
                              gpsSettings.UbxDynamicModel == GPSSETTINGS_UBXDYNAMICMODEL_AIRBORNE4G ? UBX_DYNMODEL_AIRBORNE4G :
                              UBX_DYNMODEL_AIRBORNE1G;

    switch ((GPSSettingsUbxSBASModeOptions)gpsSettings.UbxSBASMode) {
    case GPSSETTINGS_UBXSBASMODE_RANGINGCORRECTION:
    case GPSSETTINGS_UBXSBASMODE_CORRECTION:
    case GPSSETTINGS_UBXSBASMODE_RANGINGCORRECTIONINTEGRITY:
    case GPSSETTINGS_UBXSBASMODE_CORRECTIONINTEGRITY:
        newconfig.SBASCorrection = true;
        break;
    default:
        newconfig.SBASCorrection = false;
    }

    switch ((GPSSettingsUbxSBASModeOptions)gpsSettings.UbxSBASMode) {
    case GPSSETTINGS_UBXSBASMODE_RANGING:
    case GPSSETTINGS_UBXSBASMODE_RANGINGCORRECTION:
    case GPSSETTINGS_UBXSBASMODE_RANGINGINTEGRITY:
    case GPSSETTINGS_UBXSBASMODE_RANGINGCORRECTIONINTEGRITY:
        newconfig.SBASRanging = true;
        break;
    default:
        newconfig.SBASRanging = false;
    }

    switch ((GPSSettingsUbxSBASModeOptions)gpsSettings.UbxSBASMode) {
    case GPSSETTINGS_UBXSBASMODE_INTEGRITY:
    case GPSSETTINGS_UBXSBASMODE_RANGINGINTEGRITY:
    case GPSSETTINGS_UBXSBASMODE_RANGINGCORRECTIONINTEGRITY:
    case GPSSETTINGS_UBXSBASMODE_CORRECTIONINTEGRITY:
        newconfig.SBASIntegrity = true;
        break;
    default:
        newconfig.SBASIntegrity = false;
    }

    newconfig.SBASChannelsUsed = gpsSettings.UbxSBASChannelsUsed;

    newconfig.SBASSats = gpsSettings.UbxSBASSats == GPSSETTINGS_UBXSBASSATS_WAAS ? UBX_SBAS_SATS_WAAS :
                         gpsSettings.UbxSBASSats == GPSSETTINGS_UBXSBASSATS_EGNOS ? UBX_SBAS_SATS_EGNOS :
                         gpsSettings.UbxSBASSats == GPSSETTINGS_UBXSBASSATS_MSAS ? UBX_SBAS_SATS_MSAS :
                         gpsSettings.UbxSBASSats == GPSSETTINGS_UBXSBASSATS_GAGAN ? UBX_SBAS_SATS_GAGAN :
                         gpsSettings.UbxSBASSats == GPSSETTINGS_UBXSBASSATS_SDCM ? UBX_SBAS_SATS_SDCM : UBX_SBAS_SATS_AUTOSCAN;

    switch (gpsSettings.UbxGNSSMode) {
    case GPSSETTINGS_UBXGNSSMODE_GPSGLONASS:
        newconfig.enableGPS     = true;
        newconfig.enableGLONASS = true;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = false;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GLONASS:
        newconfig.enableGPS     = false;
        newconfig.enableGLONASS = true;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = false;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GPS:
        newconfig.enableGPS     = true;
        newconfig.enableGLONASS = false;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = false;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GPSBEIDOU:
        newconfig.enableGPS     = true;
        newconfig.enableGLONASS = false;
        newconfig.enableBeiDou  = true;
        newconfig.enableGalileo = false;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GLONASSBEIDOU:
        newconfig.enableGPS     = false;
        newconfig.enableGLONASS = true;
        newconfig.enableBeiDou  = true;
        newconfig.enableGalileo = false;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GPSGALILEO:
        newconfig.enableGPS     = true;
        newconfig.enableGLONASS = false;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = true;
        break;
    case GPSSETTINGS_UBXGNSSMODE_GPSGLONASSGALILEO:
        newconfig.enableGPS     = true;
        newconfig.enableGLONASS = true;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = true;
        break;
    default:
        newconfig.enableGPS     = false;
        newconfig.enableGLONASS = false;
        newconfig.enableBeiDou  = false;
        newconfig.enableGalileo = false;
        break;
    }

// handle protocol changes and devices that have just been powered up
#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)
    if (gpsSettings.DataProtocol == GPSSETTINGS_DATAPROTOCOL_UBX) {
        // do whatever the user has configured for power on startup
        // even autoconfig disabled still gets the ubx version
        gps_ubx_autoconfig_set(&newconfig);
    }
#endif
#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)
    if (gpsSettings.DataProtocol == GPSSETTINGS_DATAPROTOCOL_DJI) {
        // clear out satellite data because DJI doesn't provide it
        GPSSatellitesSetDefaults(GPSSatellitesHandle(), 0);
    }
#endif
}
#endif /* defined(ANY_FULL_GPS_PARSER) */
/**
 * @}
 * @}
 */
