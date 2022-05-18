/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup  PIOS_OPLinkRCVR OPLink Receiver Input Functions
 * @brief	Code to read the channels within the OPLinkReceiver UAVObject
 * @{
 *
 * @file       pios_opinkrcvr.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      GCS Input functions (STM32 dependent)
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

#include <pios.h>

#ifdef PIOS_INCLUDE_OPLINKRCVR

#include <uavobjectmanager.h>
#include <oplinkreceiver.h>
#include <oplinkstatus.h>
#include <pios_oplinkrcvr_priv.h>

// Put receiver in failsafe if not updated within timeout
#define PIOS_OPLINK_RCVR_TIMEOUT_MS 100

/* Provide a RCVR driver */
static int32_t PIOS_OPLinkRCVR_Get(uint32_t rcvr_id, uint8_t channel);
static void PIOS_oplinkrcvr_Supervisor(uint32_t ppm_id);
static uint8_t PIOS_OPLinkRCVR_Quality_Get(uint32_t oplinkrcvr_id);

const struct pios_rcvr_driver pios_oplinkrcvr_rcvr_driver = {
    .read        = PIOS_OPLinkRCVR_Get,
    .get_quality = PIOS_OPLinkRCVR_Quality_Get
};

/* Local Variables */
enum pios_oplinkrcvr_dev_magic {
    PIOS_OPLINKRCVR_DEV_MAGIC = 0x07ab9e2544cf5029,
};

struct pios_oplinkrcvr_dev {
    enum pios_oplinkrcvr_dev_magic magic;
    uint8_t supv_timer;
    OPLinkReceiverData oplinkreceiverdata;
    bool    Fresh;
};

static struct pios_oplinkrcvr_dev *global_oplinkrcvr_dev;

static bool PIOS_oplinkrcvr_validate(struct pios_oplinkrcvr_dev *oplinkrcvr_dev)
{
    return oplinkrcvr_dev->magic == PIOS_OPLINKRCVR_DEV_MAGIC;
}

#if defined(PIOS_INCLUDE_RFM22B)
static void PIOS_oplinkrcvr_ppm_callback(uint32_t oplinkrcvr_id, const int16_t *channels)
{
    /* Recover our device context */
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)oplinkrcvr_id;

    if (!PIOS_oplinkrcvr_validate(oplinkrcvr_dev)) {
        /* Invalid device specified */
        return;
    }

    for (uint8_t i = 0; i < OPLINKRECEIVER_CHANNEL_NUMELEM; ++i) {
        oplinkrcvr_dev->oplinkreceiverdata.Channel[i] = (i < RFM22B_PPM_NUM_CHANNELS) ? channels[i] : PIOS_RCVR_TIMEOUT;
    }

    // Update the RSSI and quality fields.
    int8_t rssi;
    OPLinkStatusRSSIGet(&rssi);
    oplinkrcvr_dev->oplinkreceiverdata.RSSI = rssi;
    uint16_t quality;
    OPLinkStatusLinkQualityGet(&quality);
    // Link quality is 0-128, so scale it down to 0-100
    oplinkrcvr_dev->oplinkreceiverdata.LinkQuality = quality * 100 / 128;

    OPLinkReceiverSet(&oplinkrcvr_dev->oplinkreceiverdata);

    oplinkrcvr_dev->Fresh = true;
}
#endif /* PIOS_INCLUDE_RFM22B */

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_oplinkrcvr_dev *PIOS_oplinkrcvr_alloc(void)
{
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev;

    oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)pios_malloc(sizeof(*oplinkrcvr_dev));
    if (!oplinkrcvr_dev) {
        return NULL;
    }

    oplinkrcvr_dev->magic = PIOS_OPLINKRCVR_DEV_MAGIC;
    oplinkrcvr_dev->Fresh = false;
    oplinkrcvr_dev->supv_timer = 0;

    /* The update callback cannot receive the device pointer, so set it in a global */
    global_oplinkrcvr_dev = oplinkrcvr_dev;

    return oplinkrcvr_dev;
}
#else
static struct pios_oplinkrcvr_dev pios_oplinkrcvr_devs[PIOS_OPLINKRCVR_MAX_DEVS];
static uint8_t pios_oplinkrcvr_num_devs;
static struct pios_oplinkrcvr_dev *PIOS_oplinkrcvr_alloc(void)
{
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev;

    if (pios_oplinkrcvr_num_devs >= PIOS_OPLINKRCVR_MAX_DEVS) {
        return NULL;
    }

    oplinkrcvr_dev = &pios_oplinkrcvr_devs[pios_oplinkrcvr_num_devs++];
    oplinkrcvr_dev->magic = PIOS_OPLINKRCVR_DEV_MAGIC;
    oplinkrcvr_dev->Fresh = false;
    oplinkrcvr_dev->supv_timer = 0;

    return oplinkrcvr_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */

static void oplinkreceiver_updated(UAVObjEvent *ev)
{
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev = global_oplinkrcvr_dev;

    if (ev->obj == OPLinkReceiverHandle()) {
        OPLinkReceiverGet(&oplinkrcvr_dev->oplinkreceiverdata);
        oplinkrcvr_dev->Fresh = true;
    }
}

#if defined(PIOS_INCLUDE_RFM22B)
extern int32_t PIOS_OPLinkRCVR_Init(uint32_t *oplinkrcvr_id, uint32_t rfm22b_id)
#else
extern int32_t PIOS_OPLinkRCVR_Init(uint32_t *oplinkrcvr_id)
#endif
{
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev;

    /* Allocate the device structure */
    oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)PIOS_oplinkrcvr_alloc();
    if (!oplinkrcvr_dev) {
        return -1;
    }

    for (uint8_t i = 0; i < OPLINKRECEIVER_CHANNEL_NUMELEM; i++) {
        /* Flush channels */
        oplinkrcvr_dev->oplinkreceiverdata.Channel[i] = PIOS_RCVR_TIMEOUT;
    }

#if defined(PIOS_INCLUDE_RFM22B)
    /* Register ppm callback */
    PIOS_RFM22B_SetPPMCallback(rfm22b_id, PIOS_oplinkrcvr_ppm_callback, (uint32_t)oplinkrcvr_dev);
#endif

    /* Updates could come over the telemetry channel, so register uavobj callback */
    OPLinkReceiverConnectCallback(oplinkreceiver_updated);

    /* Register the failsafe timer callback. */
    if (!PIOS_RTC_RegisterTickCallback(PIOS_oplinkrcvr_Supervisor, (uint32_t)oplinkrcvr_dev)) {
        PIOS_DEBUG_Assert(0);
    }

    *oplinkrcvr_id = (uint32_t)oplinkrcvr_dev;

    return 0;
}

/**
 * Get the value of an input channel
 * \param[in] channel Number of the channel desired (zero based)
 * \output PIOS_RCVR_INVALID channel not available
 * \output PIOS_RCVR_TIMEOUT failsafe condition or missing receiver
 * \output >=0 channel value
 */
static int32_t PIOS_OPLinkRCVR_Get(uint32_t oplinkrcvr_id, uint8_t channel)
{
    /* Recover our device context */
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)oplinkrcvr_id;

    if (!PIOS_oplinkrcvr_validate(oplinkrcvr_dev)) {
        /* Invalid device specified */
        return PIOS_RCVR_INVALID;
    }

    if (channel >= OPLINKRECEIVER_CHANNEL_NUMELEM) {
        /* channel is out of range */
        return PIOS_RCVR_INVALID;
    }

    return oplinkrcvr_dev->oplinkreceiverdata.Channel[channel];
}

static void PIOS_oplinkrcvr_Supervisor(uint32_t oplinkrcvr_id)
{
    /* Recover our device context */
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)oplinkrcvr_id;

    if (!PIOS_oplinkrcvr_validate(oplinkrcvr_dev)) {
        /* Invalid device specified */
        return;
    }

    /*
     * RTC runs at 625Hz.
     */
    if (++(oplinkrcvr_dev->supv_timer) < (PIOS_OPLINK_RCVR_TIMEOUT_MS * 1000 / 625)) {
        return;
    }
    oplinkrcvr_dev->supv_timer = 0;

    if (!oplinkrcvr_dev->Fresh) {
        for (int32_t i = 0; i < OPLINKRECEIVER_CHANNEL_NUMELEM; i++) {
            oplinkrcvr_dev->oplinkreceiverdata.Channel[i] = PIOS_RCVR_TIMEOUT;
        }
        oplinkrcvr_dev->oplinkreceiverdata.RSSI = -127;
        oplinkrcvr_dev->oplinkreceiverdata.LinkQuality = 0;
    }

    oplinkrcvr_dev->Fresh = false;
}

static uint8_t PIOS_OPLinkRCVR_Quality_Get(uint32_t oplinkrcvr_id)
{
    /* Recover our device context */
    struct pios_oplinkrcvr_dev *oplinkrcvr_dev = (struct pios_oplinkrcvr_dev *)oplinkrcvr_id;

    if (!PIOS_oplinkrcvr_validate(oplinkrcvr_dev)) {
        /* Invalid device specified */
        return 0;
    }

    return oplinkrcvr_dev->oplinkreceiverdata.LinkQuality;
}

#endif /* PIOS_INCLUDE_OPLINKRCVR */

/**
 * @}
 * @}
 */
