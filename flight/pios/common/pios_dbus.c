/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DBus DJI DBus receiver functions
 * @brief Code to read DJI DBus receiver serial stream
 * @{
 *
 * @file       pios_dbus.c
 * @author     The SantyPilot Team, copyright (C) 2023.
 * @brief      Code to read DJI DBus receiver serial stream
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

#include "pios.h"

#ifdef PIOS_INCLUDE_DBUS

#define DBUS_RX_BUF_NUM   36u // avoid overflow
#define DBUS_FRAME_LENGTH    18u
#define DBUS_CH_VALUE_OFFSET ((uint16_t)1024) 
#define DBUS_CHANNEL_ERROR_VALUE 700
#define DBUS_SW_DOWN       2

#ifndef PIOS_DBUS_BAUD_RATE
#define PIOS_DBUS_BAUD_RATE 100000
#endif // PIOS_DBUS_BAUD_RATE

#include <uavobjectmanager.h>
#include "pios_dbus_priv.h"

/* Forward Declarations */
static int32_t PIOS_DBus_Get(uint32_t rcvr_id, uint8_t channel);
static uint16_t PIOS_DBus_RxInCallback(uint32_t context,
                                       uint8_t *buf,
                                       uint16_t buf_len,
                                       uint16_t *headroom,
                                       bool *need_yield);
static uint8_t PIOS_DBus_Quality_Get(uint32_t rcvr_id);

/* Local Variables */
const struct pios_rcvr_driver pios_dbus_rcvr_driver = {
    .read        = PIOS_DBus_Get,
    .get_quality = PIOS_DBus_Quality_Get
};

enum pios_dbus_dev_magic {
    PIOS_DBUS_DEV_MAGIC = 0x53427600,
};

struct pios_dbus_state {
    uint16_t channel_data[PIOS_DBUS_NUM_INPUTS];
    uint8_t  received_data[DBUS_FRAME_LENGTH];
    float    quality;
};

struct pios_dbus_dev {
    enum pios_dbus_dev_magic magic;
    struct pios_dbus_state   state;
};

/* Allocate DBus device descriptor */
#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_dbus_dev *PIOS_DBus_Alloc(void)
{
    struct pios_dbus_dev *dbus_dev;

    dbus_dev = (struct pios_dbus_dev *)pios_malloc(sizeof(*dbus_dev));
    if (!dbus_dev) {
        return NULL;
    }

    dbus_dev->magic = PIOS_DBUS_DEV_MAGIC;
    return dbus_dev;
}
#else // mem malloc interface differs
static struct pios_dbus_dev pios_dbus_devs[PIOS_DBUS_MAX_DEVS];
static uint8_t pios_dbus_num_devs;
static struct pios_dbus_dev *PIOS_DBus_Alloc(void)
{
    struct pios_dbus_dev *dbus_dev;

    if (pios_dbus_num_devs >= PIOS_DBUS_MAX_DEVS) {
        return NULL;
    }

    dbus_dev = &pios_dbus_devs[pios_dbus_num_devs++];
    dbus_dev->magic = PIOS_DBUS_DEV_MAGIC;

    return dbus_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */

/* Validate DBus device descriptor */
static bool PIOS_DBus_Validate(struct pios_dbus_dev *dbus_dev)
{
    return dbus_dev->magic == PIOS_DBUS_DEV_MAGIC;
}

/* Reset channels in case of lost signal or explicit failsafe receiver flag */
static void PIOS_DBus_ResetChannels(struct pios_dbus_state *state)
{
    for (int i = 0; i < PIOS_DBUS_NUM_INPUTS; i++) {
        state->channel_data[i] = PIOS_RCVR_TIMEOUT;
        state->quality = 0.0f;
    }
}

/* Reset DBus receiver state */
static void PIOS_DBus_ResetState(struct pios_dbus_state *state)
{
    state->quality = 0.0f;
    PIOS_DBus_ResetChannels(state);
}

/* Initialise DBus receiver interface */
int32_t PIOS_DBus_Init(uint32_t *dbus_id,
                       const struct pios_com_driver *driver,
                       uint32_t lower_id) {
    PIOS_DEBUG_Assert(dbus_id);
    PIOS_DEBUG_Assert(driver);

    struct pios_dbus_dev *dbus_dev;

    dbus_dev = (struct pios_dbus_dev *)PIOS_DBus_Alloc();
    if (!dbus_dev) {
		return -1;
    }

    PIOS_DBus_ResetState(&(dbus_dev->state));

    *dbus_id = (uint32_t)dbus_dev;

    /* Set rest of the parameters and enable */
	// there is one USART_Init before
    if (driver->set_config) {
        driver->set_config(lower_id, PIOS_COM_Word_length_8b, PIOS_COM_Parity_Even, PIOS_COM_StopBits_1, PIOS_DBUS_BAUD_RATE);
    }

    /* Set inverted UART and IRQ priority */
    //if (driver->ioctl) {
    //    enum PIOS_USART_Inverted param = cfg->non_inverted ? 0 : PIOS_USART_Inverted_Rx;
    //    driver->ioctl(lower_id, PIOS_IOCTL_USART_SET_INVERTED, &param);

    //    uint8_t irq_prio = PIOS_IRQ_PRIO_HIGH;
    //    driver->ioctl(lower_id, PIOS_IOCTL_USART_SET_IRQ_PRIO, &irq_prio);
    //}

    /* Set comm driver callback */
    driver->bind_rx_cb(lower_id, PIOS_DBus_RxInCallback, *dbus_id);

    //if (!PIOS_RTC_RegisterTickCallback(PIOS_DBus_Supervisor, *dbus_id)) {
    //    PIOS_DEBUG_Assert(0);
    //}

    return 0;
}

/**
 * Get the value of an input channel
 * \param[in] channel Number of the channel desired (zero based)
 * \output PIOS_RCVR_INVALID channel not available
 * \output PIOS_RCVR_TIMEOUT failsafe condition or missing receiver
 * \output >=0 channel value
 */
static int32_t PIOS_DBus_Get(uint32_t rcvr_id, uint8_t channel)
{
    struct pios_dbus_dev *dbus_dev = (struct pios_dbus_dev *)rcvr_id;

    if (!PIOS_DBus_Validate(dbus_dev)) {
        return PIOS_RCVR_INVALID;
    }

    /* return error if channel is not available */
    if (channel >= PIOS_DBUS_NUM_INPUTS) {
        return PIOS_RCVR_INVALID;
    }

    return dbus_dev->state.channel_data[channel];
}

/**
 * Compute channel_data[] from received_data[].
 * For efficiency it unrolls first 8 channels without loops and does the
 * same for other 8 channels. Also 2 discrete channels will be set.
 */
static void PIOS_DBus_UnrollChannels(struct pios_dbus_state *state)
{
    uint8_t *s  = state->received_data;
    uint16_t *d = state->channel_data;

#define F(v, s) (((v) >> (s)) & 0x7ff)

    /* unroll channels 1-8 */
    d[0] = F(s[0] | s[1] << 8, 0);                        // Channel0
    d[1] = F(s[1] | s[2] << 8, 3);                        // Channel1
    d[2] = F(s[2] | s[3] << 8 | s[4] << 16, 6);           // Channel2
    d[3] = F(s[4] | s[5] << 8, 1);                        // Channel3
    d[4] = s[16] | (s[17] << 8);            //NULL

    d[5] = ((s[5] >> 4) & 0x0003);          //!< Switch left
    d[6] = ((s[5] >> 4) & 0x000C) >> 2;     //!< Switch right
    d[7] = s[6] | (s[7] << 8);              //!< Mouse X axis
    d[8] = s[8] | (s[9] << 8);              //!< Mouse Y axis
    d[9] = s[10] | (s[11] << 8);            //!< Mouse Z axis
    d[10] = s[12];                          //!< Mouse Left Is Press ?
    d[11] = s[13];                          //!< Mouse Right Is Press ?
    d[12] = s[14] | (s[15] << 8);           //!< KeyBoard value

    d[0] -= DBUS_CH_VALUE_OFFSET;
    d[1] -= DBUS_CH_VALUE_OFFSET;
    d[2] -= DBUS_CH_VALUE_OFFSET;
    d[3] -= DBUS_CH_VALUE_OFFSET;
    d[4] -= DBUS_CH_VALUE_OFFSET;
}

static uint8_t PIOS_DBus_ChannelDataValid(struct pios_dbus_state *state) {
    uint16_t *ch = state->channel_data;
#define ABS(x) (x > 0 ? x : -x)
	if (ABS(ch[0]) > DBUS_CHANNEL_ERROR_VALUE ||
		ABS(ch[1]) > DBUS_CHANNEL_ERROR_VALUE ||
		ABS(ch[2]) > DBUS_CHANNEL_ERROR_VALUE ||
		ABS(ch[3]) > DBUS_CHANNEL_ERROR_VALUE ||
		ch[5] == 0 || ch[6] == 0) {
		for (uint8_t i = 0; i < 5; i++) { //ch0~4
		    ch[i] = 0;
		}
		ch[5] = DBUS_SW_DOWN; //sw
		ch[6] = DBUS_SW_DOWN;
		for (uint8_t i = 7; i < 13; i++) { // mouse * 5 + key * 1
		    ch[i] = 0;
		}
	    return 0; // invalid
	}
}

/* Comm byte received callback */
static uint16_t PIOS_DBus_RxInCallback(uint32_t context,
                                       uint8_t *buf,
                                       uint16_t buf_len,
                                       uint16_t *headroom,
                                       bool *need_yield)
{
	if (buf_len != DBUS_FRAME_LENGTH) {
	    return 0;
	}
    struct pios_dbus_dev *dbus_dev = (struct pios_dbus_dev *)context;
    bool valid = PIOS_DBus_Validate(dbus_dev);
    PIOS_Assert(valid);
    struct pios_dbus_state *state = &(dbus_dev->state);
	for (auto i = 0; i < buf_len; i++) {
		state->received_data[i] = buf[i];
	}
    PIOS_DBus_UnrollChannels(state);
	volatile uint16_t ch[16];
	for (auto i = 0; i < 13; i++) {
	    ch[i] = state->channel_data[i];
	}
	if (!PIOS_DBus_ChannelDataValid(state)) {
		//PIOS_USART_DMA_Reinit(context);
	    return 0;
	}
    /* We never need a yield */
    *need_yield = false;
    return buf_len;
}

static uint8_t PIOS_DBus_Quality_Get(uint32_t dbus_id)
{
	//TBD
	return 255;
}

#endif /* PIOS_INCLUDE_DBUS */

/**
 * @}
 * @}
 */
