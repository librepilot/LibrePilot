/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup UAVOHoTTBridge UAVO to HoTT Bridge Telemetry Module
 * @{
 *
 * @file       uavohottbridge.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      sends telemery data on HoTT request
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
#include "hwsettings.h"
#include "taskinfo.h"
#include "callbackinfo.h"
#include "hottbridgesettings.h"
#include "attitudestate.h"
#include "barosensor.h"
#include "flightbatterystate.h"
#include "flightstatus.h"
#include "gyrosensor.h"
#include "gpspositionsensor.h"
#include "gpstime.h"
#include "airspeedstate.h"
#include "homelocation.h"
#include "positionstate.h"
#include "systemalarms.h"
#include "velocitystate.h"
#include "hottbridgestatus.h"
#include "hottbridgesettings.h"
#include "objectpersistence.h"
#include "pios_sensors.h"
#include "uavohottbridge.h"

#include "pios_board_io.h"

#if defined(PIOS_INCLUDE_HOTT_BRIDGE)

#if defined(PIOS_HoTT_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_HoTT_STACK_SIZE
#else
#define STACK_SIZE_BYTES 2048
#endif
#define TASK_PRIORITY    CALLBACK_TASK_AUXILIARY

static bool module_enabled = false;

// Private variables
static bool module_enabled;
static struct telemetrydata *telestate;
static HoTTBridgeStatusData status;

// Private functions
static void uavoHoTTBridgeTask(void *parameters);
static uint16_t build_VARIO_message(struct hott_vario_message *msg);
static uint16_t build_GPS_message(struct hott_gps_message *msg);
static uint16_t build_GAM_message(struct hott_gam_message *msg);
static uint16_t build_EAM_message(struct hott_eam_message *msg);
static uint16_t build_ESC_message(struct hott_esc_message *msg);
static uint16_t build_TEXT_message(struct hott_text_message *msg);
static uint8_t calc_checksum(uint8_t *data, uint16_t size);
static uint8_t generate_warning();
static void update_telemetrydata();
static void convert_long2gps(int32_t value, uint8_t *dir, uword_t *min, uword_t *sec);
static uint8_t scale_float2uint8(float value, float scale, float offset);
static int8_t scale_float2int8(float value, float scale, float offset);
static uword_t scale_float2uword(float value, float scale, float offset);

/**
 * Module start routine automatically called after initialization routine
 * @return 0 when was successful
 */
static int32_t uavoHoTTBridgeStart(void)
{
    status.TxPackets = 0;
    status.RxPackets = 0;
    status.TxFail    = 0;
    status.RxFail    = 0;


    // Start task
    if (module_enabled) {
        xTaskHandle taskHandle;

        xTaskCreate(uavoHoTTBridgeTask, "uavoHoTTBridge", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_UAVOHOTTBRIDGE, taskHandle);
    }

    return 0;
}

/**
 * Module initialization routine
 * @return 0 when initialization was successful
 */
static int32_t uavoHoTTBridgeInitialize(void)
{
    if (PIOS_COM_HOTT) {
        module_enabled = true;
        // HoTT telemetry baudrate is fixed to 19200

        PIOS_COM_ChangeBaud(PIOS_COM_HOTT, 19200);
        bool param = true;
        PIOS_COM_Ioctl(PIOS_COM_HOTT, PIOS_IOCTL_USART_SET_HALFDUPLEX, &param);
        HoTTBridgeStatusInitialize();

        // allocate memory for telemetry data
        telestate = (struct telemetrydata *)pios_malloc(sizeof(*telestate));

        if (telestate == NULL) {
            // there is not enough free memory. the module could not run.
            module_enabled = false;
            return -1;
        }
    } else {
        module_enabled = false;
    }
    return 0;
}
MODULE_INITCALL(uavoHoTTBridgeInitialize, uavoHoTTBridgeStart);

/**
 * Main task. It does not return.
 */
static void uavoHoTTBridgeTask(__attribute__((unused)) void *parameters)
{
    uint8_t rx_buffer[2];
    uint8_t tx_buffer[HOTT_MAX_MESSAGE_LENGTH];
    uint16_t message_size;

    // clear all state values
    memset(telestate, 0, sizeof(*telestate));

    // initialize timer variables
    portTickType lastSysTime = xTaskGetTickCount();
    // idle delay between telemetry request and answer
    uint32_t idledelay = IDLE_TIME;
    // data delay between transmitted bytes
    uint32_t datadelay = DATA_TIME;

    // work on hott telemetry. endless loop.
    while (1) {
        // clear message size on every loop before processing
        message_size = 0;

        // shift receiver buffer. make room for one byte.
        rx_buffer[1] = rx_buffer[0];

        // wait for a byte of telemetry request in data delay interval
        while (PIOS_COM_ReceiveBuffer(PIOS_COM_HOTT, rx_buffer, 1, 0) == 0) {
            vTaskDelayUntil(&lastSysTime, datadelay / portTICK_RATE_MS);
        }
        // set start trigger point
        lastSysTime = xTaskGetTickCount();

        // examine received data stream
        if (rx_buffer[1] == HOTT_BINARY_ID) {
            // first received byte looks like a binary request. check second received byte for a sensor id.
            switch (rx_buffer[0]) {
            case HOTT_VARIO_ID:
                message_size = build_VARIO_message((struct hott_vario_message *)tx_buffer);
                break;
            case HOTT_GPS_ID:
                message_size = build_GPS_message((struct hott_gps_message *)tx_buffer);
                break;
            case HOTT_GAM_ID:
                message_size = build_GAM_message((struct hott_gam_message *)tx_buffer);
                break;
            case HOTT_EAM_ID:
                message_size = build_EAM_message((struct hott_eam_message *)tx_buffer);
                break;
            case HOTT_ESC_ID:
                message_size = build_ESC_message((struct hott_esc_message *)tx_buffer);
                break;
            default:
                message_size = 0;
            }
        } else if (rx_buffer[1] == HOTT_TEXT_ID) {
            // first received byte looks like a text request. check second received byte for a valid button.
            switch (rx_buffer[0]) {
            case HOTT_BUTTON_DEC:
            case HOTT_BUTTON_INC:
            case HOTT_BUTTON_SET:
            case HOTT_BUTTON_NIL:
            case HOTT_BUTTON_NEXT:
            case HOTT_BUTTON_PREV:
                message_size = build_TEXT_message((struct hott_text_message *)tx_buffer);
                break;
            default:
                message_size = 0;
            }
        }

        // check if a message is in the transmit buffer.
        if (message_size > 0) {
            status.RxPackets++;

            // check idle line before transmit. pause, then check receiver buffer
            vTaskDelayUntil(&lastSysTime, idledelay / portTICK_RATE_MS);

            if (PIOS_COM_ReceiveBuffer(PIOS_COM_HOTT, rx_buffer, 1, 0) == 0) {
                // nothing received means idle line. ready to transmit the requested message
                for (int i = 0; i < message_size; i++) {
                    // send message content with pause between each byte
                    PIOS_COM_SendCharNonBlocking(PIOS_COM_HOTT, tx_buffer[i]);
                    // grab possible incoming loopback data and throw it away
                    PIOS_COM_ReceiveBuffer(PIOS_COM_HOTT, rx_buffer, sizeof(rx_buffer), 0);
                    vTaskDelayUntil(&lastSysTime, datadelay / portTICK_RATE_MS);
                }
                status.TxPackets++;


                // after transmitting the message, any loopback data needs to be cleaned up.
                vTaskDelayUntil(&lastSysTime, idledelay / portTICK_RATE_MS);
                PIOS_COM_ReceiveBuffer(PIOS_COM_HOTT, tx_buffer, message_size, 0);
            } else {
                status.RxFail++;
            }
            HoTTBridgeStatusSet(&status);
        }
    }
}

/**
 * Build requested answer messages.
 * \return value sets message size
 */
uint16_t build_VARIO_message(struct hott_vario_message *msg)
{
    update_telemetrydata();

    if (telestate->Settings.Sensor.VARIO == HOTTBRIDGESETTINGS_SENSOR_DISABLED) {
        return 0;
    }

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_VARIO_ID;
    msg->warning   = generate_warning();
    msg->sensor_text_id = HOTT_VARIO_TEXT_ID;

    // alarm inverse bits. invert display areas on limits
    msg->alarm_inverse |= (telestate->Settings.Limit.MinHeight > telestate->altitude) ? VARIO_INVERT_ALT : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.MaxHeight < telestate->altitude) ? VARIO_INVERT_ALT : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.MaxHeight < telestate->altitude) ? VARIO_INVERT_MAX : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.MinHeight > telestate->altitude) ? VARIO_INVERT_MIN : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.NegDifference1 > telestate->climbrate1s) ? VARIO_INVERT_CR1S : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.PosDifference1 < telestate->climbrate1s) ? VARIO_INVERT_CR1S : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.NegDifference2 > telestate->climbrate3s) ? VARIO_INVERT_CR3S : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.PosDifference2 < telestate->climbrate3s) ? VARIO_INVERT_CR3S : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.NegDifference2 > telestate->climbrate10s) ? VARIO_INVERT_CR10S : 0;
    msg->alarm_inverse |= (telestate->Settings.Limit.PosDifference2 < telestate->climbrate10s) ? VARIO_INVERT_CR10S : 0;

    // altitude relative to ground
    msg->altitude     = scale_float2uword(telestate->altitude, 1, OFFSET_ALTITUDE);
    msg->min_altitude = scale_float2uword(telestate->min_altitude, 1, OFFSET_ALTITUDE);
    msg->max_altitude = scale_float2uword(telestate->max_altitude, 1, OFFSET_ALTITUDE);

    // climbrate
    msg->climbrate    = scale_float2uword(telestate->climbrate1s, M_TO_CM, OFFSET_CLIMBRATE);
    msg->climbrate3s  = scale_float2uword(telestate->climbrate3s, M_TO_CM, OFFSET_CLIMBRATE);
    msg->climbrate10s = scale_float2uword(telestate->climbrate10s, M_TO_CM, OFFSET_CLIMBRATE);

    // compass
    msg->compass = scale_float2int8(telestate->Attitude.Yaw, DEG_TO_UINT, 0);

    // statusline
    memcpy(msg->ascii, telestate->statusline, sizeof(msg->ascii));

    // free display characters
    msg->ascii1   = 0;
    msg->ascii2   = 0;
    msg->ascii3   = 0;

    msg->checksum = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

uint16_t build_GPS_message(struct hott_gps_message *msg)
{
    update_telemetrydata();

    if (telestate->Settings.Sensor.GPS == HOTTBRIDGESETTINGS_SENSOR_DISABLED) {
        return 0;
    }

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_GPS_ID;
    msg->warning   = generate_warning();
    msg->sensor_text_id   = HOTT_GPS_TEXT_ID;

    // alarm inverse bits. invert display areas on limits
    msg->alarm_inverse1  |= (telestate->Settings.Limit.MaxDistance < telestate->homedistance) ? GPS_INVERT_HDIST : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.MinSpeed > telestate->GPS.Groundspeed) ? GPS_INVERT_SPEED : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.MaxSpeed < telestate->GPS.Groundspeed) ? GPS_INVERT_SPEED : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.MinHeight > telestate->altitude) ? GPS_INVERT_ALT : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.MaxHeight < telestate->altitude) ? GPS_INVERT_ALT : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.NegDifference1 > telestate->climbrate1s) ? GPS_INVERT_CR1S : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.PosDifference1 < telestate->climbrate1s) ? GPS_INVERT_CR1S : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.NegDifference2 > telestate->climbrate3s) ? GPS_INVERT_CR3S : 0;
    msg->alarm_inverse1  |= (telestate->Settings.Limit.PosDifference2 < telestate->climbrate3s) ? GPS_INVERT_CR3S : 0;
    msg->alarm_inverse2  |= (telestate->SysAlarms.Alarm.GPS != SYSTEMALARMS_ALARM_OK) ? GPS_INVERT2_POS : 0;

    // gps direction, groundspeed and postition
    msg->flight_direction = scale_float2uint8(telestate->GPS.Heading, DEG_TO_UINT, 0);
    msg->gps_speed = scale_float2uword(telestate->GPS.Groundspeed, MS_TO_KMH, 0);
    convert_long2gps(telestate->GPS.Latitude, &msg->latitude_ns, &msg->latitude_min, &msg->latitude_sec);
    convert_long2gps(telestate->GPS.Longitude, &msg->longitude_ew, &msg->longitude_min, &msg->longitude_sec);

    // homelocation distance, course and state
    msg->distance       = scale_float2uword(telestate->homedistance, 1, 0);
    msg->home_direction = scale_float2uint8(telestate->homecourse, DEG_TO_UINT, 0);
    msg->ascii5         = (telestate->Home.Set ? 'H' : '-');

    // altitude relative to ground and climb rate
    msg->altitude       = scale_float2uword(telestate->altitude, 1, OFFSET_ALTITUDE);
    msg->climbrate      = scale_float2uword(telestate->climbrate1s, M_TO_CM, OFFSET_CLIMBRATE);
    msg->climbrate3s    = scale_float2uint8(telestate->climbrate3s, 1, OFFSET_CLIMBRATE3S);

    // number of satellites,gps fix and state
    msg->gps_num_sat    = telestate->GPS.Satellites;
    switch (telestate->GPS.Status) {
    case GPSPOSITIONSENSOR_STATUS_FIX2D:
        msg->gps_fix_char = '2';
        break;
    case GPSPOSITIONSENSOR_STATUS_FIX3D:
    case GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS:
        msg->gps_fix_char = '3';
        break;
    default:
        msg->gps_fix_char = 0;
    }
    switch (telestate->SysAlarms.Alarm.GPS) {
    case SYSTEMALARMS_ALARM_UNINITIALISED:
        msg->ascii6 = 0;
        // if there is no gps, show compass flight direction
        msg->flight_direction = scale_float2int8((telestate->Attitude.Yaw > 0) ? telestate->Attitude.Yaw : 360 + telestate->Attitude.Yaw, DEG_TO_UINT, 0);
        break;
    case SYSTEMALARMS_ALARM_OK:
        msg->ascii6 = '.';
        break;
    case SYSTEMALARMS_ALARM_WARNING:
        msg->ascii6 = '?';
        break;
    case SYSTEMALARMS_ALARM_ERROR:
    case SYSTEMALARMS_ALARM_CRITICAL:
        msg->ascii6 = '!';
        break;
    default:
        msg->ascii6 = 0;
    }

    // model angles
    msg->angle_roll    = scale_float2int8(telestate->Attitude.Roll, DEG_TO_UINT, 0);
    msg->angle_nick    = scale_float2int8(telestate->Attitude.Pitch, DEG_TO_UINT, 0);
    msg->angle_compass = scale_float2int8(telestate->Attitude.Yaw, DEG_TO_UINT, 0);

    // gps time
    msg->gps_hour = telestate->GPStime.Hour;
    msg->gps_min  = telestate->GPStime.Minute;
    msg->gps_sec  = telestate->GPStime.Second;
    msg->gps_msec = 0;

    // gps MSL (NN) altitude MSL
    msg->msl      = scale_float2uword(telestate->GPS.Altitude, 1, 0);

    // free display chararacter
    msg->ascii4   = 0;

    msg->checksum = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

uint16_t build_GAM_message(struct hott_gam_message *msg)
{
    update_telemetrydata();

    if (telestate->Settings.Sensor.GAM == HOTTBRIDGESETTINGS_SENSOR_DISABLED) {
        return 0;
    }

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_GAM_ID;
    msg->warning   = generate_warning();
    msg->sensor_text_id  = HOTT_GAM_TEXT_ID;

    // alarm inverse bits. invert display areas on limits
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MaxCurrent < telestate->Battery.Current) ? GAM_INVERT2_CURRENT : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MinPowerVoltage > telestate->Battery.Voltage) ? GAM_INVERT2_VOLTAGE : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MaxPowerVoltage < telestate->Battery.Voltage) ? GAM_INVERT2_VOLTAGE : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MinHeight > telestate->altitude) ? GAM_INVERT2_ALT : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MaxHeight < telestate->altitude) ? GAM_INVERT2_ALT : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.NegDifference1 > telestate->climbrate1s) ? GAM_INVERT2_CR1S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.PosDifference1 < telestate->climbrate1s) ? GAM_INVERT2_CR1S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.NegDifference2 > telestate->climbrate3s) ? GAM_INVERT2_CR3S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.PosDifference2 < telestate->climbrate3s) ? GAM_INVERT2_CR3S : 0;

    // temperatures
    msg->temperature1    = scale_float2uint8(telestate->Gyro.temperature, 1, OFFSET_TEMPERATURE);
    msg->temperature2    = scale_float2uint8(telestate->Baro.Temperature, 1, OFFSET_TEMPERATURE);

    // altitude
    msg->altitude    = scale_float2uword(telestate->altitude, 1, OFFSET_ALTITUDE);

    // climbrate
    msg->climbrate   = scale_float2uword(telestate->climbrate1s, M_TO_CM, OFFSET_CLIMBRATE);
    msg->climbrate3s = scale_float2uint8(telestate->climbrate3s, 1, OFFSET_CLIMBRATE3S);

    // main battery
    float voltage = (telestate->Battery.Voltage > 0) ? telestate->Battery.Voltage : 0;
    float current = (telestate->Battery.Current > 0) ? telestate->Battery.Current : 0;
    float energy  = (telestate->Battery.ConsumedEnergy > 0) ? telestate->Battery.ConsumedEnergy : 0;
    msg->voltage  = scale_float2uword(voltage, 10, 0);
    msg->current  = scale_float2uword(current, 10, 0);
    msg->capacity = scale_float2uword(energy, 0.1f, 0);

    // simulate individual cell voltage
    uint8_t cell_voltage = (telestate->Battery.Voltage > 0) ? scale_float2uint8(telestate->Battery.Voltage / telestate->Battery.NbCells, 50, 0) : 0;
    msg->cell1 = (telestate->Battery.NbCells >= 1) ? cell_voltage : 0;
    msg->cell2 = (telestate->Battery.NbCells >= 2) ? cell_voltage : 0;
    msg->cell3 = (telestate->Battery.NbCells >= 3) ? cell_voltage : 0;
    msg->cell4 = (telestate->Battery.NbCells >= 4) ? cell_voltage : 0;
    msg->cell5 = (telestate->Battery.NbCells >= 5) ? cell_voltage : 0;
    msg->cell6 = (telestate->Battery.NbCells >= 6) ? cell_voltage : 0;

    msg->min_cell_volt     = cell_voltage;
    msg->min_cell_volt_num = telestate->Battery.NbCells;

    // apply main voltage to batt1 voltage
    msg->batt1_voltage     = msg->voltage;

    // AirSpeed
    float airspeed = (telestate->Airspeed.TrueAirspeed > 0) ? telestate->Airspeed.TrueAirspeed : 0;
    msg->speed    = scale_float2uword(airspeed, MS_TO_KMH, 0);

    // pressure kPa to 0.1Bar
    msg->pressure = scale_float2uint8(telestate->Baro.Pressure, 0.1f, 0);

    msg->checksum = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

uint16_t build_EAM_message(struct hott_eam_message *msg)
{
    update_telemetrydata();

    if (telestate->Settings.Sensor.EAM == HOTTBRIDGESETTINGS_SENSOR_DISABLED) {
        return 0;
    }

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_EAM_ID;
    msg->warning   = generate_warning();
    msg->sensor_text_id  = HOTT_EAM_TEXT_ID;

    // alarm inverse bits. invert display areas on limits
    msg->alarm_inverse1 |= (telestate->Settings.Limit.MaxUsedCapacity < telestate->Battery.ConsumedEnergy) ? EAM_INVERT_CAPACITY : 0;
    msg->alarm_inverse1 |= (telestate->Settings.Limit.MaxCurrent < telestate->Battery.Current) ? EAM_INVERT_CURRENT : 0;
    msg->alarm_inverse1 |= (telestate->Settings.Limit.MinPowerVoltage > telestate->Battery.Voltage) ? EAM_INVERT_VOLTAGE : 0;
    msg->alarm_inverse1 |= (telestate->Settings.Limit.MaxPowerVoltage < telestate->Battery.Voltage) ? EAM_INVERT_VOLTAGE : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MinHeight > telestate->altitude) ? EAM_INVERT2_ALT : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.MaxHeight < telestate->altitude) ? EAM_INVERT2_ALT : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.NegDifference1 > telestate->climbrate1s) ? EAM_INVERT2_CR1S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.PosDifference1 < telestate->climbrate1s) ? EAM_INVERT2_CR1S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.NegDifference2 > telestate->climbrate3s) ? EAM_INVERT2_CR3S : 0;
    msg->alarm_inverse2 |= (telestate->Settings.Limit.PosDifference2 < telestate->climbrate3s) ? EAM_INVERT2_CR3S : 0;

    // main battery
    float voltage = (telestate->Battery.Voltage > 0) ? telestate->Battery.Voltage : 0;
    float current = (telestate->Battery.Current > 0) ? telestate->Battery.Current : 0;
    float energy  = (telestate->Battery.ConsumedEnergy > 0) ? telestate->Battery.ConsumedEnergy : 0;
    msg->voltage  = scale_float2uword(voltage, 10, 0);
    msg->current  = scale_float2uword(current, 10, 0);
    msg->capacity = scale_float2uword(energy, 0.1f, 0);

    // simulate individual cell voltage
    uint8_t cell_voltage = (telestate->Battery.Voltage > 0) ? scale_float2uint8(telestate->Battery.Voltage / telestate->Battery.NbCells, 50, 0) : 0;
    msg->cell1_H = (telestate->Battery.NbCells >= 1) ? cell_voltage : 0;
    msg->cell2_H = (telestate->Battery.NbCells >= 2) ? cell_voltage : 0;
    msg->cell3_H = (telestate->Battery.NbCells >= 3) ? cell_voltage : 0;
    msg->cell4_H = (telestate->Battery.NbCells >= 4) ? cell_voltage : 0;
    msg->cell5_H = (telestate->Battery.NbCells >= 5) ? cell_voltage : 0;
    msg->cell6_H = (telestate->Battery.NbCells >= 6) ? cell_voltage : 0;
    msg->cell7_H = (telestate->Battery.NbCells >= 7) ? cell_voltage : 0;

    // apply main voltage to batt1 voltage
    msg->batt1_voltage = msg->voltage;

    // AirSpeed
    float airspeed = (telestate->Airspeed.TrueAirspeed > 0) ? telestate->Airspeed.TrueAirspeed : 0;
    msg->speed = scale_float2uword(airspeed, MS_TO_KMH, 0);

    // temperatures
    msg->temperature1 = scale_float2uint8(telestate->Gyro.temperature, 1, OFFSET_TEMPERATURE);
    msg->temperature2 = scale_float2uint8(telestate->Baro.Temperature, 1, OFFSET_TEMPERATURE);

    // altitude
    msg->altitude     = scale_float2uword(telestate->altitude, 1, OFFSET_ALTITUDE);

    // climbrate
    msg->climbrate    = scale_float2uword(telestate->climbrate1s, M_TO_CM, OFFSET_CLIMBRATE);
    msg->climbrate3s  = scale_float2uint8(telestate->climbrate3s, 1, OFFSET_CLIMBRATE3S);

    // flight time
    float flighttime = (telestate->Battery.EstimatedFlightTime <= 5999) ? telestate->Battery.EstimatedFlightTime : 5999;
    msg->electric_min = flighttime / 60;
    msg->electric_sec = flighttime - 60 * msg->electric_min;

    msg->checksum     = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

uint16_t build_ESC_message(struct hott_esc_message *msg)
{
    update_telemetrydata();

    if (telestate->Settings.Sensor.ESC == HOTTBRIDGESETTINGS_SENSOR_DISABLED) {
        return 0;
    }

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_ESC_ID;
    msg->warning   = 0;
    msg->sensor_text_id = HOTT_ESC_TEXT_ID;

    // main batterie
    float voltage     = (telestate->Battery.Voltage > 0) ? telestate->Battery.Voltage : 0;
    float current     = (telestate->Battery.Current > 0) ? telestate->Battery.Current : 0;
    float max_current = (telestate->Battery.PeakCurrent > 0) ? telestate->Battery.PeakCurrent : 0;
    float energy = (telestate->Battery.ConsumedEnergy > 0) ? telestate->Battery.ConsumedEnergy : 0;
    msg->batt_voltage       = scale_float2uword(voltage, 10, 0);
    msg->current = scale_float2uword(current, 10, 0);
    msg->max_current        = scale_float2uword(max_current, 10, 0);
    msg->batt_capacity      = scale_float2uword(energy, 0.1f, 0);

    // temperatures
    msg->temperatureESC     = scale_float2uint8(telestate->Gyro.temperature, 1, OFFSET_TEMPERATURE);
    msg->max_temperatureESC = scale_float2uint8(0, 1, OFFSET_TEMPERATURE);
    msg->temperatureMOT     = scale_float2uint8(telestate->Baro.Temperature, 1, OFFSET_TEMPERATURE);
    msg->max_temperatureMOT = scale_float2uint8(0, 1, OFFSET_TEMPERATURE);

    msg->checksum = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

uint16_t build_TEXT_message(struct hott_text_message *msg)
{
    update_telemetrydata();

    // clear message buffer
    memset(msg, 0, sizeof(*msg));

    // message header
    msg->start     = HOTT_START;
    msg->stop      = HOTT_STOP;
    msg->sensor_id = HOTT_TEXT_ID;

    msg->checksum  = calc_checksum((uint8_t *)msg, sizeof(*msg));
    return sizeof(*msg);
}

/**
 * update telemetry data
 * this is called on every telemetry request
 * calling interval is 200ms depending on TX
 * 200ms telemetry request is used as time base for timed calculations (5Hz interval)
 */
void update_telemetrydata()
{
    // update all available data
    if (HoTTBridgeSettingsHandle() != NULL) {
        HoTTBridgeSettingsGet(&telestate->Settings);
    }
    if (AttitudeStateHandle() != NULL) {
        AttitudeStateGet(&telestate->Attitude);
    }
    if (BaroSensorHandle() != NULL) {
        BaroSensorGet(&telestate->Baro);
    }
    if (FlightBatteryStateHandle() != NULL) {
        FlightBatteryStateGet(&telestate->Battery);
    }
    if (FlightStatusHandle() != NULL) {
        FlightStatusGet(&telestate->FlightStatus);
    }
    if (GPSPositionSensorHandle() != NULL) {
        GPSPositionSensorGet(&telestate->GPS);
    }
    if (AirspeedStateHandle() != NULL) {
        AirspeedStateGet(&telestate->Airspeed);
    }
    if (GPSTimeHandle() != NULL) {
        GPSTimeGet(&telestate->GPStime);
    }
    if (GyroSensorHandle() != NULL) {
        GyroSensorGet(&telestate->Gyro);
    }
    if (HomeLocationHandle() != NULL) {
        HomeLocationGet(&telestate->Home);
    }
    if (PositionStateHandle() != NULL) {
        PositionStateGet(&telestate->Position);
    }
    if (SystemAlarmsHandle() != NULL) {
        SystemAlarmsGet(&telestate->SysAlarms);
    }
    if (VelocityStateHandle() != NULL) {
        VelocityStateGet(&telestate->Velocity);
    }

    // send actual climbrate value to ring buffer as mm per 0.2s values
    uint8_t n = telestate->climbrate_pointer;
    telestate->climbratebuffer[telestate->climbrate_pointer++] = -telestate->Velocity.Down * 200;
    telestate->climbrate_pointer %= climbratesize;

    // calculate avarage climbrates in meters per 1, 3 and 10 second(s) based on 200ms interval
    telestate->climbrate1s  = 0;
    telestate->climbrate3s  = 0;
    telestate->climbrate10s = 0;
    for (uint8_t i = 0; i < climbratesize; i++) {
        telestate->climbrate1s  += (i < 5) ? telestate->climbratebuffer[n] : 0;
        telestate->climbrate3s  += (i < 15) ? telestate->climbratebuffer[n] : 0;
        telestate->climbrate10s += (i < 50) ? telestate->climbratebuffer[n] : 0;
        n += climbratesize - 1;
        n %= climbratesize;
    }
    telestate->climbrate1s  = telestate->climbrate1s / 1000;
    telestate->climbrate3s  = telestate->climbrate3s / 1000;
    telestate->climbrate10s = telestate->climbrate10s / 1000;

    // set altitude offset and clear min/max values when arming
    if ((telestate->FlightStatus.Armed == FLIGHTSTATUS_ARMED_ARMING) || ((telestate->last_armed != FLIGHTSTATUS_ARMED_ARMED) && (telestate->FlightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED))) {
        telestate->min_altitude = 0;
        telestate->max_altitude = 0;
    }
    telestate->last_armed = telestate->FlightStatus.Armed;

    // calculate altitude relative to start position
    telestate->altitude   = -telestate->Position.Down;

    // check and set min/max values when armed
    // and without receiver input for standalone board used as sensor
    if ((telestate->FlightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED) || ((telestate->SysAlarms.Alarm.Attitude == SYSTEMALARMS_ALARM_OK) && (telestate->SysAlarms.Alarm.Receiver != SYSTEMALARMS_ALARM_OK))) {
        if (telestate->min_altitude > telestate->altitude) {
            telestate->min_altitude = telestate->altitude;
        }
        if (telestate->max_altitude < telestate->altitude) {
            telestate->max_altitude = telestate->altitude;
        }
    }

    // gps home position and course
    telestate->homedistance = sqrtf(telestate->Position.North * telestate->Position.North + telestate->Position.East * telestate->Position.East);
    telestate->homecourse   = acosf(-telestate->Position.North / telestate->homedistance) / 3.14159265f * 180;
    if (telestate->Position.East > 0) {
        telestate->homecourse = 360 - telestate->homecourse;
    }

    // statusline
    const char *txt_unknown          = "unknown";
    const char *txt_manual           = "Manual";
    const char *txt_stabilized1      = "Stabilized1";
    const char *txt_stabilized2      = "Stabilized2";
    const char *txt_stabilized3      = "Stabilized3";
    const char *txt_stabilized4      = "Stabilized4";
    const char *txt_stabilized5      = "Stabilized5";
    const char *txt_stabilized6      = "Stabilized6";
    const char *txt_positionhold     = "PositionHold";
    const char *txt_courselock       = "CourseLock";
    const char *txt_velocityroam     = "VelocityRoam";
    const char *txt_homeleash        = "HomeLeash";
    const char *txt_absoluteposition = "AbsolutePosition";
    const char *txt_returntobase     = "ReturnToBase";
    const char *txt_land = "Land";
    const char *txt_pathplanner      = "PathPlanner";
    const char *txt_poi = "PointOfInterest";
    const char *txt_autocruise       = "AutoCruise";
    const char *txt_autotakeoff      = "AutoTakeOff";
    const char *txt_autotune         = "Autotune";
    const char *txt_disarmed         = "Disarmed";
    const char *txt_arming           = "Arming";
    const char *txt_armed = "Armed";

    const char *txt_flightmode;
    switch (telestate->FlightStatus.FlightMode) {
    case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
        txt_flightmode = txt_manual;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
        txt_flightmode = txt_stabilized1;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
        txt_flightmode = txt_stabilized2;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
        txt_flightmode = txt_stabilized3;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED4:
        txt_flightmode = txt_stabilized4;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED5:
        txt_flightmode = txt_stabilized5;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_STABILIZED6:
        txt_flightmode = txt_stabilized6;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
        txt_flightmode = txt_positionhold;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_COURSELOCK:
        txt_flightmode = txt_courselock;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_VELOCITYROAM:
        txt_flightmode = txt_velocityroam;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_HOMELEASH:
        txt_flightmode = txt_homeleash;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_ABSOLUTEPOSITION:
        txt_flightmode = txt_absoluteposition;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE:
        txt_flightmode = txt_returntobase;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_LAND:
        txt_flightmode = txt_land;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
        txt_flightmode = txt_pathplanner;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_POI:
        txt_flightmode = txt_poi;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_AUTOCRUISE:
        txt_flightmode = txt_autocruise;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTAKEOFF:
        txt_flightmode = txt_autotakeoff;
        break;
    case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
        txt_flightmode = txt_autotune;
        break;
    default:
        txt_flightmode = txt_unknown;
    }

    const char *txt_armstate;
    switch (telestate->FlightStatus.Armed) {
    case FLIGHTSTATUS_ARMED_DISARMED:
        txt_armstate = txt_disarmed;
        break;
    case FLIGHTSTATUS_ARMED_ARMING:
        txt_armstate = txt_arming;
        break;
    case FLIGHTSTATUS_ARMED_ARMED:
        txt_armstate = txt_armed;
        break;
    default:
        txt_armstate = txt_unknown;
    }

    snprintf(telestate->statusline, sizeof(telestate->statusline), "%12s,%8s", txt_flightmode, txt_armstate);
}

/**
 * generate warning beeps or spoken announcements
 */
uint8_t generate_warning()
{
    bool gps_ok = (telestate->SysAlarms.Alarm.GPS == SYSTEMALARMS_ALARM_OK);

    // set warning tone with hardcoded priority
    if ((telestate->Settings.Warning.MinSpeed == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MinSpeed > telestate->GPS.Groundspeed * MS_TO_KMH) && gps_ok) {
        return HOTT_TONE_A; // minimum speed
    }
    if ((telestate->Settings.Warning.NegDifference2 == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.NegDifference2 > telestate->climbrate3s)) {
        return HOTT_TONE_B; // sink rate 3s
    }
    if ((telestate->Settings.Warning.NegDifference1 == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.NegDifference1 > telestate->climbrate1s)) {
        return HOTT_TONE_C; // sink rate 1s
    }
    if ((telestate->Settings.Warning.MaxDistance == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxDistance < telestate->homedistance) && gps_ok) {
        return HOTT_TONE_D; // maximum distance
    }
    if ((telestate->Settings.Warning.MinSensor1Temp == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MinSensor1Temp > telestate->Gyro.temperature)) {
        return HOTT_TONE_F; // minimum temperature sensor 1
    }
    if ((telestate->Settings.Warning.MinSensor2Temp == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MinSensor2Temp > telestate->Baro.Temperature)) {
        return HOTT_TONE_G; // minimum temperature sensor 2
    }
    if ((telestate->Settings.Warning.MaxSensor1Temp == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxSensor1Temp < telestate->Gyro.temperature)) {
        return HOTT_TONE_H; // maximum temperature sensor 1
    }
    if ((telestate->Settings.Warning.MaxSensor2Temp == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxSensor2Temp < telestate->Baro.Temperature)) {
        return HOTT_TONE_I; // maximum temperature sensor 2
    }
    if ((telestate->Settings.Warning.MaxSpeed == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxSpeed < telestate->GPS.Groundspeed * MS_TO_KMH) && gps_ok) {
        return HOTT_TONE_L; // maximum speed
    }
    if ((telestate->Settings.Warning.PosDifference2 == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.PosDifference2 > telestate->climbrate3s)) {
        return HOTT_TONE_M; // climb rate 3s
    }
    if ((telestate->Settings.Warning.PosDifference1 == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.PosDifference1 > telestate->climbrate1s)) {
        return HOTT_TONE_N; // climb rate 1s
    }
    if ((telestate->Settings.Warning.MinHeight == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MinHeight > telestate->altitude)) {
        return HOTT_TONE_O; // minimum height
    }
    if ((telestate->Settings.Warning.MinPowerVoltage == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MinPowerVoltage > telestate->Battery.Voltage)) {
        return HOTT_TONE_P; // minimum input voltage
    }
    if ((telestate->Settings.Warning.MaxUsedCapacity == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxUsedCapacity < telestate->Battery.ConsumedEnergy)) {
        return HOTT_TONE_V; // capacity
    }
    if ((telestate->Settings.Warning.MaxCurrent == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxCurrent < telestate->Battery.Current)) {
        return HOTT_TONE_W; // maximum current
    }
    if ((telestate->Settings.Warning.MaxPowerVoltage == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxPowerVoltage < telestate->Battery.Voltage)) {
        return HOTT_TONE_X; // maximum input voltage
    }
    if ((telestate->Settings.Warning.MaxHeight == HOTTBRIDGESETTINGS_WARNING_ENABLED) &&
        (telestate->Settings.Limit.MaxHeight < telestate->altitude)) {
        return HOTT_TONE_Z; // maximum height
    }
    // altitude beeps when crossing altitude limits at 20,40,60,80,100,200,400,600,800 and 1000 meters
    if (telestate->Settings.Warning.AltitudeBeep == HOTTBRIDGESETTINGS_WARNING_ENABLED) {
        // update altitude when checked for beeps
        float last   = telestate->altitude_last;
        float actual = telestate->altitude;
        telestate->altitude_last = telestate->altitude;
        if (((last < 20) && (actual >= 20)) || ((last > 20) && (actual <= 20))) {
            return HOTT_TONE_20M;
        }
        if (((last < 40) && (actual >= 40)) || ((last > 40) && (actual <= 40))) {
            return HOTT_TONE_40M;
        }
        if (((last < 60) && (actual >= 60)) || ((last > 60) && (actual <= 60))) {
            return HOTT_TONE_60M;
        }
        if (((last < 80) && (actual >= 80)) || ((last > 80) && (actual <= 80))) {
            return HOTT_TONE_80M;
        }
        if (((last < 100) && (actual >= 100)) || ((last > 100) && (actual <= 100))) {
            return HOTT_TONE_100M;
        }
        if (((last < 200) && (actual >= 200)) || ((last > 200) && (actual <= 200))) {
            return HOTT_TONE_200M;
        }
        if (((last < 400) && (actual >= 400)) || ((last > 400) && (actual <= 400))) {
            return HOTT_TONE_400M;
        }
        if (((last < 600) && (actual >= 600)) || ((last > 600) && (actual <= 600))) {
            return HOTT_TONE_600M;
        }
        if (((last < 800) && (actual >= 800)) || ((last > 800) && (actual <= 800))) {
            return HOTT_TONE_800M;
        }
        if (((last < 1000) && (actual >= 1000)) || ((last > 1000) && (actual <= 1000))) {
            return HOTT_TONE_1000M;
        }
    }

    // there is no warning
    return 0;
}

/**
 * calculate checksum of data buffer
 */
uint8_t calc_checksum(uint8_t *data, uint16_t size)
{
    uint16_t sum = 0;

    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * scale float value with scale and offset to unsigned byte
 */
uint8_t scale_float2uint8(float value, float scale, float offset)
{
    uint16_t temp = (uint16_t)roundf(value * scale + offset);
    uint8_t result;

    result = (uint8_t)temp & 0xff;
    return result;
}

/**
 * scale float value with scale and offset to signed byte (int8_t)
 */
int8_t scale_float2int8(float value, float scale, float offset)
{
    int8_t result = (int8_t)roundf(value * scale + offset);

    return result;
}

/**
 * scale float value with scale and offset to word
 */
uword_t scale_float2uword(float value, float scale, float offset)
{
    uint16_t temp = (uint16_t)roundf(value * scale + offset);
    uword_t result;

    result.l = (uint8_t)temp & 0xff;
    result.h = (uint8_t)(temp >> 8) & 0xff;
    return result;
}

/**
 * convert dword gps value into HoTT gps format and write result to given pointers
 */
void convert_long2gps(int32_t value, uint8_t *dir, uword_t *min, uword_t *sec)
{
    // convert gps decigrad value into degrees, minutes and seconds
    uword_t temp;
    uint32_t absvalue = abs(value);
    uint16_t degrees  = (absvalue / 10000000);
    uint32_t seconds  = (absvalue - degrees * 10000000) * 6;
    uint16_t minutes  = seconds / 1000000;

    seconds %= 1000000;
    seconds  = seconds / 100;
    uint16_t degmin = degrees * 100 + minutes;
    // write results
    *dir     = (value < 0) ? 1 : 0;
    temp.l   = (uint8_t)degmin & 0xff;
    temp.h   = (uint8_t)(degmin >> 8) & 0xff;
    *min     = temp;
    temp.l   = (uint8_t)seconds & 0xff;
    temp.h   = (uint8_t)(seconds >> 8) & 0xff;
    *sec     = temp;
}

#endif // PIOS_INCLUDE_HOTT_BRIDGE
/**
 * @}
 * @}
 */
