/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GPSModule GPS Module
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

#if defined(PIOS_INCLUDE_GPS_DJI_PARSER)

#include "inc/DJI.h"
#include "inc/GPS.h"
#include <string.h>
#include <auxmagsupport.h>

// parsing functions
static void parse_dji_gps(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition);
#if !defined(PIOS_GPS_MINIMAL)
static void parse_dji_mag(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition);
static void parse_dji_ver(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition);
#endif /* !defined(PIOS_GPS_MINIMAL) */

static bool checksum_dji_message(struct DJIPacket *dji);
static uint32_t parse_dji_message(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition);

// parse table item
typedef struct {
    uint8_t msgId;
    void (*handler)(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition);
} djiMessageHandler;

const djiMessageHandler djiHandlerTable[] = {
    { .msgId = DJI_ID_GPS, .handler = &parse_dji_gps },
#if !defined(PIOS_GPS_MINIMAL)
    { .msgId = DJI_ID_MAG, .handler = &parse_dji_mag },
    { .msgId = DJI_ID_VER, .handler = &parse_dji_ver },
#endif /* !defined(PIOS_GPS_MINIMAL) */
};
#define DJI_HANDLER_TABLE_SIZE NELEMENTS(djiHandlerTable)

#if !defined(PIOS_GPS_MINIMAL)
static bool useMag    = false;

// detected hw version
uint32_t djiHwVersion = -1;
uint32_t djiSwVersion = -1;
#endif /* !defined(PIOS_GPS_MINIMAL) */


// parse incoming character stream for messages in DJI binary format
#define djiPacket ((struct DJIPacket *)parsedDjiStruct)
int parse_dji_stream(uint8_t *inputBuffer, uint16_t inputBufferLength, char *parsedDjiStruct, GPSPositionSensorData *gpsPosition, struct GPS_RX_STATS *gpsRxStats)
{
    enum ProtocolStates {
        START,
        DJI_SY2,
        DJI_ID,
        DJI_LEN,
        DJI_PAYLOAD,
        DJI_CHK1,
        DJI_CHK2,
        FINISHED
    };
    enum RestartStates {
        RESTART_WITH_ERROR,
        RESTART_NO_ERROR
    };
    static uint16_t payloadCount   = 0;
    static enum ProtocolStates protocolState = START;
    static bool previousPacketGood = true;
    int ret = PARSER_INCOMPLETE; // message not (yet) complete
    uint16_t inputBufferIndex = 0;
    uint16_t restartIndex     = 0;  // input buffer location to restart from
    enum RestartStates restartState;
    uint8_t inputByte;
    bool currentPacketGood;

    // switch continue is the normal condition and comes back to here for another byte
    // switch break is the error state that branches to the end and restarts the scan at the byte after the first sync byte
    while (inputBufferIndex < inputBufferLength) {
        inputByte = inputBuffer[inputBufferIndex++];
        switch (protocolState) {
        case START: // detect protocol
            if (inputByte == DJI_SYNC1) { // first DJI sync char found
                protocolState = DJI_SY2;
                // restart here, at byte after SYNC1, if we fail to parse
                restartIndex  = inputBufferIndex;
            }
            continue;
        case DJI_SY2:
            if (inputByte == DJI_SYNC2) { // second DJI sync char found
                protocolState = DJI_ID;
            } else {
                restartState = RESTART_NO_ERROR;
                break;
            }
            continue;
        case DJI_ID:
            djiPacket->header.id = inputByte;
            protocolState = DJI_LEN;
            continue;
        case DJI_LEN:
            if (inputByte > sizeof(DJIPayload)) {
                gpsRxStats->gpsRxOverflow++;
                restartState = RESTART_WITH_ERROR;
                break;
            } else {
                djiPacket->header.len = inputByte;
                if (inputByte == 0) {
                    protocolState = DJI_CHK1;
                } else {
                    payloadCount  = 0;
                    protocolState = DJI_PAYLOAD;
                }
            }
            continue;
        case DJI_PAYLOAD:
            if (payloadCount < djiPacket->header.len) {
                djiPacket->payload.payload[payloadCount] = inputByte;
                if (++payloadCount == djiPacket->header.len) {
                    protocolState = DJI_CHK1;
                }
            }
            continue;
        case DJI_CHK1:
            djiPacket->header.checksumA = inputByte;
            protocolState = DJI_CHK2;
            continue;
        case DJI_CHK2:
            djiPacket->header.checksumB = inputByte;
            // ignore checksum errors on correct mag packets that nonetheless have checksum errors
            // these checksum errors happen very often on clone DJI GPS (never on real DJI GPS)
            // and are caused by a clone DJI GPS firmware error
            // the errors happen when it is time to send a non-mag packet (4 or 5 per second)
            // instead of a mag packet (30 per second)
            currentPacketGood = checksum_dji_message(djiPacket);
            // message complete and valid or (it's a mag packet and the previous "any" packet was good)
            if (currentPacketGood || (djiPacket->header.id == DJI_ID_MAG && previousPacketGood)) {
                parse_dji_message(djiPacket, gpsPosition);
                gpsRxStats->gpsRxReceived++;
                protocolState = START;
                // overwrite PARSER_INCOMPLETE with PARSER_COMPLETE
                // but don't overwrite PARSER_ERROR with PARSER_COMPLETE
                // pass PARSER_ERROR to caller if it happens even once
                // only pass PARSER_COMPLETE back to caller if we parsed a full set of GPS data
                // that allows the caller to know if we are parsing GPS data
                // or just other packets for some reason (DJI clone firmware bug that happens sometimes)
                if (djiPacket->header.id == DJI_ID_GPS && ret == PARSER_INCOMPLETE) {
                    ret = PARSER_COMPLETE; // message complete & processed
                }
            } else {
                gpsRxStats->gpsRxChkSumError++;
                restartState = RESTART_WITH_ERROR;
                previousPacketGood = false;
                break;
            }
            previousPacketGood = currentPacketGood;
            continue;
        default:
            continue;
        }

        // this simple restart doesn't work across calls
        // but it does work within a single call
        // and it does the expected thing across calls
        // if restarting due to error detected in 2nd call to this function (on split packet)
        // then we just restart at index 0, which is mid-packet, not the second byte
        if (restartState == RESTART_WITH_ERROR) {
            ret = PARSER_ERROR; // inform caller that we found at least one error (along with 0 or more good packets)
        }
        inputBuffer       += restartIndex; // restart parsing just past the most recent SYNC1
        inputBufferLength -= restartIndex;
        inputBufferIndex   = 0;
        protocolState      = START;
    }

    return ret;
}


bool checksum_dji_message(struct DJIPacket *dji)
{
    int i;
    uint8_t checksumA, checksumB;

    checksumA  = dji->header.id;
    checksumB  = checksumA;

    checksumA += dji->header.len;
    checksumB += checksumA;

    for (i = 0; i < dji->header.len; i++) {
        checksumA += dji->payload.payload[i];
        checksumB += checksumA;
    }

    if (dji->header.checksumA == checksumA &&
        dji->header.checksumB == checksumB) {
        return true;
    } else {
        return false;
    }
}


static void parse_dji_gps(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition)
{
    GPSVelocitySensorData gpsVelocity;
    struct DjiGps *djiGps = &dji->payload.gps;

    // decode with xor mask
    uint8_t mask = djiGps->unused5;

    // some bytes at the end are not xored
    // some bytes in the middle are not xored
    for (uint8_t i = 0; i < GPS_DECODED_LENGTH; ++i) {
        if (i != GPS_NOT_XORED_BYTE_1 && i != GPS_NOT_XORED_BYTE_2) {
            dji->payload.payload[i] ^= mask;
        }
    }

    gpsVelocity.North = (float)djiGps->velN * 0.01f;
    gpsVelocity.East  = (float)djiGps->velE * 0.01f;
    gpsVelocity.Down  = (float)djiGps->velD * 0.01f;
    GPSVelocitySensorSet(&gpsVelocity);

#if !defined(PIOS_GPS_MINIMAL)
    gpsPosition->Groundspeed = sqrtf(gpsVelocity.North * gpsVelocity.North + gpsVelocity.East * gpsVelocity.East);
#else
    gpsPosition->Groundspeed = fmaxf(gpsVelocity.North, gpsVelocity.East) * 1.2f; // within 20% or so
#endif /* !defined(PIOS_GPS_MINIMAL) */
    // don't allow a funny number like 4.87416e-06 to show up in Uavo Browser for Heading
    // smallest groundspeed is 0.01f from (int)1 * (float)0.01
    // so this is saying if groundspeed is zero
    if (gpsPosition->Groundspeed < 0.009f) {
        gpsPosition->Heading = 0.0f;
    } else {
        gpsPosition->Heading = RAD2DEG(atan2f(-gpsVelocity.East, -gpsVelocity.North)) + 180.0f;
    }
    gpsPosition->Altitude = (float)djiGps->hMSL * 0.001f;
    // there is no source of geoid separation data in the DJI protocol
    // Is there a reasonable world model calculation we can do to get a value for geoid separation?
    gpsPosition->GeoidSeparation = 0.0f;
    gpsPosition->Latitude   = djiGps->lat;
    gpsPosition->Longitude  = djiGps->lon;
    gpsPosition->Satellites = djiGps->numSV;
    gpsPosition->PDOP = djiGps->pDOP * 0.01f;
#if !defined(PIOS_GPS_MINIMAL)
    gpsPosition->HDOP = sqrtf((float)djiGps->nDOP * (float)djiGps->nDOP + (float)djiGps->eDOP * (float)djiGps->eDOP) * 0.01f;
    if (gpsPosition->HDOP > 99.99f) {
        gpsPosition->HDOP = 99.99f;
    }
#else
    gpsPosition->HDOP = MAX(djiGps->nDOP, djiGps->eDOP) * 0.01f;
#endif
    gpsPosition->VDOP = djiGps->vDOP * 0.01f;
    if (djiGps->flags & FLAGS_GPSFIX_OK) {
        gpsPosition->Status = djiGps->fixType == FIXTYPE_3D ?
                              GPSPOSITIONSENSOR_STATUS_FIX3D : GPSPOSITIONSENSOR_STATUS_FIX2D;
    } else {
        gpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
    }
    gpsPosition->SensorType = GPSPOSITIONSENSOR_SENSORTYPE_DJI;
    gpsPosition->AutoConfigStatus = GPSPOSITIONSENSOR_AUTOCONFIGSTATUS_DISABLED;
    GPSPositionSensorSet(gpsPosition);

#if !defined(PIOS_GPS_MINIMAL)
    // Time is valid, set GpsTime
    GPSTimeData gpsTime;
    // the lowest bit of day and the highest bit of hour overlap (xored? no, stranger than that)
    // this causes strange day/hour changes
    // we could track it here and even if we guess wrong initially
    // we can massage the data so that time doesn't jump at least
    // and maybe make the assumption that most people will fly at 5pm, not 1am
    // this is part of the DJI protocol
    // see DJI.h for further info
    gpsTime.Year   = (int16_t)djiGps->year + 2000;
    gpsTime.Month  = djiGps->month;
    gpsTime.Day    = djiGps->day;
    gpsTime.Hour   = djiGps->hour;
    gpsTime.Minute = djiGps->min;
    gpsTime.Second = djiGps->sec;
    GPSTimeSet(&gpsTime);
#endif /* !defined(PIOS_GPS_MINIMAL) */
}


#if !defined(PIOS_GPS_MINIMAL)
void dji_load_mag_settings()
{
    if (auxmagsupport_get_type() == AUXMAGSETTINGS_TYPE_DJI) {
        useMag = true;
    } else {
        useMag = false;
    }
}


static void parse_dji_mag(struct DJIPacket *dji, __attribute__((unused)) GPSPositionSensorData *gpsPosition)
{
    if (!useMag) {
        return;
    }
    struct DjiMag *mag = &dji->payload.mag;
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


static void parse_dji_ver(struct DJIPacket *dji, __attribute__((unused)) GPSPositionSensorData *gpsPosition)
{
    {
        struct DjiVer *ver = &dji->payload.ver;
        // decode with xor mask
        uint8_t mask = (uint8_t)(ver->unused1);

        // first 4 bytes are unused and 0 before the encryption
        // so any one of them can be used for the decrypting xor mask
        for (uint8_t i = VER_FIRST_DECODED_BYTE; i < sizeof(struct DjiVer); ++i) {
            dji->payload.payload[i] ^= mask;
        }

        djiHwVersion = ver->hwVersion;
        djiSwVersion = ver->swVersion;
    }
    {
        GPSPositionSensorSensorTypeOptions sensorType;
        sensorType = GPSPOSITIONSENSOR_SENSORTYPE_DJI;
        GPSPositionSensorSensorTypeSet((uint8_t *)&sensorType);
    }
}
#endif /* !defined(PIOS_GPS_MINIMAL) */


// DJI message parser
// returns UAVObjectID if a UAVObject structure is ready for further processing
uint32_t parse_dji_message(struct DJIPacket *dji, GPSPositionSensorData *gpsPosition)
{
    static bool djiInitialized = false;
    uint32_t id = 0;

    if (!djiInitialized) {
        // initialize dop values. If no DOP sentence is received it is safer to initialize them to a high value rather than 0.
        gpsPosition->HDOP = 99.99f;
        gpsPosition->PDOP = 99.99f;
        gpsPosition->VDOP = 99.99f;
        djiInitialized    = true;
    }

    for (uint8_t i = 0; i < DJI_HANDLER_TABLE_SIZE; i++) {
        const djiMessageHandler *handler = &djiHandlerTable[i];
        if (handler->msgId == dji->header.id) {
            handler->handler(dji, gpsPosition);
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
#endif // PIOS_INCLUDE_GPS_DJI_PARSER
