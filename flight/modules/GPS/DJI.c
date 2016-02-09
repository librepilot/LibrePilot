/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GSPModule GPS Module
 * @brief Process GPS information (DJI-Naza binary format)
 * @{
 *
 * @file       DJI.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @brief      GPS module, handles DJI stream
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
#include "pios_math.h"
#include <pios_helpers.h>
#include <pios_delay.h>
// dji parser is required for sensorType
#if (defined(PIOS_INCLUDE_GPS_DJI_PARSER) && defined(PIOS_INCLUDE_GPS_DJI_PARSER))
#include "inc/DJI.h"
#include "inc/GPS.h"
#include <string.h>

#include <auxmagsupport.h>

bool useMag = false;

// this is defined in DJI.c
extern GPSPositionSensorSensorTypeOptions sensorType;

// parsing functions, roughly ordered by reception rate (higher rate messages on top)
static void parse_dji_mag(struct DJIPacket *dji, GPSPositionSensorData *GpsPosition);
static void parse_dji_gps(struct DJIPacket *dji, GPSPositionSensorData *GpsPosition);
static void parse_dji_ver(struct DJIPacket *dji, GPSPositionSensorData *GpsPosition);

// parse table item
typedef struct {
    uint8_t msgID;
    void (*handler)(struct DJIPacket *, GPSPositionSensorData *GpsPosition);
} dji_message_handler;

const dji_message_handler dji_handler_table[] = {
    { .msgID = DJI_ID_GPS, .handler = &parse_dji_gps },
    { .msgID = DJI_ID_MAG, .handler = &parse_dji_mag },
    { .msgID = DJI_ID_VER, .handler = &parse_dji_ver },
};
#define DJI_HANDLER_TABLE_SIZE NELEMENTS(dji_handler_table)


// detected hw version
uint32_t djiHwVersion = -1;
uint32_t djiSwVersion = -1;

// parse incoming character stream for messages in DJI binary format
int parse_dji_stream(uint8_t *rx, uint16_t len, char *gps_rx_buffer, GPSPositionSensorData *GpsData, struct GPS_RX_STATS *gpsRxStats)
{
    int ret = PARSER_INCOMPLETE; // message not (yet) complete
    enum proto_states {
        START,
        DJI_SY2,
        DJI_ID,
        DJI_LEN,
        DJI_PAYLOAD,
        DJI_CHK1,
        DJI_CHK2,
        FINISHED
    };
    enum restart_states {
        RESTART_WITH_ERROR,
        RESTART_NO_ERROR
    };
    uint8_t c;
    static enum proto_states proto_state = START;
    static uint16_t rx_count = 0;
    struct DJIPacket *dji    = (struct DJIPacket *)gps_rx_buffer;
    uint16_t i = 0;
    uint16_t restart_index   = 0;
    enum restart_states restart_state;
    static bool previous_packet_good = true;
    bool current_packet_good;

    // switch continue is the normal condition and comes back to here for another byte
    // switch break is the error state that branches to the end and restarts the scan at the byte after the first sync byte
    while (i < len) {
        c = rx[i++];
        switch (proto_state) {
        case START: // detect protocol
            if (c == DJI_SYNC1) { // first DJI sync char found
                proto_state   = DJI_SY2;
                // restart here, at byte after SYNC1, if we fail to parse
                restart_index = i;
            }
            continue;
        case DJI_SY2:
            if (c == DJI_SYNC2) { // second DJI sync char found
                proto_state = DJI_ID;
            } else {
                restart_state = RESTART_NO_ERROR;
                break;
            }
            continue;
        case DJI_ID:
            dji->header.id = c;
            proto_state    = DJI_LEN;
            continue;
        case DJI_LEN:
            if (c > sizeof(DJIPayload)) {
                gpsRxStats->gpsRxOverflow++;
#if defined(PIOS_GPS_MINIMAL)
                restart_state = RESTART_NO_ERROR;
                break;
#else
                restart_state = RESTART_WITH_ERROR;
                break;
#endif
            } else {
                dji->header.len = c;
                if (c == 0) {
                    proto_state = DJI_CHK1;
                } else {
                    rx_count    = 0;
                    proto_state = DJI_PAYLOAD;
                }
            }
            continue;
        case DJI_PAYLOAD:
            if (rx_count < dji->header.len) {
                dji->payload.payload[rx_count] = c;
                if (++rx_count == dji->header.len) {
                    proto_state = DJI_CHK1;
                }
            }
            continue;
        case DJI_CHK1:
            dji->header.ck_a = c;
            proto_state = DJI_CHK2;
            continue;
        case DJI_CHK2:
            dji->header.ck_b = c;
            // ignore checksum errors on correct mag packets that nonetheless have checksum errors
            // these checksum errors happen very often on clone DJI GPS (never on real DJI GPS)
            // and are caused by a clone DJI GPS firmware error
            // the errors happen when it is time to send a non-mag packet (4 or 5 per second)
            // instead of a mag packet (30 per second)
            current_packet_good = checksum_dji_message(dji);
            // message complete and valid or (it's a mag packet and the previous "any" packet was good)
            if (current_packet_good || (dji->header.id == DJI_ID_MAG && previous_packet_good)) {
                parse_dji_message(dji, GpsData);
                gpsRxStats->gpsRxReceived++;
                proto_state = START;
                // overwrite PARSER_INCOMPLETE with PARSER_COMPLETE
                // but don't overwrite PARSER_ERROR with PARSER_COMPLETE
                // pass PARSER_ERROR to caller if it happens even once
                // only pass PARSER_COMPLETE back to caller if we parsed a full set of GPS data
                // that allows the caller to know if we are parsing GPS data
                // or just other packets for some reason (DJI clone firmware bug that happens sometimes)
                if (dji->header.id == DJI_ID_GPS && ret == PARSER_INCOMPLETE) {
                    ret = PARSER_COMPLETE; // message complete & processed
                }
            } else {
                gpsRxStats->gpsRxChkSumError++;
                restart_state = RESTART_WITH_ERROR;
                previous_packet_good = false;
                break;
            }
            previous_packet_good = current_packet_good;
            continue;
        default:
            continue;
        }

        // this simple restart doesn't work across calls
        // but it does work within a single call
        // and it does the expected thing across calls
        // if restarting due to error detected in 2nd call to this function (on split packet)
        // then we just restart at index 0, which is mid-packet, not the second byte
        if (restart_state == RESTART_WITH_ERROR) {
            ret = PARSER_ERROR; // inform caller that we found at least one error (along with 0 or more good packets)
        }
        rx  += restart_index; // restart parsing just past the most recent SYNC1
        len -= restart_index;
        i    = 0;
        proto_state = START;
    }

    return ret;
}


bool checksum_dji_message(struct DJIPacket *dji)
{
    int i;
    uint8_t ck_a, ck_b;

    ck_a  = dji->header.id;
    ck_b  = ck_a;

    ck_a += dji->header.len;
    ck_b += ck_a;

    for (i = 0; i < dji->header.len; i++) {
        ck_a += dji->payload.payload[i];
        ck_b += ck_a;
    }

    if (dji->header.ck_a == ck_a &&
        dji->header.ck_b == ck_b) {
        return true;
    } else {
        return false;
    }
}


static void parse_dji_gps(struct DJIPacket *dji, GPSPositionSensorData *GpsPosition)
{
    static bool inited = false;

    if (!inited) {
        inited = true;
        // Is there a model calculation we can do to get a reasonable value for geoid separation?
    }

    GPSVelocitySensorData GpsVelocity;
    struct DJI_GPS *gps = &dji->payload.gps;

    // decode with xor mask
    uint8_t mask = gps->unused5;
    // for (uint8_t i=0; i<dji->header->len; ++i) {
    for (uint8_t i = 0; i < 56; ++i) {
        // if (i!=48 && i!=49 && i<=55) {
        if (i != 48 && i != 49) {
            dji->payload.payload[i] ^= mask;
        }
    }

    GpsVelocity.North = (float)gps->velN * 0.01f;
    GpsVelocity.East  = (float)gps->velE * 0.01f;
    GpsVelocity.Down  = (float)gps->velD * 0.01f;
    GPSVelocitySensorSet(&GpsVelocity);

    GpsPosition->Groundspeed     = sqrtf(GpsVelocity.North * GpsVelocity.North + GpsVelocity.East * GpsVelocity.East);
    GpsPosition->Heading         = RAD2DEG(atan2f(-GpsVelocity.East, -GpsVelocity.North)) + 180.0f;
    GpsPosition->Altitude        = (float)gps->hMSL * 0.001f;
    // there is no source of geoid separation data in the DJI protocol
    GpsPosition->GeoidSeparation = 0.0f;
    GpsPosition->Latitude        = gps->lat;
    GpsPosition->Longitude       = gps->lon;
    GpsPosition->Satellites      = gps->numSV;
    GpsPosition->PDOP = gps->pDOP * 0.01f;
    GpsPosition->HDOP = sqrtf((float)gps->nDOP * (float)gps->nDOP + (float)gps->eDOP * (float)gps->eDOP) * 0.01f;
    GpsPosition->VDOP = gps->vDOP * 0.01f;
    if (gps->flags & FLAGS_GPSFIX_OK) {
        GpsPosition->Status = gps->fixType == FIXTYPE_3D ?
                              GPSPOSITIONSENSOR_STATUS_FIX3D : GPSPOSITIONSENSOR_STATUS_FIX2D;
    } else {
        GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
    }
    GpsPosition->SensorType = GPSPOSITIONSENSOR_SENSORTYPE_DJI;
    GpsPosition->AutoConfigStatus = GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DISABLED;
    GPSPositionSensorSet(GpsPosition);

    // Time is valid, set GpsTime
    GPSTimeData GpsTime;
    // the lowest bit of day and the highest bit of hour overlap (xored? no, stranger than that)
    // this causes strange day/hour changes
    // we could track it here and even if we guess wrong initially
    // we can massage the data so that time doesn't jump
    // and maybe make the assumption that most people will fly at 5pm, not 1am
    // this is part of the DJI protocol
    // see DJI.h for further info
    GpsTime.Year   = (int16_t)gps->year + 2000;
    GpsTime.Month  = gps->month;
    GpsTime.Day    = gps->day;
    GpsTime.Hour   = gps->hour;
    GpsTime.Minute = gps->min;
    GpsTime.Second = gps->sec;
    GPSTimeSet(&GpsTime);
}


static void parse_dji_mag(struct DJIPacket *dji, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    if (!useMag) {
        return;
    }
    struct DJI_MAG *mag = &dji->payload.mag;
    union {
        struct {
            int8_t mask;
            int8_t mask2;
        };
        int16_t maskmask;
    } u;
    u.mask = (int8_t)(dji->payload.payload[4]);
    u.mask = u.mask2 = (((u.mask ^ (u.mask >> 4)) & 0x0F) | ((u.mask << 3) & 0xF0)) ^ (((u.mask & 0x01) << 3) | ((u.mask & 0x01) << 7));
    // yes, z is only xored by mask<<8, not maskmask
    float mags[3] = { mag->x ^ u.maskmask, mag->y ^ u.maskmask, mag->z ^ ((int16_t)u.mask << 8) };
    auxmagsupport_publish_samples(mags, AUXMAGSENSOR_STATUS_OK);
}


static void parse_dji_ver(struct DJIPacket *dji, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    struct DJI_VER *ver = &dji->payload.ver;

    // decode with xor mask
    uint8_t mask = (uint8_t)(ver->unused1);

    // for (uint8_t i=0; i<dji->header->len; ++i) {
    for (uint8_t i = 4; i < 12; ++i) {
        dji->payload.payload[i] ^= mask;
    }

    djiHwVersion = ver->hwVersion;
    djiSwVersion = ver->swVersion;
    sensorType   = GPSPOSITIONSENSOR_SENSORTYPE_DJI;
    GPSPositionSensorSensorTypeSet((uint8_t *)&sensorType);
}


// DJI message parser
// returns UAVObjectID if a UAVObject structure is ready for further processing

uint32_t parse_dji_message(struct DJIPacket *dji, GPSPositionSensorData *GpsPosition)
{
    uint32_t id = 0;
    static bool djiInitialized = false;

    if (!djiInitialized) {
        // initialize dop values. If no DOP sentence is received it is safer to initialize them to a high value rather than 0.
        GpsPosition->HDOP = 99.99f;
        GpsPosition->PDOP = 99.99f;
        GpsPosition->VDOP = 99.99f;
        djiInitialized    = true;
    }

    for (uint8_t i = 0; i < DJI_HANDLER_TABLE_SIZE; i++) {
        const dji_message_handler *handler = &dji_handler_table[i];
        if (handler->msgID == dji->header.id) {
            handler->handler(dji, GpsPosition);
            break;
        }
    }

    {
        uint8_t status;
        GPSPositionSensorStatusGet(&status);
        if (status == GPSPOSITIONSENSOR_STATUS_NOGPS) {
            // Some dji thing has been received so GPS is there
            status = GPSPOSITIONSENSOR_STATUS_NOFIX;
            GPSPositionSensorStatusSet(&status);
        }
    }

    return id;
}

void dji_load_mag_settings()
{
    if (auxmagsupport_get_type() == AUXMAGSETTINGS_TYPE_DJI) {
        useMag = true;
    } else {
        useMag = false;
    }
}
#endif // PIOS_INCLUDE_GPS_DJI_PARSER
