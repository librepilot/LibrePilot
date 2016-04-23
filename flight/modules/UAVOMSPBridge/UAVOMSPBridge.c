/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup UAVOMSPBridge UAVO to MSP Bridge Module
 * @{
 *
 * @file       uavomspbridge.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             Tau Labs, http://taulabs.org, Copyright (C) 2015
 *             dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      Bridges selected UAVObjects to MSP for MWOSD and the like.
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

#include "openpilot.h"
#include "receiverstatus.h"
#include "hwsettings.h"
#include "flightmodesettings.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "gpspositionsensor.h"
#include "manualcontrolcommand.h"
#include "accessorydesired.h"
#include "attitudestate.h"
#include "airspeedstate.h"
#include "actuatorsettings.h"
#include "actuatordesired.h"
#include "flightstatus.h"
#include "systemstats.h"
#include "systemalarms.h"
#include "homelocation.h"
#include "positionstate.h"
#include "velocitystate.h"
#include "stabilizationdesired.h"
#include "taskinfo.h"
#include "stabilizationsettings.h"
#include "stabilizationbank.h"
#include "stabilizationsettingsbank1.h"
#include "stabilizationsettingsbank2.h"
#include "stabilizationsettingsbank3.h"
#include "magstate.h"

#include "pios_sensors.h"


#define WGS84_RADIUS_EARTH_KM 6371.008f // Earth's radius in km
#define PI                    3.14159265358979323846f // [-]

#define PIOS_INCLUDE_MSP_BRIDGE

#if defined(PIOS_INCLUDE_MSP_BRIDGE)

#define MSP_SENSOR_ACC   (1 << 0)
#define MSP_SENSOR_BARO  (1 << 1)
#define MSP_SENSOR_MAG   (1 << 2)
#define MSP_SENSOR_GPS   (1 << 3)
#define MSP_SENSOR_SONAR (1 << 4)

// Magic numbers copied from mwosd
#define  MSP_IDENT       100 // multitype + multiwii version + protocol version + capability variable
#define  MSP_STATUS      101 // cycletime & errors_count & sensor present & box activation & current setting number
#define  MSP_RAW_IMU     102 // 9 DOF
#define  MSP_SERVO       103 // 8 servos
#define  MSP_MOTOR       104 // 8 motors
#define  MSP_RC          105 // 8 rc chan and more
#define  MSP_RAW_GPS     106 // fix, numsat, lat, lon, alt, speed, ground course
#define  MSP_COMP_GPS    107 // distance home, direction home
#define  MSP_ATTITUDE    108 // 2 angles 1 heading
#define  MSP_ALTITUDE    109 // altitude, variometer
#define  MSP_ANALOG      110 // vbat, powermetersum, rssi if available on RX
#define  MSP_RC_TUNING   111 // rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define  MSP_PID         112 // P I D coeff (10 are used currently)
#define  MSP_BOX         113 // BOX setup (number is dependant of your setup)
#define  MSP_MISC        114 // powermeter trig
#define  MSP_MOTOR_PINS  115 // which pins are in use for motors & servos, for GUI
#define  MSP_BOXNAMES    116 // the aux switch names
#define  MSP_PIDNAMES    117 // the PID names
#define  MSP_BOXIDS      119 // get the permanent IDs associated to BOXes
#define  MSP_NAV_STATUS  121 // Returns navigation status
#define  MSP_CELLS       130 // FrSky SPort Telemtry
#define  MSP_ALARMS      242 // Alarm request

#define  MSP_SET_PID     202 // set P I D coeff

typedef enum {
    MSP_BOX_ID_ARM,
    MSP_BOX_ID_ANGLE, // mode.stable (attitude??) - [sensor icon] [ fligth mode icon ]
    MSP_BOX_ID_HORIZON, // [sensor icon] [ flight mode icon ]
    MSP_BOX_ID_BARO, // [sensor icon]
    MSP_BOX_ID_VARIO,
    MSP_BOX_ID_MAG, // [sensor icon]
    MSP_BOX_ID_HEADFREE,
    MSP_BOX_ID_HEADADJ,
    MSP_BOX_ID_CAMSTAB, // [gimbal icon]
    MSP_BOX_ID_CAMTRIG,
    MSP_BOX_ID_GPSHOME, // [ flight mode icon ]
    MSP_BOX_ID_GPSHOLD, // [ flight mode icon ]
    MSP_BOX_ID_PASSTHRU, // [ flight mode icon ]
    MSP_BOX_ID_BEEPER,
    MSP_BOX_ID_LEDMAX,
    MSP_BOX_ID_LEDLOW,
    MSP_BOX_ID_LLIGHTS, // landing lights
    MSP_BOX_ID_CALIB,
    MSP_BOX_ID_GOVERNOR,
    MSP_BOX_ID_OSD_SWITCH, // OSD on or off, maybe.
    MSP_BOX_ID_GPSMISSION, // [ flight mode icon ]
    MSP_BOX_ID_GPSLAND, // [ flight mode icon ]

    MSP_BOX_ID_AIR = 28, // this box will add AIRMODE icon to flight mode.
    MSP_BOX_ID_ACROPLUS = 29, // this will add PLUS icon to ACRO. ACRO = !ANGLE && !HORIZON
} msp_boxid_t;

enum {
    MSP_STATUS_ARMED,
    MSP_STATUS_FLIGHTMODE_STABILIZED,
    MSP_STATUS_FLIGHTMODE_HORIZON,
    MSP_STATUS_SENSOR_BARO,
    MSP_STATUS_SENSOR_MAG,
    MSP_STATUS_MISC_GIMBAL,
    MSP_STATUS_FLIGHTMODE_GPSHOME,
    MSP_STATUS_FLIGHTMODE_GPSHOLD,
    MSP_STATUS_FLIGHTMODE_GPSMISSION,
    MSP_STATUS_FLIGHTMODE_GPSLAND,
    MSP_STATUS_FLIGHTMODE_PASSTHRU,
    MSP_STATUS_OSD_SWITCH,
    MSP_STATUS_ICON_AIRMODE,
    MSP_STATUS_ICON_ACROPLUS,
};

static const uint8_t msp_boxes[] = {
    [MSP_STATUS_ARMED] = MSP_BOX_ID_ARM,
    [MSP_STATUS_FLIGHTMODE_STABILIZED] = MSP_BOX_ID_ANGLE, // flight mode
    [MSP_STATUS_FLIGHTMODE_HORIZON]    = MSP_BOX_ID_HORIZON, // flight mode
    [MSP_STATUS_SENSOR_BARO] = MSP_BOX_ID_BARO,      // sensor icon only
    [MSP_STATUS_SENSOR_MAG]  = MSP_BOX_ID_MAG, // sensor icon only
    [MSP_STATUS_MISC_GIMBAL] = MSP_BOX_ID_CAMSTAB,   // gimbal icon only
    [MSP_STATUS_FLIGHTMODE_GPSHOME]    = MSP_BOX_ID_GPSHOME, // flight mode
    [MSP_STATUS_FLIGHTMODE_GPSHOLD]    = MSP_BOX_ID_GPSHOLD, // flight mode
    [MSP_STATUS_FLIGHTMODE_GPSMISSION] = MSP_BOX_ID_GPSMISSION, // flight mode
    [MSP_STATUS_FLIGHTMODE_GPSLAND]    = MSP_BOX_ID_GPSLAND, // flight mode
    [MSP_STATUS_FLIGHTMODE_PASSTHRU]   = MSP_BOX_ID_PASSTHRU, // flight mode
    [MSP_STATUS_OSD_SWITCH]    = MSP_BOX_ID_OSD_SWITCH, // switch OSD mode
    [MSP_STATUS_ICON_AIRMODE]  = MSP_BOX_ID_AIR,
    [MSP_STATUS_ICON_ACROPLUS] = MSP_BOX_ID_ACROPLUS,
};

typedef enum {
    MSP_IDLE,
    MSP_HEADER_START,
    MSP_HEADER_M,
    MSP_HEADER_SIZE,
    MSP_HEADER_CMD,
    MSP_FILLBUF,
    MSP_CHECKSUM,
    MSP_DISCARD,
    MSP_MAYBE_UAVTALK2,
    MSP_MAYBE_UAVTALK3,
    MSP_MAYBE_UAVTALK4,
    MSP_MAYBE_UAVTALK_SLOW2,
    MSP_MAYBE_UAVTALK_SLOW3,
    MSP_MAYBE_UAVTALK_SLOW4,
    MSP_MAYBE_UAVTALK_SLOW5,
    MSP_MAYBE_UAVTALK_SLOW6
} msp_state;

typedef struct __attribute__((packed)) {
    uint8_t P, I, D;
}
msp_pid_t;

typedef enum {
    PIDROLL,
    PIDPITCH,
    PIDYAW,
    PIDALT,
    PIDPOS,
    PIDPOSR,
    PIDNAVR,
    PIDLEVEL,
    PIDMAG,
    PIDVEL,
    PID_ITEM_COUNT
} pidIndex_e;


#define MSP_ANALOG_VOLTAGE (1 << 0)
#define MSP_ANALOG_CURRENT (1 << 1)

struct msp_bridge {
    uintptr_t com;

    uint8_t   sensors;
    uint8_t   analog;

    msp_state state;
    uint8_t   cmd_size;
    uint8_t   cmd_id;
    uint8_t   cmd_i;
    uint8_t   checksum;
    union {
        uint8_t   data[0];
        // Specific packed data structures go here.
        msp_pid_t piditems[PID_ITEM_COUNT];
    } cmd_data;
};

#if defined(PIOS_MSP_STACK_SIZE)
#define STACK_SIZE_BYTES     PIOS_MSP_STACK_SIZE
#else
#define STACK_SIZE_BYTES     768
#endif
#define TASK_PRIORITY        (tskIDLE_PRIORITY)

#define MAX_ALARM_LEN        30

#define BOOT_DISPLAY_TIME_MS (10 * 1000)

static bool module_enabled = false;
static struct msp_bridge *msp;
static int32_t uavoMSPBridgeInitialize(void);
static void uavoMSPBridgeTask(void *parameters);

static void msp_send(struct msp_bridge *m, uint8_t cmd, const uint8_t *data, size_t len)
{
    uint8_t buf[5];
    uint8_t cs = (uint8_t)(len) ^ cmd;

    buf[0] = '$';
    buf[1] = 'M';
    buf[2] = '>';
    buf[3] = (uint8_t)(len);
    buf[4] = cmd;

    PIOS_COM_SendBuffer(m->com, buf, sizeof(buf));

    if (len > 0) {
        PIOS_COM_SendBuffer(m->com, data, len);

        for (unsigned i = 0; i < len; i++) {
            cs ^= data[i];
        }
    }

    cs    ^= 0;

    buf[0] = cs;
    PIOS_COM_SendBuffer(m->com, buf, 1);
}

static msp_state msp_state_size(struct msp_bridge *m, uint8_t b)
{
    m->cmd_size = b;
    m->checksum = b;
    return MSP_HEADER_CMD;
}

static msp_state msp_state_cmd(struct msp_bridge *m, uint8_t b)
{
    m->cmd_i     = 0;
    m->cmd_id    = b;
    m->checksum ^= m->cmd_id;

    if (m->cmd_size > sizeof(m->cmd_data)) {
        // Too large a body.  Let's ignore it.
        return MSP_DISCARD;
    }

    return m->cmd_size == 0 ? MSP_CHECKSUM : MSP_FILLBUF;
}

static msp_state msp_state_fill_buf(struct msp_bridge *m, uint8_t b)
{
    m->cmd_data.data[m->cmd_i++] = b;
    m->checksum ^= b;
    return m->cmd_i == m->cmd_size ? MSP_CHECKSUM : MSP_FILLBUF;
}

static void msp_send_attitude(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            int16_t x;
            int16_t y;
            int16_t h;
        } att;
    } data;
    AttitudeStateData attState;

    AttitudeStateGet(&attState);

    // Roll and Pitch are in 10ths of a degree.
    data.att.x = attState.Roll * 10;
    data.att.y = attState.Pitch * -10;
    // Yaw is just -180 -> 180
    data.att.h = attState.Yaw;

    msp_send(m, MSP_ATTITUDE, data.buf, sizeof(data));
}

#define IS_STAB_MODE(d, m) (((d).Roll == (m)) && ((d).Pitch == (m)))

static void msp_send_status(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            uint16_t cycleTime;
            uint16_t i2cErrors;
            uint16_t sensors;
            uint32_t flags;
            uint8_t  setting;
        } __attribute__((packed)) status;
    } data;
    // TODO: https://github.com/TauLabs/TauLabs/blob/next/shared/uavobjectdefinition/actuatordesired.xml#L8
    data.status.cycleTime = 0;
    data.status.i2cErrors = 0;

    data.status.sensors   = m->sensors;

    if (GPSPositionSensorHandle() != NULL) {
        GPSPositionSensorStatusOptions gpsStatus;
        GPSPositionSensorStatusGet(&gpsStatus);

        if (gpsStatus != GPSPOSITIONSENSOR_STATUS_NOGPS) {
            data.status.sensors |= MSP_SENSOR_GPS;
        }
    }

    data.status.flags   = 0;
    data.status.setting = 0;

    FlightStatusData flightStatus;
    FlightStatusGet(&flightStatus);

    StabilizationDesiredStabilizationModeData stabModeData;
    StabilizationDesiredStabilizationModeGet(&stabModeData);

    if (flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) {
        data.status.flags |= (1 << MSP_STATUS_ARMED);
    }

    // flightmode

    if (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD) {
        data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_GPSHOLD);
    } else if (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE) {
        data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_GPSHOME);
    } else if (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER) {
        data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_GPSMISSION);
    } else if (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_LAND) {
        data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_GPSLAND);
    } else if (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_MANUAL) {
        data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_PASSTHRU);
    } else { //
             // if Roll(Rate) && Pitch(Rate) => Acro
             // if Roll(Acro+) && Pitch(Acro+) => Acro+
             // if Roll(Rattitude) && Pitch(Rattitude) => Horizon
             // else => Stabilized

        if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATE)
            || IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATETRAINER)) {
            // data.status.flags should not set any mode flags
        } else if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_ACRO)) { // this is Acro+ mode
            data.status.flags |= (1 << MSP_STATUS_ICON_ACROPLUS);
        } else if (IS_STAB_MODE(stabModeData, STABILIZATIONDESIRED_STABILIZATIONMODE_RATTITUDE)) {
            data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_HORIZON);
        } else { // looks like stabilized mode, whichever it is..
            data.status.flags |= (1 << MSP_STATUS_FLIGHTMODE_STABILIZED);
        }
    }

    // sensors in use?
    // flight mode HORIZON or STABILIZED will turn on Accelerometer icon.
    // Barometer?

    if ((stabModeData.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEHOLD)
        || (stabModeData.Thrust == STABILIZATIONDESIRED_STABILIZATIONMODE_ALTITUDEVARIO)) {
        data.status.flags |= (1 << MSP_STATUS_SENSOR_BARO);
    }

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    if (MagStateHandle() != NULL) {
        MagStateSourceOptions magSource;
        MagStateSourceGet(&magSource);

        if (magSource != MAGSTATE_SOURCE_INVALID) {
            data.status.flags |= (1 << MSP_STATUS_SENSOR_MAG);
        }
    }
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */

    if (flightStatus.AlwaysStabilizeWhenArmed == FLIGHTSTATUS_ALWAYSSTABILIZEWHENARMED_TRUE) {
        data.status.flags |= (1 << MSP_STATUS_ICON_AIRMODE);
    }

    msp_send(m, MSP_STATUS, data.buf, sizeof(data));
}

static void msp_send_analog(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            uint8_t  vbat;
            uint16_t powerMeterSum;
            uint16_t rssi;
            uint16_t current;
        } __attribute__((packed)) status;
    } data;

    memset(&data, 0, sizeof(data));

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    if (FlightBatteryStateHandle() != NULL) {
        FlightBatteryStateData batState;
        FlightBatteryStateGet(&batState);

        if (m->analog & MSP_ANALOG_VOLTAGE) {
            data.status.vbat = (uint8_t)lroundf(batState.Voltage * 10);
        }
        if (m->analog & MSP_ANALOG_CURRENT) {
            data.status.current = lroundf(batState.Current * 100);
            data.status.powerMeterSum = lroundf(batState.ConsumedEnergy);
        }
    }
#endif
    // OPLink supports RSSI reported in dBm, but updates different value in ReceiverStatus.Quality (link_quality - function of lost, repaired and good packet count).
    // We use same method as in Receiver module to decide if oplink is used for control:
    // Check for Throttle channel group, if this belongs to oplink, we will use oplink rssi instead of ReceiverStatus.Quality.

    ManualControlSettingsChannelGroupsData channelGroups;
    ManualControlSettingsChannelGroupsGet(&channelGroups);
    
    if(channelGroups.Throttle == MANUALCONTROLSETTINGS_CHANNELGROUPS_OPLINK)
    {
        int8_t rssi;
        OPLinkStatusRSSIGet(&rssi);
        
        // oplink rssi - low: -127 noise floor (set by software when link is completely lost)
        //               max: various articles found on web quote -10 as maximum received signal power expressed in dBm.
        // MSP values have no units, and OSD rssi display requires calibration anyway, so we will translate -127 to -10 -> 0-1023
        
        data.status.rssi = ((127 + rssi) * 1023) / (127 - 10);
    }
    else
    {
        uint8_t quality;
        ReceiverStatusQualityGet(&quality);

        // MSP RSSI's range is 0-1023

        data.status.rssi = (quality * 1023) / 100;
    }

    if (data.status.rssi > 1023) {
        data.status.rssi = 1023;
    }

    msp_send(m, MSP_ANALOG, data.buf, sizeof(data));
}

static void msp_send_ident(__attribute__((unused)) struct msp_bridge *m)
{
    // TODO
}

static void msp_send_raw_gps(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            uint8_t  fix;                 // 0 or 1
            uint8_t  num_sat;
            int32_t  lat;                  // 1 / 10 000 000 deg
            int32_t  lon;                  // 1 / 10 000 000 deg
            uint16_t alt; // meter
            uint16_t speed; // cm/s
            int16_t  ground_course;        // degree * 10
        } __attribute__((packed)) raw_gps;
    } data;

    GPSPositionSensorData gps_data;

    if (GPSPositionSensorHandle() != NULL) {
        GPSPositionSensorGet(&gps_data);

        data.raw_gps.fix     = (gps_data.Status >= GPSPOSITIONSENSOR_STATUS_FIX2D ? 1 : 0);  // Data will display on OSD if 2D fix or better
        data.raw_gps.num_sat = gps_data.Satellites;
        data.raw_gps.lat     = gps_data.Latitude;
        data.raw_gps.lon     = gps_data.Longitude;
        data.raw_gps.alt     = (uint16_t)gps_data.Altitude;
        data.raw_gps.speed   = (uint16_t)gps_data.Groundspeed;
        data.raw_gps.ground_course = (int16_t)(gps_data.Heading * 10.0f);

        msp_send(m, MSP_RAW_GPS, data.buf, sizeof(data));
    }
}

#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
static void msp_send_comp_gps(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            uint16_t distance_to_home; // meter
            int16_t  direction_to_home;    // degree [-180:180]
            uint8_t  home_position_valid;  // 0 = Invalid, Dronin MSP specific
        } __attribute__((packed)) comp_gps;
    } data;
    
    GPSPositionSensorData gps_data;
    HomeLocationData home_data;

    if ((GPSPositionSensorHandle() == NULL) || (HomeLocationHandle() == NULL)) {
        data.comp_gps.distance_to_home    = 0;
        data.comp_gps.direction_to_home   = 0;
        data.comp_gps.home_position_valid = 0; // Home distance and direction will not display on OSD
    } else {
        GPSPositionSensorGet(&gps_data);
        HomeLocationGet(&home_data);

        if ((gps_data.Status < GPSPOSITIONSENSOR_STATUS_FIX2D) || (home_data.Set == HOMELOCATION_SET_FALSE)) {
            data.comp_gps.distance_to_home    = 0;
            data.comp_gps.direction_to_home   = 0;
            data.comp_gps.home_position_valid = 0; // Home distance and direction will not display on OSD
        } else {
            data.comp_gps.home_position_valid = 1; // Home distance and direction will display on OSD

            int32_t delta_lon = (home_data.Longitude - gps_data.Longitude); // degrees * 1e7
            int32_t delta_lat = (home_data.Latitude - gps_data.Latitude); // degrees * 1e7

            float delta_y     = DEG2RAD((float)delta_lon * WGS84_RADIUS_EARTH_KM);  // KM * 1e7
            float delta_x     = DEG2RAD((float)delta_lat * WGS84_RADIUS_EARTH_KM);  // KM * 1e7

            delta_y *= cosf(DEG2RAD((float)home_data.Latitude * 1e-7f)); // Latitude compression correction

            data.comp_gps.distance_to_home = (uint16_t)(sqrtf(delta_x * delta_x + delta_y * delta_y) * 1e-4f); // meters

            if ((delta_lon == 0) && (delta_lat == 0)) {
    data.comp_gps.direction_to_home   = 0;
            } else {
                data.comp_gps.direction_to_home = RAD2DEG((int16_t)(atan2f(delta_y, delta_x))); // degrees;
            }
        }
    
    msp_send(m, MSP_COMP_GPS, data.buf, sizeof(data));
}
}

static void msp_send_altitude(struct msp_bridge *m)
{
    union {
        uint8_t buf[0];
        struct {
            int32_t  estimatedAltitude; // cm
            int16_t  vario; // cm/s
        } __attribute__((packed)) altitude;
    } data;


    if (PositionStateHandle() != NULL) {
        PositionStateData positionState;
        PositionStateGet(&positionState);

        data.altitude.estimatedAltitude = (int32_t)roundf(positionState.Down * -100.0f);
    } else {
        data.altitude.estimatedAltitude = 0;
    }
    
    if( VelocityStateHandle() != NULL) {
        VelocityStateData velocityState;
        VelocityStateGet(&velocityState);
        
        data.altitude.vario = (int16_t)roundf(velocityState.Down * -100.0f);
    } else {
        data.altitude.vario = 0;
    }

    msp_send(m, MSP_ALTITUDE, data.buf, sizeof(data));
}
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */


// Scale stick values whose input range is -1 to 1 to MSP's expected
// PWM range of 1000-2000.
static uint16_t msp_scale_rc(float percent)
{
    return percent * 500 + 1500;
}

// Throttle maps to 1100-1900 and a bit differently as -1 == 1000 and
// then jumps immediately to 0 -> 1 for the rest of the range.  MWOSD
// can learn ranges as wide as they are sent, but defaults to
// 1100-1900 so the throttle indicator will vary for the same stick
// position unless we send it what it wants right away.
static uint16_t msp_scale_rc_thr(float percent)
{
    return percent <= 0 ? 1100 : percent * 800 + 1100;
}

// MSP RC order is Roll/Pitch/Yaw/Throttle/AUX1/AUX2/AUX3/AUX4
static void msp_send_channels(struct msp_bridge *m)
{
    AccessoryDesiredData acc0, acc1, acc2, acc3;
    ManualControlCommandData manualState;

    ManualControlCommandGet(&manualState);
    AccessoryDesiredInstGet(0, &acc0);
    AccessoryDesiredInstGet(1, &acc1);
    AccessoryDesiredInstGet(2, &acc2);
    AccessoryDesiredInstGet(3, &acc2);

    union {
        uint8_t  buf[0];
        uint16_t channels[8];
    } data = {
        .channels = {
            msp_scale_rc(manualState.Roll),
            msp_scale_rc(manualState.Pitch * -1),  // TL pitch is backwards
            msp_scale_rc(manualState.Yaw),
            msp_scale_rc_thr(manualState.Throttle),
            msp_scale_rc(acc0.AccessoryVal),
            msp_scale_rc(acc1.AccessoryVal),
            msp_scale_rc(acc2.AccessoryVal),
            msp_scale_rc(acc3.AccessoryVal),
        }
    };

    msp_send(m, MSP_RC, data.buf, sizeof(data));
}

static void msp_send_boxids(struct msp_bridge *m) // This is actually sending a map of MSP_STATUS.flag bits to BOX ids.
{
    msp_send(m, MSP_BOXIDS, msp_boxes, sizeof(msp_boxes));
}

static StabilizationSettingsFlightModeMapOptions get_current_stabilization_bank()
{
    uint8_t fm;

    ManualControlCommandFlightModeSwitchPositionGet(&fm);

    if (fm >= FLIGHTMODESETTINGS_FLIGHTMODEPOSITION_NUMELEM) {
        return STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK1;
    }

    StabilizationSettingsFlightModeMapOptions flightModeMap[STABILIZATIONSETTINGS_FLIGHTMODEMAP_NUMELEM];
    StabilizationSettingsFlightModeMapGet(flightModeMap);

    return flightModeMap[fm];
}

static void get_current_stabilizationbankdata(StabilizationBankData *bankData)
{
    switch (get_current_stabilization_bank()) {
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK1:
        StabilizationSettingsBank1Get((StabilizationSettingsBank1Data *)bankData);
        break;
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK2:
        StabilizationSettingsBank2Get((StabilizationSettingsBank2Data *)bankData);
        break;
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK3:
        StabilizationSettingsBank3Get((StabilizationSettingsBank3Data *)bankData);
        break;
    }
}

static void set_current_stabilizationbankdata(const StabilizationBankData *bankData)
{
    // update just relevant parts. or not.
    switch (get_current_stabilization_bank()) {
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK1:
        StabilizationSettingsBank1Set((const StabilizationSettingsBank1Data *)bankData);
        break;
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK2:
        StabilizationSettingsBank2Set((const StabilizationSettingsBank2Data *)bankData);
        break;
    case STABILIZATIONSETTINGS_FLIGHTMODEMAP_BANK3:
        StabilizationSettingsBank3Set((const StabilizationSettingsBank3Data *)bankData);
        break;
    }
}

static void pid_native2msp(const float *native, msp_pid_t *piditem)
{
    piditem->P = lroundf(native[0] * 10000);
    piditem->I = lroundf(native[1] * 10000);
    piditem->D = lroundf(native[2] * 10000);
}

static void pid_msp2native(const msp_pid_t *piditem, float *native)
{
    native[0] = (float)piditem->P / 10000;
    native[1] = (float)piditem->I / 10000;
    native[2] = (float)piditem->D / 10000;
}

static void msp_send_pid(struct msp_bridge *m)
{
    StabilizationBankData bankData;

    get_current_stabilizationbankdata(&bankData);

    msp_pid_t piditems[PID_ITEM_COUNT];

    memset(piditems, 0, sizeof(piditems));

    pid_native2msp((float *)&bankData.RollRatePID, &piditems[PIDROLL]);
    pid_native2msp((float *)&bankData.PitchRatePID, &piditems[PIDPITCH]);
    pid_native2msp((float *)&bankData.YawRatePID, &piditems[PIDYAW]);

    msp_send(m, MSP_PID, (const uint8_t *)piditems, sizeof(piditems));
}

static void msp_set_pid(struct msp_bridge *m)
{
    StabilizationBankData bankData;

    get_current_stabilizationbankdata(&bankData);

    pid_msp2native(&m->cmd_data.piditems[PIDROLL], (float *)&bankData.RollRatePID);
    pid_msp2native(&m->cmd_data.piditems[PIDPITCH], (float *)&bankData.PitchRatePID);
    pid_msp2native(&m->cmd_data.piditems[PIDYAW], (float *)&bankData.YawRatePID);

    set_current_stabilizationbankdata(&bankData);

    msp_send(m, MSP_SET_PID, 0, 0); // send ack.
}

#define ALARM_OK    0
#define ALARM_WARN  1
#define ALARM_ERROR 2
#define ALARM_CRIT  3

#define MS2TICKS(m) ((m) / (portTICK_RATE_MS))

static void msp_send_alarms(__attribute__((unused)) struct msp_bridge *m)
{
#if 0
    union {
        uint8_t buf[0];
        struct {
            uint8_t state;
            char    msg[MAX_ALARM_LEN];
        } __attribute__((packed)) alarm;
    } data;

    SystemAlarmsData alarm;
    SystemAlarmsGet(&alarm);

    // Special case early boot times -- just report boot reason

#if 0
    if (xTaskGetTickCount() < MS2TICKS(10 * 1000)) {
        data.alarm.state = ALARM_CRIT;
        const char *boot_reason = AlarmBootReason(alarm.RebootCause);
        strncpy((char *)data.alarm.msg, boot_reason, MAX_ALARM_LEN);
        data.alarm.msg[MAX_ALARM_LEN - 1] = '\0';
        msp_send(m, MSP_ALARMS, data.buf, strlen((char *)data.alarm.msg) + 1);
        return;
    }
#endif

    uint8_t state;
    data.alarm.state = ALARM_OK;
    int32_t len = AlarmString(&alarm, data.alarm.msg,
                              sizeof(data.alarm.msg), SYSTEMALARMS_ALARM_CRITICAL, &state); // Include only CRITICAL and ERROR
    switch (state) {
    case SYSTEMALARMS_ALARM_WARNING:
        data.alarm.state = ALARM_WARN;
        break;
    case SYSTEMALARMS_ALARM_ERROR:
        break;
    case SYSTEMALARMS_ALARM_CRITICAL:
        data.alarm.state = ALARM_CRIT;;
    }

    msp_send(m, MSP_ALARMS, data.buf, len + 1);
#endif /* if 0 */
}

static msp_state msp_state_checksum(struct msp_bridge *m, uint8_t b)
{
    if ((m->checksum ^ b) != 0) {
        return MSP_IDLE;
    }

    // Respond to interesting things.
    switch (m->cmd_id) {
    case MSP_IDENT:
        msp_send_ident(m);
        break;
    case MSP_RAW_GPS:
        msp_send_raw_gps(m);
        break;
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
    case MSP_COMP_GPS:
        msp_send_comp_gps(m);
        break;
    case MSP_ALTITUDE:
        msp_send_altitude(m);
        break;
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */
    case MSP_ATTITUDE:
        msp_send_attitude(m);
        break;
    case MSP_STATUS:
        msp_send_status(m);
        break;
    case MSP_ANALOG:
        msp_send_analog(m);
        break;
    case MSP_RC:
        msp_send_channels(m);
        break;
    case MSP_BOXIDS:
        msp_send_boxids(m);
        break;
    case MSP_ALARMS:
        msp_send_alarms(m);
        break;
    case MSP_PID:
        msp_send_pid(m);
        break;
    case MSP_SET_PID:
        msp_set_pid(m);
        break;
    }

    return MSP_IDLE;
}

static msp_state msp_state_discard(struct msp_bridge *m, __attribute__((unused)) uint8_t b)
{
    return m->cmd_i++ == m->cmd_size ? MSP_IDLE : MSP_DISCARD;
}

/**
 * Process incoming bytes from an MSP query thing.
 * @param[in] b received byte
 * @return true if we should continue processing bytes
 */
static bool msp_receive_byte(struct msp_bridge *m, uint8_t b)
{
    switch (m->state) {
    case MSP_IDLE:
        switch (b) {
        case 0xe0: // uavtalk matching first part of 0x3c @ 57600 baud
            m->state = MSP_MAYBE_UAVTALK_SLOW2;
            break;
        case '<': // uavtalk matching with 0x3c 0x2x 0xxx 0x0x
            m->state = MSP_MAYBE_UAVTALK2;
            break;
        case '$':
            m->state = MSP_HEADER_START;
            break;
        default:
            m->state = MSP_IDLE;
        }
        break;
    case MSP_HEADER_START:
        m->state = b == 'M' ? MSP_HEADER_M : MSP_IDLE;
        break;
    case MSP_HEADER_M:
        m->state = b == '<' ? MSP_HEADER_SIZE : MSP_IDLE;
        break;
    case MSP_HEADER_SIZE:
        m->state = msp_state_size(m, b);
        break;
    case MSP_HEADER_CMD:
        m->state = msp_state_cmd(m, b);
        break;
    case MSP_FILLBUF:
        m->state = msp_state_fill_buf(m, b);
        break;
    case MSP_CHECKSUM:
        m->state = msp_state_checksum(m, b);
        break;
    case MSP_DISCARD:
        m->state = msp_state_discard(m, b);
        break;
    case MSP_MAYBE_UAVTALK2:
        // e.g. 3c 20 1d 00
        // second possible uavtalk byte
        m->state = (b & 0xf0) == 0x20 ? MSP_MAYBE_UAVTALK3 : MSP_IDLE;
        break;
    case MSP_MAYBE_UAVTALK3:
        // third possible uavtalk byte can be anything
        m->state = MSP_MAYBE_UAVTALK4;
        break;
    case MSP_MAYBE_UAVTALK4:
        m->state = MSP_IDLE;
        // If this looks like the fourth possible uavtalk byte, we're done
        if ((b & 0xf0) == 0) {
            PIOS_COM_TELEM_RF = m->com;
            return false;
        }
        break;
    case MSP_MAYBE_UAVTALK_SLOW2:
        m->state = b == 0x18 ? MSP_MAYBE_UAVTALK_SLOW3 : MSP_IDLE;
        break;
    case MSP_MAYBE_UAVTALK_SLOW3:
        m->state = b == 0x98 ? MSP_MAYBE_UAVTALK_SLOW4 : MSP_IDLE;
        break;
    case MSP_MAYBE_UAVTALK_SLOW4:
        m->state = b == 0x7e ? MSP_MAYBE_UAVTALK_SLOW5 : MSP_IDLE;
        break;
    case MSP_MAYBE_UAVTALK_SLOW5:
        m->state = b == 0x00 ? MSP_MAYBE_UAVTALK_SLOW6 : MSP_IDLE;
        break;
    case MSP_MAYBE_UAVTALK_SLOW6:
        m->state = MSP_IDLE;
        // If this looks like the sixth possible 57600 baud uavtalk byte, we're done
        if (b == 0x60) {
            PIOS_COM_ChangeBaud(m->com, 57600);
            PIOS_COM_TELEM_RF = m->com;
            return false;
        }
        break;
    }

    return true;
}

/**
 * Module start routine automatically called after initialization routine
 * @return 0 when was successful
 */
static int32_t uavoMSPBridgeStart(void)
{
    if (!module_enabled) {
        // give port to telemetry if it doesn't have one
        // stops board getting stuck in condition where it can't be connected to gcs
        if (!PIOS_COM_TELEM_RF) {
            PIOS_COM_TELEM_RF = pios_com_msp_id;
        }

        return -1;
    }

    xTaskHandle taskHandle;

    xTaskCreate(uavoMSPBridgeTask, "uavoMSPBridge", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
    PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_UAVOMSPBRIDGE, taskHandle);

    return 0;
}

static uint32_t hwsettings_mspspeed_enum_to_baud(uint8_t baud)
{
    switch (baud) {
    case HWSETTINGS_MSPSPEED_2400:
        return 2400;

    case HWSETTINGS_MSPSPEED_4800:
        return 4800;

    case HWSETTINGS_MSPSPEED_9600:
        return 9600;

    case HWSETTINGS_MSPSPEED_19200:
        return 19200;

    case HWSETTINGS_MSPSPEED_38400:
        return 38400;

    case HWSETTINGS_MSPSPEED_57600:
        return 57600;

    default:
    case HWSETTINGS_MSPSPEED_115200:
        return 115200;
    }
}


/**
 * Module initialization routine
 * @return 0 when initialization was successful
 */
static int32_t uavoMSPBridgeInitialize(void)
{
    if (pios_com_msp_id) {
        msp = pios_malloc(sizeof(*msp));
        if (msp != NULL) {
            memset(msp, 0x00, sizeof(*msp));

            msp->com = pios_com_msp_id;

            // now figure out enabled features: registered sensors, ADC routing, GPS

#ifdef PIOS_EXCLUDE_ADVANCED_FEATURES
            msp->sensors |= MSP_SENSOR_ACC;
#else

            if (PIOS_SENSORS_GetInstanceByType(0, PIOS_SENSORS_TYPE_3AXIS_ACCEL)) {
                msp->sensors |= MSP_SENSOR_ACC;
            }
            if (PIOS_SENSORS_GetInstanceByType(0, PIOS_SENSORS_TYPE_1AXIS_BARO)) {
                msp->sensors |= MSP_SENSOR_BARO;
            }
            if (PIOS_SENSORS_GetInstanceByType(0, PIOS_SENSORS_TYPE_3AXIS_MAG | PIOS_SENSORS_TYPE_3AXIS_AUXMAG)) {
                msp->sensors |= MSP_SENSOR_MAG;
            }
#ifdef PIOS_SENSORS_TYPE_1AXIS_SONAR
            if (PIOS_SENSORS_GetInstanceByType(0, PIOS_SENSORS_TYPE_1AXIS_SONAR)) {
                msp->sensors |= MSP_SENSOR_SONAR;
            }
#endif /* PIOS_SENSORS_TYPE_1AXIS_SONAR */
#endif /* PIOS_EXCLUDE_ADVANCED_FEATURES */

            // MAP_SENSOR_GPS is hot-pluggable type

            HwSettingsADCRoutingDataArray adcRoutingDataArray;
            HwSettingsADCRoutingArrayGet(adcRoutingDataArray.array);

            for (unsigned i = 0; i < sizeof(adcRoutingDataArray.array) / sizeof(adcRoutingDataArray.array[0]); ++i) {
                switch (adcRoutingDataArray.array[i]) {
                case HWSETTINGS_ADCROUTING_BATTERYVOLTAGE:
                    msp->analog |= MSP_ANALOG_VOLTAGE;
                    break;
                case HWSETTINGS_ADCROUTING_BATTERYCURRENT:
                    msp->analog |= MSP_ANALOG_CURRENT;
                    break;
                default:;
                }
            }


            HwSettingsMSPSpeedOptions mspSpeed;
            HwSettingsMSPSpeedGet(&mspSpeed);

            PIOS_COM_ChangeBaud(pios_com_msp_id, hwsettings_mspspeed_enum_to_baud(mspSpeed));

            module_enabled = true;
            return 0;
        }
    }

    return -1;
}
MODULE_INITCALL(uavoMSPBridgeInitialize, uavoMSPBridgeStart);

/**
 * Main task routine
 * @param[in] parameters parameter given by PIOS_Thread_Create()
 */
static void uavoMSPBridgeTask(__attribute__((unused)) void *parameters)
{
    while (1) {
        uint8_t b = 0;
        uint16_t count = PIOS_COM_ReceiveBuffer(msp->com, &b, 1, ~0);
        if (count) {
            if (!msp_receive_byte(msp, b)) {
                // Returning is considered risky here as
                // that's unusual and this is an edge case.
                while (1) {
                    PIOS_DELAY_WaitmS(60 * 1000);
                }
            }
        }
    }
}

#endif // PIOS_INCLUDE_MSP_BRIDGE
/**
 * @}
 * @}
 */
