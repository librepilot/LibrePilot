/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_RFM22B Radio Functions
 * @brief PIOS OpenLRS interface for for the RFM22B radio
 * @{
 *
 * @file       pios_openlrs_rcvr.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @brief      Implements an OpenLRS driver for the RFM22B
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

#ifdef PIOS_INCLUDE_OPENLRS

#include "pios_openlrs_priv.h"

#include <uavobjectmanager.h>
#include <oplinkreceiver.h>
#include <oplinkstatus.h>
#include <pios_openlrs_priv.h>
#include <pios_openlrs_rcvr_priv.h>

#define PIOS_OPENLRS_RCVR_TIMEOUT_MS 100

/* Provide a RCVR driver */
static int32_t PIOS_OpenLRS_Rcvr_Get(uint32_t rcvr_id, uint8_t channel);
static void PIOS_OpenLRS_Rcvr_Supervisor(uint32_t ppm_id);
static void PIOS_OpenLRS_Rcvr_ppm_callback(uint32_t openlrs_rcvr_id, const int16_t *channels);
static uint8_t PIOS_OpenLRSRCVR_Quality_Get(uint32_t openlrs_rcvr_id);

const struct pios_rcvr_driver pios_openlrs_rcvr_driver = {
    .read        = PIOS_OpenLRS_Rcvr_Get,
    .get_quality = PIOS_OpenLRSRCVR_Quality_Get
};

/* Local Variables */
enum pios_openlrs_rcvr_dev_magic {
    PIOS_OPENLRS_RCVR_DEV_MAGIC = 0x07ac9e2144ff5329,
};

struct pios_openlrs_rcvr_dev {
    enum pios_openlrs_rcvr_dev_magic magic;
    int16_t channels[OPENLRS_PPM_NUM_CHANNELS];
    int8_t  RSSI;
    uint8_t LinkQuality;
    uint8_t supv_timer;
    bool    fresh;
};

static void openlrs_rcvr_update_uavo(struct pios_openlrs_rcvr_dev *pios_rfm22b_rcvr_dev);

static bool PIOS_OpenLRS_Rcvr_Validate(struct pios_openlrs_rcvr_dev
                                       *openlrs_rcvr_dev)
{
    return openlrs_rcvr_dev->magic == PIOS_OPENLRS_RCVR_DEV_MAGIC;
}

static struct pios_openlrs_rcvr_dev *PIOS_OpenLRS_Rcvr_alloc(void)
{
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev;

    openlrs_rcvr_dev =
        (struct pios_openlrs_rcvr_dev *)
        pios_malloc(sizeof(*openlrs_rcvr_dev));
    if (!openlrs_rcvr_dev) {
        return NULL;
    }

    openlrs_rcvr_dev->magic = PIOS_OPENLRS_RCVR_DEV_MAGIC;
    openlrs_rcvr_dev->fresh = false;
    openlrs_rcvr_dev->supv_timer = 0;

    return openlrs_rcvr_dev;
}

extern int32_t PIOS_OpenLRS_Rcvr_Init(uint32_t *openlrs_rcvr_id, uintptr_t openlrs_id)
{
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev;

    /* Allocate the device structure */
    openlrs_rcvr_dev =
        (struct pios_openlrs_rcvr_dev *)PIOS_OpenLRS_Rcvr_alloc();
    if (!openlrs_rcvr_dev) {
        return -1;
    }

    /* Register uavobj callback */
    OPLinkReceiverInitialize();

    *openlrs_rcvr_id = (uintptr_t)openlrs_rcvr_dev;
    PIOS_OpenLRS_RegisterPPMCallback(openlrs_id, PIOS_OpenLRS_Rcvr_ppm_callback, *openlrs_rcvr_id);

    /* Register the failsafe timer callback. */
    if (!PIOS_RTC_RegisterTickCallback
            (PIOS_OpenLRS_Rcvr_Supervisor, *openlrs_rcvr_id)) {
        PIOS_DEBUG_Assert(0);
    }

    return 0;
}

/**
 * Called from the core driver to set the channel values whenever a
 * PPM packet is received. This method stores the data locally as well
 * as sets the data into the OPLinkReceiver UAVO for visibility
 */
static void PIOS_OpenLRS_Rcvr_ppm_callback(uint32_t openlrs_rcvr_id, const int16_t *channels)
{
    /* Recover our device context */
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev =
        (struct pios_openlrs_rcvr_dev *)openlrs_rcvr_id;

    if (!PIOS_OpenLRS_Rcvr_Validate(openlrs_rcvr_dev)) {
        /* Invalid device specified */
        return;
    }

    for (uint32_t i = 0; i < OPENLRS_PPM_NUM_CHANNELS; i++) {
        openlrs_rcvr_dev->channels[i] = channels[i];
    }

    // Update the RSSI and quality fields.
    int8_t rssi;
    OPLinkStatusRSSIGet(&rssi);
    openlrs_rcvr_dev->RSSI = rssi;
    uint16_t quality;
    OPLinkStatusLinkQualityGet(&quality);
    // Link quality is 0-128, so scale it down to 0-100
    openlrs_rcvr_dev->LinkQuality = quality * 100 / 128;

    openlrs_rcvr_update_uavo(openlrs_rcvr_dev);

    // let supervisor know we have new data
    openlrs_rcvr_dev->fresh = true;
}

/**
 * Get the value of an input channel
 * \param[in] channel Number of the channel desired (zero based)
 * \output PIOS_RCVR_INVALID channel not available
 * \output PIOS_RCVR_TIMEOUT failsafe condition or missing receiver
 * \output >=0 channel value
 */
static int32_t PIOS_OpenLRS_Rcvr_Get(uint32_t openlrs_rcvr_id, uint8_t channel)
{
    if (channel >= OPENLRS_PPM_NUM_CHANNELS) {
        /* channel is out of range */
        return PIOS_RCVR_INVALID;
    }

    /* Recover our device context */
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev =
        (struct pios_openlrs_rcvr_dev *)openlrs_rcvr_id;

    if (!PIOS_OpenLRS_Rcvr_Validate(openlrs_rcvr_dev)) {
        /* Invalid device specified */
        return PIOS_RCVR_INVALID;
    }

    return openlrs_rcvr_dev->channels[channel];
}

static void PIOS_OpenLRS_Rcvr_Supervisor(uint32_t openlrs_rcvr_id)
{
    /* Recover our device context */
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev =
        (struct pios_openlrs_rcvr_dev *)openlrs_rcvr_id;

    if (!PIOS_OpenLRS_Rcvr_Validate(openlrs_rcvr_dev)) {
        /* Invalid device specified */
        return;
    }

    /*
     * RTC runs at 625Hz.
     */
    if (++(openlrs_rcvr_dev->supv_timer) <
        (PIOS_OPENLRS_RCVR_TIMEOUT_MS * 1000 / 625)) {
        return;
    }
    openlrs_rcvr_dev->supv_timer = 0;

    if (!openlrs_rcvr_dev->fresh) {
        for (int32_t i = 0; i < OPLINKRECEIVER_CHANNEL_NUMELEM;
             i++) {
            openlrs_rcvr_dev->channels[i] = PIOS_RCVR_TIMEOUT;
        }
        openlrs_rcvr_dev->RSSI = -127;
        openlrs_rcvr_dev->LinkQuality = 0;
    }

    openlrs_rcvr_dev->fresh = false;
}

static void openlrs_rcvr_update_uavo(struct pios_openlrs_rcvr_dev *rcvr_dev)
{
    // Also store the received data in a UAVO for easy
    // debugging. However this is not what is used in
    // ManualControl (it fetches directly from this driver)
    OPLinkReceiverData rcvr;

    for (uint8_t i = 0; i < OPENLRS_PPM_NUM_CHANNELS; i++) {
        if (i < OPLINKRECEIVER_CHANNEL_NUMELEM) {
            rcvr.Channel[i] = rcvr_dev->channels[i];
        }
    }
    for (int i = OPENLRS_PPM_NUM_CHANNELS - 1; i < OPLINKRECEIVER_CHANNEL_NUMELEM; i++) {
        rcvr.Channel[i] = PIOS_RCVR_INVALID;
    }

    rcvr.RSSI = rcvr_dev->RSSI;
    rcvr.LinkQuality = rcvr_dev->LinkQuality;

    OPLinkReceiverSet(&rcvr);
}


static uint8_t PIOS_OpenLRSRCVR_Quality_Get(uint32_t openlrs_rcvr_id)
{
    /* Recover our device context */
    struct pios_openlrs_rcvr_dev *openlrs_rcvr_dev = (struct pios_openlrs_rcvr_dev *)openlrs_rcvr_id;

    if (!PIOS_OpenLRS_Rcvr_Validate(openlrs_rcvr_dev)) {
        /* Invalid device specified */
        return 0;
    }

    return openlrs_rcvr_dev->LinkQuality;
}

#endif /* PIOS_INCLUDE_OPENLRS */

/**
 * @}
 * @}
 */
