/**
 ******************************************************************************
 * @file       pios_exbus.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_EXBUS Jeti EX Bus receiver functions
 * @{
 * @brief Jeti EX Bus receiver functions
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
#include "pios_exbus_priv.h"

#if defined(PIOS_INCLUDE_EXBUS)

#if !defined(PIOS_INCLUDE_RTC)
#error PIOS_INCLUDE_RTC must be used to use EXBUS
#endif

/**
 * Based on Jeti EX Bus protocol version 1.21
 * Available at http://www.jetimodel.com/en/Telemetry-Protocol/
 */

/* EXBUS frame size and contents definitions */
#define EXBUS_HEADER_LENGTH       4
#define EXBUS_CRC_LENGTH          2
#define EXBUS_MAX_CHANNELS        16
#define EXBUS_OVERHEAD_LENGTH     (EXBUS_HEADER_LENGTH + EXBUS_CRC_LENGTH)
#define EXBUS_MAX_FRAME_LENGTH    (EXBUS_MAX_CHANNELS * 2 + 10 + EXBUS_OVERHEAD_LENGTH)

#define EXBUS_SYNC_CHANNEL        0x3E
#define EXBUS_SYNC_TELEMETRY      0x3D
#define EXBUS_BYTE_REQ            0x01
#define EXBUS_BYTE_NOREQ          0x03

#define EXBUS_DATA_CHANNEL        0x31
#define EXBUS_DATA_TELEMETRY      0x3A
#define EXBUS_DATA_JETIBOX        0x3B

#define EXBUS_LOW_BAUD_RATE       125000
#define EXBUS_HIGH_BAUD_RATE      250000
#define EXBUS_BAUD_RATE_LIMIT     64
#define EXBUS_FRAME_TIMEOUT       4
#define EXBUS_FAILSAFE_TIMEOUT    64

/* With an Ex.Bus frame rate of 10ms (100Hz) averaging over 20 samples
 * gives about a 200ms response.
 */
#define EXBUS_FL_WEIGHTED_AVERAGE 20

/* Forward Declarations */
static int32_t PIOS_EXBUS_Get(uint32_t rcvr_id, uint8_t channel);
static uint16_t PIOS_EXBUS_RxInCallback(uint32_t context,
                                        uint8_t *buf,
                                        uint16_t buf_len,
                                        uint16_t *headroom,
                                        bool *need_yield);
static void PIOS_EXBUS_Supervisor(uint32_t exbus_id);
static uint16_t PIOS_EXBUS_CRC_Update(uint16_t crc, uint8_t data);
static uint8_t PIOS_EXBUS_Quality_Get(uint32_t rcvr_id);

/* Local Variables */
const struct pios_rcvr_driver pios_exbus_rcvr_driver = {
    .read        = PIOS_EXBUS_Get,
    .get_quality = PIOS_EXBUS_Quality_Get,
};

enum pios_exbus_dev_magic {
    PIOS_EXBUS_DEV_MAGIC = 0x485355FF,
};

enum pios_exbus_frame_state {
    EXBUS_STATE_SYNC,
    EXBUS_STATE_REQ,
    EXBUS_STATE_LEN,
    EXBUS_STATE_PACKET_ID,
    EXBUS_STATE_DATA_ID,
    EXBUS_STATE_SUBLEN,
    EXBUS_STATE_DATA
};

struct pios_exbus_state {
    uint16_t channel_data[PIOS_EXBUS_NUM_INPUTS];
    uint8_t  received_data[EXBUS_MAX_FRAME_LENGTH];
    uint8_t  receive_timer;
    uint8_t  failsafe_timer;
    uint8_t  failsafe_count;
    uint8_t  byte_count;
    uint8_t  frame_length;
    uint16_t crc;
    bool     high_baud_rate;
    bool     frame_found;
    float    quality;
};

struct pios_exbus_dev {
    enum pios_exbus_dev_magic    magic;
    uint32_t com_port_id;
    const struct pios_com_driver *driver;
    struct pios_exbus_state state;
};

/* Allocate EXBUS device descriptor */
static struct pios_exbus_dev *PIOS_EXBUS_Alloc(void)
{
    struct pios_exbus_dev *exbus_dev;

    exbus_dev = (struct pios_exbus_dev *)pios_malloc(sizeof(*exbus_dev));
    if (!exbus_dev) {
        return NULL;
    }

    exbus_dev->magic = PIOS_EXBUS_DEV_MAGIC;
    return exbus_dev;
}

/* Validate EXBUS device descriptor */
static bool PIOS_EXBUS_Validate(struct pios_exbus_dev *exbus_dev)
{
    return exbus_dev->magic == PIOS_EXBUS_DEV_MAGIC;
}

/* Reset channels in case of lost signal or explicit failsafe receiver flag */
static void PIOS_EXBUS_ResetChannels(struct pios_exbus_dev *exbus_dev)
{
    struct pios_exbus_state *state = &(exbus_dev->state);

    for (int i = 0; i < PIOS_EXBUS_NUM_INPUTS; i++) {
        state->channel_data[i] = PIOS_RCVR_TIMEOUT;
    }
}

/* Reset EXBUS receiver state */
static void PIOS_EXBUS_ResetState(struct pios_exbus_dev *exbus_dev)
{
    struct pios_exbus_state *state = &(exbus_dev->state);

    state->receive_timer  = 0;
    state->failsafe_timer = 0;
    state->failsafe_count = 0;
    state->high_baud_rate = false;
    state->frame_found    = false;
    state->quality = 0.0f;
    PIOS_EXBUS_ResetChannels(exbus_dev);
}

/**
 * Check and unroll complete frame data.
 * \output 0 frame data accepted
 * \output -1 frame error found
 */
static int PIOS_EXBUS_UnrollChannels(struct pios_exbus_dev *exbus_dev)
{
    struct pios_exbus_state *state = &(exbus_dev->state);

    if (state->crc != 0) {
        /* crc failed */
        return -1;
    }

    enum pios_exbus_frame_state exbus_state = EXBUS_STATE_SYNC;
    uint8_t *byte      = state->received_data;
    uint8_t sub_length;
    uint8_t n_channels = 0;
    uint8_t channel    = 0;

    while ((byte - state->received_data) < state->frame_length) {
        switch (exbus_state) {
        case EXBUS_STATE_SYNC:
            /* sync byte */
            if (*byte == EXBUS_SYNC_CHANNEL) {
                exbus_state = EXBUS_STATE_REQ;
            } else {
                return -1;
            }
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_REQ:
            /*
             * tells us whether the rx is requesting a reply or not,
             * currently nothing is actually done with this information...
             */
            if (*byte == EXBUS_BYTE_REQ) {
                exbus_state = EXBUS_STATE_LEN;
            } else if (*byte == EXBUS_BYTE_NOREQ) {
                exbus_state = EXBUS_STATE_LEN;
            } else {
                return -1;
            }
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_LEN:
            /* total frame length */
            exbus_state = EXBUS_STATE_PACKET_ID;
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_PACKET_ID:
            /* packet id */
            exbus_state = EXBUS_STATE_DATA_ID;
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_DATA_ID:
            /* checks the type of data, ignore non-rc data */
            if (*byte == EXBUS_DATA_CHANNEL) {
                exbus_state = EXBUS_STATE_SUBLEN;
            } else {
                return -1;
            }
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_SUBLEN:
            sub_length  = *byte;
            n_channels  = sub_length / 2;
            exbus_state = EXBUS_STATE_DATA;
            byte += sizeof(uint8_t);
            break;

        case EXBUS_STATE_DATA:
            if (channel < n_channels) {
                /* 1 lsb = 1/8 us */
                state->channel_data[channel++] = (byte[1] << 8 | byte[0]) / 8;
            }
            byte += sizeof(uint16_t);
            break;
        }
    }
    return 0;
}

/* Update decoder state processing input byte from the stream */
static void PIOS_EXBUS_UpdateState(struct pios_exbus_dev *exbus_dev, uint8_t byte)
{
    struct pios_exbus_state *state = &(exbus_dev->state);

    if (!state->frame_found) {
        if (byte == EXBUS_SYNC_CHANNEL) {
            state->frame_found  = true;
            state->byte_count   = 0;
            state->frame_length = EXBUS_MAX_FRAME_LENGTH;
            state->crc = PIOS_EXBUS_CRC_Update(0, byte);
            state->received_data[state->byte_count++] = byte;
        }
    } else if (state->byte_count < EXBUS_MAX_FRAME_LENGTH) {
        state->crc = PIOS_EXBUS_CRC_Update(state->crc, byte);
        state->received_data[state->byte_count++] = byte;

        if (state->byte_count == 3) {
            state->frame_length = byte;
        }
        if (state->byte_count == state->frame_length) {
            uint8_t quality_trend = 0;
            if (!PIOS_EXBUS_UnrollChannels(exbus_dev)) {
                /* data looking good */
                state->failsafe_timer = 0;
                state->failsafe_count = 0;
                quality_trend = 100;
            }
            // Calculate quality trend using weighted average of good frames
            state->quality     = ((state->quality * (EXBUS_FL_WEIGHTED_AVERAGE - 1)) +
                                  quality_trend) / EXBUS_FL_WEIGHTED_AVERAGE;
            /* get ready for the next frame */
            state->frame_found = false;
        }
    } else {
        state->frame_found = false;
    }
}

/* Initialise EX Bus receiver interface */
int32_t PIOS_EXBUS_Init(uint32_t *exbus_id,
                        const struct pios_com_driver *driver,
                        uint32_t lower_id)
{
    PIOS_DEBUG_Assert(exbus_id);
    PIOS_DEBUG_Assert(driver);

    struct pios_exbus_dev *exbus_dev;

    exbus_dev = (struct pios_exbus_dev *)PIOS_EXBUS_Alloc();
    if (!exbus_dev) {
        return -1;
    }

    PIOS_EXBUS_ResetState(exbus_dev);

    *exbus_id = (uint32_t)exbus_dev;

    /* Set comm driver parameters */
    PIOS_DEBUG_Assert(driver->set_config);
    driver->set_config(lower_id, PIOS_COM_Word_length_8b, PIOS_COM_Parity_No, PIOS_COM_StopBits_1, 125000);

    /* Set irq priority */
    if (driver->ioctl) {
        uint8_t irq_prio = PIOS_IRQ_PRIO_HIGH;
        driver->ioctl(lower_id, PIOS_IOCTL_USART_SET_IRQ_PRIO, &irq_prio);
    }

    /* Set comm driver callback */
    driver->bind_rx_cb(lower_id, PIOS_EXBUS_RxInCallback, *exbus_id);

    if (!PIOS_RTC_RegisterTickCallback(PIOS_EXBUS_Supervisor, *exbus_id)) {
        PIOS_DEBUG_Assert(0);
    }

    exbus_dev->driver = driver;
    exbus_dev->com_port_id = lower_id;

    return 0;
}

/* Comm byte received callback */
static uint16_t PIOS_EXBUS_RxInCallback(uint32_t context,
                                        uint8_t *buf,
                                        uint16_t buf_len,
                                        uint16_t *headroom,
                                        bool *need_yield)
{
    struct pios_exbus_dev *exbus_dev = (struct pios_exbus_dev *)context;

    bool valid = PIOS_EXBUS_Validate(exbus_dev);

    PIOS_Assert(valid);

    /* process byte(s) and clear receive timer */
    for (uint8_t i = 0; i < buf_len; i++) {
        PIOS_EXBUS_UpdateState(exbus_dev, buf[i]);
        exbus_dev->state.receive_timer = 0;
    }

    /* Always signal that we can accept more data */
    if (headroom) {
        *headroom = EXBUS_MAX_FRAME_LENGTH;
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
static int32_t PIOS_EXBUS_Get(uint32_t rcvr_id, uint8_t channel)
{
    struct pios_exbus_dev *exbus_dev = (struct pios_exbus_dev *)rcvr_id;

    if (!PIOS_EXBUS_Validate(exbus_dev)) {
        return PIOS_RCVR_INVALID;
    }

    /* return error if channel is not available */
    if (channel >= PIOS_EXBUS_NUM_INPUTS) {
        return PIOS_RCVR_INVALID;
    }

    /* may also be PIOS_RCVR_TIMEOUT set by other function */
    return exbus_dev->state.channel_data[channel];
}

static void PIOS_EXBUS_Change_BaudRate(struct pios_exbus_dev *device)
{
    struct pios_exbus_state *state = &(device->state);

    if (++state->failsafe_count >= EXBUS_BAUD_RATE_LIMIT) {
        state->high_baud_rate = !state->high_baud_rate;
        (device->driver->set_baud)(device->com_port_id,
                                   state->high_baud_rate ? EXBUS_HIGH_BAUD_RATE : EXBUS_LOW_BAUD_RATE);
        state->failsafe_count = 0;
    }
}

/**
 * Input data supervisor is called periodically and provides
 * two functions: frame syncing and failsafe triggering.
 *
 * EX.Bus frames come at 20ms or 10ms rate at 125 or 250 kbaud.
 * RTC timer is running at 625Hz (1.6ms). So with divider 5 it gives
 * 8ms pause between frames which is good for both EX.Bus frame rates.
 *
 * Data receive function must clear the receive_timer to confirm new
 * data reception. If no new data received in 100ms, we must call the
 * failsafe function which clears all channels.
 */
static void PIOS_EXBUS_Supervisor(uint32_t exbus_id)
{
    struct pios_exbus_dev *exbus_dev = (struct pios_exbus_dev *)exbus_id;

    bool valid = PIOS_EXBUS_Validate(exbus_dev);

    PIOS_Assert(valid);

    struct pios_exbus_state *state = &(exbus_dev->state);

    /* waiting for new frame if no bytes were received in 8ms */
    if (++state->receive_timer > EXBUS_FRAME_TIMEOUT) {
        state->frame_found   = false;
        state->byte_count    = 0;
        state->receive_timer = 0;
        state->frame_length  = EXBUS_MAX_FRAME_LENGTH;
    }

    /* activate failsafe if no frames have arrived in 102.4ms */
    if (++state->failsafe_timer > EXBUS_FAILSAFE_TIMEOUT) {
        PIOS_EXBUS_ResetChannels(exbus_dev);
        state->failsafe_timer = 0;
        state->quality = 0.0f;
        PIOS_EXBUS_Change_BaudRate(exbus_dev);
    }
}

static uint8_t PIOS_EXBUS_Quality_Get(uint32_t exbus_id)
{
    struct pios_exbus_dev *exbus_dev = (struct pios_exbus_dev *)exbus_id;

    bool valid = PIOS_EXBUS_Validate(exbus_dev);

    PIOS_Assert(valid);

    struct pios_exbus_state *state = &(exbus_dev->state);

    return (uint8_t)(state->quality + 0.5f);
}

static uint16_t PIOS_EXBUS_CRC_Update(uint16_t crc, uint8_t data)
{
    data ^= (uint8_t)crc & (uint8_t)0xFF;
    data ^= data << 4;
    crc   = ((((uint16_t)data << 8) | ((crc & 0xFF00) >> 8))
             ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));

    return crc;
}

#endif /* PIOS_INCLUDE_EXBUS */

/**
 * @}
 * @}
 */
