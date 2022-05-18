/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GPSModule GPS Module
 * @brief Process GPS information (UBX binary format)
 * @{
 *
 * @file       UBX.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      GPS module, handles GPS and UBX stream
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

#if defined(PIOS_INCLUDE_GPS_UBX_PARSER)

#include "inc/UBX.h"
#include "inc/GPS.h"
#include <string.h>

#if !defined(PIOS_GPS_MINIMAL)
#include <auxmagsupport.h>
static bool useMag = false;
#endif /* !defined(PIOS_GPS_MINIMAL) */

// this is set and used by this low level ubx code
// it is also reset by the ubx configuration code (UBX6 vs. UBX7) in ubx_autoconfig.c
GPSPositionSensorSensorTypeOptions ubxSensorType = GPSPOSITIONSENSOR_SENSORTYPE_UNKNOWN;

static bool usePvt = false;
static uint32_t lastPvtTime = 0;

// parse table item
typedef struct {
    uint8_t msgClass;
    uint8_t msgID;
    void (*handler)(struct UBXPacket *, GPSPositionSensorData *GpsPosition);
} ubx_message_handler;

// parsing functions, roughly ordered by reception rate (higher rate messages on top)
static void parse_ubx_nav_posllh(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_nav_velned(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_nav_sol(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_nav_dop(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
#if !defined(PIOS_GPS_MINIMAL)
static void parse_ubx_nav_pvt(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_nav_timeutc(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_nav_svinfo(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_op_sys(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_op_mag(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_ack_ack(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_ack_nak(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
static void parse_ubx_mon_ver(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition);
#endif /* !defined(PIOS_GPS_MINIMAL) */

const ubx_message_handler ubx_handler_table[] = {
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_POSLLH,  .handler = &parse_ubx_nav_posllh  },
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_VELNED,  .handler = &parse_ubx_nav_velned  },
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_SOL,     .handler = &parse_ubx_nav_sol     },
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_DOP,     .handler = &parse_ubx_nav_dop     },
#if !defined(PIOS_GPS_MINIMAL)
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_PVT,     .handler = &parse_ubx_nav_pvt     },
    { .msgClass = UBX_CLASS_OP_CUST, .msgID = UBX_ID_OP_MAG,      .handler = &parse_ubx_op_mag      },
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_SVINFO,  .handler = &parse_ubx_nav_svinfo  },
    { .msgClass = UBX_CLASS_NAV,     .msgID = UBX_ID_NAV_TIMEUTC, .handler = &parse_ubx_nav_timeutc },

    { .msgClass = UBX_CLASS_OP_CUST, .msgID = UBX_ID_OP_SYS,      .handler = &parse_ubx_op_sys      },
    { .msgClass = UBX_CLASS_ACK,     .msgID = UBX_ID_ACK_ACK,     .handler = &parse_ubx_ack_ack     },
    { .msgClass = UBX_CLASS_ACK,     .msgID = UBX_ID_ACK_NAK,     .handler = &parse_ubx_ack_nak     },

    { .msgClass = UBX_CLASS_MON,     .msgID = UBX_ID_MON_VER,     .handler = &parse_ubx_mon_ver     },
#endif /* !defined(PIOS_GPS_MINIMAL) */
};
#define UBX_HANDLER_TABLE_SIZE NELEMENTS(ubx_handler_table)

// detected hw version
int32_t ubxHwVersion = -1;

// Last received Ack/Nak
struct UBX_ACK_ACK ubxLastAck;
struct UBX_ACK_NAK ubxLastNak;

// If a PVT sentence is received in the last UBX_PVT_TIMEOUT (ms) timeframe it disables VELNED/POSLLH/SOL/TIMEUTC
#define UBX_PVT_TIMEOUT (1000)

// parse incoming character stream for messages in UBX binary format
int parse_ubx_stream(uint8_t *rx, uint16_t len, char *gps_rx_buffer, GPSPositionSensorData *GpsData, struct GPS_RX_STATS *gpsRxStats)
{
    enum proto_states {
        START,
        UBX_SY2,
        UBX_CLASS,
        UBX_ID,
        UBX_LEN1,
        UBX_LEN2,
        UBX_PAYLOAD,
        UBX_CHK1,
        UBX_CHK2,
        FINISHED
    };
    enum restart_states {
        RESTART_WITH_ERROR,
        RESTART_NO_ERROR
    };
    static uint16_t rx_count = 0;
    static enum proto_states proto_state = START;
    struct UBXPacket *ubx    = (struct UBXPacket *)gps_rx_buffer;
    int ret = PARSER_INCOMPLETE; // message not (yet) complete
    uint16_t i = 0;
    uint16_t restart_index   = 0;
    enum restart_states restart_state;
    uint8_t c;

    // switch continue is the normal condition and comes back to here for another byte
    // switch break is the error state that branches to the end and restarts the scan at the byte after the first sync byte
    while (i < len) {
        c = rx[i++];
        switch (proto_state) {
        case START: // detect protocol
            if (c == UBX_SYNC1) { // first UBX sync char found
                proto_state   = UBX_SY2;
                // restart here, at byte after SYNC1, if we fail to parse
                restart_index = i;
            }
            continue;
        case UBX_SY2:
            if (c == UBX_SYNC2) { // second UBX sync char found
                proto_state = UBX_CLASS;
            } else {
                restart_state = RESTART_NO_ERROR;
                break;
            }
            continue;
        case UBX_CLASS:
            ubx->header.class = c;
            proto_state      = UBX_ID;
            continue;
        case UBX_ID:
            ubx->header.id   = c;
            proto_state      = UBX_LEN1;
            continue;
        case UBX_LEN1:
            ubx->header.len  = c;
            proto_state      = UBX_LEN2;
            continue;
        case UBX_LEN2:
            ubx->header.len += (c << 8);
            if (ubx->header.len > sizeof(UBXPayload)) {
                gpsRxStats->gpsRxOverflow++;
#if defined(PIOS_GPS_MINIMAL)
                restart_state = RESTART_NO_ERROR;
#else
                /*  UBX-NAV-SVINFO (id 30) and UBX-NAV-SAT (id 35) packets are NOT critical to the navigation.
                    Their only use is to update the nice GPS constellation widget in the GCS.
                    These packets become very large when a lot of SV (Space Vehicles) are tracked. (8 + 12 * <number of SV>) bytes

                    In the case of 3 simultaneously enabled GNSS, it is easy to reach the currently defined tracking limit of 32 SV.
                    The memory taken up by this is 8 + 12 * 32 = 392 bytes

                    The NEO-M8N has been seen to send out information for more than 32 SV. This causes overflow errors.
                    We will ignore these informational packets if they become too large.
                    The downside of this is no more constellation updates in the GCS when we reach the limit.

                    An alternative fix could be to increment the maximum number of satellites: MAX_SVS and UBX_CFG_GNSS_NUMCH_VER8 in UBX.h
                    This would use extra memory for little gain. Both fixes can be combined.

                    Tests indicate that, once we reach this amount of tracked SV, the NEO-M8N module positioning output
                    becomes jittery (in time) and therefore less accurate.

                    The recommendation is to limit the maximum number of simultaneously used GNSS to a value of 2.
                    This will help keep the number of tracked satellites in line.
                 */
                if ((ubx->header.class == 0x01) && ((ubx->header.id == 0x30) || (ubx->header.id == 0x35))) {
                    restart_state = RESTART_NO_ERROR;
                } else {
                    restart_state = RESTART_WITH_ERROR;
                }
#endif
                // We won't see the end of the packet. Which means it is useless to do any further processing.
                // Therefore scan for the start of the next packet.
                proto_state = START;
                break;
            } else {
                if (ubx->header.len == 0) {
                    proto_state = UBX_CHK1;
                } else {
                    proto_state = UBX_PAYLOAD;
                    rx_count    = 0;
                }
            }
            continue;
        case UBX_PAYLOAD:
            if (rx_count < ubx->header.len) {
                ubx->payload.payload[rx_count] = c;
                if (++rx_count == ubx->header.len) {
                    proto_state = UBX_CHK1;
                }
            }
            continue;
        case UBX_CHK1:
            ubx->header.ck_a = c;
            proto_state = UBX_CHK2;
            continue;
        case UBX_CHK2:
            ubx->header.ck_b = c;
            // OP GPSV9 sends data with bad checksums this appears to happen because it drops data
            // this has been proven by running it without autoconfig and testing:
            // data coming from OPV9 "GPS+MCU" port the checksum errors happen roughly every 5 to 30 seconds
            // same data coming from OPV9 "GPS Only" port the checksums are always good
            // this also occasionally causes parse_ubx_message() to issue alarms because not all the messages were received
            // see OP GPSV9 comment in parse_ubx_message() for further information
            if (checksum_ubx_message(ubx)) {
                gpsRxStats->gpsRxReceived++;
                proto_state = START;
                // overwrite PARSER_INCOMPLETE with PARSER_COMPLETE
                // but don't overwrite PARSER_ERROR with PARSER_COMPLETE
                // pass PARSER_ERROR to caller if it happens even once
                // only pass PARSER_COMPLETE back to caller if we parsed a full set of GPS data
                // that allows the caller to know if we are parsing GPS data
                // or just other packets for some reason (mis-configuration)
                if (parse_ubx_message(ubx, GpsData) == GPSPOSITIONSENSOR_OBJID
                    && ret == PARSER_INCOMPLETE) {
                    ret = PARSER_COMPLETE;
                }
            } else {
                gpsRxStats->gpsRxChkSumError++;
                restart_state = RESTART_WITH_ERROR;
                break;
            }
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

// Keep track of various GPS messages needed to make up a single UAVO update
// time-of-week timestamp is used to correlate matching messages
#define POSLLH_RECEIVED (1 << 0)
#define STATUS_RECEIVED (1 << 1)
#define DOP_RECEIVED    (1 << 2)
#define VELNED_RECEIVED (1 << 3)
#define SOL_RECEIVED    (1 << 4)
#define ALL_RECEIVED    (SOL_RECEIVED | VELNED_RECEIVED | DOP_RECEIVED | POSLLH_RECEIVED)
#define NONE_RECEIVED   0

static struct msgtracker {
    uint32_t currentTOW; // TOW of the message set currently in progress
    uint8_t  msg_received;   // keep track of received message types
} msgtracker;

// Check if a message belongs to the current data set and register it as 'received'
bool check_msgtracker(uint32_t tow, uint8_t msg_flag)
{
    if (tow > msgtracker.currentTOW ? true // start of a new message set
        : (msgtracker.currentTOW - tow > 6 * 24 * 3600 * 1000)) { // 6 days, TOW wrap around occured
        msgtracker.currentTOW   = tow;
        msgtracker.msg_received = NONE_RECEIVED;
    } else if (tow < msgtracker.currentTOW) { // message outdated (don't process)
        return false;
    }

    msgtracker.msg_received |= msg_flag; // register reception of this msg type
    return true;
}

bool checksum_ubx_message(struct UBXPacket *ubx)
{
    int i;
    uint8_t ck_a, ck_b;

    ck_a  = ubx->header.class;
    ck_b  = ck_a;

    ck_a += ubx->header.id;
    ck_b += ck_a;

    ck_a += ubx->header.len & 0xff;
    ck_b += ck_a;

    ck_a += ubx->header.len >> 8;
    ck_b += ck_a;

    for (i = 0; i < ubx->header.len; i++) {
        ck_a += ubx->payload.payload[i];
        ck_b += ck_a;
    }

    if (ubx->header.ck_a == ck_a &&
        ubx->header.ck_b == ck_b) {
        return true;
    } else {
        return false;
    }
}

static void parse_ubx_nav_posllh(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    if (usePvt) {
        return;
    }
    struct UBX_NAV_POSLLH *posllh = &ubx->payload.nav_posllh;

    if (check_msgtracker(posllh->iTOW, POSLLH_RECEIVED)) {
        if (GpsPosition->Status != GPSPOSITIONSENSOR_STATUS_NOFIX) {
            GpsPosition->Altitude  = (float)posllh->hMSL * 0.001f;
            GpsPosition->GeoidSeparation = (float)(posllh->height - posllh->hMSL) * 0.001f;
            GpsPosition->Latitude  = posllh->lat;
            GpsPosition->Longitude = posllh->lon;
        }
    }
}

static void parse_ubx_nav_sol(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    if (usePvt) {
        return;
    }
    struct UBX_NAV_SOL *sol = &ubx->payload.nav_sol;
    if (check_msgtracker(sol->iTOW, SOL_RECEIVED)) {
        GpsPosition->Satellites = sol->numSV;

        if (sol->flags & STATUS_FLAGS_GPSFIX_OK) {
            switch (sol->gpsFix) {
            case STATUS_GPSFIX_2DFIX:
                GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_FIX2D;
                break;
            case STATUS_GPSFIX_3DFIX:
                GpsPosition->Status = (sol->flags & STATUS_FLAGS_DIFFSOLN) ? GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS : GPSPOSITIONSENSOR_STATUS_FIX3D;
                break;
            default: GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
            }
        } else { // fix is not valid so we make sure to treat is as NOFIX
            GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
        }
    }
}

static void parse_ubx_nav_dop(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    struct UBX_NAV_DOP *dop = &ubx->payload.nav_dop;

    if (check_msgtracker(dop->iTOW, DOP_RECEIVED)) {
        GpsPosition->HDOP = (float)dop->hDOP * 0.01f;
        GpsPosition->VDOP = (float)dop->vDOP * 0.01f;
        GpsPosition->PDOP = (float)dop->pDOP * 0.01f;
    }
}

static void parse_ubx_nav_velned(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    if (usePvt) {
        return;
    }
    GPSVelocitySensorData GpsVelocity;
    struct UBX_NAV_VELNED *velned = &ubx->payload.nav_velned;
    if (check_msgtracker(velned->iTOW, VELNED_RECEIVED)) {
        if (GpsPosition->Status != GPSPOSITIONSENSOR_STATUS_NOFIX) {
            GpsVelocity.North        = (float)velned->velN / 100.0f;
            GpsVelocity.East         = (float)velned->velE / 100.0f;
            GpsVelocity.Down         = (float)velned->velD / 100.0f;
            GPSVelocitySensorSet(&GpsVelocity);
            GpsPosition->Groundspeed = (float)velned->gSpeed * 0.01f;
            GpsPosition->Heading     = (float)velned->heading * 1.0e-5f;
        }
    }
}

#if !defined(PIOS_GPS_MINIMAL)
static void parse_ubx_nav_pvt(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    lastPvtTime = PIOS_DELAY_GetuS();

    GPSVelocitySensorData GpsVelocity;
    struct UBX_NAV_PVT *pvt = &ubx->payload.nav_pvt;
    check_msgtracker(pvt->iTOW, (ALL_RECEIVED));

    GpsVelocity.North = (float)pvt->velN * 0.001f;
    GpsVelocity.East  = (float)pvt->velE * 0.001f;
    GpsVelocity.Down  = (float)pvt->velD * 0.001f;
    GPSVelocitySensorSet(&GpsVelocity);

    GpsPosition->Groundspeed     = (float)pvt->gSpeed * 0.001f;
    GpsPosition->Heading         = (float)pvt->heading * 1.0e-5f;
    GpsPosition->Altitude        = (float)pvt->hMSL * 0.001f;
    GpsPosition->GeoidSeparation = (float)(pvt->height - pvt->hMSL) * 0.001f;
    GpsPosition->Latitude        = pvt->lat;
    GpsPosition->Longitude       = pvt->lon;
    GpsPosition->Satellites      = pvt->numSV;
    GpsPosition->PDOP = pvt->pDOP * 0.01f;

    if (pvt->flags & PVT_FLAGS_GNSSFIX_OK) {
        switch (pvt->fixType) {
        case PVT_FIX_TYPE_2D:
            GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_FIX2D;
            break;
        case PVT_FIX_TYPE_3D:
            GpsPosition->Status = (pvt->flags & PVT_FLAGS_DIFFSOLN) ? GPSPOSITIONSENSOR_STATUS_FIX3DDGNSS : GPSPOSITIONSENSOR_STATUS_FIX3D;
            break;
        default: GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
        }
    } else { // fix is not valid so we make sure to treat is as NOFIX
        GpsPosition->Status = GPSPOSITIONSENSOR_STATUS_NOFIX;
    }

    if (pvt->valid & PVT_VALID_VALIDTIME) {
        // Time is valid, set GpsTime
        GPSTimeData GpsTime;

        GpsTime.Year   = pvt->year;
        GpsTime.Month  = pvt->month;
        GpsTime.Day    = pvt->day;
        GpsTime.Hour   = pvt->hour;
        GpsTime.Minute = pvt->min;
        GpsTime.Second = pvt->sec;

        GPSTimeSet(&GpsTime);
    }
}

static void parse_ubx_nav_timeutc(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    if (usePvt) {
        return;
    }

    struct UBX_NAV_TIMEUTC *timeutc = &ubx->payload.nav_timeutc;
    // Test if time is valid
    if ((timeutc->valid & TIMEUTC_VALIDTOW) && (timeutc->valid & TIMEUTC_VALIDWKN)) {
        // Time is valid, set GpsTime
        GPSTimeData GpsTime;

        GpsTime.Year   = timeutc->year;
        GpsTime.Month  = timeutc->month;
        GpsTime.Day    = timeutc->day;
        GpsTime.Hour   = timeutc->hour;
        GpsTime.Minute = timeutc->min;
        GpsTime.Second = timeutc->sec;
        GpsTime.Millisecond = (int16_t)(timeutc->nano / 1000000);
        GPSTimeSet(&GpsTime);
    } else {
        // Time is not valid, nothing to do
        return;
    }
}

static void parse_ubx_nav_svinfo(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    uint8_t chan;
    GPSSatellitesData svdata;
    struct UBX_NAV_SVINFO *svinfo = &ubx->payload.nav_svinfo;

    svdata.SatsInView = 0;

    // First, use slots for SVs actually being received
    for (chan = 0; chan < svinfo->numCh; chan++) {
        if (svdata.SatsInView < GPSSATELLITES_PRN_NUMELEM && svinfo->sv[chan].cno > 0) {
            svdata.Azimuth[svdata.SatsInView]   = svinfo->sv[chan].azim;
            svdata.Elevation[svdata.SatsInView] = svinfo->sv[chan].elev;
            svdata.PRN[svdata.SatsInView] = svinfo->sv[chan].svid;
            svdata.SNR[svdata.SatsInView] = svinfo->sv[chan].cno;
            svdata.SatsInView++;
        }
    }

    // Now try to add the rest
    for (chan = 0; chan < svinfo->numCh; chan++) {
        if (svdata.SatsInView < GPSSATELLITES_PRN_NUMELEM && 0 == svinfo->sv[chan].cno) {
            svdata.Azimuth[svdata.SatsInView]   = svinfo->sv[chan].azim;
            svdata.Elevation[svdata.SatsInView] = svinfo->sv[chan].elev;
            svdata.PRN[svdata.SatsInView] = svinfo->sv[chan].svid;
            svdata.SNR[svdata.SatsInView] = svinfo->sv[chan].cno;
            svdata.SatsInView++;
        }
    }

    // fill remaining slots (if any)
    for (chan = svdata.SatsInView; chan < GPSSATELLITES_PRN_NUMELEM; chan++) {
        svdata.Azimuth[chan]   = 0;
        svdata.Elevation[chan] = 0;
        svdata.PRN[chan] = 0;
        svdata.SNR[chan] = 0;
    }

    GPSSatellitesSet(&svdata);
}

static void parse_ubx_ack_ack(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    struct UBX_ACK_ACK *ack_ack = &ubx->payload.ack_ack;

    ubxLastAck = *ack_ack;
}

static void parse_ubx_ack_nak(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    struct UBX_ACK_NAK *ack_nak = &ubx->payload.ack_nak;

    ubxLastNak = *ack_nak;
}

static void parse_ubx_mon_ver(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    struct UBX_MON_VER *mon_ver = &ubx->payload.mon_ver;

    ubxHwVersion  = atoi(mon_ver->hwVersion);
    ubxSensorType = (ubxHwVersion >= UBX_HW_VERSION_8) ? GPSPOSITIONSENSOR_SENSORTYPE_UBX8 :
                    ((ubxHwVersion >= UBX_HW_VERSION_7) ? GPSPOSITIONSENSOR_SENSORTYPE_UBX7 : GPSPOSITIONSENSOR_SENSORTYPE_UBX);
    // send sensor type right now because on UBX NEMA we don't get a full set of messages
    // and we want to be able to see sensor type even on UBX NEMA GPS's
    GPSPositionSensorSensorTypeSet((uint8_t *)&ubxSensorType);
}

static void parse_ubx_op_sys(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    struct UBX_OP_SYSINFO *sysinfo = &ubx->payload.op_sysinfo;
    GPSExtendedStatusData data;

    data.FlightTime   = sysinfo->flightTime;
    data.BoardType[0] = sysinfo->board_type;
    data.BoardType[1] = sysinfo->board_revision;
    memcpy(&data.FirmwareHash, &sysinfo->sha1sum, GPSEXTENDEDSTATUS_FIRMWAREHASH_NUMELEM);
    memcpy(&data.FirmwareTag, &sysinfo->commit_tag_name, GPSEXTENDEDSTATUS_FIRMWARETAG_NUMELEM);
    data.Options = sysinfo->options;
    data.Status  = GPSEXTENDEDSTATUS_STATUS_GPSV9;
    GPSExtendedStatusSet(&data);
}

static void parse_ubx_op_mag(struct UBXPacket *ubx, __attribute__((unused)) GPSPositionSensorData *GpsPosition)
{
    if (!useMag) {
        return;
    }
    struct UBX_OP_MAG *mag = &ubx->payload.op_mag;
    float mags[3] = { mag->x, mag->y, mag->z };
    auxmagsupport_publish_samples(mags, AUXMAGSENSOR_STATUS_OK);
}
#endif /* if !defined(PIOS_GPS_MINIMAL) */

// UBX message parser
// returns UAVObjectID if a UAVObject structure is ready for further processing
uint32_t parse_ubx_message(struct UBXPacket *ubx, GPSPositionSensorData *GpsPosition)
{
    uint32_t id = 0;
    static bool ubxInitialized = false;

    if (!ubxInitialized) {
        // initialize dop values. If no DOP sentence is received it is safer to initialize them to a high value rather than 0.
        GpsPosition->HDOP = 99.99f;
        GpsPosition->PDOP = 99.99f;
        GpsPosition->VDOP = 99.99f;
        ubxInitialized    = true;
    }
    // is it using PVT?
    usePvt = (lastPvtTime) && (PIOS_DELAY_GetuSSince(lastPvtTime) < UBX_PVT_TIMEOUT * 1000);
    for (uint8_t i = 0; i < UBX_HANDLER_TABLE_SIZE; i++) {
        const ubx_message_handler *handler = &ubx_handler_table[i];
        if (handler->msgClass == ubx->header.class && handler->msgID == ubx->header.id) {
            handler->handler(ubx, GpsPosition);
            break;
        }
    }

    GpsPosition->SensorType = ubxSensorType;

    if (msgtracker.msg_received == ALL_RECEIVED) {
        // leave BaudRate field alone!
        GPSPositionSensorBaudRateGet(&GpsPosition->BaudRate);
        GPSPositionSensorSet(GpsPosition);
        msgtracker.msg_received = NONE_RECEIVED;
        id = GPSPOSITIONSENSOR_OBJID;
    } else {
        uint8_t status;
        GPSPositionSensorStatusGet(&status);
        if (status == GPSPOSITIONSENSOR_STATUS_NOGPS) {
            // Some ubx thing has been received so GPS is there
            //
            // OP GPSV9 will sometimes cause this NOFIX
            // because GPSV9 drops data which causes checksum errors which causes GPS.c to set the status to NOGPS
            // see OP GPSV9 comment in parse_ubx_stream() for further information
            status = GPSPOSITIONSENSOR_STATUS_NOFIX;
            GPSPositionSensorStatusSet(&status);
        }
    }
    return id;
}

#if !defined(PIOS_GPS_MINIMAL)
void op_gpsv9_load_mag_settings()
{
    if (auxmagsupport_get_type() == AUXMAGSETTINGS_TYPE_GPSV9) {
        useMag = true;
    } else {
        useMag = false;
    }
}
#endif // !defined(PIOS_GPS_MINIMAL)
#endif // defined(PIOS_INCLUDE_GPS_UBX_PARSER)
