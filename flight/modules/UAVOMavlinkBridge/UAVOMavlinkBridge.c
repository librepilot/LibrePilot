/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup UAVOMavlinkBridge UAVO to Mavlink Bridge Module
 * @{
 *
 * @file       UAVOMavlinkBridge.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 *             Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      Bridges selected UAVObjects to Mavlink
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

// ****************
#include "openpilot.h"
#include "stabilizationdesired.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "gpspositionsensor.h"
#include "manualcontrolcommand.h"
#include "attitudestate.h"
#include "airspeedstate.h"
#include "actuatordesired.h"
#include "flightstatus.h"
#include "systemstats.h"
#include "homelocation.h"
#include "positionstate.h"
#include "velocitystate.h"
#include "taskinfo.h"
#include "mavlink.h"
#include "hwsettings.h"
#include "oplinkstatus.h"
#include "receiverstatus.h"
#include "manualcontrolsettings.h"

#include "custom_types.h"

#define OPLINK_LOW_RSSI  -110
#define OPLINK_HIGH_RSSI -10

// ****************
// Private functions

static void uavoMavlinkBridgeTask(void *parameters);

// ****************
// Private constants

#if defined(PIOS_MAVLINK_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MAVLINK_STACK_SIZE
#else
#define STACK_SIZE_BYTES 696
#endif

#define TASK_PRIORITY    (tskIDLE_PRIORITY)
#define TASK_RATE_HZ     10

static void mavlink_send_extended_status();
static void mavlink_send_rc_channels();
static void mavlink_send_position();
static void mavlink_send_extra1();
static void mavlink_send_extra2();

static const struct {
    uint8_t rate;
    void    (*handler)();
} mav_rates[] = {
    [MAV_DATA_STREAM_EXTENDED_STATUS] = {
        .rate    = 2, // Hz
        .handler = mavlink_send_extended_status,
    },
    [MAV_DATA_STREAM_RC_CHANNELS] =     {
        .rate    = 5, // Hz
        .handler = mavlink_send_rc_channels,
    },
    [MAV_DATA_STREAM_POSITION] =        {
        .rate    = 2, // Hz
        .handler = mavlink_send_position,
    },
    [MAV_DATA_STREAM_EXTRA1] =          {
        .rate    = 10, // Hz
        .handler = mavlink_send_extra1,
    },
    [MAV_DATA_STREAM_EXTRA2] =          {
        .rate    = 2, // Hz
        .handler = mavlink_send_extra2,
    },
};

#define MAXSTREAMS NELEMENTS(mav_rates)

// ****************
// Private variables

static bool module_enabled = false;

static uint8_t *stream_ticks;

static mavlink_message_t *mav_msg;

static void updateSettings();

/**
 * Initialise the module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
static int32_t uavoMavlinkBridgeStart(void)
{
    if (module_enabled) {
        // Start tasks
        xTaskHandle taskHandle;
        xTaskCreate(uavoMavlinkBridgeTask, "uavoMavlinkBridge", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_UAVOMAVLINKBRIDGE, taskHandle);
        return 0;
    }
    return -1;
}
/**
 * Initialise the module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
static int32_t uavoMavlinkBridgeInitialize(void)
{
    if (PIOS_COM_MAVLINK) {
        updateSettings();

        mav_msg = pios_malloc(sizeof(*mav_msg));
        stream_ticks = pios_malloc(MAXSTREAMS);

        if (mav_msg && stream_ticks) {
            for (unsigned x = 0; x < MAXSTREAMS; ++x) {
                stream_ticks[x] = (TASK_RATE_HZ / mav_rates[x].rate);
            }

            module_enabled = true;
        }
    }

    return 0;
}
MODULE_INITCALL(uavoMavlinkBridgeInitialize, uavoMavlinkBridgeStart);

static void send_message()
{
    uint16_t msg_length = MAVLINK_NUM_NON_PAYLOAD_BYTES +
                          mav_msg->len;

    PIOS_COM_SendBuffer(PIOS_COM_MAVLINK, &mav_msg->magic, msg_length);
}

static void mavlink_send_extended_status()
{
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    FlightBatteryStateData batState;
    SystemStatsData systemStats;

    if (FlightBatteryStateHandle() != NULL) {
        FlightBatteryStateGet(&batState);
    }

    SystemStatsGet(&systemStats);

    uint32_t battery_capacity = 0;
    if (FlightBatterySettingsHandle() != NULL) {
        FlightBatterySettingsCapacityGet(&battery_capacity);
    }

    int8_t battery_remaining = 0;
    if (battery_capacity != 0) {
        if (batState.ConsumedEnergy < battery_capacity) {
            battery_remaining = 100 - lroundf(batState.ConsumedEnergy / battery_capacity * 100);
        }
    }

    mavlink_msg_sys_status_pack(0, 200, mav_msg,
                                // onboard_control_sensors_present Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
                                0,
                                // onboard_control_sensors_enabled Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
                                0,
                                // onboard_control_sensors_health Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
                                0,
                                // load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
                                (uint16_t)systemStats.CPULoad * 10,
                                // voltage_battery Battery voltage, in millivolts (1 = 1 millivolt)
                                lroundf(batState.Voltage * 1000), // No need to check for validity, Voltage reads 0.0 when measurement is not configured,
                                // current_battery Battery current, in 10*milliamperes (1 = 10 milliampere), -1: autopilot does not measure the current
                                lroundf(batState.Current * 100), // Same as for Voltage. 0 means no measurement
                                // battery_remaining Remaining battery energy: (0%: 0, 100%: 100), -1: autopilot estimate the remaining battery
                                battery_remaining,
                                // drop_rate_comm Communication drops in percent, (0%: 0, 100%: 10'000), (UART, I2C, SPI, CAN), dropped packets on all links (packets that were corrupted on reception on the MAV)
                                0,
                                // errors_comm Communication errors (UART, I2C, SPI, CAN), dropped packets on all links (packets that were corrupted on reception on the MAV)
                                0,
                                // errors_count1 Autopilot-specific errors
                                0,
                                // errors_count2 Autopilot-specific errors
                                0,
                                // errors_count3 Autopilot-specific errors
                                0,
                                // errors_count4 Autopilot-specific errors
                                0);

    send_message();
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */
}

static void mavlink_send_rc_channels()
{
    ManualControlCommandData manualState;
    FlightStatusData flightStatus;
    SystemStatsData systemStats;

    ManualControlCommandGet(&manualState);
    FlightStatusGet(&flightStatus);
    SystemStatsGet(&systemStats);

    uint8_t mavlinkRssi;

    ManualControlSettingsChannelGroupsData channelGroups;
    ManualControlSettingsChannelGroupsGet(&channelGroups);

#ifdef PIOS_INCLUDE_OPLINKRCVR
    if (channelGroups.Throttle == MANUALCONTROLSETTINGS_CHANNELGROUPS_OPLINK) {
        int8_t rssi;
        OPLinkStatusRSSIGet(&rssi);

        if (rssi < OPLINK_LOW_RSSI) {
            rssi = OPLINK_LOW_RSSI;
        } else if (rssi > OPLINK_HIGH_RSSI) {
            rssi = OPLINK_HIGH_RSSI;
        }

        mavlinkRssi = ((rssi - OPLINK_LOW_RSSI) * 255) / (OPLINK_HIGH_RSSI - OPLINK_LOW_RSSI);
    } else {
#endif /* PIOS_INCLUDE_OPLINKRCVR */
    uint8_t quality;
    ReceiverStatusQualityGet(&quality);

    // MAVLink RSSI's range is 0-255
    mavlinkRssi = (quality * 255) / 100;
#ifdef PIOS_INCLUDE_OPLINKRCVR
}
#endif

    mavlink_msg_rc_channels_raw_pack(0, 200, mav_msg,
                                     // time_boot_ms Timestamp (milliseconds since system boot)
                                     systemStats.FlightTime,
                                     // port Servo output port (set of 8 outputs = 1 port). Most MAVs will just use one, but this allows to encode more than 8 servos.
                                     0,
                                     // chan1_raw RC channel 1 value, in microseconds
                                     manualState.Channel[0],
                                     // chan2_raw RC channel 2 value, in microseconds
                                     manualState.Channel[1],
                                     // chan3_raw RC channel 3 value, in microseconds
                                     manualState.Channel[2],
                                     // chan4_raw RC channel 4 value, in microseconds
                                     manualState.Channel[3],
                                     // chan5_raw RC channel 5 value, in microseconds
                                     manualState.Channel[4],
                                     // chan6_raw RC channel 6 value, in microseconds
                                     manualState.Channel[5],
                                     // chan7_raw RC channel 7 value, in microseconds
                                     manualState.Channel[6],
                                     // chan8_raw RC channel 8 value, in microseconds
                                     manualState.Channel[7],
                                     // rssi Receive signal strength indicator, 0: 0%, 255: 100%
                                     mavlinkRssi);

    send_message();
}

static void mavlink_send_position()
{
    SystemStatsData systemStats;

    SystemStatsGet(&systemStats);

    if (GPSPositionSensorHandle() != NULL) {
        GPSPositionSensorData gpsPosData;
        GPSPositionSensorGet(&gpsPosData);

        uint8_t gps_fix_type = 0;

        switch (gpsPosData.Status) {
        case GPSPOSITIONSENSOR_STATUS_NOGPS:
            gps_fix_type = 0;
            break;
        case GPSPOSITIONSENSOR_STATUS_NOFIX:
            gps_fix_type = 1;
            break;
        case GPSPOSITIONSENSOR_STATUS_FIX2D:
            gps_fix_type = 2;
            break;
        case GPSPOSITIONSENSOR_STATUS_FIX3D:
            gps_fix_type = 3;
            break;
        }

        mavlink_msg_gps_raw_int_pack(0, 200, mav_msg,
                                     // time_usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
                                     (uint64_t)systemStats.FlightTime * 1000,
                                     // fix_type 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
                                     gps_fix_type,
                                     // lat Latitude in 1E7 degrees
                                     gpsPosData.Latitude,
                                     // lon Longitude in 1E7 degrees
                                     gpsPosData.Longitude,
                                     // alt Altitude in 1E3 meters (millimeters) above MSL
                                     gpsPosData.Altitude * 1000,
                                     // eph GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
                                     gpsPosData.HDOP * 100,
                                     // epv GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
                                     gpsPosData.VDOP * 100,
                                     // vel GPS ground speed (m/s * 100). If unknown, set to: 65535
                                     gpsPosData.Groundspeed * 100,
                                     // cog Course over ground (NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
                                     gpsPosData.Heading * 100,
                                     // satellites_visible Number of satellites visible. If unknown, set to 255
                                     gpsPosData.Satellites);

        send_message();
    }

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    if (HomeLocationHandle() != NULL) {
        HomeLocationData homeLocation;
        HomeLocationGet(&homeLocation);

        mavlink_msg_gps_global_origin_pack(0, 200, mav_msg,
                                           // latitude Latitude (WGS84), expressed as * 1E7
                                           homeLocation.Latitude,
                                           // longitude Longitude (WGS84), expressed as * 1E7
                                           homeLocation.Longitude,
                                           // altitude Altitude(WGS84), expressed as * 1000
                                           homeLocation.Altitude * 1000);

        send_message();
    }
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */

    // TODO add waypoint nav stuff
    // wp_target_bearing
    // wp_dist = mavlink_msg_nav_controller_output_get_wp_dist(&msg);
    // alt_error = mavlink_msg_nav_controller_output_get_alt_error(&msg);
    // aspd_error = mavlink_msg_nav_controller_output_get_aspd_error(&msg);
    // xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg);
    // mavlink_msg_nav_controller_output_pack
    // wp_number
    // mavlink_msg_mission_current_pack
}

static void mavlink_send_extra1()
{
    AttitudeStateData attState;
    SystemStatsData systemStats;

    AttitudeStateGet(&attState);
    SystemStatsGet(&systemStats);

    mavlink_msg_attitude_pack(0, 200, mav_msg,
                              // time_boot_ms Timestamp (milliseconds since system boot)
                              systemStats.FlightTime,
                              // roll Roll angle (rad)
                              DEG2RAD(attState.Roll),
                              // pitch Pitch angle (rad)
                              DEG2RAD(attState.Pitch),
                              // yaw Yaw angle (rad)
                              DEG2RAD(attState.Yaw),
                              // rollspeed Roll angular speed (rad/s)
                              0,
                              // pitchspeed Pitch angular speed (rad/s)
                              0,
                              // yawspeed Yaw angular speed (rad/s)
                              0);

    send_message();
}

static inline float Sq(float x)
{
    return x * x;
}

#define IS_STAB_MODE(d, m) (((d).Roll == (m)) && ((d).Pitch == (m)))

static void mavlink_send_extra2()
{
    float airspeed    = 0;
    float altitude    = 0;
    float groundspeed = 0;
    float climbrate   = 0;


#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    if (AirspeedStateHandle() != NULL) {
        AirspeedStateTrueAirspeedGet(&airspeed);
    }

    if (PositionStateHandle() != NULL) {
        PositionStateDownGet(&altitude);
        altitude *= -1;
    }

    if (VelocityStateHandle() != NULL) {
        VelocityStateData velocityState;
        VelocityStateGet(&velocityState);

        groundspeed = sqrtf(Sq(velocityState.North) + Sq(velocityState.East));
        climbrate   = velocityState.Down * -1;
    }
#endif

    float attitudeYaw = 0;

    AttitudeStateYawGet(&attitudeYaw);
    // round attState.Yaw to nearest int and transfer from (-180 ... 180) to (0 ... 360)
    int16_t heading = lroundf(attitudeYaw);
    if (heading < 0) {
        heading += 360;
    }


    float thrust;
    ActuatorDesiredThrustGet(&thrust);

    mavlink_msg_vfr_hud_pack(0, 200, mav_msg,
                             // airspeed Current airspeed in m/s
                             airspeed,
                             // groundspeed Current ground speed in m/s
                             groundspeed,
                             // heading Current heading in degrees, in compass units (0..360, 0=north)
                             heading,
                             // throttle Current throttle setting in integer percent, 0 to 100
                             thrust * 100,
                             // alt Current altitude (MSL), in meters
                             altitude,
                             // climb Current climb rate in meters/second
                             climbrate);

    send_message();

    FlightStatusData flightStatus;
    FlightStatusGet(&flightStatus);

    StabilizationDesiredStabilizationModeData stabModeData;
    StabilizationDesiredStabilizationModeGet(&stabModeData);

    uint8_t armed_mode = 0;
    if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
        armed_mode |= MAV_MODE_FLAG_SAFETY_ARMED;
    }

    uint8_t custom_mode = CUSTOM_MODE_STAB;

    switch (flightStatus.FlightMode) {
    case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
        custom_mode = CUSTOM_MODE_PHLD;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE:
        custom_mode = CUSTOM_MODE_RTL;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_AUTOCRUISE:
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF:
    case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
        custom_mode = CUSTOM_MODE_AUTO;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_LAND:
        custom_mode = CUSTOM_MODE_LAND;
        break;

    case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
        custom_mode = CUSTOM_MODE_ACRO; // or
        break;
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
        custom_mode = CUSTOM_MODE_ATUN;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED4:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED5:
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED6:

        if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATE)
            || IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATETRAINER)) {
            custom_mode = CUSTOM_MODE_ACRO;
        } else if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_ACRO)) { // this is Acro+ mode
            custom_mode = CUSTOM_MODE_ACRO;
        } else if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATTITUDE)) {
            custom_mode = CUSTOM_MODE_SPORT;
        } else if ((stabModeData.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEHOLD)
                   || (stabModeData.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO)) {
            custom_mode = CUSTOM_MODE_ALTH;
        } else { // looks like stabilized mode, whichever it is..
            custom_mode = CUSTOM_MODE_STAB;
        }

        break;
    case FLIGHTSTATUS_FLIGHTMODE_COURSELOCK:
    case FLIGHTSTATUS_FLIGHTMODE_VELOCITYROAM:
    case FLIGHTSTATUS_FLIGHTMODE_HOMELEASH:
    case FLIGHTSTATUS_FLIGHTMODE_ABSOLUTEPOSITION:
    case FLIGHTSTATUS_FLIGHTMODE_POI:
        custom_mode = CUSTOM_MODE_STAB; // Don't know any better
        break;
    }

    mavlink_msg_heartbeat_pack(0, 200, mav_msg,
                               // type Type of the MAV (quadrotor, helicopter, etc., up to 15 types, defined in MAV_TYPE ENUM)
                               MAV_TYPE_GENERIC,
                               // autopilot Autopilot type / class. defined in MAV_AUTOPILOT ENUM
                               MAV_AUTOPILOT_GENERIC, // or MAV_AUTOPILOT_OPENPILOT
                               // base_mode System mode bitfield, see MAV_MODE_FLAGS ENUM in mavlink/include/mavlink_types.h
                               armed_mode,
                               // custom_mode A bitfield for use for autopilot-specific flags.
                               custom_mode,
                               // system_status System status flag, see MAV_STATE ENUM
                               0);

    send_message();
}

/**
 * Main task. It does not return.
 */

static void uavoMavlinkBridgeTask(__attribute__((unused)) void *parameters)
{
    uint32_t lastSysTime;

    // Main task loop
    lastSysTime = xTaskGetTickCount(); // portTICK_RATE_MS;

    while (1) {
        vTaskDelayUntil(&lastSysTime, (1000 / TASK_RATE_HZ) / portTICK_RATE_MS);

        for (unsigned i = 0; i < MAXSTREAMS; ++i) {
            if (!mav_rates[i].rate || !mav_rates[i].handler) {
                continue;
            }

            if (stream_ticks[i] == 0) {
                // trigger now
                uint8_t rate = mav_rates[i].rate;
                if (rate > TASK_RATE_HZ) {
                    rate = TASK_RATE_HZ;
                }
                stream_ticks[i] = TASK_RATE_HZ / rate;

                mav_rates[i].handler();
            }

            --stream_ticks[i];
        }
    }
}

static uint32_t hwsettings_mavlinkspeed_enum_to_baud(uint8_t baud)
{
    switch (baud) {
    case HWSETTINGS_MAVLINKSPEED_2400:
        return 2400;

    case HWSETTINGS_MAVLINKSPEED_4800:
        return 4800;

    case HWSETTINGS_MAVLINKSPEED_9600:
        return 9600;

    case HWSETTINGS_MAVLINKSPEED_19200:
        return 19200;

    case HWSETTINGS_MAVLINKSPEED_38400:
        return 38400;

    case HWSETTINGS_MAVLINKSPEED_57600:
        return 57600;

    default:
    case HWSETTINGS_MAVLINKSPEED_115200:
        return 115200;
    }
}


static void updateSettings()
{
    if (PIOS_COM_MAVLINK) {
        // Retrieve settings
        HwSettingsMAVLinkSpeedOptions mavlinkSpeed;
        HwSettingsMAVLinkSpeedGet(&mavlinkSpeed);

        PIOS_COM_ChangeBaud(PIOS_COM_MAVLINK, hwsettings_mavlinkspeed_enum_to_baud(mavlinkSpeed));
    }
}
/**
 * @}
 * @}
 */
