/**
 ******************************************************************************
 * @file       pios_ibus.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_IBus PiOS IBus receiver driver
 * @{
 * @brief Receives and decodes IBus protocol reciever packets
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

#include "pios_ibus_priv.h"

#ifdef PIOS_INCLUDE_IBUS

/*
    iBus packet format learned from: https://bitbucket.org/daveborthwick/ibus_pic12f1572

    An iBus packet consists of:
    - the total packet length (1 byte)
    - command / type field (1 byte)
    - sequence of 16-bit (little endian) variables (2 bytes each)
    - checksum (2 bytes)

    The most common packet length is 0x20 = 32 bytes
    A packet of this type has enough room to transport data for 14 channels.

    The command / type field tells you the meaning of the next variables.
    This field is set to 0x40 for RC command channels.
    We must verify this field to be 0x40 so we can be sure that the next fields
    can be interpreted as RC command channels.

    The checksum equals 0xFFFF minus the sum of all previous bytes in the packet.

 */

// 1 length byte, 1 type byte, 10x channels (uint16_t), 4 extra channels (unint16_t), 2 crc bytes
// We only decode the first 10 channels at this time.
#define PIOS_IBUS_BUFLEN   (1 + 1 + PIOS_IBUS_NUM_INPUTS * 2 + (14 - PIOS_IBUS_NUM_INPUTS) * 2 + 2)
#define PIOS_IBUS_SYNCBYTE 0x20 // sync on the packet length byte
#define PIOS_IBUS_TYPEBYTE 0x40
#define PIOS_IBUS_MAGIC    0x84fd9a39

// make sure to always allocate sufficient bufferspace:
#if (PIOS_IBUS_SYNCBYTE != PIOS_IBUS_BUFLEN)
#error PIOS_IBUS_BUFLEN must be updated to match the expected packet length
#endif

/**
 * @brief IBus receiver driver internal state data
 */
struct pios_ibus_dev {
    uint32_t magic;
    int      buf_pos;
    int      rx_timer;
    int      failsafe_timer;
    uint16_t checksum;
    uint16_t channel_data[PIOS_IBUS_NUM_INPUTS];
    uint8_t  rx_buf[PIOS_IBUS_BUFLEN];
};

/**
 * @brief Allocates a driver instance
 * @retval pios_ibus_dev pointer on success, NULL on failure
 */
static struct pios_ibus_dev *PIOS_IBUS_Alloc(void);
/**
 * @brief Validate a driver instance
 * @param[in] dev device driver instance pointer
 * @retval true on success, false on failure
 */
static bool PIOS_IBUS_Validate(const struct pios_ibus_dev *ibus_dev);
/**
 * @brief Read a channel from the last received frame
 * @param[in] id Driver instance
 * @param[in] channel 0-based channel index
 * @retval raw channel value, or error value (see pios_rcvr.h)
 */
static int32_t PIOS_IBUS_Read(uint32_t id, uint8_t channel);
/**
 * @brief Set all channels in the last frame buffer to a given value
 * @param[in] dev Driver instance
 * @param[in] value channel value
 */
static void PIOS_IBUS_SetAllChannels(struct pios_ibus_dev *ibus_dev, uint16_t value);
/**
 * @brief Serial receive callback
 * @param[in] context Driver instance handle
 * @param[in] buf Receive buffer
 * @param[in] buf_len Number of bytes available
 * @param[out] headroom Number of bytes remaining in internal buffer
 * @param[out] task_woken Did we awake a task?
 * @retval Number of bytes consumed from the buffer
 */
static uint16_t PIOS_IBUS_Receive(uint32_t context, uint8_t *buf, uint16_t buf_len,
                                  uint16_t *headroom, bool *task_woken);
/**
 * @brief Reset the internal receive buffer state
 * @param[in] ibus_dev device driver instance pointer
 */
static void PIOS_IBUS_ResetBuffer(struct pios_ibus_dev *ibus_dev);
/**
 * @brief Unpack a frame from the internal receive buffer to the channel buffer
 * @param[in] ibus_dev device driver instance pointer
 */
static void PIOS_IBUS_UnpackFrame(struct pios_ibus_dev *ibus_dev);
/**
 * @brief RTC tick callback
 * @param[in] context Driver instance handle
 */
static void PIOS_IBUS_Supervisor(uint32_t context);

// public
const struct pios_rcvr_driver pios_ibus_rcvr_driver = {
    .read = PIOS_IBUS_Read,
};


static struct pios_ibus_dev *PIOS_IBUS_Alloc(void)
{
    struct pios_ibus_dev *ibus_dev;

    ibus_dev = (struct pios_ibus_dev *)pios_malloc(sizeof(*ibus_dev));

    if (!ibus_dev) {
        return NULL;
    }

    memset(ibus_dev, 0, sizeof(*ibus_dev));
    ibus_dev->magic = PIOS_IBUS_MAGIC;

    return ibus_dev;
}

static bool PIOS_IBUS_Validate(const struct pios_ibus_dev *ibus_dev)
{
    return ibus_dev && ibus_dev->magic == PIOS_IBUS_MAGIC;
}

int32_t PIOS_IBUS_Init(uint32_t *ibus_id, const struct pios_com_driver *driver,
                       uint32_t lower_id)
{
    struct pios_ibus_dev *ibus_dev = PIOS_IBUS_Alloc();

    if (!ibus_dev) {
        return -1;
    }

    *ibus_id = (uint32_t)ibus_dev;

    PIOS_IBUS_SetAllChannels(ibus_dev, PIOS_RCVR_INVALID);

    if (!PIOS_RTC_RegisterTickCallback(PIOS_IBUS_Supervisor, *ibus_id)) {
        PIOS_Assert(0);
    }

    /* Set comm driver parameters */
    PIOS_DEBUG_Assert(driver->set_config);
    driver->set_config(lower_id, PIOS_COM_Word_length_8b, PIOS_COM_Parity_No, PIOS_COM_StopBits_1, 115200);

    /* Set irq priority */
    if (driver->ioctl) {
        uint8_t irq_prio = PIOS_IRQ_PRIO_HIGH;
        driver->ioctl(lower_id, PIOS_IOCTL_USART_SET_IRQ_PRIO, &irq_prio);
    }

    driver->bind_rx_cb(lower_id, PIOS_IBUS_Receive, *ibus_id);

    return 0;
}

static int32_t PIOS_IBUS_Read(uint32_t context, uint8_t channel)
{
    if (channel > PIOS_IBUS_NUM_INPUTS) {
        return PIOS_RCVR_INVALID;
    }

    struct pios_ibus_dev *ibus_dev = (struct pios_ibus_dev *)context;
    if (!PIOS_IBUS_Validate(ibus_dev)) {
        return PIOS_RCVR_NODRIVER;
    }

    return ibus_dev->channel_data[channel];
}

static void PIOS_IBUS_SetAllChannels(struct pios_ibus_dev *ibus_dev, uint16_t value)
{
    for (int i = 0; i < PIOS_IBUS_NUM_INPUTS; i++) {
        ibus_dev->channel_data[i] = value;
    }
}

static uint16_t PIOS_IBUS_Receive(uint32_t context, uint8_t *buf, uint16_t buf_len,
                                  uint16_t *headroom, bool *task_woken)
{
    struct pios_ibus_dev *ibus_dev = (struct pios_ibus_dev *)context;

    if (!PIOS_IBUS_Validate(ibus_dev)) {
        goto out_fail;
    }

    for (int i = 0; i < buf_len; i++) {
        // byte 0: sync byte (packet length) should match:
        if (ibus_dev->buf_pos == 0 && buf[i] != PIOS_IBUS_SYNCBYTE) {
            continue;
        }
        // byte 1: type field should match:
        if (ibus_dev->buf_pos == 1 && buf[i] != PIOS_IBUS_TYPEBYTE) {
            // not the correct type of data, restart from byte 0
            ibus_dev->buf_pos = 0;
            continue;
        }

        ibus_dev->rx_buf[ibus_dev->buf_pos++] = buf[i];
        if (ibus_dev->buf_pos <= PIOS_IBUS_BUFLEN - 2) {
            ibus_dev->checksum -= buf[i];
        } else if (ibus_dev->buf_pos == PIOS_IBUS_BUFLEN) {
            PIOS_IBUS_UnpackFrame(ibus_dev);
        }
    }

    ibus_dev->rx_timer = 0;

    *headroom   = PIOS_IBUS_BUFLEN - ibus_dev->buf_pos;
    *task_woken = false;
    return buf_len;

out_fail:
    *headroom   = 0;
    *task_woken = false;
    return 0;
}

static void PIOS_IBUS_ResetBuffer(struct pios_ibus_dev *ibus_dev)
{
    ibus_dev->checksum = 0xffff;
    ibus_dev->buf_pos  = 0;
}

static void PIOS_IBUS_UnpackFrame(struct pios_ibus_dev *ibus_dev)
{
    uint16_t rxsum = ibus_dev->rx_buf[PIOS_IBUS_BUFLEN - 1] << 8 |
                     ibus_dev->rx_buf[PIOS_IBUS_BUFLEN - 2];

    if (ibus_dev->checksum != rxsum) {
        goto out_fail;
    }

    uint16_t *chan = (uint16_t *)&ibus_dev->rx_buf[2];
    for (int i = 0; i < PIOS_IBUS_NUM_INPUTS; i++) {
        ibus_dev->channel_data[i] = *chan++;
    }

    ibus_dev->failsafe_timer = 0;

out_fail:
    PIOS_IBUS_ResetBuffer(ibus_dev);
}

static void PIOS_IBUS_Supervisor(uint32_t context)
{
    struct pios_ibus_dev *ibus_dev = (struct pios_ibus_dev *)context;

    PIOS_Assert(PIOS_IBUS_Validate(ibus_dev));

    // clear rx buffer after 4.8 ms without input
    if (++ibus_dev->rx_timer > 3) {
        PIOS_IBUS_ResetBuffer(ibus_dev);
    }

    // mark all channels invalid after 51.2 ms without valid data
    if (++ibus_dev->failsafe_timer > 32) {
        PIOS_IBUS_SetAllChannels(ibus_dev, PIOS_RCVR_TIMEOUT);
    }
}

#endif // PIOS_INCLUDE_IBUS

/**
 * @}
 * @}
 */
