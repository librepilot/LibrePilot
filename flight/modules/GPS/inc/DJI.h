/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GPSModule GPS Module
 * @brief Process GPS information (DJI-Naza binary format)
 * @{
 *
 * @file       DJI.h
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

#ifndef DJI_H
#define DJI_H
#include "openpilot.h"
#include "gpspositionsensor.h"
#include "gpsextendedstatus.h"
#ifndef PIOS_GPS_MINIMAL
#include "auxmagsensor.h"
#endif

#include "GPS.h"

#define DJI_SYNC1 0x55 // DJI protocol synchronization characters
#define DJI_SYNC2 0xaa

// Message IDs
typedef enum {
    DJI_ID_GPS = 0x10,
    DJI_ID_MAG = 0x20,
    DJI_ID_VER = 0x30,
} dji_msg_id;

// private structures

/*
   GPS protocol info from
   http://www.rcgroups.com/forums/showpost.php?p=26210591&postcount=15

   The 0x10 message contains GPS data. Here is the structure of the message,
   fields marked with XX I'm not sure about yet. The others will be described below.

   55 AA 10 3A DT DT DT DT LO LO LO LO LA LA LA LA AL AL AL AL HA HA HA HA VA VA VA VA XX XX XX XX
   NV NV NV NV EV EV EV EV DV DV DV DV PD PD VD VD ND ND ED ED NS XX FT XX SF XX XX XM SN SN CS CS

   The payload is XORed with a mask that changes over time (see below for more details).

   Values in the message are stored in little endian.

   HEADER
   -------------
   BYTE 1-2: message header - always 55 AA
   BYTE 3: message id (0x10 for GPS message)
   BYTE 4: lenght of the payload (0x3A or 58 decimal for 0x10 message)

   PAYLOAD
   --------------
   BYTE 5-8 (DT)  : date and time, see details below
   BYTE 9-12 (LO) : longitude (x10^7, degree decimal)
   BYTE 13-16 (LA): latitude (x10^7, degree decimal)
   BYTE 17-20 (AL): altitude (in milimeters)
   BYTE 21-24 (HA): horizontal accuracy estimate (see uBlox NAV-POSLLH message for details)
   BYTE 25-28 (VA): vertical accuracy estimate (see uBlox NAV-POSLLH message for details)
   BYTE 29-32     : ??? (seems to be always 0)
   BYTE 33-36 (NV): NED north velocity (see uBlox NAV-VELNED message for details)
   BYTE 37-40 (EV): NED east velocity (see uBlox NAV-VELNED message for details)
   BYTE 41-44 (DV): NED down velocity (see uBlox NAV-VELNED message for details)
   BYTE 45-46 (PD): position DOP (see uBlox NAV-DOP message for details)
   BYTE 47-48 (VD): vertical DOP (see uBlox NAV-DOP message for details)
   BYTE 49-50 (ND): northing DOP (see uBlox NAV-DOP message for details)
   BYTE 51-52 (ED): easting DOP (see uBlox NAV-DOP message for details)
   BYTE 53 (NS)   : number of satellites (not XORed)
   BYTE 54        : ??? (not XORed, seems to be always 0)
   BYTE 55 (FT)   : fix type (0 - no lock, 2 - 2D lock, 3 - 3D lock, not sure if other values can be expected
               : see uBlox NAV-SOL message for details)
   BYTE 56        : ??? (seems to be always 0)
   BYTE 57 (SF)   : fix status flags (see uBlox NAV-SOL message for details)
   BYTE 58-59     : ??? (seems to be always 0)
   BYTE 60 (XM)   : not sure yet, but I use it as the XOR mask
   BYTE 61-62 (SN): sequence number (not XORed), once there is a lock - increases with every message.
                 When the lock is lost later LSB and MSB are swapped with every message.

   CHECKSUM
   -----------------
   BYTE 63-64 (CS): checksum, calculated the same way as for uBlox binary messages


   XOR mask
   ---------------
   All bytes of the payload except 53rd (NS), 54th, 61st (SN LSB) and 62nd (SN MSB) are XORed with a mask.
   Mask is calculated based on the value of byte 53rd (NS) and 61st (SN LSB).

   If we index bits from LSB to MSB as 0-7 we have:
   mask[0] = 53rdByte[0] xor 61stByte[4]
   mask[1] = 53rdByte[1] xor 61stByte[5]
   mask[2] = 53rdByte[2] xor 61stByte[6]
   mask[3] = 53rdByte[3] xor 61stByte[7] xor 53rdByte[0];
   mask[4] = 53rdByte[1];
   mask[5] = 53rdByte[2];
   mask[6] = 53rdByte[3];
   mask[7] = 53rdByte[0] xor 61stByte[4];

   To simplify calculations any of the unknown bytes that when XORer seem to be always 0 (29-32, 56, 58-60)
   can be used as XOR mask (based on the fact that 0 XOR mask == mask). In the library I use byte 60.

   Date and time format
   ----------------------------
   Date (Year, Month, Day) and time (Hour, Minute, Second) are stored as little endian 32bit unsigned integer,
   the meaning of particular bits is as follows:

   YYYYYYYMMMMDDDDDHHHHMMMMMMSSSSSS

   NOTE 1: to get the day value correct you must add 1 when hour is > 7
   NOTE 2: for the time between 16:00 and 23:59 the hour will be returned as 0-7
        and there seems to be no way to differentiate between 00:00 - 07:59 and 16:00 - 23:59.

   From further discussion in the thread, it sounds like the day is written into the buffer
   (buffer initially zero bits) and then the hour is xored into the buffer
   with the bottom bit of day and top bit of hour mapped to the same buffer bit?
   Is that even correct?  Or could we have a correct hour and the day is just wrong?

   http://www.rcgroups.com/forums/showpost.php?p=28158918&postcount=180
   Midnight between 13th and 14th of March
   0001110 0011 01110 0000 000000 000000 -> 14.3.14 00:00:00 -> 0
   identical with
   4PM, 14th of March
   0001110 0011 01110 0000 000000 000000 -> 14.3.14 00:00:00 -> 0

   Midnight between 14th and 15th of March
   0001110 0011 01111 0000 000000 000000 -> 14.3.15 00:00:00 -> 16
   identical with
   4PM, 15th of March
   0001110 0011 01111 0000 000000 000000 -> 14.3.15 00:00:00 -> 16

   So as you can see even if we take 5 bits the hour is not correct either
   Are they are xored?  If we knew the date from a different source we would know the time.
   Maybe the xor mask itself contains the bit.  Does the mask change?  and how?  across the transitions.

   http://www.rcgroups.com/forums/showpost.php?p=28168741&postcount=182
   Originally Posted by gwouite View Post
   Question, are you sure that at 4PM, you're day value doesn't increase of 1 ?
   It does, but it also does decrease by 1 at 8am so you have:
   00:00 - 07:59 => day = X, hour = 0 - 7
   08:00 - 15:59 => day = X - 1, hour = 8 - 15
   16:00 - 23:59 => day = X, hour = 0 - 7

   http://www.rcgroups.com/forums/showpost.php?p=28782603&postcount=218
   Here is the SBAS config from the Naza GPS
   CFG-SBAS - 06 16 08 00 01 03 03 00 51 62 06 00
   If I read it correctly EGNOS (PRN 124/124/126), MSAS (PRN 129/137) and WAAS (PRN 133/134/135/138) are enabled.
 */

// DJI GPS packet
struct DjiGps { // byte offset from beginning of packet, subtract 5 for struct offset
    struct { // YYYYYYYMMMMDDDDDHHHHMMMMMMSSSSSS
        uint32_t sec : 6;
        uint32_t min : 6;
        uint32_t hour : 4;
        uint32_t day : 5;
        uint32_t month : 4;
        uint32_t year : 7;
    }; // BYTE 5-8 (DT): date and time, see details above
    int32_t  lon;     // BYTE 9-12 (LO): longitude (x10^7, degree decimal)
    int32_t  lat;     // BYTE 13-16 (LA): latitude (x10^7, degree decimal)
    int32_t  hMSL;    // BYTE 17-20 (AL): altitude (in millimeters) (is this MSL or geoid?)
    uint32_t hAcc; // BYTE 21-24 (HA): horizontal accuracy estimate (see uBlox NAV-POSLLH message for details)
    uint32_t vAcc; // BYTE 25-28 (VA): vertical accuracy estimate (see uBlox NAV-POSLLH message for details)
    uint32_t unused1; // BYTE 29-32: ??? (seems to be always 0)
    int32_t  velN;    // BYTE 33-36 (NV): NED north velocity (see uBlox NAV-VELNED message for details)
    int32_t  velE;    // BYTE 37-40 (EV): NED east velocity (see uBlox NAV-VELNED message for details)
    int32_t  velD;    // BYTE 41-44 (DV): NED down velocity (see uBlox NAV-VELNED message for details)
    uint16_t pDOP; // BYTE 45-46 (PD): position DOP (see uBlox NAV-DOP message for details)
    uint16_t vDOP; // BYTE 47-48 (VD): vertical DOP (see uBlox NAV-DOP message for details)
    uint16_t nDOP; // BYTE 49-50 (ND): northing DOP (see uBlox NAV-DOP message for details)
    uint16_t eDOP; // BYTE 51-52 (ED): easting DOP (see uBlox NAV-DOP message for details)
    uint8_t  numSV;   // BYTE 53 (NS): number of satellites (not XORed)
    uint8_t  unused2; // BYTE 54: ??? (not XORed, seems to be always 0)
    uint8_t  fixType; // BYTE 55 (FT): fix type (0 - no lock, 2 - 2D lock, 3 - 3D lock, not sure if other values can be expected
                      // see uBlox NAV-SOL message for details)
    uint8_t  unused3; // BYTE 56: ??? (seems to be always 0)
    uint8_t  flags;   // BYTE 57 (SF): fix status flags (see uBlox NAV-SOL message for details)
    uint16_t unused4; // BYTE 58-59: ??? (seems to be always 0)
    uint8_t  unused5; // BYTE 60 (XM): not sure yet, but I use it as the XOR mask
    uint16_t seqNo; // BYTE 61-62 (SN): sequence number (not XORed), once there is a lock
                    // increases with every message. When the lock is lost later LSB and MSB are swapped (in all messages where lock is lost).
} __attribute__((packed));

#define FLAGS_GPSFIX_OK          (1 << 0)
#define FLAGS_DIFFSOLN           (1 << 1)
#define FLAGS_WKNSET             (1 << 2)
#define FLAGS_TOWSET             (1 << 3)

#define FIXTYPE_NO_FIX           0x00 /* No Fix */
#define FIXTYPE_DEAD_RECKON      0x01 /* Dead Reckoning only */
#define FIXTYPE_2D               0x02 /* 2D-Fix */
#define FIXTYPE_3D               0x03 /* 3D-Fix */
#define FIXTYPE_GNSS_DEAD_RECKON 0x04 /* GNSS + dead reckoning combined */
#define FIXTYPE_TIME_ONLY        0x05 /* Time only fix */

#define GPS_DECODED_LENGTH       offsetof(struct DjiGps, seqNo)
#define GPS_NOT_XORED_BYTE_1     offsetof(struct DjiGps, numSV)
#define GPS_NOT_XORED_BYTE_2     offsetof(struct DjiGps, unused2)


/*
   mag protocol info from
   http://www.rcgroups.com/forums/showpost.php?p=26248426&postcount=62

   The 0x20 message contains compass data. Here is the structure of the message,
   fields marked with XX I'm not sure about yet. The others will be described below.

   55 AA 20 06 CX CX CY CY CZ CZ CS CS

   Values in the message are stored in little endian.

   HEADER
   -------------
   BYTE 1-2: message header - always 55 AA
   BYTE 3: message id (0x20 for compass message)
   BYTE 4: length of the payload (0x06 or 6 decimal for 0x20 message)

   PAYLOAD
   --------------
   BYTE 5-6 (CX): compass X axis data (signed) - see comments below
   BYTE 7-8 (CY): compass Y axis data (signed) - see comments below
   BYTE 9-10 (CZ): compass Z axis data (signed) - see comments below

   CHECKSUM
   -----------------
   BYTE 11-12 (CS): checksum, calculated the same way as for uBlox binary messages

   All the bytes of the payload except 9th are XORed with a mask.
   Mask is calculated based on the value of the 9th byte.

   If we index bits from LSB to MSB as 0-7 we have:
   mask[0] = 9thByte[0] xor 9thByte[4]
   mask[1] = 9thByte[1] xor 9thByte[5]
   mask[2] = 9thByte[2] xor 9thByte[6]
   mask[3] = 9thByte[3] xor 9thByte[7] xor 9thByte[0];
   mask[4] = 9thByte[1];
   mask[5] = 9thByte[2];
   mask[6] = 9thByte[3];
   mask[7] = 9thByte[4] xor 9thByte[0];

   To calculate the heading (not tilt compensated) you need to do atan2 on the resulting
   y any a (y and x?) values, convert radians to degrees and add 360 if the result is negative.
 */

struct DjiMag { // byte offset from beginning of packet, subtract 5 for struct offset
    int16_t x; // BYTE 5-6 (CX): compass X axis data (signed) - see comments below
    int16_t y; // BYTE 7-8 (CY): compass Y axis data (signed) - see comments below
    int16_t z; // BYTE 9-10 (CZ): compass Z axis data (signed) - see comments below
} __attribute__((packed));


/*
   version info from
   http://www.rcgroups.com/forums/showpost.php?p=27058649&postcount=120

   This is still to be confirmed but I believe the 0x30 message carries the GPS module hardware id and firmware version.

   55 AA 30 0C XX XX XX XX FW FW FW FW HW HW HW HW CS CS

   Note that you need to read version numbers backwards (02 01 00 06 means v6.0.1.2)

   HEADER
   -------------
   BYTE 1-2: message header - always 55 AA
   BYTE 3: message id (0x30 for GPS module versions message)
   BYTE 4: length of the payload (0x0C or 12 decimal for 0x30 message)

   PAYLOAD
   --------------
   BYTE 5-8" ??? (seems to be always 0)
   BYTE 9-12 (FW): firmware version
   BYTE 13-16 (HW): hardware id

   CHECKSUM
   -----------------
   BYTE 17-18 (CS): checksum, calculated the same way as for uBlox binary messages
 */

struct DjiVer { // byte offset from beginning of packet, subtract 5 for struct offset
    uint32_t unused1; // BYTE 5-8" ??? (seems to be always 0)
    uint32_t swVersion; // BYTE 9-12 (FW): firmware version
    uint32_t hwVersion; // BYTE 13-16 (HW): hardware id
} __attribute__((packed));
#define VER_FIRST_DECODED_BYTE offsetof(struct DjiVer, swVersion)


typedef union {
    uint8_t payload[0];
    // Nav Class
    struct DjiGps gps;
    struct DjiMag mag;
    struct DjiVer ver;
} DJIPayload;

struct DJIHeader {
    uint8_t id;
    uint8_t len;
    uint8_t checksumA; // these are not part of the dji header, they are actually in the trailer
    uint8_t checksumB; // but they are kept here for parsing ease
} __attribute__((packed));

struct DJIPacket {
    struct DJIHeader header;
    DJIPayload payload;
} __attribute__((packed));

int parse_dji_stream(uint8_t *inputBuffer, uint16_t inputBufferLength, char *parsedDjiStruct, GPSPositionSensorData *GpsData, struct GPS_RX_STATS *GpsRxStats);
void dji_load_mag_settings();

#endif /* DJI_H */
