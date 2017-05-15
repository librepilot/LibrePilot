/**
 ******************************************************************************
 * @file       pios_hott.c
 * @author     The LibrePilot Project, http://www.librepilot.org, Copyright (c) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_HOTT Graupner HoTT receiver functions
 * @{
 * @brief Graupner HoTT receiver functions for SUMD/H
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
/* Project Includes */
#include "pios_hott_priv.h"

#if defined(PIOS_INCLUDE_HOTT)

#if !defined(PIOS_INCLUDE_RTC)
#error PIOS_INCLUDE_RTC must be used to use HOTT
#endif

/**
 * HOTT protocol documentation
 *
 * Currently known Graupner HoTT serial port settings:
 *  115200bps serial stream, 8 bits, no parity, 1 stop bit
 *  size of each frame: 11..37 bytes
 *  data resolution: 14 bit
 *  frame period: 11ms or 22ms
 *
 * Currently known SUMD/SUMH frame structure:
 *  Section          Byte_Number        Byte_Name      Byte_Value Remark
 *  Header           0                  Vendor_ID      0xA8       Graupner
 *  Header           1                  Status         0x00       valid and live SUMH data frame
 *                                                     0x01       valid and live SUMD data frame
 *                                                     0x81       valid SUMD/H data frame with
 *                                                                transmitter in fail safe condition
 *                                                     others     invalid frame
 *  Header           2                  N_Channels     0x02..0x20 number of transmitted channels
 *  Data             n*2+1              Channel n MSB  0x00..0xff High Byte of channel n data
 *  Data             n*2+2              Channel n LSB  0x00..0xff Low Byte of channel n data
 *  SUMD_CRC         (N_Channels+1)*2+1 CRC High Byte  0x00..0xff High Byte of 16 Bit CRC
 *  SUMD_CRC         (N_Channels+1)*2+2 CRC Low Byte   0x00..0xff Low Byte of 16 Bit CRC
 *  SUMH_Telemetry   (N_Channels+1)*2+1 Telemetry_Req  0x00..0xff 0x00 no telemetry request
 *  SUMH_CRC         (N_Channels+1)*2+2 CRC Byte       0x00..0xff Low Byte of all added data bytes

 * Channel Data Interpretation
 *  Stick Positon    Channel Data Remark
 *  ext. low (-150%) 0x1c20       900µs
 *  low (-100%)      0x2260       1100µs
 *  neutral (0%)     0x2ee0       1500µs
 *  high (100%)      0x3b60       1900µs
 *  ext. high(150%)  0x41a0       2100µs

 * Channel Mapping (not sure)
 *  1 Pitch
 *  2 Aileron
 *  3 Elevator
 *  4 Yaw
 *  5 Aux/Gyro on MX-12
 *  6 ESC
 *  7 Aux/Gyr
 */

/* HOTT frame size and contents definitions */
#define HOTT_HEADER_LENGTH          3
#define HOTT_CRC_LENGTH             2
#define HOTT_MAX_CHANNELS_PER_FRAME 32
#define HOTT_OVERHEAD_LENGTH        (HOTT_HEADER_LENGTH + HOTT_CRC_LENGTH)
#define HOTT_MAX_FRAME_LENGTH       (HOTT_MAX_CHANNELS_PER_FRAME * 2 + HOTT_OVERHEAD_LENGTH)

#define HOTT_GRAUPNER_ID            0xA8
#define HOTT_STATUS_LIVING_SUMH     0x00
#define HOTT_STATUS_LIVING_SUMD     0x01
#define HOTT_STATUS_FAILSAFE        0x81
#define HOTT_FRAME_TIMEOUT          4
#define HOTT_FAILSAFE_TIMEOUT       64

/* With an Ex.Bus frame rate of 11/22ms (90/45Hz) averaging over 15 samples
 * gives about a 165/330ms response.
 */
#define HOTT_FL_WEIGHTED_AVERAGE    20


/* Forward Declarations */
static int32_t PIOS_HOTT_Get(uint32_t rcvr_id, uint8_t channel);
static uint16_t PIOS_HOTT_RxInCallback(uint32_t context,
                                       uint8_t *buf,
                                       uint16_t buf_len,
                                       uint16_t *headroom,
                                       bool *need_yield);
static void PIOS_HOTT_Supervisor(uint32_t hott_id);
static uint8_t PIOS_HOTT_Quality_Get(uint32_t rcvr_id);

/* Local Variables */
const struct pios_rcvr_driver pios_hott_rcvr_driver = {
    .read        = PIOS_HOTT_Get,
    .get_quality = PIOS_HOTT_Quality_Get,
};

enum pios_hott_dev_magic {
    PIOS_HOTT_DEV_MAGIC = 0x4853554D,
};

struct pios_hott_state {
    uint16_t channel_data[PIOS_HOTT_NUM_INPUTS];
    uint8_t  received_data[HOTT_MAX_FRAME_LENGTH];
    uint8_t  receive_timer;
    uint8_t  failsafe_timer;
    uint8_t  frame_found;
    uint8_t  tx_connected;
    uint8_t  byte_count;
    uint8_t  frame_length;
    float    quality;
};

struct pios_hott_dev {
    enum pios_hott_dev_magic   magic;
    const struct pios_hott_cfg *cfg;
    enum pios_hott_proto proto;
    struct pios_hott_state     state;
};

/* Allocate HOTT device descriptor */
static struct pios_hott_dev *PIOS_HOTT_Alloc(void)
{
    struct pios_hott_dev *hott_dev;

    hott_dev = (struct pios_hott_dev *)pios_malloc(sizeof(*hott_dev));
    if (!hott_dev) {
        return NULL;
    }

    hott_dev->magic = PIOS_HOTT_DEV_MAGIC;
    return hott_dev;
}

/* Validate HOTT device descriptor */
static bool PIOS_HOTT_Validate(struct pios_hott_dev *hott_dev)
{
    return hott_dev->magic == PIOS_HOTT_DEV_MAGIC;
}

/* Reset channels in case of lost signal or explicit failsafe receiver flag */
static void PIOS_HOTT_ResetChannels(struct pios_hott_state *state)
{
    for (int i = 0; i < PIOS_HOTT_NUM_INPUTS; i++) {
        state->channel_data[i] = PIOS_RCVR_TIMEOUT;
    }
}

/* Reset HOTT receiver state */
static void PIOS_HOTT_ResetState(struct pios_hott_state *state)
{
    state->receive_timer  = 0;
    state->failsafe_timer = 0;
    state->frame_found    = 0;
    state->tx_connected   = 0;
    state->quality = 0.0f;
    PIOS_HOTT_ResetChannels(state);
}

/**
 * Check and unroll complete frame data.
 * \output 0 frame data accepted
 * \output -1 frame error found
 */
static int PIOS_HOTT_UnrollChannels(struct pios_hott_dev *hott_dev)
{
    struct pios_hott_state *state = &(hott_dev->state);

    /* check the header and crc for a valid HoTT SUM stream */
    uint8_t vendor = state->received_data[0];
    uint8_t status = state->received_data[1];

    if (vendor != HOTT_GRAUPNER_ID) {
        /* Graupner ID was expected */
        goto stream_error;
    }

    switch (status) {
    case HOTT_STATUS_LIVING_SUMH:
    case HOTT_STATUS_LIVING_SUMD:
    case HOTT_STATUS_FAILSAFE:
        /* check crc before processing */
        if (hott_dev->proto == PIOS_HOTT_PROTO_SUMD) {
            /* SUMD has 16 bit CCITT CRC */
            uint16_t crc = 0;
            uint8_t *s   = &(state->received_data[0]);
            int len = state->byte_count - 2;
            for (int n = 0; n < len; n++) {
                crc ^= (uint16_t)s[n] << 8;
                for (int i = 0; i < 8; i++) {
                    crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
                }
            }
            if (crc ^ (((uint16_t)s[len] << 8) | s[len + 1])) {
                /* wrong crc checksum found */
                goto stream_error;
            }
        }
        if (hott_dev->proto == PIOS_HOTT_PROTO_SUMH) {
            /* SUMH has only 8 bit added CRC */
            uint8_t crc = 0;
            uint8_t *s  = &(state->received_data[0]);
            int len     = state->byte_count - 1;
            for (int n = 0; n < len; n++) {
                crc += s[n];
            }
            if (crc ^ s[len]) {
                /* wrong crc checksum found */
                goto stream_error;
            }
        }
        /* check for a living connect */
        state->tx_connected |= (status != HOTT_STATUS_FAILSAFE);
        break;
    default:
        /* wrong header format */
        goto stream_error;
    }

    /* check initial connection since reset or timeout */
    if (!(state->tx_connected)) {
        /* these are failsafe data without a first connect. ignore it */
        PIOS_HOTT_ResetChannels(state);
        return 0;
    }

    /* unroll channels */
    uint8_t n_channels = state->received_data[2];
    uint8_t *s = &(state->received_data[3]);
    uint16_t word;

    for (int i = 0; i < HOTT_MAX_CHANNELS_PER_FRAME; i++) {
        if (i < n_channels) {
            word = ((uint16_t)s[0] << 8) | s[1];
            s   += sizeof(uint16_t);
            /* save the channel value */
            if (i < PIOS_HOTT_NUM_INPUTS) {
                /* floating version. channel limits from -100..+100% are mapped to 1000..2000 */
                state->channel_data[i] = (uint16_t)(word / 6.4f - 375);
            }
        } else {
            /* this channel was not received */
            state->channel_data[i] = PIOS_RCVR_INVALID;
        }
    }

    /* all channels processed */
    return 0;

stream_error:
    /* either SUMD selected with SUMH stream found, or vice-versa */
    return -1;
}

/* Update decoder state processing input byte from the HoTT stream */
static void PIOS_HOTT_UpdateState(struct pios_hott_dev *hott_dev, uint8_t byte)
{
    struct pios_hott_state *state = &(hott_dev->state);

    if (state->frame_found) {
        /* receiving the data frame */
        if (state->byte_count < HOTT_MAX_FRAME_LENGTH) {
            /* store next byte */
            state->received_data[state->byte_count++] = byte;
            if (state->byte_count == HOTT_HEADER_LENGTH) {
                /* 3rd byte contains the number of channels. calculate frame size */
                state->frame_length = HOTT_OVERHEAD_LENGTH + 2 * byte;
            }
            if (state->byte_count == state->frame_length) {
                uint8_t quality_trend = 0;
                /* full frame received - process and wait for new one */
                if (!PIOS_HOTT_UnrollChannels(hott_dev)) {
                    /* data looking good */
                    state->failsafe_timer = 0;
                    quality_trend = 100;
                }
                // Calculate quality trend using weighted average of good frames
                state->quality = ((state->quality * (HOTT_FL_WEIGHTED_AVERAGE - 1)) +
                                  quality_trend) / HOTT_FL_WEIGHTED_AVERAGE;

                /* prepare for the next frame */
                state->frame_found = 0;
            }
        }
    }
}

/* Initialise HoTT receiver interface */
int32_t PIOS_HOTT_Init(uint32_t *hott_id,
                       const struct pios_com_driver *driver,
                       uint32_t lower_id,
                       enum pios_hott_proto proto)
{
    PIOS_DEBUG_Assert(hott_id);
    PIOS_DEBUG_Assert(driver);

    struct pios_hott_dev *hott_dev;

    hott_dev = (struct pios_hott_dev *)PIOS_HOTT_Alloc();
    if (!hott_dev) {
        return -1;
    }

    /* Bind the configuration to the device instance */
    hott_dev->proto = proto;

    PIOS_HOTT_ResetState(&(hott_dev->state));

    *hott_id = (uint32_t)hott_dev;

    /* Set comm driver parameters */
    PIOS_DEBUG_Assert(driver->set_config);
    driver->set_config(lower_id, PIOS_COM_Word_length_8b, PIOS_COM_Parity_No, PIOS_COM_StopBits_1, 115200);

    /* Set irq priority */
    if (driver->ioctl) {
        uint8_t irq_prio = PIOS_IRQ_PRIO_HIGH;
        driver->ioctl(lower_id, PIOS_IOCTL_USART_SET_IRQ_PRIO, &irq_prio);
    }

    /* Set comm driver callback */
    (driver->bind_rx_cb)(lower_id, PIOS_HOTT_RxInCallback, *hott_id);

    if (!PIOS_RTC_RegisterTickCallback(PIOS_HOTT_Supervisor, *hott_id)) {
        PIOS_DEBUG_Assert(0);
    }

    return 0;
}

/* Comm byte received callback */
static uint16_t PIOS_HOTT_RxInCallback(uint32_t context,
                                       uint8_t *buf,
                                       uint16_t buf_len,
                                       uint16_t *headroom,
                                       bool *need_yield)
{
    struct pios_hott_dev *hott_dev = (struct pios_hott_dev *)context;

    bool valid = PIOS_HOTT_Validate(hott_dev);

    PIOS_Assert(valid);

    /* process byte(s) and clear receive timer */
    for (uint8_t i = 0; i < buf_len; i++) {
        PIOS_HOTT_UpdateState(hott_dev, buf[i]);
        hott_dev->state.receive_timer = 0;
    }

    /* Always signal that we can accept more data */
    if (headroom) {
        *headroom = HOTT_MAX_FRAME_LENGTH;
    }

    /* We never need a yield */
    *need_yield = false;

    /* Always indicate that all bytes were consumed */
    return buf_len;
}

/**
 * Get the value of an input channel
 * \param[in] channel Number of the channel desired (zero based)
 * \output PIOS_RCVR_INVALID channel not available
 * \output PIOS_RCVR_TIMEOUT failsafe condition or missing receiver
 * \output >=0 channel value
 */
static int32_t PIOS_HOTT_Get(uint32_t rcvr_id, uint8_t channel)
{
    struct pios_hott_dev *hott_dev = (struct pios_hott_dev *)rcvr_id;

    if (!PIOS_HOTT_Validate(hott_dev)) {
        return PIOS_RCVR_INVALID;
    }

    /* return error if channel is not available */
    if (channel >= PIOS_HOTT_NUM_INPUTS) {
        return PIOS_RCVR_INVALID;
    }

    /* may also be PIOS_RCVR_TIMEOUT set by other function */
    return hott_dev->state.channel_data[channel];
}

static uint8_t PIOS_HOTT_Quality_Get(uint32_t hott_id)
{
    struct pios_hott_dev *hott_dev = (struct pios_hott_dev *)hott_id;

    bool valid = PIOS_HOTT_Validate(hott_dev);

    PIOS_Assert(valid);

    struct pios_hott_state *state = &(hott_dev->state);

    return (uint8_t)(state->quality + 0.5f);
}

/**
 * Input data supervisor is called periodically and provides
 * two functions: frame syncing and failsafe triggering.
 *
 * HOTT frames come at 11ms or 22ms rate at 115200bps.
 * RTC timer is running at 625Hz (1.6ms). So with divider 5 it gives
 * 8ms pause between frames which is good for both HOTT frame rates.
 *
 * Data receive function must clear the receive_timer to confirm new
 * data reception. If no new data received in 100ms, we must call the
 * failsafe function which clears all channels.
 */
static void PIOS_HOTT_Supervisor(uint32_t hott_id)
{
    struct pios_hott_dev *hott_dev = (struct pios_hott_dev *)hott_id;

    bool valid = PIOS_HOTT_Validate(hott_dev);

    PIOS_Assert(valid);

    struct pios_hott_state *state = &(hott_dev->state);

    /* waiting for new frame if no bytes were received in 8ms */
    if (++state->receive_timer > HOTT_FRAME_TIMEOUT) {
        state->frame_found   = 1;
        state->byte_count    = 0;
        state->receive_timer = 0;
        state->frame_length  = HOTT_MAX_FRAME_LENGTH;
    }

    /* activate failsafe if no frames have arrived in 102.4ms */
    if (++state->failsafe_timer > HOTT_FAILSAFE_TIMEOUT) {
        PIOS_HOTT_ResetChannels(state);
        state->failsafe_timer = 0;
        state->tx_connected   = 0;
        state->quality = 0.0f;
    }
}

#endif /* PIOS_INCLUDE_HOTT */

/**
 * @}
 * @}
 */
