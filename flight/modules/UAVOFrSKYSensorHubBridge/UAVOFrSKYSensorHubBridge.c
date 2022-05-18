/**
 ******************************************************************************
 * @addtogroup LibrePilotModules LibrePilot Modules
 * @{
 * @addtogroup UAVOFrSKYSensorHubBridge UAVO to FrSKY Bridge Module
 * @{
 *
 * @file       UAVOFrSKYSensorHubBridge.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             dRonin, http://dRonin.org/, Copyright (C) 2016
 *             Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief      Bridges selected UAVObjects to FrSKY Sensor Hub
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

// ****************
#include "pios.h"
#include "pios_board_io.h"
#include "openpilot.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "homelocation.h"
#include "flightstatus.h"
#include "pios_thread.h"
#include "barosensor.h"
#include "accelsensor.h"
#include "gpspositionsensor.h"
#include "taskinfo.h"

// ****************
// Private functions

static void uavoFrSKYSensorHubBridgeTask(void *parameters);

static uint16_t frsky_pack_altitude(
    float altitude,
    uint8_t *serial_buf);

static uint16_t frsky_pack_temperature_01(
    float temperature_01,
    uint8_t *serial_buf);

static uint16_t frsky_pack_temperature_02(
    float temperature_02,
    uint8_t *serial_buf);

static uint16_t frsky_pack_accel(
    float accels_x,
    float accels_y,
    float accels_z,
    uint8_t *serial_buf);

static uint16_t frsky_pack_cellvoltage(
    uint8_t cell,
    float cell_voltage,
    uint8_t *serial_buf);

static uint16_t frsky_pack_fas(
    float voltage,
    float current,
    uint8_t *serial_buf);

static uint16_t frsky_pack_rpm(
    uint16_t rpm,
    uint8_t *serial_buf);

static uint16_t frsky_pack_gps(
    float course,
    int32_t latitude,
    int32_t longitude,
    float altitude,
    float speed,
    uint8_t *serial_buf);

static uint16_t frsky_pack_fuel(
    float fuel_level,
    uint8_t *serial_buf);

static uint16_t frsky_pack_stop(
    uint8_t *serial_buf);

static int16_t frsky_acceleration_unit(float accel);

static void frsky_serialize_value(uint8_t valueid,
                                  uint8_t *value,
                                  uint8_t *serial_buf,
                                  uint8_t *index);

static void frsky_write_userdata_byte(uint8_t byte,
                                      uint8_t *serial_buf,
                                      uint8_t *index);

static bool frame_trigger(uint8_t frame_num);
// ****************
// Private constants


#if defined(PIOS_FRSKY_SENSOR_HUB_STACK_SIZE)
#define STACK_SIZE_BYTES        PIOS_FRSKY_SENSOR_HUB_STACK_SIZE
#else
#define STACK_SIZE_BYTES        672
#endif

#define TASK_PRIORITY           (tskIDLE_PRIORITY)
#define TASK_RATE_HZ            10

#define FRSKY_MAX_PACKET_LEN    106
#define FRSKY_BAUD_RATE         9600

#define FRSKY_FRAME_DATA_HEADER 0x5E
#define FRSKY_FRAME_STOP        0x5E

enum FRSKY_VALUE_ID {
    FRSKY_GPS_ALTITUDE_INTEGER = 0x01,
    FRSKY_GPS_ALTITUDE_DECIMAL = FRSKY_GPS_ALTITUDE_INTEGER + 8,
    FRSKY_TEMPERATURE_1         = 0x02,
    FRSKY_RPM = 0x03,
    FRSKY_FUEL_LEVEL            = 0x04,
    FRSKY_TEMPERATURE_2         = 0x05,
    FRSKY_VOLTAGE = 0x06,
    FRSKY_ALTITUDE_INTEGER      = 0x10,
    FRSKY_ALTITUDE_DECIMAL      = 0x21,
    FRSKY_GPS_SPEED_INTEGER     = 0x11,
    FRSKY_GPS_SPEED_DECIMAL     = 0x11 + 8,
    FRSKY_GPS_LONGITUDE_INTEGER = 0x12,
    FRSKY_GPS_LONGITUDE_DECIMAL = FRSKY_GPS_LONGITUDE_INTEGER + 8,
    FRSKY_GPS_E_W = 0x1A + 8,
    FRSKY_GPS_LATITUDE_INTEGER  = 0x13,
    FRSKY_GPS_LATITUDE_DECIMAL  = FRSKY_GPS_LATITUDE_INTEGER + 8,
    FRSKY_GPS_N_S = 0x1B + 8,
    FRSKY_GPS_COURSE_INTEGER    = 0x14,
    FRSKY_GPS_COURSE_DECIMAL    = FRSKY_GPS_COURSE_INTEGER + 8,
    FRSKY_DATE_MONTH            = 0x15,
    FRSKY_DATE_YEAR = 0x16,
    FRSKY_HOUR_MINUTE           = 0x17,
    FRSKY_SECOND = 0x18,
    FRSKY_ACCELERATION_X        = 0x24,
    FRSKY_ACCELERATION_Y        = 0x25,
    FRSKY_ACCELERATION_Z        = 0x26,
    FRSKY_VOLTAGE_AMPERE_SENSOR_INTEGER = 0x3A,
    FRSKY_VOLTAGE_AMPERE_SENSOR_DECIMAL = 0x3B,
    FRSKY_CURRENT = 0x28,
};

enum FRSKY_FRAME {
    FRSKY_FRAME_VARIO,
    FRSKY_FRAME_BATTERY,
    FRSKY_FRAME_GPS,
};

static const uint8_t frsky_rates[] = {
    [FRSKY_FRAME_VARIO]   = 5, // 5Hz
    [FRSKY_FRAME_BATTERY] = 5, // 5Hz
    [FRSKY_FRAME_GPS]     = 2,  // 1Hz
};

#define MAXSTREAMS         sizeof(frsky_rates)
#define KNOTS2M_PER_SECOND 0.514444F
#define GRAVITY            9.8F

// ****************
// Private variables

static struct {
    uint32_t frsky_port;
    uint8_t  frame_ticks[MAXSTREAMS];
    uint8_t  serial_buf[FRSKY_MAX_PACKET_LEN];
} *shub_global;

/**
 * Start the module
 * \return -1 if start failed
 * \return 0 on success
 */
static int32_t uavoFrSKYSensorHubBridgeStart(void)
{
    if (shub_global) {
        xTaskHandle taskHandle;
        xTaskCreate(uavoFrSKYSensorHubBridgeTask, "uavoFrSKYSensorHubBridge", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_UAVOFRSKYSENSORHUBBRIDGE, taskHandle);
        return 0;
    }
    return -1;
}

/**
 * Initialize the module
 * \return -1 if initialization failed
 * \return 0 on success
 */
static int32_t uavoFrSKYSensorHubBridgeInitialize(void)
{
    uintptr_t frsky_port = PIOS_COM_FRSKY_SENSORHUB;

    if (frsky_port) {
        shub_global = pios_malloc(sizeof(*shub_global));

        if (shub_global == 0) {
            return -1;
        }

        PIOS_COM_ChangeBaud(frsky_port, FRSKY_BAUD_RATE);

        shub_global->frsky_port = frsky_port;

        for (uint32_t x = 0; x < MAXSTREAMS; ++x) {
            shub_global->frame_ticks[x] =
                (TASK_RATE_HZ / frsky_rates[x]);
        }

        return 0;
    }


    return -1;
}
MODULE_INITCALL(uavoFrSKYSensorHubBridgeInitialize, uavoFrSKYSensorHubBridgeStart);

/**
 * Main task. It does not return.
 */
static void uavoFrSKYSensorHubBridgeTask(__attribute__((unused)) void *parameters)
{
    uint32_t lastSysTime = PIOS_Thread_Systime();

    // Main task loop
    while (1) {
        PIOS_Thread_Sleep_Until(&lastSysTime, 1000 / TASK_RATE_HZ);

        if (frame_trigger(FRSKY_FRAME_VARIO)) {
            uint16_t msg_length = 0;

            // Get the accelerometer values
            float accX = 0, accY = 0, accZ = 0;
            if (AccelSensorHandle() != NULL) {
                AccelSensorxGet(&accX);
                AccelSensoryGet(&accY);
                AccelSensorzGet(&accZ);
            }
            msg_length += frsky_pack_accel(
                accX,
                accY,
                accZ,
                shub_global->serial_buf + msg_length);

            // Get the altritude from the barometer.
            float altitude;
            if (BaroSensorHandle() != NULL) {
                BaroSensorAltitudeGet(&altitude);
            }
            msg_length += frsky_pack_altitude(
                altitude,
                shub_global->serial_buf + msg_length);
            msg_length += frsky_pack_stop(shub_global->serial_buf + msg_length);

            PIOS_COM_SendBuffer(shub_global->frsky_port,
                                shub_global->serial_buf, msg_length);
        }

        if (frame_trigger(FRSKY_FRAME_BATTERY)) {
            uint16_t msg_length = 0;

            FlightBatteryStateData batState;
            if (FlightBatteryStateHandle() != NULL) {
                FlightBatteryStateGet(&batState);
            }

            float voltage = voltage = batState.Voltage;

            float current = current = batState.Current;

            // As long as there is no voltage for each cell
            // all cells will have the same voltage.
            // Receiver will know number of cells.
            FlightBatterySettingsData batSettings;
            if (FlightBatterySettingsHandle() != NULL) {
                FlightBatterySettingsGet(&batSettings);
            }
            if (batState.NbCellsAutodetected == FLIGHTBATTERYSTATE_NBCELLSAUTODETECTED_TRUE) {
                batSettings.NbCells = batState.NbCells;
            }

            if (batSettings.NbCells > 0) {
                float cell_v = voltage / batSettings.NbCells;
                for (uint8_t i = 0; i < batSettings.NbCells; ++i) {
                    msg_length += frsky_pack_cellvoltage(
                        i,
                        cell_v,
                        shub_global->serial_buf + msg_length);
                }
            }

            msg_length += frsky_pack_fas(
                voltage,
                current,
                shub_global->serial_buf + msg_length);

            if (batSettings.Capacity > 0) {
                float fuel = 1.0f - batState.ConsumedEnergy / batSettings.Capacity;
                msg_length += frsky_pack_fuel(
                    fuel,
                    shub_global->serial_buf + msg_length);
            }

            msg_length += frsky_pack_stop(shub_global->serial_buf + msg_length);

            PIOS_COM_SendBuffer(shub_global->frsky_port,
                                shub_global->serial_buf, msg_length);
        }

        if (frame_trigger(FRSKY_FRAME_GPS)) {
            uint16_t msg_length = 0;

            /**
             * Encodes ARM status and flight mode number as RPM value
             * Since there is no RPM information in any UAVO available,
             * we will intentionally misuse this item to encode other useful information.
             * It will encode flight status as three-digit number as follow:
             * most left digit encodes arm status (200=armed, 100=disarmed)
             * two most right digits encode flight mode number (see FlightStatus UAVO FlightMode enum)
             * To work properly on Taranis, you have to set Blades to "60" in telemetry setting
             */
            FlightStatusData flight_status;
            FlightStatusGet(&flight_status);

            uint16_t status = 0;
            float hdop, vdop;

            status      = (flight_status.Armed == FLIGHTSTATUS_ARMED_ARMED) ? 200 : 100;
            status     += flight_status.FlightMode;

            msg_length += frsky_pack_rpm(status, shub_global->serial_buf + msg_length);

            uint8_t hl_set = HOMELOCATION_SET_FALSE;

            GPSPositionSensorData gpsPosData;
            if (GPSPositionSensorHandle() != NULL) {
                GPSPositionSensorGet(&gpsPosData);
            }

            if (HomeLocationHandle() != NULL) {
                HomeLocationSetGet(&hl_set);
            }

            /**
             * Encode GPS status and visible satellites as T1 value
             * We will intentionally misuse this item to encode other useful information.
             * Right-most two digits encode visible satellite count, left-most digit has following meaning:
             * 1 - no GPS connected
             * 2 - no fix
             * 3 - 2D fix
             * 4 - 3D fix
             * 5 - 3D fix and HomeLocation is SET - should be safe for navigation
             */
            switch (gpsPosData.Status) {
            case GPSPOSITIONSENSOR_STATUS_NOGPS:
                status = 100;
                break;
            case GPSPOSITIONSENSOR_STATUS_NOFIX:
                status = 200;
                break;
            case GPSPOSITIONSENSOR_STATUS_FIX2D:
                status = 300;
                break;
            case GPSPOSITIONSENSOR_STATUS_FIX3D:
            case GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS:
                if (hl_set == HOMELOCATION_SET_TRUE) {
                    status = 500;
                } else {
                    status = 400;
                }
                break;
            }

            if (gpsPosData.Satellites > 0) {
                status += gpsPosData.Satellites;
            }

            msg_length += frsky_pack_temperature_01((float)status, shub_global->serial_buf + msg_length);

            /**
             * Encode GPS HDOP and VDOP as T2 value
             * We will intentionally misuse this item to encode other useful information.
             * VDOP in the upper 16 bits, max 256 (2.56 * 100)
             * HDOP in the lower 16 bits, max 256 (2.56 * 100)
             */
            hdop = gpsPosData.HDOP * 100.0f;
            hdop = 0;

            if (hdop > 255.0f) {
                hdop = 255.0f;
            }

            vdop = gpsPosData.VDOP * 100.0f;
            vdop = 0;

            if (vdop > 255.0f) {
                vdop = 255.0f;
            }

            msg_length += frsky_pack_temperature_02((vdop * 256 + hdop), shub_global->serial_buf + msg_length);

            if (gpsPosData.Status == GPSPOSITIONSENSOR_STATUS_FIX2D ||
                gpsPosData.Status == GPSPOSITIONSENSOR_STATUS_FIX3D ||
                gpsPosData.Status == GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS) {
                msg_length += frsky_pack_gps(
                    gpsPosData.Heading,
                    gpsPosData.Latitude,
                    gpsPosData.Longitude,
                    gpsPosData.Altitude,
                    gpsPosData.Groundspeed,
                    shub_global->serial_buf + msg_length);
            } else {
                msg_length += frsky_pack_gps(0, 0, 0, 0, 0, shub_global->serial_buf + msg_length);
            }

            msg_length += frsky_pack_stop(shub_global->serial_buf + msg_length);

            PIOS_COM_SendBuffer(shub_global->frsky_port,
                                shub_global->serial_buf, msg_length);
        }
    }
}

static bool frame_trigger(uint8_t frame_num)
{
    uint8_t rate = frsky_rates[frame_num];

    if (rate == 0) {
        return false;
    }

    if (shub_global->frame_ticks[frame_num] == 0) {
        // we're triggering now, setup the next trigger point
        if (rate > TASK_RATE_HZ) {
            rate = TASK_RATE_HZ;
        }
        shub_global->frame_ticks[frame_num] = (TASK_RATE_HZ / rate);
        return true;
    }

    // count down at 50Hz
    shub_global->frame_ticks[frame_num]--;
    return false;
}

/**
 * Writes altitude to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_altitude(float altitude, uint8_t *serial_buf)
{
    uint8_t index = 0;

    float altitudeInteger = 0.0f;

    uint16_t decimalValue = lroundf(modff(altitude, &altitudeInteger) * 100);
    int16_t integerValue  = lroundf(altitude);

    frsky_serialize_value(FRSKY_ALTITUDE_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
    frsky_serialize_value(FRSKY_ALTITUDE_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);

    return index;
}

/**
 * Writes baro temperature to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_temperature_01(
    float temperature_01,
    uint8_t *serial_buf)
{
    uint8_t index = 0;

    int16_t temperature = lroundf(temperature_01);

    frsky_serialize_value(FRSKY_TEMPERATURE_1, (uint8_t *)&temperature, serial_buf, &index);

    return index;
}

/**
 * Writes accel temperature to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_temperature_02(
    float temperature_02,
    uint8_t *serial_buf)
{
    uint8_t index = 0;

    int16_t temperature = lroundf(temperature_02);

    frsky_serialize_value(FRSKY_TEMPERATURE_2, (uint8_t *)&temperature, serial_buf, &index);

    return index;
}

/**
 * Writes acceleration values to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_accel(
    float accels_x,
    float accels_y,
    float accels_z,
    uint8_t *serial_buf)
{
    uint8_t index = 0;

    int16_t accel = frsky_acceleration_unit(accels_x);

    frsky_serialize_value(FRSKY_ACCELERATION_X, (uint8_t *)&accel, serial_buf, &index);

    accel = frsky_acceleration_unit(accels_y);
    frsky_serialize_value(FRSKY_ACCELERATION_Y, (uint8_t *)&accel, serial_buf, &index);

    accel = frsky_acceleration_unit(accels_z);
    frsky_serialize_value(FRSKY_ACCELERATION_Z, (uint8_t *)&accel, serial_buf, &index);

    return index;
}

static uint16_t frsky_pack_cellvoltage(
    uint8_t cell,
    float cell_voltage,
    uint8_t *serial_buf)
{
    // its not possible to address more than 15 cells
    if (cell > 15) {
        return 0;
    }

    uint8_t index = 0;
    uint16_t v    = lroundf((cell_voltage / 4.2f) * 2100);
    if (v > 2100) {
        v = 2100;
    }

    uint16_t voltage = ((v & 0x00ff) << 8) | (cell << 4) | ((v & 0x0f00) >> 8);
    frsky_serialize_value(FRSKY_VOLTAGE, (uint8_t *)&voltage, serial_buf, &index);

    return index;
}
/**
 * Writes battery values to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_fas(
    float voltage,
    float current,
    uint8_t *serial_buf)
{
    uint8_t index = 0;

    voltage = (voltage * 110.0f) / 21.0f;

    uint16_t integerValue = lroundf(voltage) / 100;
    uint16_t decimalValue = lroundf(voltage - integerValue);

    frsky_serialize_value(FRSKY_VOLTAGE_AMPERE_SENSOR_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
    frsky_serialize_value(FRSKY_VOLTAGE_AMPERE_SENSOR_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);

    integerValue = lroundf(current * 10);
    frsky_serialize_value(FRSKY_CURRENT, (uint8_t *)&integerValue, serial_buf, &index);

    return index;
}

/**
 * Writes rpm value to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_rpm(uint16_t rpm, uint8_t *serial_buf)
{
    uint8_t index = 0;

    frsky_serialize_value(FRSKY_RPM, (uint8_t *)&rpm, serial_buf, &index);

    return index;
}

/**
 * Writes fuel value [0.0 ... 1.0] to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_fuel(
    float fuel_level,
    uint8_t *serial_buf)
{
    uint8_t index  = 0;

    uint16_t level = lroundf(abs(fuel_level * 100));

    // Use fixed levels here because documentation says so.
    if (level > 94) {
        level = 100;
    } else if (level > 69) {
        level = 75;
    } else if (level > 44) {
        level = 50;
    } else if (level > 19) {
        level = 25;
    } else {
        level = 0;
    }

    frsky_serialize_value(FRSKY_FUEL_LEVEL, (uint8_t *)&level, serial_buf, &index);

    return index;
}

/**
 * Writes gps values to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_gps(
    float course,
    int32_t latitude,
    int32_t longitude,
    float altitude,
    float speed,
    uint8_t *serial_buf)
{
    uint8_t index = 0;

    {
        float courseInteger   = 0.0f;
        uint16_t decimalValue = lroundf(modff(course, &courseInteger) * 100);
        uint16_t integerValue = lroundf(courseInteger);

        frsky_serialize_value(FRSKY_GPS_COURSE_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
        frsky_serialize_value(FRSKY_GPS_COURSE_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);
    }

    // latitude
    {
        uint16_t integerValue = (abs(latitude) / 100000);
        uint16_t decimalValue = (abs(latitude) / 10) % 10000;

        frsky_serialize_value(FRSKY_GPS_LATITUDE_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
        frsky_serialize_value(FRSKY_GPS_LATITUDE_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);

        uint16_t hemisphere = 'N';
        if (latitude < 0) {
            hemisphere = 'S';
        }
        frsky_serialize_value(FRSKY_GPS_N_S, (uint8_t *)&hemisphere, serial_buf, &index);
    }

    // longitude
    {
        uint16_t integerValue = (abs(longitude) / 100000);
        uint16_t decimalValue = (abs(longitude) / 10) % 10000;

        uint16_t hemisphere   = 'E';
        if (longitude < 0) {
            hemisphere = 'W';
        }

        frsky_serialize_value(FRSKY_GPS_LONGITUDE_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
        frsky_serialize_value(FRSKY_GPS_LONGITUDE_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);

        frsky_serialize_value(FRSKY_GPS_E_W, (uint8_t *)&hemisphere, serial_buf, &index);
    }

    // speed
    {
        float knots = speed / KNOTS2M_PER_SECOND;

        float knotsInteger    = 0.0f;
        uint16_t decimalValue = lroundf(modff(knots, &knotsInteger) * 100);
        int16_t integerValue  = lroundf(knotsInteger);

        frsky_serialize_value(FRSKY_GPS_SPEED_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
        frsky_serialize_value(FRSKY_GPS_SPEED_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);
    }

    // altitude
    {
        float altitudeInteger = 0.0F;
        uint16_t decimalValue = lroundf(modff(altitude, &altitudeInteger) * 100);
        int16_t integerValue  = lroundf(altitudeInteger);

        frsky_serialize_value(FRSKY_GPS_ALTITUDE_INTEGER, (uint8_t *)&integerValue, serial_buf, &index);
        frsky_serialize_value(FRSKY_GPS_ALTITUDE_DECIMAL, (uint8_t *)&decimalValue, serial_buf, &index);
    }

    return index;
}

/**
 * Writes stop byte to buffer
 * \return number of bytes written to the buffer
 */
static uint16_t frsky_pack_stop(uint8_t *serial_buf)
{
    serial_buf[0] = FRSKY_FRAME_STOP;

    return 1;
}

/**
 * Convert acceleration value to frsky acceleration unit
 * \return Acceleration value
 */
static int16_t frsky_acceleration_unit(float accel)
{
    accel  = accel / GRAVITY; // convert to gravity
    accel *= 1000;
    return lroundf(accel);
}

/**
 * Write value to buffer
 */
static void frsky_serialize_value(uint8_t valueid, uint8_t *value, uint8_t *serial_buf, uint8_t *index)
{
    serial_buf[(*index)++] = FRSKY_FRAME_DATA_HEADER;
    serial_buf[(*index)++] = valueid;

    frsky_write_userdata_byte(value[0], serial_buf, index);
    frsky_write_userdata_byte(value[1], serial_buf, index);
}

/**
 * Write a byte to the buffer and do byte stuffing if needed.
 */
static void frsky_write_userdata_byte(uint8_t byte, uint8_t *serial_buf, uint8_t *index)
{
    // ** byte stuffing
    if ((byte == 0x5E) || (byte == 0x5D)) {
        serial_buf[(*index)++] = 0x5D;
        serial_buf[(*index)++] = ~(byte ^ 0x60);
    } else {
        serial_buf[(*index)++] = byte;
    }
}

/**
 * @}
 * @}
 */
