/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RFM22B Radio Functions
 * @brief PIOS interface for for the RFM22B radio
 * @{
 *
 * @file       pios_rfm22b.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Implements a driver the the RFM22B driver
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

// *****************************************************************
// RFM22B hardware layer
//
// This module uses the RFM22B's internal packet handling hardware to
// encapsulate our own packet data.
//
// The RFM22B internal hardware packet handler configuration is as follows:
//
// 6-byte (32-bit) preamble .. alternating 0's & 1's
// 4-byte (32-bit) sync
// 1-byte packet length (number of data bytes to follow)
// 0 to 251 user data bytes
// 4 byte ECC
//
// OR in PPM only mode:
//
// 6-byte (32-bit) preamble .. alternating 0's & 1's
// 4-byte (32-bit) sync
// 1-byte packet length (number of data bytes to follow)
// 1 byte PPM values LSB (bit 0)
// 8 bytes PPM values MSBs (bit 8:1)
// 1 byte CRC
//
// *****************************************************************

#include "pios.h"

#ifdef PIOS_INCLUDE_RFM22B

#include <pios_spi_priv.h>
#include <pios_rfm22b_regs.h>
#include <pios_rfm22b_priv.h>
#include <pios_ppm_out.h>
#include <ecc.h>
#include <sha1.h>

/* Local Defines */
#define STACK_SIZE_BYTES                     200
#define TASK_PRIORITY                        (tskIDLE_PRIORITY + 4) // flight control relevant device driver (ppm link)
#define ISR_TIMEOUT                          1 // ms
#define EVENT_QUEUE_SIZE                     5
#define RFM22B_DEFAULT_RX_DATARATE           RFM22_datarate_9600
#define RFM22B_DEFAULT_TX_POWER              RFM22_tx_pwr_txpow_0
#define RFM22B_NOMINAL_CARRIER_FREQUENCY_433 430000000
#define RFM22B_NOMINAL_CARRIER_FREQUENCY_868 860000000
#define RFM22B_NOMINAL_CARRIER_FREQUENCY_915 900000000
#define RFM22B_LINK_QUALITY_THRESHOLD        20
#define RFM22B_DEFAULT_MIN_CHANNEL           0
#define RFM22B_DEFAULT_MAX_CHANNEL           250
#define RFM22B_PPM_ONLY_DATARATE             RFM22_datarate_9600

// PPM encoding limits
#define RFM22B_PPM_MIN                       1
#define RFM22B_PPM_MAX                       511
#define RFM22B_PPM_INVALID                   0
#define RFM22B_PPM_SCALE                     2
#define RFM22B_PPM_MIN_US                    990
#define RFM22B_PPM_MAX_US                    (RFM22B_PPM_MIN_US + (RFM22B_PPM_MAX - RFM22B_PPM_MIN) * RFM22B_PPM_SCALE)

// The maximum amount of time without activity before initiating a reset.
#define PIOS_RFM22B_SUPERVISOR_TIMEOUT       150  // ms

// this is too adjust the RF module so that it is on frequency
#define OSC_LOAD_CAP                         0x7F  // cap = 12.5pf .. default

#define TX_PREAMBLE_NIBBLES                  12  // 7 to 511 (number of nibbles)
#define RX_PREAMBLE_NIBBLES                  6   // 5 to 31 (number of nibbles)
#define SYNC_BYTES                           4
#define HEADER_BYTES                         4
#define LENGTH_BYTES                         1

// the size of the rf modules internal FIFO buffers
#define FIFO_SIZE                            64

#define TX_FIFO_HI_WATERMARK                 62  // 0-63
#define TX_FIFO_LO_WATERMARK                 32  // 0-63

#define RX_FIFO_HI_WATERMARK                 32 // 0-63

// preamble byte (preceeds SYNC_BYTE's)
#define PREAMBLE_BYTE                        0x55

// RF sync bytes (32-bit in all)
#define SYNC_BYTE_1                          0x2D
#define SYNC_BYTE_2                          0xD4
#define SYNC_BYTE_3                          0x4B
#define SYNC_BYTE_4                          0x59

#ifndef RX_LED_ON
#define RX_LED_ON
#define RX_LED_OFF
#define TX_LED_ON
#define TX_LED_OFF
#define LINK_LED_ON
#define LINK_LED_OFF
#define USB_LED_ON
#define USB_LED_OFF
#endif

#define CONNECTED_TIMEOUT (250 / portTICK_RATE_MS) /* ms */
#define MAX_CHANNELS      32

/* Local type definitions */

struct pios_rfm22b_transition {
    enum pios_radio_event (*entry_fn)(struct pios_rfm22b_dev *rfm22b_dev);
    enum pios_radio_state next_state[RADIO_EVENT_NUM_EVENTS];
};

// Must ensure these prefilled arrays match the define sizes
static const uint8_t FULL_PREAMBLE[FIFO_SIZE] = {
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE,
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE
}; // 64 bytes

static const uint8_t HEADER[(TX_PREAMBLE_NIBBLES + 1) / 2 + 2] = {
    PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, PREAMBLE_BYTE, SYNC_BYTE_1, SYNC_BYTE_2
};

static const uint8_t OUT_FF[64] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/* Local function forwared declarations */
static void pios_rfm22_task(void *parameters);
static bool pios_rfm22_readStatus(struct pios_rfm22b_dev *rfm22b_dev);
static void pios_rfm22_setDatarate(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_rxFailure(struct pios_rfm22b_dev *rfm22b_dev);
static void pios_rfm22_inject_event(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event, bool inISR);
static enum pios_radio_event rfm22_init(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event radio_setRxMode(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event radio_rxData(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event radio_receivePacket(struct pios_rfm22b_dev *rfm22b_dev, uint8_t *p, uint16_t rx_len);
static enum pios_radio_event radio_txStart(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event radio_txData(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event rfm22_txFailure(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event rfm22_process_state_transition(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event);
static void rfm22_process_event(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event);
static enum pios_radio_event rfm22_timeout(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event rfm22_error(struct pios_rfm22b_dev *rfm22b_dev);
static enum pios_radio_event rfm22_fatal_error(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22b_add_rx_status(struct pios_rfm22b_dev *rfm22b_dev, enum pios_rfm22b_rx_packet_status status);
static void rfm22_setNominalCarrierFrequency(struct pios_rfm22b_dev *rfm22b_dev, uint8_t init_chan, uint32_t frequency_hz);
static bool rfm22_setFreqHopChannel(struct pios_rfm22b_dev *rfm22b_dev, uint8_t channel);
static void rfm22_generateDeviceID(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_updateStats(struct pios_rfm22b_dev *rfm22b_dev);
static bool rfm22_checkTimeOut(struct pios_rfm22b_dev *rfm22b_dev);
static bool rfm22_isConnected(struct pios_rfm22b_dev *rfm22b_dev);
static bool rfm22_isCoordinator(struct pios_rfm22b_dev *rfm22b_dev);
static uint32_t rfm22_destinationID(struct pios_rfm22b_dev *rfm22b_dev);
static bool rfm22_timeToSend(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_synchronizeClock(struct pios_rfm22b_dev *rfm22b_dev);
static uint32_t rfm22_coordinatorTime(struct pios_rfm22b_dev *rfm22b_dev);
static uint8_t rfm22_calcChannel(struct pios_rfm22b_dev *rfm22b_dev, uint8_t index);
static uint8_t rfm22_calcChannelFromClock(struct pios_rfm22b_dev *rfm22b_dev);
static bool rfm22_changeChannel(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_clearLEDs();

// Utility functions.
static uint32_t pios_rfm22_time_ms();
static uint32_t pios_rfm22_time_difference_ms(uint32_t start_time, uint32_t end_time);
static struct pios_rfm22b_dev *pios_rfm22_alloc(void);
static void rfm22_hmac_sha1(const uint8_t *data, size_t len, uint8_t key[SHA1_DIGEST_LENGTH],
                            uint8_t digest[SHA1_DIGEST_LENGTH]);
static bool rfm22_gen_channels(uint32_t coordid, enum rfm22b_datarate datarate, uint8_t min,
                               uint8_t max, uint8_t channels[MAX_CHANNELS], uint8_t *clen);

// SPI read/write functions
static void rfm22_assertCs(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_deassertCs(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_claimBus(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_releaseBus(struct pios_rfm22b_dev *rfm22b_dev);
static void rfm22_write_claim(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr, uint8_t data);
static void rfm22_write(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr, uint8_t data);
static uint8_t rfm22_read(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr);


/* The state transition table */
static const struct pios_rfm22b_transition rfm22b_transitions[RADIO_STATE_NUM_STATES] = {
    // Initialization thread
    [RADIO_STATE_UNINITIALIZED] = {
        .entry_fn   = 0,
        .next_state =             {
            [RADIO_EVENT_INITIALIZE] = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_ERROR] = RADIO_STATE_ERROR,
        },
    },
    [RADIO_STATE_INITIALIZING] =  {
        .entry_fn   = rfm22_init,
        .next_state =             {
            [RADIO_EVENT_INITIALIZED] = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_ERROR] = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },

    [RADIO_STATE_RX_MODE] =       {
        .entry_fn   = radio_setRxMode,
        .next_state =             {
            [RADIO_EVENT_INT_RECEIVED] = RADIO_STATE_RX_DATA,
            [RADIO_EVENT_TX_START]     = RADIO_STATE_TX_START,
            [RADIO_EVENT_RX_MODE]     = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_TIMEOUT]     = RADIO_STATE_TIMEOUT,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_RX_DATA] =       {
        .entry_fn   = radio_rxData,
        .next_state =             {
            [RADIO_EVENT_INT_RECEIVED] = RADIO_STATE_RX_DATA,
            [RADIO_EVENT_TX_START]     = RADIO_STATE_TX_START,
            [RADIO_EVENT_RX_COMPLETE]  = RADIO_STATE_TX_START,
            [RADIO_EVENT_RX_MODE]     = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_TIMEOUT]     = RADIO_STATE_TIMEOUT,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_TX_START] =      {
        .entry_fn   = radio_txStart,
        .next_state =             {
            [RADIO_EVENT_INT_RECEIVED] = RADIO_STATE_TX_DATA,
            [RADIO_EVENT_RX_MODE]     = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_TIMEOUT]     = RADIO_STATE_TIMEOUT,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_TX_DATA] =       {
        .entry_fn   = radio_txData,
        .next_state =             {
            [RADIO_EVENT_INT_RECEIVED] = RADIO_STATE_TX_DATA,
            [RADIO_EVENT_RX_MODE]     = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_TIMEOUT]     = RADIO_STATE_TIMEOUT,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_TX_FAILURE] =    {
        .entry_fn   = rfm22_txFailure,
        .next_state =             {
            [RADIO_EVENT_TX_START]    = RADIO_STATE_TX_START,
            [RADIO_EVENT_TIMEOUT]     = RADIO_STATE_TIMEOUT,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_TIMEOUT] =       {
        .entry_fn   = rfm22_timeout,
        .next_state =             {
            [RADIO_EVENT_TX_START]    = RADIO_STATE_TX_START,
            [RADIO_EVENT_RX_MODE]     = RADIO_STATE_RX_MODE,
            [RADIO_EVENT_ERROR]       = RADIO_STATE_ERROR,
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_ERROR] =         {
        .entry_fn   = rfm22_error,
        .next_state =             {
            [RADIO_EVENT_INITIALIZE]  = RADIO_STATE_INITIALIZING,
            [RADIO_EVENT_FATAL_ERROR] = RADIO_STATE_FATAL_ERROR,
        },
    },
    [RADIO_STATE_FATAL_ERROR] =   {
        .entry_fn   = rfm22_fatal_error,
        .next_state =             {},
    },
};

// xtal 10 ppm, 434MHz
static const uint32_t data_rate[] = {
    9600, // 96 kbps, 433 HMz, 30 khz freq dev
    19200, // 19.2 kbps, 433 MHz, 45 khz freq dev
    32000, // 32 kbps, 433 MHz, 45 khz freq dev
    57600, // 57.6 kbps, 433 MHz, 45 khz freq dev
    64000, // 64 kbps, 433 MHz, 45 khz freq dev
    100000, // 100 kbps, 433 MHz, 60 khz freq dev
    128000, // 128 kbps, 433 MHz, 90 khz freq dev
    192000, // 192 kbps, 433 MHz, 128 khz freq dev
    256000, // 256 kbps, 433 MHz, 150 khz freq dev
};

static const uint8_t channel_spacing[] = {
    1, /* 9.6kbps */
    2, /* 19.2kbps */
    2, /* 32kbps */
    2, /* 57.6kbps */
    2, /* 64kbps */
    3, /* 100kbps */
    4, /* 128kbps */
    4, /* 192kbps */
    5, /* 256kbps */
};

static const uint8_t channel_limits[] = {
    1, /* 9.6kbps */
    1, /* 19.2kbps */
    1, /* 32kbps */
    1, /* 57.6kbps */
    1, /* 64kbps */
    1, /* 100kbps */
    2, /* 128kbps */
    2, /* 192kbps */
    2, /* 256kbps */
};

static const uint8_t reg_1C[] = { 0x01, 0x05, 0x06, 0x95, 0x95, 0x81, 0x88, 0x8B, 0x8D }; // rfm22_if_filter_bandwidth

static const uint8_t reg_1D[] = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 }; // rfm22_afc_loop_gearshift_override
static const uint8_t reg_1E[] = { 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x02 }; // rfm22_afc_timing_control

static const uint8_t reg_1F[] = { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 }; // rfm22_clk_recovery_gearshift_override
static const uint8_t reg_20[] = { 0xA1, 0xD0, 0x7D, 0x68, 0x5E, 0x78, 0x5E, 0x3F, 0x2F }; // rfm22_clk_recovery_oversampling_ratio
static const uint8_t reg_21[] = { 0x20, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02 }; // rfm22_clk_recovery_offset2
static const uint8_t reg_22[] = { 0x4E, 0x9D, 0x06, 0x3A, 0x5D, 0x11, 0x5D, 0x0C, 0xBB }; // rfm22_clk_recovery_offset1
static const uint8_t reg_23[] = { 0xA5, 0x49, 0x25, 0x93, 0x86, 0x11, 0x86, 0x4A, 0x0D }; // rfm22_clk_recovery_offset0
static const uint8_t reg_24[] = { 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x06, 0x07 }; // rfm22_clk_recovery_timing_loop_gain1
static const uint8_t reg_25[] = { 0x34, 0x88, 0x77, 0x29, 0xE2, 0x90, 0xE2, 0x1A, 0xFF }; // rfm22_clk_recovery_timing_loop_gain0

static const uint8_t reg_2A[] = { 0x1E, 0x24, 0x28, 0x3C, 0x3C, 0x50, 0x50, 0x50, 0x50 }; // rfm22_afc_limiter .. AFC_pull_in_range = ±AFCLimiter[7:0] x (hbsel+1) x 625 Hz

static const uint8_t reg_58[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xED }; // rfm22_cpcuu
static const uint8_t reg_69[] = { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 }; // rfm22_agc_override1
static const uint8_t reg_6E[] = { 0x4E, 0x9D, 0x08, 0x0E, 0x10, 0x19, 0x20, 0x31, 0x41 }; // rfm22_tx_data_rate1
static const uint8_t reg_6F[] = { 0xA5, 0x49, 0x31, 0xBF, 0x62, 0x9A, 0xC5, 0x27, 0x89 }; // rfm22_tx_data_rate0

static const uint8_t reg_70[] = { 0x2C, 0x2C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C }; // rfm22_modulation_mode_control1
static const uint8_t reg_71[] = { 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23 }; // rfm22_modulation_mode_control2

static const uint8_t reg_72[] = { 0x30, 0x48, 0x48, 0x48, 0x48, 0x60, 0x90, 0xCD, 0xF0 }; // rfm22_frequency_deviation

static const uint8_t packet_time[] = { 80, 40, 25, 15, 13, 10, 8, 6, 5 };
static const uint8_t packet_time_ppm[] = { 26, 25, 25, 15, 13, 10, 8, 6, 5 };
static const uint8_t num_channels[] = { 32, 32, 32, 32, 32, 32, 32, 32, 32 };

static struct pios_rfm22b_dev *g_rfm22b_dev = NULL;


/*****************************************************************************
* External Interface Functions
*****************************************************************************/

/**
 * Initialise an RFM22B device
 *
 * @param[out] rfm22b_id  A pointer to store the device ID in.
 * @param[in] spi_id  The SPI bus index.
 * @param[in] slave_num  The SPI bus slave number.
 * @param[in] cfg  The device configuration.
 */
int32_t PIOS_RFM22B_Init(uint32_t *rfm22b_id, uint32_t spi_id, uint32_t slave_num, const struct pios_rfm22b_cfg *cfg, OPLinkSettingsRFBandOptions band)
{
    PIOS_DEBUG_Assert(rfm22b_id);
    PIOS_DEBUG_Assert(cfg);

    // Allocate the device structure.
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)pios_rfm22_alloc();
    if (!rfm22b_dev) {
        return -1;
    }
    *rfm22b_id   = (uint32_t)rfm22b_dev;
    g_rfm22b_dev = rfm22b_dev;

    // Store the SPI handle
    rfm22b_dev->slave_num = slave_num;
    rfm22b_dev->spi_id    = spi_id;

    // Initialize our configuration parameters
    rfm22b_dev->datarate  = RFM22B_DEFAULT_RX_DATARATE;
    rfm22b_dev->tx_power  = RFM22B_DEFAULT_TX_POWER;

    // Set the frequency band
    switch (band) {
    case OPLINKSETTINGS_RFBAND_915MHZ:
        rfm22b_dev->base_freq = RFM22B_NOMINAL_CARRIER_FREQUENCY_915;
        break;
    case OPLINKSETTINGS_RFBAND_868MHZ:
        rfm22b_dev->base_freq = RFM22B_NOMINAL_CARRIER_FREQUENCY_868;
        break;
    case OPLINKSETTINGS_RFBAND_433MHZ:
    default:
        rfm22b_dev->base_freq = RFM22B_NOMINAL_CARRIER_FREQUENCY_433;
        break;
    }

    // Initialize the channels.
    PIOS_RFM22B_SetChannelConfig(*rfm22b_id, RFM22B_DEFAULT_RX_DATARATE, RFM22B_DEFAULT_MIN_CHANNEL,
                                 RFM22B_DEFAULT_MAX_CHANNEL, false, true, false);

    // Create the event queue
    rfm22b_dev->eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(enum pios_radio_event));

    // Bind the configuration to the device instance
    rfm22b_dev->cfg = *cfg;

    // Create a semaphore to know if an ISR needs responding to
    vSemaphoreCreateBinary(rfm22b_dev->isrPending);

    // Create default (hopefully) unique 32 bit id from the processor serial number.
    rfm22_generateDeviceID(rfm22b_dev);

    // Initialize the external interrupt.
    PIOS_EXTI_Init(cfg->exti_cfg);

    // Register the watchdog timer for the radio driver task
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
    PIOS_WDG_RegisterFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

    // Initialize the ECC library.
    initialize_ecc();

    // Set the state to initializing.
    rfm22b_dev->state = RADIO_STATE_UNINITIALIZED;

    // Initialize the radio device.
    pios_rfm22_inject_event(rfm22b_dev, RADIO_EVENT_INITIALIZE, false);

    // Start the driver task.  This task controls the radio state machine and removed all of the IO from the IRQ handler.
    xTaskCreate(pios_rfm22_task, "PIOS_RFM22B_Task", STACK_SIZE_BYTES, (void *)rfm22b_dev, TASK_PRIORITY, &(rfm22b_dev->taskHandle));

    return 0;
}

/**
 * Re-initialize the modem after a configuration change.
 *
 * @param[in] rbm22b_id  The RFM22B device ID.
 */
void PIOS_RFM22B_Reinit(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        pios_rfm22_inject_event(rfm22b_dev, RADIO_EVENT_INITIALIZE, false);
    }
}

/**
 * The RFM22B external interrupt routine.
 */
bool PIOS_RFM22_EXT_Int(void)
{
    if (!PIOS_RFM22B_Validate(g_rfm22b_dev)) {
        return false;
    }

    // Inject an interrupt event into the state machine.
    pios_rfm22_inject_event(g_rfm22b_dev, RADIO_EVENT_INT_RECEIVED, true);
    return false;
}


/**
 * Set the device ID for the RFM22B device.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 *
 */
void PIOS_RFM22B_SetDeviceID(uint32_t rfm22b_id, uint32_t custom_device_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (custom_device_id > 0) {
        rfm22b_dev->deviceID = custom_device_id;
    } else {
        rfm22_generateDeviceID(rfm22b_dev);
    }

    DEBUG_PRINTF(2, "RF device ID: %x\n\r", rfm22b_dev->deviceID);
}

/**
 * Returns the unique device ID for the RFM22B device.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @return The unique device ID
 */
uint32_t PIOS_RFM22B_DeviceID(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        return rfm22b_dev->deviceID;
    }
    return 0;
}

/**
 * Are we connected to the remote modem?
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool rfm22_isConnected(struct pios_rfm22b_dev *rfm22b_dev)
{
    return (rfm22b_dev->stats.link_state == OPLINKSTATUS_LINKSTATE_CONNECTED) || (rfm22b_dev->stats.link_state == OPLINKSTATUS_LINKSTATE_CONNECTING);
}

/**
 * Returns true if the modem is not actively sending or receiving a packet.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @return True if the modem is not actively sending or receiving a packet.
 */
bool PIOS_RFM22B_InRxWait(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        return (rfm22b_dev->rfm22b_state == RFM22B_STATE_RX_WAIT) || (rfm22b_dev->rfm22b_state == RFM22B_STATE_TRANSITION);
    }
    return false;
}

/**
 * Sets the radio device transmit power.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @param[in] tx_pwr The transmit power.
 */
void PIOS_RFM22B_SetTxPower(uint32_t rfm22b_id, enum rfm22b_tx_power tx_pwr)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }
    rfm22b_dev->tx_power = tx_pwr;
}

/**
 * Sets the range and number of channels to use for the radio.
 * The channels are 0 to 250 divided across the 430-440 MHz range.
 * The number of channels configured will be spread across the selected channel range.
 * The channel spacing is 10MHz / 250 = 40kHz
 *
 * @param[in] rfm22b_id  The RFM22B device index.
 * @param[in] datarate  The desired datarate.
 * @param[in] min_chan  The minimum channel.
 * @param[in] max_chan  The maximum channel.
 * @param[in] coordinator Is this modem an coordinator.
 * @param[in] data_mode Should this modem send/receive data packets?
 * @param[in] ppm_mode Should this modem send/receive ppm packets?
 */
void PIOS_RFM22B_SetChannelConfig(uint32_t rfm22b_id, enum rfm22b_datarate datarate, uint8_t min_chan, uint8_t max_chan, bool coordinator, bool data_mode, bool ppm_mode)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;
    bool ppm_only = ppm_mode && !data_mode;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }
    rfm22b_dev->coordinator   = coordinator;
    rfm22b_dev->ppm_send_mode = ppm_mode && coordinator;
    rfm22b_dev->ppm_recv_mode = ppm_mode && !coordinator;
    if (ppm_mode && (datarate <= RFM22B_PPM_ONLY_DATARATE)) {
        ppm_only = true;
    }
    rfm22b_dev->ppm_only_mode = ppm_only;
    if (ppm_only) {
        rfm22b_dev->one_way_link = true;
        datarate = RFM22B_PPM_ONLY_DATARATE;
        rfm22b_dev->datarate     = RFM22B_PPM_ONLY_DATARATE;
    } else {
        rfm22b_dev->one_way_link = false;
        rfm22b_dev->datarate     = datarate;
    }
    rfm22b_dev->packet_time = (ppm_mode ? packet_time_ppm[datarate] : packet_time[datarate]);

    uint8_t num_found = 0;
    rfm22_gen_channels(rfm22_destinationID(rfm22b_dev), datarate, min_chan, max_chan,
                       rfm22b_dev->channels, &num_found);

    rfm22b_dev->num_channels = num_found;

    // Calculate the maximum packet length from the datarate.
    float bytes_per_period = (float)data_rate[datarate] * (float)(rfm22b_dev->packet_time - 2) / 9000;

    rfm22b_dev->max_packet_len = bytes_per_period - TX_PREAMBLE_NIBBLES / 2 - SYNC_BYTES - HEADER_BYTES - LENGTH_BYTES;
    if (rfm22b_dev->max_packet_len > RFM22B_MAX_PACKET_LEN) {
        rfm22b_dev->max_packet_len = RFM22B_MAX_PACKET_LEN;
    }
}

/**
 * Set a XtalCap
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @param[in] XtalCap Value.
 */
void PIOS_RFM22B_SetXtalCap(uint32_t rfm22b_id, uint8_t xtal_cap)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        rfm22b_dev->cfg.RFXtalCap = xtal_cap;
    }
}

/**
 * Set a modem to be a coordinator or not.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @param[in] coordinator If true, this modem will be configured as a coordinator.
 */
extern void PIOS_RFM22B_SetCoordinator(uint32_t rfm22b_id, bool coordinator)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        rfm22b_dev->coordinator = coordinator;
    }
}

/**
 * Sets the device coordinator ID.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @param[in] coord_id The coordinator ID.
 */
void PIOS_RFM22B_SetCoordinatorID(uint32_t rfm22b_id, uint32_t coord_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (PIOS_RFM22B_Validate(rfm22b_dev)) {
        rfm22b_dev->coordinatorID = coord_id;
    }
}

/**
 * Returns the device statistics RFM22B device.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @param[out] stats The stats are returned in this structure
 */
void PIOS_RFM22B_GetStats(uint32_t rfm22b_id, struct rfm22b_stats *stats)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    // Update the current stats
    rfm22_updateStats(rfm22b_dev);

    // Return the stats.
    *stats = rfm22b_dev->stats;
}

/**
 * Check the radio device for a valid connection
 *
 * @param[in] rfm22b_id  The rfm22b device.
 * @return true if there is a valid connection to paired radio, false otherwise.
 */
bool PIOS_RFM22B_LinkStatus(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return false;
    }
    return rfm22b_dev->stats.link_quality > RFM22B_LINK_QUALITY_THRESHOLD;
}

/**
 * Put the RFM22B device into receive mode.
 *
 * @param[in] rfm22b_id  The rfm22b device.
 * @param[in] p  The packet to receive into.
 * @return true if Rx mode was entered sucessfully.
 */
bool PIOS_RFM22B_ReceivePacket(uint32_t rfm22b_id, uint8_t *p)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return false;
    }
    rfm22b_dev->rx_packet_handle = p;

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // disable interrupts
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable1, 0x00);
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable2, 0x00);

    // Switch to TUNE mode
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon);

#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
    D2_LED_OFF;
#endif // PIOS_RFM22B_DEBUG_ON_TELEM
    RX_LED_OFF;
    TX_LED_OFF;

    // empty the rx buffer
    rfm22b_dev->rx_buffer_wr = 0;

    // Clear the TX buffer.
    rfm22b_dev->tx_data_rd   = rfm22b_dev->tx_data_wr = 0;

    // clear FIFOs
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl2, RFM22_opfc2_ffclrrx | RFM22_opfc2_ffclrtx);
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl2, 0x00);

    // enable RX interrupts
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable1, RFM22_ie1_encrcerror | RFM22_ie1_enpkvalid |
                RFM22_ie1_enrxffafull | RFM22_ie1_enfferr);
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable2, RFM22_ie2_enpreaval | RFM22_ie2_enswdet);

    // enable the receiver
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon | RFM22_opfc1_rxon);

    // Release the SPI bus.
    rfm22_releaseBus(rfm22b_dev);

    // Indicate that we're in RX wait mode.
    rfm22b_dev->rfm22b_state = RFM22B_STATE_RX_WAIT;

    return true;
}

/**
 * Transmit a packet via the RFM22B device.
 *
 * @param[in] rfm22b_id  The rfm22b device.
 * @param[in] p  The packet to transmit.
 * @return true if there if the packet was queued for transmission.
 */
bool PIOS_RFM22B_TransmitPacket(uint32_t rfm22b_id, uint8_t *p, uint8_t len)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return false;
    }

    rfm22b_dev->tx_packet_handle  = p;
    rfm22b_dev->packet_start_time = pios_rfm22_time_ms();
    if (rfm22b_dev->packet_start_time == 0) {
        rfm22b_dev->packet_start_time = 1;
    }

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // Disable interrupts
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable1, 0x00);
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable2, 0x00);

    // set the tx power
    rfm22_write(rfm22b_dev, RFM22_tx_power, RFM22_tx_pwr_lna_sw | rfm22b_dev->tx_power);

    // TUNE mode
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon);

    // Queue the data up for sending
    rfm22b_dev->tx_data_wr = len;

    RX_LED_OFF;

    // Set the destination address in the transmit header.
    uint32_t id = rfm22_destinationID(rfm22b_dev);
    rfm22_write(rfm22b_dev, RFM22_transmit_header0, id & 0xff);
    rfm22_write(rfm22b_dev, RFM22_transmit_header1, (id >> 8) & 0xff);
    rfm22_write(rfm22b_dev, RFM22_transmit_header2, (id >> 16) & 0xff);
    rfm22_write(rfm22b_dev, RFM22_transmit_header3, (id >> 24) & 0xff);

    // FIFO mode, GFSK modulation
    uint8_t fd_bit = rfm22_read(rfm22b_dev, RFM22_modulation_mode_control2) & RFM22_mmc2_fd;
    rfm22_write(rfm22b_dev, RFM22_modulation_mode_control2, fd_bit | RFM22_mmc2_dtmod_fifo | RFM22_mmc2_modtyp_gfsk);

    // Clear the FIFOs.
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl2, RFM22_opfc2_ffclrrx | RFM22_opfc2_ffclrtx);
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl2, 0x00);

    // Set the total number of data bytes we are going to transmit.
    rfm22_write(rfm22b_dev, RFM22_transmit_packet_length, len);

    // Add some data to the chips TX FIFO before enabling the transmitter
    uint8_t *tx_buffer = rfm22b_dev->tx_packet_handle;
    rfm22_assertCs(rfm22b_dev);
    PIOS_SPI_TransferByte(rfm22b_dev->spi_id, RFM22_fifo_access | 0x80);
    int bytes_to_write = (rfm22b_dev->tx_data_wr - rfm22b_dev->tx_data_rd);
    bytes_to_write = (bytes_to_write > FIFO_SIZE) ? FIFO_SIZE : bytes_to_write;
    PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, &tx_buffer[rfm22b_dev->tx_data_rd], NULL, bytes_to_write, NULL);
    rfm22b_dev->tx_data_rd += bytes_to_write;
    rfm22_deassertCs(rfm22b_dev);

    // Enable TX interrupts.
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable1, RFM22_ie1_enpksent | RFM22_ie1_entxffaem);

    // Enable the transmitter.
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon | RFM22_opfc1_txon);

    // Release the SPI bus.
    rfm22_releaseBus(rfm22b_dev);

    // We're in Tx mode.
    rfm22b_dev->rfm22b_state = RFM22B_STATE_TX_MODE;

#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
    D1_LED_ON;
#endif

    return true;
}

/**
 * Process a Tx interrupt from the RFM22B device.
 *
 * @param[in] rfm22b_id  The rfm22b device.
 * @return PIOS_RFM22B_TX_COMPLETE on completed Tx, or PIOS_RFM22B_INT_SUCCESS/PIOS_RFM22B_INT_FAILURE.
 */
pios_rfm22b_int_result PIOS_RFM22B_ProcessTx(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return PIOS_RFM22B_INT_FAILURE;
    }

    // Read the device status registers
    if (!pios_rfm22_readStatus(rfm22b_dev)) {
        return PIOS_RFM22B_INT_FAILURE;
    }

    // TX FIFO almost empty, it needs filling up
    if (rfm22b_dev->status_regs.int_status_1.tx_fifo_almost_empty) {
        // Add data to the TX FIFO buffer
        uint8_t *tx_buffer = rfm22b_dev->tx_packet_handle;
        uint16_t max_bytes = FIFO_SIZE - TX_FIFO_LO_WATERMARK - 1;
        rfm22_claimBus(rfm22b_dev);
        rfm22_assertCs(rfm22b_dev);
        PIOS_SPI_TransferByte(rfm22b_dev->spi_id, RFM22_fifo_access | 0x80);
        int bytes_to_write = (rfm22b_dev->tx_data_wr - rfm22b_dev->tx_data_rd);
        bytes_to_write = (bytes_to_write > max_bytes) ? max_bytes : bytes_to_write;
        PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, &tx_buffer[rfm22b_dev->tx_data_rd], NULL, bytes_to_write, NULL);
        rfm22b_dev->tx_data_rd += bytes_to_write;
        rfm22_deassertCs(rfm22b_dev);
        rfm22_releaseBus(rfm22b_dev);

        return PIOS_RFM22B_INT_SUCCESS;
    } else if (rfm22b_dev->status_regs.int_status_1.packet_sent_interrupt) {
        // Transition out of Tx mode.
        rfm22b_dev->rfm22b_state = RFM22B_STATE_TRANSITION;
        return PIOS_RFM22B_TX_COMPLETE;
    }

    return 0;
}

/**
 * Process a Rx interrupt from the RFM22B device.
 *
 * @param[in] rfm22b_id  The rfm22b device.
 * @return PIOS_RFM22B_RX_COMPLETE on completed Rx, or PIOS_RFM22B_INT_SUCCESS/PIOS_RFM22B_INT_FAILURE.
 */
pios_rfm22b_int_result PIOS_RFM22B_ProcessRx(uint32_t rfm22b_id)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return PIOS_RFM22B_INT_FAILURE;
    }
    uint8_t *rx_buffer = rfm22b_dev->rx_packet_handle;
    pios_rfm22b_int_result ret = PIOS_RFM22B_INT_SUCCESS;

    // Read the device status registers
    if (!pios_rfm22_readStatus(rfm22b_dev)) {
        rfm22_rxFailure(rfm22b_dev);
        return PIOS_RFM22B_INT_FAILURE;
    }

    // FIFO under/over flow error.  Restart RX mode.
    if (rfm22b_dev->status_regs.int_status_1.fifo_underoverflow_error ||
        rfm22b_dev->status_regs.int_status_1.crc_error) {
        rfm22_rxFailure(rfm22b_dev);
        return PIOS_RFM22B_INT_FAILURE;
    }

    // Valid packet received
    if (rfm22b_dev->status_regs.int_status_1.valid_packet_received) {
        // Claim the SPI bus.
        rfm22_claimBus(rfm22b_dev);

        // read the total length of the packet data
        uint32_t len = rfm22_read(rfm22b_dev, RFM22_received_packet_length);

        // The received packet is going to be larger than the receive buffer
        if (len > rfm22b_dev->max_packet_len) {
            rfm22_releaseBus(rfm22b_dev);
            rfm22_rxFailure(rfm22b_dev);
            return PIOS_RFM22B_INT_FAILURE;
        }

        // there must still be data in the RX FIFO we need to get
        if (rfm22b_dev->rx_buffer_wr < len) {
            int32_t bytes_to_read = len - rfm22b_dev->rx_buffer_wr;
            // Fetch the data from the RX FIFO
            rfm22_assertCs(rfm22b_dev);
            PIOS_SPI_TransferByte(rfm22b_dev->spi_id, RFM22_fifo_access & 0x7F);
            rfm22b_dev->rx_buffer_wr += (PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, OUT_FF, (uint8_t *)&rx_buffer[rfm22b_dev->rx_buffer_wr],
                                                                bytes_to_read, NULL) == 0) ? bytes_to_read : 0;
            rfm22_deassertCs(rfm22b_dev);
        }

        // Read the packet header (destination ID)
        rfm22b_dev->rx_destination_id  = rfm22_read(rfm22b_dev, RFM22_received_header0);
        rfm22b_dev->rx_destination_id |= (rfm22_read(rfm22b_dev, RFM22_received_header1) << 8);
        rfm22b_dev->rx_destination_id |= (rfm22_read(rfm22b_dev, RFM22_received_header2) << 16);
        rfm22b_dev->rx_destination_id |= (rfm22_read(rfm22b_dev, RFM22_received_header3) << 24);

        // Release the SPI bus.
        rfm22_releaseBus(rfm22b_dev);

        // Is there a length error?
        if (rfm22b_dev->rx_buffer_wr != len) {
            rfm22_rxFailure(rfm22b_dev);
            return PIOS_RFM22B_INT_FAILURE;
        }

        // Increment the total byte received count.
        rfm22b_dev->stats.rx_byte_count += rfm22b_dev->rx_buffer_wr;

        // We're finished with Rx mode
        rfm22b_dev->rfm22b_state = RFM22B_STATE_TRANSITION;

        ret = PIOS_RFM22B_RX_COMPLETE;
    } else if (rfm22b_dev->status_regs.int_status_1.rx_fifo_almost_full) {
        // RX FIFO almost full, it needs emptying
        // read data from the rf chips FIFO buffer

        // Claim the SPI bus.
        rfm22_claimBus(rfm22b_dev);

        // Read the total length of the packet data
        uint16_t len = rfm22_read(rfm22b_dev, RFM22_received_packet_length);

        // The received packet is going to be larger than the specified length
        if ((rfm22b_dev->rx_buffer_wr + RX_FIFO_HI_WATERMARK) > len) {
            rfm22_releaseBus(rfm22b_dev);
            rfm22_rxFailure(rfm22b_dev);
            return PIOS_RFM22B_INT_FAILURE;
        }

        // The received packet is going to be larger than the receive buffer
        if ((rfm22b_dev->rx_buffer_wr + RX_FIFO_HI_WATERMARK) > rfm22b_dev->max_packet_len) {
            rfm22_releaseBus(rfm22b_dev);
            rfm22_rxFailure(rfm22b_dev);
            return PIOS_RFM22B_INT_FAILURE;
        }

        // Fetch the data from the RX FIFO
        rfm22_assertCs(rfm22b_dev);
        PIOS_SPI_TransferByte(rfm22b_dev->spi_id, RFM22_fifo_access & 0x7F);
        rfm22b_dev->rx_buffer_wr += (PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, OUT_FF, (uint8_t *)&rx_buffer[rfm22b_dev->rx_buffer_wr],
                                                            RX_FIFO_HI_WATERMARK, NULL) == 0) ? RX_FIFO_HI_WATERMARK : 0;
        rfm22_deassertCs(rfm22b_dev);

        // Release the SPI bus.
        rfm22_releaseBus(rfm22b_dev);

        // Make sure that we're in RX mode.
        rfm22b_dev->rfm22b_state = RFM22B_STATE_RX_MODE;
    } else if (rfm22b_dev->status_regs.int_status_2.valid_preamble_detected) {
        // Valid preamble detected
        RX_LED_ON;

        // Sync word detected
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
        D2_LED_ON;
#endif // PIOS_RFM22B_DEBUG_ON_TELEM
        rfm22b_dev->packet_start_time = pios_rfm22_time_ms();
        if (rfm22b_dev->packet_start_time == 0) {
            rfm22b_dev->packet_start_time = 1;
        }

        // We detected the preamble, now wait for sync.
        rfm22b_dev->rfm22b_state = RFM22B_STATE_RX_WAIT_SYNC;
    } else if (rfm22b_dev->status_regs.int_status_2.sync_word_detected) {
        // Claim the SPI bus.
        rfm22_claimBus(rfm22b_dev);

        // read the 10-bit signed afc correction value
        // bits 9 to 2
        int16_t afc_correction = (uint16_t)rfm22_read(rfm22b_dev, RFM22_afc_correction_read) << 8;
        // bits 1 & 0
        afc_correction  |= (int16_t)rfm22_read(rfm22b_dev, RFM22_ook_counter_value1) & 0x00c0;
        afc_correction >>= 6;
        // convert the afc value to Hz
        int32_t afc_corr = (int32_t)(rfm22b_dev->frequency_step_size * afc_correction + 0.5f);
        rfm22b_dev->afc_correction_Hz = afc_corr;

        // read rx signal strength .. 45 = -100dBm, 205 = -20dBm
        uint8_t rssi = rfm22_read(rfm22b_dev, RFM22_rssi);
        // convert to dBm
        rfm22b_dev->rssi_dBm = (int8_t)(rssi >> 1) - 122;

        // Release the SPI bus.
        rfm22_releaseBus(rfm22b_dev);

        // Indicate that we're in RX mode.
        rfm22b_dev->rfm22b_state = RFM22B_STATE_RX_MODE;
    } else if ((rfm22b_dev->rfm22b_state == RFM22B_STATE_RX_WAIT_SYNC) && !rfm22b_dev->status_regs.int_status_2.valid_preamble_detected) {
        // Waiting for the preamble timed out.
        rfm22_rxFailure(rfm22b_dev);
        return PIOS_RFM22B_INT_FAILURE;
    }

    return ret;
}

/**
 * Set the PPM packet received callback.
 *
 * @param[in] rfm22b_dev  The RFM22B device ID.
 * @param[in] cb          The callback function pointer.
 */
void PIOS_RFM22B_SetPPMCallback(uint32_t rfm22b_id, PPMReceivedCallback cb, uint32_t cb_context)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    /*
     * Order is important in these assignments since rfm22_task uses ppm_callback
     * field to determine if it's ok to dereference ppm_callback and ppm_context
     */
    rfm22b_dev->ppm_context  = cb_context;
    rfm22b_dev->ppm_callback = cb;
}

/**
 * Set the PPM values to be transmitted.
 *
 * @param[in] rfm22b_dev  The RFM22B device ID.
 * @param[in] channels    The PPM channel values.
 * @param[out] nchan      The number of channels to set.
 */
extern void PIOS_RFM22B_PPMSet(uint32_t rfm22b_id, int16_t *channels, uint8_t nchan)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS; ++i) {
        rfm22b_dev->ppm[i] = (i < nchan) ? channels[i] : PIOS_RCVR_INVALID;
    }
}

/**
 * Fetch the PPM values that have been received.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 * @param[out] channels   The PPM channel values.
 * @param[out] nchan      The number of channels to get.
 */
extern void PIOS_RFM22B_PPMGet(uint32_t rfm22b_id, int16_t *channels, uint8_t nchan)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    if (!rfm22_isCoordinator(rfm22b_dev) && !rfm22_isConnected(rfm22b_dev)) {
        // Set the PPM channels values to INVALID
        for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS; ++i) {
            channels[i] = PIOS_RCVR_INVALID;
        }
        return;
    }

    for (uint8_t i = 0; i < nchan; ++i) {
        channels[i] = (i < RFM22B_PPM_NUM_CHANNELS) ? rfm22b_dev->ppm[i] : PIOS_RCVR_INVALID;
    }
}

/**
 * Validate that the device structure is valid.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
inline bool PIOS_RFM22B_Validate(struct pios_rfm22b_dev *rfm22b_dev)
{
    return rfm22b_dev != NULL && rfm22b_dev->magic == PIOS_RFM22B_DEV_MAGIC;
}


/*****************************************************************************
* The Device Control Thread
*****************************************************************************/

/**
 * The task that controls the radio state machine.
 *
 * @param[in] paramters  The task parameters.
 */
static void pios_rfm22_task(void *parameters)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)parameters;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }
    uint32_t lastEventTime = pios_rfm22_time_ms();

    while (1) {
#if defined(PIOS_INCLUDE_WDG) && defined(PIOS_WDG_RFM22B)
        // Update the watchdog timer
        PIOS_WDG_UpdateFlag(PIOS_WDG_RFM22B);
#endif /* PIOS_WDG_RFM22B */

        // Wait for a signal indicating an external interrupt or a pending send/receive request.
        if (xSemaphoreTake(rfm22b_dev->isrPending, ISR_TIMEOUT / portTICK_RATE_MS) == pdTRUE) {
            lastEventTime = pios_rfm22_time_ms();

            // Process events through the state machine.
            enum pios_radio_event event;
            while (xQueueReceive(rfm22b_dev->eventQueue, &event, 0) == pdTRUE) {
                if ((event == RADIO_EVENT_INT_RECEIVED) &&
                    ((rfm22b_dev->state == RADIO_STATE_UNINITIALIZED) || (rfm22b_dev->state == RADIO_STATE_INITIALIZING))) {
                    continue;
                }
                rfm22_process_event(rfm22b_dev, event);
            }
        } else {
            // Has it been too long since the last event?
            uint32_t curTime = pios_rfm22_time_ms();
            if (pios_rfm22_time_difference_ms(lastEventTime, curTime) > PIOS_RFM22B_SUPERVISOR_TIMEOUT) {
                // Clear the event queue.
                enum pios_radio_event event;
                while (xQueueReceive(rfm22b_dev->eventQueue, &event, 0) == pdTRUE) {
                    // Do nothing;
                }
                lastEventTime = pios_rfm22_time_ms();

                // Transsition through an error event.
                rfm22_process_event(rfm22b_dev, RADIO_EVENT_ERROR);
            }
        }

        // Change channels if necessary.
        if (rfm22_changeChannel(rfm22b_dev)) {
            rfm22_process_event(rfm22b_dev, RADIO_EVENT_RX_MODE);
        }

        // Have we been sending / receiving this packet too long?
        uint32_t curTime = pios_rfm22_time_ms();
        if ((rfm22b_dev->packet_start_time > 0) &&
            (pios_rfm22_time_difference_ms(rfm22b_dev->packet_start_time, curTime) > (rfm22b_dev->packet_time * 3))) {
            rfm22_process_event(rfm22b_dev, RADIO_EVENT_TIMEOUT);
        }

        // Start transmitting a packet if it's time.
        bool time_to_send = rfm22_timeToSend(rfm22b_dev);
   #ifdef PIOS_RFM22B_DEBUG_ON_TELEM
        if (time_to_send) {
            D4_LED_ON;
        } else {
            D4_LED_OFF;
        }
   #endif
        if (time_to_send && PIOS_RFM22B_InRxWait((uint32_t)rfm22b_dev)) {
            rfm22_process_event(rfm22b_dev, RADIO_EVENT_TX_START);
        }
    }
}


/*****************************************************************************
* The State Machine Functions
*****************************************************************************/

/**
 * Inject an event into the RFM22B state machine.
 *
 * @param[in] rfm22b_dev The device structure
 * @param[in] event The event to inject
 * @param[in] inISR Is this being called from an interrrup service routine?
 */
static void pios_rfm22_inject_event(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event, bool inISR)
{
    if (inISR) {
        // Store the event.
        portBASE_TYPE pxHigherPriorityTaskWoken1;
        if (xQueueSendFromISR(rfm22b_dev->eventQueue, &event, &pxHigherPriorityTaskWoken1) != pdTRUE) {
            return;
        }
        // Signal the semaphore to wake up the handler thread.
        portBASE_TYPE pxHigherPriorityTaskWoken2;
        if (xSemaphoreGiveFromISR(rfm22b_dev->isrPending, &pxHigherPriorityTaskWoken2) != pdTRUE) {
            // Something went fairly seriously wrong
            rfm22b_dev->errors++;
        }
        portEND_SWITCHING_ISR((pxHigherPriorityTaskWoken1 == pdTRUE) || (pxHigherPriorityTaskWoken2 == pdTRUE));
    } else {
        // Store the event.
        if (xQueueSend(rfm22b_dev->eventQueue, &event, portMAX_DELAY) != pdTRUE) {
            return;
        }
        // Signal the semaphore to wake up the handler thread.
        if (xSemaphoreGive(rfm22b_dev->isrPending) != pdTRUE) {
            // Something went fairly seriously wrong
            rfm22b_dev->errors++;
        }
    }
}

/**
 * Process the next state transition from the given event.
 *
 * @param[in] rfm22b_dev The device structure
 * @param[in] event The event to process
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_process_state_transition(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event)
{
    // No event
    if (event >= RADIO_EVENT_NUM_EVENTS) {
        return RADIO_EVENT_NUM_EVENTS;
    }

    // Don't transition if there is no transition defined
    enum pios_radio_state next_state = rfm22b_transitions[rfm22b_dev->state].next_state[event];
    if (!next_state) {
        return RADIO_EVENT_NUM_EVENTS;
    }

    /*
     * Move to the next state
     *
     * This is done prior to calling the new state's entry function to
     * guarantee that the entry function never depends on the previous
     * state.  This way, it cannot ever know what the previous state was.
     */
    rfm22b_dev->state = next_state;

    /* Call the entry function (if any) for the next state. */
    if (rfm22b_transitions[rfm22b_dev->state].entry_fn) {
        return rfm22b_transitions[rfm22b_dev->state].entry_fn(rfm22b_dev);
    }

    return RADIO_EVENT_NUM_EVENTS;
}

/**
 * Process the given event through the state transition table.
 * This could cause a series of events and transitions to take place.
 *
 * @param[in] rfm22b_dev The device structure
 * @param[in] event The event to process
 */
static void rfm22_process_event(struct pios_rfm22b_dev *rfm22b_dev, enum pios_radio_event event)
{
    // Process all state transitions.
    while (event != RADIO_EVENT_NUM_EVENTS) {
        event = rfm22_process_state_transition(rfm22b_dev, event);
    }
}


/*****************************************************************************
* The Device Initialization / Configuration Functions
*****************************************************************************/

/**
 * Initialize (or re-initialize) the RFM22B radio device.
 *
 * @param[in] rfm22b_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_init(struct pios_rfm22b_dev *rfm22b_dev)
{
    // Initialize the register values.
    rfm22b_dev->status_regs.int_status_1.raw  = 0;
    rfm22b_dev->status_regs.int_status_2.raw  = 0;
    rfm22b_dev->status_regs.device_status.raw = 0;
    rfm22b_dev->status_regs.ezmac_status.raw  = 0;

    // Clean the LEDs
    rfm22_clearLEDs();

    // Initlize the link stats.
    for (uint8_t i = 0; i < RFM22B_RX_PACKET_STATS_LEN; ++i) {
        rfm22b_dev->rx_packet_stats[i] = 0;
    }

    // Initialize the state
    rfm22b_dev->stats.link_state  = OPLINKSTATUS_LINKSTATE_ENABLED;

    // Initialize the packets.
    rfm22b_dev->rx_packet_len     = 0;
    rfm22b_dev->rx_destination_id = 0;
    rfm22b_dev->tx_packet_handle  = NULL;

    // Initialize the devide state
    rfm22b_dev->rx_buffer_wr      = 0;
    rfm22b_dev->tx_data_rd        = rfm22b_dev->tx_data_wr = 0;
    rfm22b_dev->channel = 0;
    rfm22b_dev->channel_index     = 0;
    rfm22b_dev->afc_correction_Hz = 0;
    rfm22b_dev->packet_start_time = 0;
    rfm22b_dev->rfm22b_state      = RFM22B_STATE_INITIALIZING;
    rfm22b_dev->last_contact      = 0;

    // software reset the RF chip .. following procedure according to Si4x3x Errata (rev. B)
    rfm22_write_claim(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_swres);

    for (uint8_t i = 0; i < 50; ++i) {
        // read the status registers
        pios_rfm22_readStatus(rfm22b_dev);

        // Is the chip ready?
        if (rfm22b_dev->status_regs.int_status_2.chip_ready) {
            break;
        }

        // Wait 1ms if not.
        vTaskDelay(1 + (1 / (portTICK_RATE_MS + 1)));
    }

    // ****************

    // read status - clears interrupt
    pios_rfm22_readStatus(rfm22b_dev);

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // disable all interrupts
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable1, 0x00);
    rfm22_write(rfm22b_dev, RFM22_interrupt_enable2, 0x00);

    // read the RF chip ID bytes

    // read the device type
    uint8_t device_type    = rfm22_read(rfm22b_dev, RFM22_DEVICE_TYPE) & RFM22_DT_MASK;
    // read the device version
    uint8_t device_version = rfm22_read(rfm22b_dev, RFM22_DEVICE_VERSION) & RFM22_DV_MASK;

#if defined(RFM22_DEBUG)
    DEBUG_PRINTF(2, "rf device type: %d\n\r", device_type);
    DEBUG_PRINTF(2, "rf device version: %d\n\r", device_version);
#endif

    if (device_type != 0x08) {
#if defined(RFM22_DEBUG)
        DEBUG_PRINTF(2, "rf device type: INCORRECT - should be 0x08\n\r");
#endif

        // incorrect RF module type
        return RADIO_EVENT_FATAL_ERROR;
    }
    if (device_version != RFM22_DEVICE_VERSION_B1) {
#if defined(RFM22_DEBUG)
        DEBUG_PRINTF(2, "rf device version: INCORRECT\n\r");
#endif
        // incorrect RF module version
        return RADIO_EVENT_FATAL_ERROR;
    }

    // calibrate our RF module to be exactly on frequency .. different for every module
    rfm22_write(rfm22b_dev, RFM22_xtal_osc_load_cap, OSC_LOAD_CAP);

    // disable Low Duty Cycle Mode
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl2, 0x00);

    // 1MHz clock output
    rfm22_write(rfm22b_dev, RFM22_cpu_output_clk, RFM22_coc_1MHz);

    // READY mode
    rfm22_write(rfm22b_dev, RFM22_op_and_func_ctrl1, RFM22_opfc1_xton);

    // choose the 3 GPIO pin functions
    // GPIO port use default value
    rfm22_write(rfm22b_dev, RFM22_io_port_config, RFM22_io_port_default);
    if (rfm22b_dev->cfg.gpio_direction == GPIO0_TX_GPIO1_RX) {
        // GPIO0 = TX State (to control RF Switch)
        rfm22_write(rfm22b_dev, RFM22_gpio0_config, RFM22_gpio0_config_drv3 | RFM22_gpio0_config_txstate);
        // GPIO1 = RX State (to control RF Switch)
        rfm22_write(rfm22b_dev, RFM22_gpio1_config, RFM22_gpio1_config_drv3 | RFM22_gpio1_config_rxstate);
    } else {
        // GPIO0 = TX State (to control RF Switch)
        rfm22_write(rfm22b_dev, RFM22_gpio0_config, RFM22_gpio0_config_drv3 | RFM22_gpio0_config_rxstate);
        // GPIO1 = RX State (to control RF Switch)
        rfm22_write(rfm22b_dev, RFM22_gpio1_config, RFM22_gpio1_config_drv3 | RFM22_gpio1_config_txstate);
    }
    // GPIO2 = Clear Channel Assessment
    rfm22_write(rfm22b_dev, RFM22_gpio2_config, RFM22_gpio2_config_drv3 | RFM22_gpio2_config_cca);

    // FIFO mode, GFSK modulation
    uint8_t fd_bit = rfm22_read(rfm22b_dev, RFM22_modulation_mode_control2) & RFM22_mmc2_fd;
    rfm22_write(rfm22b_dev, RFM22_modulation_mode_control2, RFM22_mmc2_trclk_clk_none | RFM22_mmc2_dtmod_fifo | fd_bit | RFM22_mmc2_modtyp_gfsk);

    // setup to read the internal temperature sensor

    // ADC used to sample the temperature sensor
    uint8_t adc_config = RFM22_ac_adcsel_temp_sensor | RFM22_ac_adcref_bg;
    rfm22_write(rfm22b_dev, RFM22_adc_config, adc_config);

    // adc offset
    rfm22_write(rfm22b_dev, RFM22_adc_sensor_amp_offset, 0);

    // temp sensor calibration .. �40C to +64C 0.5C resolution
    rfm22_write(rfm22b_dev, RFM22_temp_sensor_calib, RFM22_tsc_tsrange0 | RFM22_tsc_entsoffs);

    // temp sensor offset
    rfm22_write(rfm22b_dev, RFM22_temp_value_offset, 0);

    // start an ADC conversion
    rfm22_write(rfm22b_dev, RFM22_adc_config, adc_config | RFM22_ac_adcstartbusy);

    // set the RSSI threshold interrupt to about -90dBm
    rfm22_write(rfm22b_dev, RFM22_rssi_threshold_clear_chan_indicator, (-90 + 122) * 2);

    // enable the internal Tx & Rx packet handlers (without CRC)
    rfm22_write(rfm22b_dev, RFM22_data_access_control, RFM22_dac_enpacrx | RFM22_dac_enpactx);

    // x-nibbles tx preamble
    rfm22_write(rfm22b_dev, RFM22_preamble_length, TX_PREAMBLE_NIBBLES);
    // x-nibbles rx preamble detection
    rfm22_write(rfm22b_dev, RFM22_preamble_detection_ctrl1, RX_PREAMBLE_NIBBLES << 3);

    // Release the bus
    rfm22_releaseBus(rfm22b_dev);

    // Yield the CPU.
    vTaskDelay(1 + (1 / (portTICK_RATE_MS + 1)));

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // header control - using a 4 by header with broadcast of 0xffffffff
    rfm22_write(rfm22b_dev, RFM22_header_control1,
                RFM22_header_cntl1_bcen_0 |
                RFM22_header_cntl1_bcen_1 |
                RFM22_header_cntl1_bcen_2 |
                RFM22_header_cntl1_bcen_3 |
                RFM22_header_cntl1_hdch_0 |
                RFM22_header_cntl1_hdch_1 |
                RFM22_header_cntl1_hdch_2 |
                RFM22_header_cntl1_hdch_3);
    // Check all bit of all bytes of the header, unless we're an unbound modem.
    uint8_t header_mask = (rfm22_destinationID(rfm22b_dev) == 0xffffffff) ? 0 : 0xff;
    rfm22_write(rfm22b_dev, RFM22_header_enable0, header_mask);
    rfm22_write(rfm22b_dev, RFM22_header_enable1, header_mask);
    rfm22_write(rfm22b_dev, RFM22_header_enable2, header_mask);
    rfm22_write(rfm22b_dev, RFM22_header_enable3, header_mask);
    // The destination ID and receive ID should be the same.
    uint32_t id = rfm22_destinationID(rfm22b_dev);
    rfm22_write(rfm22b_dev, RFM22_check_header0, id & 0xff);
    rfm22_write(rfm22b_dev, RFM22_check_header1, (id >> 8) & 0xff);
    rfm22_write(rfm22b_dev, RFM22_check_header2, (id >> 16) & 0xff);
    rfm22_write(rfm22b_dev, RFM22_check_header3, (id >> 24) & 0xff);
    // 4 header bytes, synchronization word length 3, 2, 1 & 0 used, packet length included in header.
    rfm22_write(rfm22b_dev, RFM22_header_control2,
                RFM22_header_cntl2_hdlen_3210 |
                RFM22_header_cntl2_synclen_3210 |
                ((TX_PREAMBLE_NIBBLES >> 8) & 0x01));

    // sync word
    rfm22_write(rfm22b_dev, RFM22_sync_word3, SYNC_BYTE_1);
    rfm22_write(rfm22b_dev, RFM22_sync_word2, SYNC_BYTE_2);
    rfm22_write(rfm22b_dev, RFM22_sync_word1, SYNC_BYTE_3);
    rfm22_write(rfm22b_dev, RFM22_sync_word0, SYNC_BYTE_4);

    // TX FIFO Almost Full Threshold (0 - 63)
    rfm22_write(rfm22b_dev, RFM22_tx_fifo_control1, TX_FIFO_HI_WATERMARK);

    // TX FIFO Almost Empty Threshold (0 - 63)
    rfm22_write(rfm22b_dev, RFM22_tx_fifo_control2, TX_FIFO_LO_WATERMARK);

    // RX FIFO Almost Full Threshold (0 - 63)
    rfm22_write(rfm22b_dev, RFM22_rx_fifo_control, RX_FIFO_HI_WATERMARK);

    // Set the xtal capacitor for frequency calibration
    // Cint = 1.8 pF + 0.085 pF x xlc[6:0] + 3.7 pF x xlc[7] (xtalshift)
    // cfg.RFXtalCap 0 to 171 range give Cint = 1.8pF to 16.295pF range
    // Default is 127, equal to 12.595pF
    rfm22_write(rfm22b_dev,
                RFM22_xtal_osc_load_cap,
                (rfm22b_dev->cfg.RFXtalCap < 128) ? rfm22b_dev->cfg.RFXtalCap : (rfm22b_dev->cfg.RFXtalCap + 84));

    // Release the bus
    rfm22_releaseBus(rfm22b_dev);

    // Yield the CPU.
    vTaskDelay(1 + (1 / (portTICK_RATE_MS + 1)));

    // Initialize the frequency and datarate to te default.
    rfm22_setNominalCarrierFrequency(rfm22b_dev, 0, rfm22b_dev->base_freq);
    pios_rfm22_setDatarate(rfm22b_dev);

    return RADIO_EVENT_INITIALIZED;
}

/**
 * Set the air datarate for the RFM22B device.
 *
 * Carson's rule:
 *  The signal bandwidth is about 2(Delta-f + fm) ..
 *
 * Delta-f = frequency deviation
 * fm = maximum frequency of the signal
 *
 * @param[in] rfm33b_dev  The device structure pointer.
 * @param[in] datarate  The air datarate.
 * @param[in] data_whitening  Is data whitening desired?
 */
static void pios_rfm22_setDatarate(struct pios_rfm22b_dev *rfm22b_dev)
{
    enum rfm22b_datarate datarate = rfm22b_dev->datarate;
    bool data_whitening = true;

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // rfm22_if_filter_bandwidth
    rfm22_write(rfm22b_dev, 0x1C, reg_1C[datarate]);

    // rfm22_afc_loop_gearshift_override
    rfm22_write(rfm22b_dev, 0x1D, reg_1D[datarate]);
    // RFM22_afc_timing_control
    rfm22_write(rfm22b_dev, 0x1E, reg_1E[datarate]);

    // RFM22_clk_recovery_gearshift_override
    rfm22_write(rfm22b_dev, 0x1F, reg_1F[datarate]);
    // rfm22_clk_recovery_oversampling_ratio
    rfm22_write(rfm22b_dev, 0x20, reg_20[datarate]);
    // rfm22_clk_recovery_offset2
    rfm22_write(rfm22b_dev, 0x21, reg_21[datarate]);
    // rfm22_clk_recovery_offset1
    rfm22_write(rfm22b_dev, 0x22, reg_22[datarate]);
    // rfm22_clk_recovery_offset0
    rfm22_write(rfm22b_dev, 0x23, reg_23[datarate]);
    // rfm22_clk_recovery_timing_loop_gain1
    rfm22_write(rfm22b_dev, 0x24, reg_24[datarate]);
    // rfm22_clk_recovery_timing_loop_gain0
    rfm22_write(rfm22b_dev, 0x25, reg_25[datarate]);
    // rfm22_agc_override1
    rfm22_write(rfm22b_dev, RFM22_agc_override1, reg_69[datarate]);

    // rfm22_afc_limiter
    rfm22_write(rfm22b_dev, 0x2A, reg_2A[datarate]);

    // rfm22_tx_data_rate1
    rfm22_write(rfm22b_dev, 0x6E, reg_6E[datarate]);
    // rfm22_tx_data_rate0
    rfm22_write(rfm22b_dev, 0x6F, reg_6F[datarate]);

    if (!data_whitening) {
        // rfm22_modulation_mode_control1
        rfm22_write(rfm22b_dev, 0x70, reg_70[datarate] & ~RFM22_mmc1_enwhite);
    } else {
        // rfm22_modulation_mode_control1
        rfm22_write(rfm22b_dev, 0x70, reg_70[datarate] | RFM22_mmc1_enwhite);
    }

    // rfm22_modulation_mode_control2
    rfm22_write(rfm22b_dev, 0x71, reg_71[datarate]);

    // rfm22_frequency_deviation
    rfm22_write(rfm22b_dev, 0x72, reg_72[datarate]);

    // rfm22_cpcuu
    rfm22_write(rfm22b_dev, 0x58, reg_58[datarate]);

    rfm22_write(rfm22b_dev, RFM22_ook_counter_value1, 0x00);
    rfm22_write(rfm22b_dev, RFM22_ook_counter_value2, 0x00);

    // Release the bus
    rfm22_releaseBus(rfm22b_dev);
}

/**
 * Set the nominal carrier frequency, channel step size, and initial channel
 *
 * @param[in] rfm33b_dev  The device structure pointer.
 * @param[in] init_chan  The initial channel to tune to.
 */
static void rfm22_setNominalCarrierFrequency(struct pios_rfm22b_dev *rfm22b_dev, uint8_t init_chan, uint32_t frequency_hz)
{
    // The step size is 10MHz / 250 = 40khz, and the step size is specified in 10khz increments.
    uint8_t freq_hop_step_size = 4;

    // holds the hbsel (1 or 2)
    uint8_t hbsel;

    if (frequency_hz < 480000000) {
        hbsel = 0;
    } else {
        hbsel = 1;
    }
    float freq_mhz = (float)(frequency_hz) / 1000000.0f;
    float xtal_freq_khz = 30000.0f;
    float sfreq    = freq_mhz / (10.0f * (xtal_freq_khz / 30000.0f) * (1 + hbsel));
    uint32_t fb    = (uint32_t)sfreq - 24 + (64 + 32 * hbsel);
    uint32_t fc    = (uint32_t)((sfreq - (uint32_t)sfreq) * 64000.0f);
    uint8_t fch    = (fc >> 8) & 0xff;
    uint8_t fcl    = fc & 0xff;

    // Claim the SPI bus.
    rfm22_claimBus(rfm22b_dev);

    // Set the frequency hopping step size.
    rfm22_write(rfm22b_dev, RFM22_frequency_hopping_step_size, freq_hop_step_size);

    // frequency step
    rfm22b_dev->frequency_step_size = 156.25f * (hbsel + 1);

    // frequency hopping channel (0-255)
    rfm22b_dev->channel = init_chan;
    rfm22_write(rfm22b_dev, RFM22_frequency_hopping_channel_select, init_chan);

    // no frequency offset
    rfm22_write(rfm22b_dev, RFM22_frequency_offset1, 0);
    rfm22_write(rfm22b_dev, RFM22_frequency_offset2, 0);

    // set the carrier frequency
    rfm22_write(rfm22b_dev, RFM22_frequency_band_select, fb & 0xff);
    rfm22_write(rfm22b_dev, RFM22_nominal_carrier_frequency1, fch);
    rfm22_write(rfm22b_dev, RFM22_nominal_carrier_frequency0, fcl);

    // Release the bus
    rfm22_releaseBus(rfm22b_dev);
}


/**
 * Set the frequency hopping channel.
 *
 * @param[in] rfm33b_dev  The device structure pointer.
 */
static bool rfm22_setFreqHopChannel(struct pios_rfm22b_dev *rfm22b_dev, uint8_t channel)
{
    // set the frequency hopping channel
    if (rfm22b_dev->channel == channel) {
        return false;
    }
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
    D3_LED_TOGGLE;
#endif // PIOS_RFM22B_DEBUG_ON_TELEM
    rfm22b_dev->channel = channel;
    rfm22_write_claim(rfm22b_dev, RFM22_frequency_hopping_channel_select, channel);
    return true;
}

/**
 * Generate the unique device ID for the RFM22B device.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 *
 */
void rfm22_generateDeviceID(struct pios_rfm22b_dev *rfm22b_dev)
{
    // Create our (hopefully) unique 32 bit id from the processor serial number.
    uint8_t crcs[] = { 0, 0, 0, 0 };
    {
        char serial_no_str[33];
        PIOS_SYS_SerialNumberGet(serial_no_str);
        // Create a 32 bit value using 4 8 bit CRC values.
        for (uint8_t i = 0; serial_no_str[i] != 0; ++i) {
            crcs[i % 4] = PIOS_CRC_updateByte(crcs[i % 4], serial_no_str[i]);
        }
    }

    rfm22b_dev->deviceID = crcs[0] | crcs[1] << 8 | crcs[2] << 16 | crcs[3] << 24;
    DEBUG_PRINTF(2, "Generated RF device ID: %x\n\r", rfm22b_dev->deviceID);
}

/**
 * Read the RFM22B interrupt and device status registers
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool pios_rfm22_readStatus(struct pios_rfm22b_dev *rfm22b_dev)
{
    // 1. Read the interrupt statuses with burst read
    rfm22_claimBus(rfm22b_dev); // Set RC and the semaphore
    uint8_t write_buf[3] = { RFM22_interrupt_status1 &0x7f, 0xFF, 0xFF };
    uint8_t read_buf[3];
    rfm22_assertCs(rfm22b_dev);
    PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, write_buf, read_buf, sizeof(write_buf), NULL);
    rfm22_deassertCs(rfm22b_dev);
    rfm22b_dev->status_regs.int_status_1.raw  = read_buf[1];
    rfm22b_dev->status_regs.int_status_2.raw  = read_buf[2];

    // Device status
    rfm22b_dev->status_regs.device_status.raw = rfm22_read(rfm22b_dev, RFM22_device_status);

    // EzMAC status
    rfm22b_dev->status_regs.ezmac_status.raw  = rfm22_read(rfm22b_dev, RFM22_ezmac_status);

    // Release the bus
    rfm22_releaseBus(rfm22b_dev);

    // the RF module has gone and done a reset - we need to re-initialize the rf module
    if (rfm22b_dev->status_regs.int_status_2.poweron_reset) {
        return false;
    }

    return true;
}

/**
 * Recover from a failure in receiving a packet.
 *
 * @param[in] rfm22b_dev  The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static void rfm22_rxFailure(struct pios_rfm22b_dev *rfm22b_dev)
{
    rfm22b_add_rx_status(rfm22b_dev, RADIO_FAILURE_RX_PACKET);
    rfm22b_dev->rx_buffer_wr = 0;
    rfm22b_dev->packet_start_time = 0;
    rfm22b_dev->rfm22b_state = RFM22B_STATE_TRANSITION;
}


/*****************************************************************************
* Radio Transmit and Receive functions.
*****************************************************************************/

/**
 * Start a transmit if possible
 *
 * @param[in] radio_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event radio_txStart(struct pios_rfm22b_dev *radio_dev)
{
    uint8_t *p  = radio_dev->tx_packet;
    uint8_t len = 0;
    uint8_t max_data_len = radio_dev->max_packet_len - (radio_dev->ppm_only_mode ? 0 : RS_ECC_NPARITY);

    // Don't send if it's not our turn, or if we're receiving a packet.
    if (!rfm22_timeToSend(radio_dev) || !PIOS_RFM22B_InRxWait((uint32_t)radio_dev)) {
        return RADIO_EVENT_RX_MODE;
    }

    // Don't send anything if we're bound to a coordinator and not yet connected.
    if (!rfm22_isCoordinator(radio_dev) && !rfm22_isConnected(radio_dev)) {
        return RADIO_EVENT_RX_MODE;
    }

    // Should we append PPM data to the packet?
    bool ppm_valid = false;
    if (radio_dev->ppm_send_mode) {
        len = RFM22B_PPM_NUM_CHANNELS + (radio_dev->ppm_only_mode ? 2 : 1);

        // Ensure we can fit the PPM data in the packet.
        if (max_data_len < len) {
            return RADIO_EVENT_RX_MODE;
        }

        // The first byte stores the LSB of each channel
        p[0] = 0;

        // Read the PPM input.
        for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS; ++i) {
            int32_t val = radio_dev->ppm[i];

            // Clamp and translate value, or transmit as reserved "invalid" constant
            if ((val == PIOS_RCVR_INVALID) || (val == PIOS_RCVR_TIMEOUT)) {
                val = RFM22B_PPM_INVALID;
            } else if (val > RFM22B_PPM_MAX_US) {
                ppm_valid = true;
                val = RFM22B_PPM_MAX;
            } else if (val < RFM22B_PPM_MIN_US) {
                ppm_valid = true;
                val = RFM22B_PPM_MIN;
            } else {
                ppm_valid = true;
                val = (val - RFM22B_PPM_MIN_US) / RFM22B_PPM_SCALE + RFM22B_PPM_MIN;
            }

            // Store LSB
            if (val & 1) {
                p[0] |= (1 << i);
            }

            // Store upper 8 bits in array
            p[i + 1] = val >> 1;
        }

        // The last byte is a CRC.
        if (radio_dev->ppm_only_mode) {
            uint8_t crc = 0;
            for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS + 1; ++i) {
                crc = PIOS_CRC_updateByte(crc, p[i]);
            }
            p[RFM22B_PPM_NUM_CHANNELS + 1] = crc;
        }
    }

    // Append data from the com interface if applicable.
    bool packet_data = false;
    if (!radio_dev->ppm_only_mode) {
        uint8_t newlen  = 0;
        bool need_yield = false;
        uint8_t i = 0;
        // Try to get some data to send
        while (newlen == 0 && i < 2) {
            radio_dev->last_stream_sent = (radio_dev->last_stream_sent + 1) % 2;
            if (!radio_dev->last_stream_sent) {
                if (radio_dev->tx_out_cb) {
                    newlen = (radio_dev->tx_out_cb)(radio_dev->tx_out_context, p + len + 1, max_data_len - len - 1, NULL, &need_yield);
                }
            } else {
                if (radio_dev->aux_tx_out_cb) {
                    newlen = (radio_dev->aux_tx_out_cb)(radio_dev->aux_tx_out_context, p + len + 1, max_data_len - len - 1, NULL, &need_yield);
                }
            }
            i++;
        }
        if (newlen) {
            packet_data = true;
            *(p + len)  = radio_dev->last_stream_sent;
            len += newlen + 1;
        }
    }

    // Always send a packet if this modem is a coordinator.
    if ((len == 0) && !rfm22_isCoordinator(radio_dev)) {
        return RADIO_EVENT_RX_MODE;
    }

    // Increment the packet sequence number.
    radio_dev->stats.tx_seq++;

    // Add the error correcting code.
    if (!radio_dev->ppm_only_mode) {
        if (len != 0) {
            encode_data((unsigned char *)p, len, (unsigned char *)p);
        }
        len += RS_ECC_NPARITY;
    }

    // Only count the packet if it contains valid data.
    if (ppm_valid || packet_data) {
        TX_LED_ON;
        radio_dev->stats.tx_byte_count += len;
    }

    // Transmit the packet.
    PIOS_RFM22B_TransmitPacket((uint32_t)radio_dev, p, len);

    return RADIO_EVENT_NUM_EVENTS;
}

/**
 * Transmit packet data.
 *
 * @param[in] rfm22b_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event radio_txData(struct pios_rfm22b_dev *radio_dev)
{
    enum pios_radio_event ret_event = RADIO_EVENT_NUM_EVENTS;
    pios_rfm22b_int_result res = PIOS_RFM22B_ProcessTx((uint32_t)radio_dev);

    // Is the transmition complete
    if (res == PIOS_RFM22B_TX_COMPLETE) {
        // Is this an ACK?
        ret_event = RADIO_EVENT_RX_MODE;
        radio_dev->tx_packet_handle  = 0;
        radio_dev->tx_data_wr = radio_dev->tx_data_rd = 0;
        // Start a new transaction
        radio_dev->packet_start_time = 0;

#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
        D1_LED_OFF;
#endif
    }

    return ret_event;
}

/**
 * Switch the radio into receive mode.
 *
 * @param[in] rfm22b_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event radio_setRxMode(struct pios_rfm22b_dev *rfm22b_dev)
{
    if (!PIOS_RFM22B_ReceivePacket((uint32_t)rfm22b_dev, rfm22b_dev->rx_packet)) {
        return RADIO_EVENT_NUM_EVENTS;
    }
    rfm22b_dev->packet_start_time = 0;

    // No event generated
    return RADIO_EVENT_NUM_EVENTS;
}

/**
 * Complete the receipt of a packet.
 *
 * @param[in] radio_dev  The device structure
 * @param[in] p  The packet handle of the received packet.
 * @param[in] rc_len  The number of bytes received.
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event radio_receivePacket(struct pios_rfm22b_dev *radio_dev, uint8_t *p, uint16_t rx_len)
{
    bool good_packet      = true;
    bool corrected_packet = false;
    uint8_t stream_num    = 0;
    uint8_t data_len      = rx_len;

    // We don't rsencode ppm only packets.
    if (!radio_dev->ppm_only_mode) {
        data_len -= RS_ECC_NPARITY;

        // Attempt to correct any errors in the packet.
        if (data_len > 0) {
            decode_data((unsigned char *)p, rx_len);
            good_packet = check_syndrome() == 0;

            // We have an error.  Try to correct it.
            if (!good_packet && (correct_errors_erasures((unsigned char *)p, rx_len, 0, 0) != 0)) {
                // We corrected it
                corrected_packet = true;
            }
        }
    }

    // Should we pull PPM data off of the head of the packet?
    if ((good_packet || corrected_packet) && radio_dev->ppm_recv_mode) {
        uint8_t ppm_len = RFM22B_PPM_NUM_CHANNELS + (radio_dev->ppm_only_mode ? 2 : 1);

        // Ensure the packet it long enough
        if (data_len < ppm_len) {
            good_packet = false;
        }

        // Verify the CRC if this is a PPM only packet.
        if ((good_packet || corrected_packet) && radio_dev->ppm_only_mode) {
            uint8_t crc = 0;
            for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS + 1; ++i) {
                crc = PIOS_CRC_updateByte(crc, p[i]);
            }
            if (p[RFM22B_PPM_NUM_CHANNELS + 1] != crc) {
                good_packet = false;
                corrected_packet = false;
            }
        }

        if (good_packet || corrected_packet) {
            for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS; ++i) {
                // Calculate 9-bit value taking the LSB from byte 0
                uint32_t val = (p[i + 1] << 1) + ((p[0] >> i) & 1);
                // Is this a valid channel?
                if (val != RFM22B_PPM_INVALID) {
                    radio_dev->ppm[i] = (uint16_t)(RFM22B_PPM_MIN_US + (val - RFM22B_PPM_MIN) * RFM22B_PPM_SCALE);
                } else {
                    // Set failsafe value
                    radio_dev->ppm[i] = PIOS_RCVR_TIMEOUT;
                }
            }

            p += RFM22B_PPM_NUM_CHANNELS + 1;
            data_len -= RFM22B_PPM_NUM_CHANNELS + 1;

            // Call the PPM received callback if it's available.
            if (radio_dev->ppm_callback) {
                radio_dev->ppm_callback(radio_dev->ppm_context, radio_dev->ppm);
            }
        }
    }

    // Set the packet status
    if (good_packet) {
        rfm22b_add_rx_status(radio_dev, RADIO_GOOD_RX_PACKET);
    } else if (corrected_packet) {
        // We corrected the error.
        rfm22b_add_rx_status(radio_dev, RADIO_CORRECTED_RX_PACKET);
    } else {
        // We couldn't correct the error, so drop the packet.
        rfm22b_add_rx_status(radio_dev, RADIO_ERROR_RX_PACKET);
    }

    // Increment the packet sequence number.
    radio_dev->stats.rx_seq++;

    enum pios_radio_event ret_event = RADIO_EVENT_RX_COMPLETE;
    if (good_packet || corrected_packet) {
        // Send the data to the com port
        bool rx_need_yield;


        if ((data_len > 0) && !radio_dev->ppm_only_mode) {
            stream_num = *p;
            p++;
            data_len--;
            if (!stream_num) {
                if (radio_dev->rx_in_cb) {
                    (radio_dev->rx_in_cb)(radio_dev->rx_in_context, p, data_len, NULL, &rx_need_yield);
                }
            } else {
                if (radio_dev->aux_rx_in_cb) {
                    (radio_dev->aux_rx_in_cb)(radio_dev->aux_rx_in_context, p, data_len, NULL, &rx_need_yield);
                }
            }
        }
        /*
         * If the packet is valid and destined for us we synchronize the clock.
         */
        if (!rfm22_isCoordinator(radio_dev) &&
            radio_dev->rx_destination_id == rfm22_destinationID(radio_dev)) {
            rfm22_synchronizeClock(radio_dev);
        }
        radio_dev->stats.link_state     = OPLINKSTATUS_LINKSTATE_CONNECTED;
        radio_dev->last_contact         = pios_rfm22_time_ms();
        radio_dev->stats.rssi = radio_dev->rssi_dBm;
        radio_dev->stats.afc_correction = radio_dev->afc_correction_Hz;
    } else {
        ret_event = RADIO_EVENT_RX_COMPLETE;
    }

    return ret_event;
}

/**
 * Receive the packet data.
 *
 * @param[in] rfm22b_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event radio_rxData(struct pios_rfm22b_dev *radio_dev)
{
    enum pios_radio_event ret_event = RADIO_EVENT_NUM_EVENTS;
    pios_rfm22b_int_result res = PIOS_RFM22B_ProcessRx((uint32_t)radio_dev);

    switch (res) {
    case PIOS_RFM22B_RX_COMPLETE:

        // Receive the packet.
        ret_event = radio_receivePacket(radio_dev, radio_dev->rx_packet_handle, radio_dev->rx_buffer_wr);
        radio_dev->rx_buffer_wr = 0;
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
        D2_LED_OFF;
#endif

        // Start a new transaction
        radio_dev->packet_start_time = 0;
        break;

    case PIOS_RFM22B_INT_FAILURE:

        ret_event = RADIO_EVENT_RX_MODE;
        break;

    default:
        // do nothing.
        break;
    }

    return ret_event;
}

/*****************************************************************************
* Link Statistics Functions
*****************************************************************************/

/**
 * Calculate stats from the packet receipt, transmission statistics.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static void rfm22_updateStats(struct pios_rfm22b_dev *rfm22b_dev)
{
    // Add the RX packet statistics
    rfm22b_dev->stats.rx_good      = 0;
    rfm22b_dev->stats.rx_corrected = 0;
    rfm22b_dev->stats.rx_error     = 0;
    rfm22b_dev->stats.rx_failure   = 0;

    if (!rfm22_isConnected(rfm22b_dev)) {
        // Set link_quality to 0 and Rssi to noise floor if disconnected
        rfm22b_dev->stats.link_quality = 0;
        rfm22b_dev->stats.rssi = -127;
        return;
    }

    // Check if connection is timed out
    if (rfm22_checkTimeOut(rfm22b_dev)) {
        // Set the link state to disconnected.
        rfm22b_dev->stats.link_state = OPLINKSTATUS_LINKSTATE_DISCONNECTED;
    }

    for (uint8_t i = 0; i < RFM22B_RX_PACKET_STATS_LEN; ++i) {
        uint32_t val = rfm22b_dev->rx_packet_stats[i];
        for (uint8_t j = 0; j < 16; ++j) {
            switch ((val >> (j * 2)) & 0x3) {
            case RADIO_GOOD_RX_PACKET:
                rfm22b_dev->stats.rx_good++;
                break;
            case RADIO_CORRECTED_RX_PACKET:
                rfm22b_dev->stats.rx_corrected++;
                break;
            case RADIO_ERROR_RX_PACKET:
                rfm22b_dev->stats.rx_error++;
                break;
            case RADIO_FAILURE_RX_PACKET:
                rfm22b_dev->stats.rx_failure++;
                break;
            }
        }
    }

    // Calculate the link quality metric, which is related to the number of good packets in relation to the number of bad packets.
    // Note: This assumes that the number of packets sampled for the stats is 64.
    // Using this equation, error and resent packets are counted as -2, and corrected packets are counted as -1.
    // The range is 0 (all error or resent packets) to 128 (all good packets).
    rfm22b_dev->stats.link_quality = 64 + rfm22b_dev->stats.rx_good - rfm22b_dev->stats.rx_error - rfm22b_dev->stats.rx_failure;
}

/**
 * A timeout occured ?
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool rfm22_checkTimeOut(struct pios_rfm22b_dev *rfm22b_dev)
{
    return pios_rfm22_time_difference_ms(rfm22b_dev->last_contact, pios_rfm22_time_ms()) >= CONNECTED_TIMEOUT;
}

/**
 * Add a status value to the RX packet status array.
 *
 * @param[in] rfm22b_dev  The device structure
 * @param[in] status  The packet status value
 */
static void rfm22b_add_rx_status(struct pios_rfm22b_dev *rfm22b_dev, enum pios_rfm22b_rx_packet_status status)
{
    // Shift the status registers
    for (uint8_t i = RFM22B_RX_PACKET_STATS_LEN - 1; i > 0; --i) {
        rfm22b_dev->rx_packet_stats[i] = (rfm22b_dev->rx_packet_stats[i] << 2) | (rfm22b_dev->rx_packet_stats[i - 1] >> 30);
    }
    rfm22b_dev->rx_packet_stats[0] = (rfm22b_dev->rx_packet_stats[0] << 2) | status;
}


/*****************************************************************************
* Connection Handling Functions
*****************************************************************************/

/**
 * Are we a coordinator modem?
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool rfm22_isCoordinator(struct pios_rfm22b_dev *rfm22b_dev)
{
    return rfm22b_dev->coordinator;
}

/**
 * Returns the destination ID to send packets to.
 *
 * @param[in] rfm22b_id The RFM22B device index.
 * @return The destination ID
 */
uint32_t rfm22_destinationID(struct pios_rfm22b_dev *rfm22b_dev)
{
    if (rfm22_isCoordinator(rfm22b_dev)) {
        return rfm22b_dev->deviceID;
    } else if (rfm22b_dev->coordinatorID) {
        return rfm22b_dev->coordinatorID;
    } else {
        return 0xffffffff;
    }
}


/*****************************************************************************
* Frequency Hopping Functions
*****************************************************************************/

/**
 * Synchronize the clock after a packet receive from our coordinator on the syncronization channel.
 * This function should be called when a packet is received on the synchronization channel.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static void rfm22_synchronizeClock(struct pios_rfm22b_dev *rfm22b_dev)
{
    uint32_t start_time = rfm22b_dev->packet_start_time;

    // This packet was transmitted on channel 0, calculate the time delta that will force us to transmit on channel 0 at the time this packet started.
    uint16_t frequency_hop_cycle_time = rfm22b_dev->packet_time * rfm22b_dev->num_channels;
    uint16_t time_delta = start_time % frequency_hop_cycle_time;

    // Calculate the adjustment for the preamble
    uint8_t offset = (uint8_t)ceil(35000.0F / data_rate[rfm22b_dev->datarate]);

    rfm22b_dev->time_delta = frequency_hop_cycle_time - time_delta + offset +
                             rfm22b_dev->packet_time * rfm22b_dev->channel_index;
}

/**
 * Return the estimated current time on the coordinator modem.
 * This is the master clock used for all synchronization.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static uint32_t rfm22_coordinatorTime(struct pios_rfm22b_dev *rfm22b_dev)
{
    uint32_t time = pios_rfm22_time_ms();

    if (rfm22_isCoordinator(rfm22b_dev)) {
        return time;
    }
    return time + rfm22b_dev->time_delta;
}

/**
 * Return true if this modem is in the send interval, which allows the modem to initiate a transmit.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool rfm22_timeToSend(struct pios_rfm22b_dev *rfm22b_dev)
{
    uint32_t time = rfm22_coordinatorTime(rfm22b_dev);
    bool is_coordinator   = rfm22_isCoordinator(rfm22b_dev);

    // If this is a one-way link, only the coordinator can send.
    uint8_t packet_period = rfm22b_dev->packet_time;

    if (rfm22b_dev->one_way_link) {
        if (is_coordinator) {
            return ((time - 1) % (packet_period)) == 0;
        } else {
            return false;
        }
    }

    if (!is_coordinator) {
        time += packet_period - 1;
    } else {
        time -= 1;
    }
    return (time % (packet_period * 2)) == 0;
}

/**
 * Calculate the nth channel index.
 *
 * @param[in] rfm22b_dev  The device structure
 * @param[in] index  The channel index to calculate
 */
static uint8_t rfm22_calcChannel(struct pios_rfm22b_dev *rfm22b_dev, uint8_t index)
{
    // Make sure we don't index outside of the range.
    uint8_t idx = index % rfm22b_dev->num_channels;

    // Are we switching to a new channel?
    if ((idx != rfm22b_dev->channel_index) && !rfm22_isCoordinator(rfm22b_dev) && rfm22_checkTimeOut(rfm22b_dev)) {
        // Set the link state to disconnected.
        rfm22b_dev->stats.link_state = OPLINKSTATUS_LINKSTATE_DISCONNECTED;
        // Update stats
        rfm22_updateStats(rfm22b_dev);
        // Stay on first channel.
        idx = 0;
    }

    rfm22b_dev->channel_index = idx;

    return rfm22b_dev->channels[idx];
}

/**
 * Calculate what the current channel shold be.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static uint8_t rfm22_calcChannelFromClock(struct pios_rfm22b_dev *rfm22b_dev)
{
    uint32_t time = rfm22_coordinatorTime(rfm22b_dev);
    // Divide time into 8ms blocks.  Coordinator sends in first 2 ms, and remote send in 5th and 6th ms.
    // Channel changes occur in the last 2 ms.
    uint8_t n     = (time / rfm22b_dev->packet_time) % rfm22b_dev->num_channels;

    return rfm22_calcChannel(rfm22b_dev, n);
}

/**
 * Change channels to the calculated current channel.
 *
 * @param[in] rfm22b_dev  The device structure
 */
static bool rfm22_changeChannel(struct pios_rfm22b_dev *rfm22b_dev)
{
    // A disconnected non-coordinator modem should sit on the sync channel until connected.
    if (!rfm22_isCoordinator(rfm22b_dev) && !rfm22_isConnected(rfm22b_dev)) {
        return rfm22_setFreqHopChannel(rfm22b_dev, rfm22_calcChannel(rfm22b_dev, 0));
    } else {
        return rfm22_setFreqHopChannel(rfm22b_dev, rfm22_calcChannelFromClock(rfm22b_dev));
    }
}


/*****************************************************************************
* Error Handling Functions
*****************************************************************************/

/**
 * Recover from a transmit failure.
 *
 * @param[in] rfm22b_dev The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_txFailure(struct pios_rfm22b_dev *rfm22b_dev)
{
    rfm22b_dev->stats.tx_failure++;
    rfm22b_dev->packet_start_time = 0;
    rfm22b_dev->tx_data_wr = rfm22b_dev->tx_data_rd = 0;
    return RADIO_EVENT_TX_START;
}

/**
 * Recover from a timeout event.
 *
 * @param[in] rfm22b_dev  The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_timeout(struct pios_rfm22b_dev *rfm22b_dev)
{
    rfm22b_dev->stats.timeouts++;
    rfm22b_dev->packet_start_time = 0;
    // Release the Tx packet if it's set.
    if (rfm22b_dev->tx_packet_handle != 0) {
        rfm22b_dev->tx_data_rd = rfm22b_dev->tx_data_wr = 0;
    }
    rfm22b_dev->rfm22b_state = RFM22B_STATE_TRANSITION;
    rfm22b_dev->rx_buffer_wr = 0;
    TX_LED_OFF;
    RX_LED_OFF;
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
    D1_LED_OFF;
    D2_LED_OFF;
    D3_LED_OFF;
    D4_LED_OFF;
#endif
    return RADIO_EVENT_RX_MODE;
}

/**
 * Recover from a severe error.
 *
 * @param[in] rfm22b_dev  The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_error(struct pios_rfm22b_dev *rfm22b_dev)
{
    rfm22b_dev->stats.resets++;
    rfm22_clearLEDs();
    return RADIO_EVENT_INITIALIZE;
}

/**
 * A fatal error has occured in the state machine.
 * this should not happen.
 *
 * @parem [in] rfm22b_dev  The device structure
 * @return enum pios_radio_event  The next event to inject
 */
static enum pios_radio_event rfm22_fatal_error(__attribute__((unused)) struct pios_rfm22b_dev *rfm22b_dev)
{
    // RF module error .. flash the LED's
    rfm22_clearLEDs();
    for (unsigned int j = 0; j < 16; j++) {
        USB_LED_ON;
        LINK_LED_ON;
        RX_LED_OFF;
        TX_LED_OFF;

        PIOS_DELAY_WaitmS(200);

        USB_LED_OFF;
        LINK_LED_OFF;
        RX_LED_ON;
        TX_LED_ON;

        PIOS_DELAY_WaitmS(200);
    }

    PIOS_DELAY_WaitmS(1000);

    PIOS_Assert(0);

    return RADIO_EVENT_FATAL_ERROR;
}


/*****************************************************************************
* Utility Functions
*****************************************************************************/

/**
 * Get the current time in ms from the ticks counter.
 */
static uint32_t pios_rfm22_time_ms()
{
    return xTaskGetTickCount() * portTICK_RATE_MS;
}

/**
 * Calculate the time difference between the start time and end time.
 * Times are in ms.  Also handles rollover.
 *
 * @param[in] start_time  The start time in ms.
 * @param[in] end_time  The end time in ms.
 */
static uint32_t pios_rfm22_time_difference_ms(uint32_t start_time, uint32_t end_time)
{
    if (end_time >= start_time) {
        return end_time - start_time;
    }
    // Rollover
    return (UINT32_MAX - start_time) + end_time;
}

/**
 * Allocate the device structure
 */
#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_rfm22b_dev *pios_rfm22_alloc(void)
{
    struct pios_rfm22b_dev *rfm22b_dev;

    rfm22b_dev = (struct pios_rfm22b_dev *)pios_malloc(sizeof(*rfm22b_dev));
    if (!rfm22b_dev) {
        return NULL;
    }

    memset(rfm22b_dev, 0, sizeof(*rfm22b_dev));
    rfm22b_dev->magic = PIOS_RFM22B_DEV_MAGIC;

    return rfm22b_dev;
}
#else
static struct pios_rfm22b_dev pios_rfm22b_devs[PIOS_RFM22B_MAX_DEVS];
static uint8_t pios_rfm22b_num_devs;
static struct pios_rfm22b_dev *pios_rfm22_alloc(void)
{
    struct pios_rfm22b_dev *rfm22b_dev;

    if (pios_rfm22b_num_devs >= PIOS_RFM22B_MAX_DEVS) {
        return NULL;
    }

    rfm22b_dev = &pios_rfm22b_devs[pios_rfm22b_num_devs++];

    memset(rfm22b_dev, 0, sizeof(*rfm22b_dev));
    rfm22b_dev->magic = PIOS_RFM22B_DEV_MAGIC;

    return rfm22b_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */

/**
 * Turn off all of the LEDs
 */
static void rfm22_clearLEDs(void)
{
    LINK_LED_OFF;
    RX_LED_OFF;
    TX_LED_OFF;
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
    D1_LED_OFF;
    D2_LED_OFF;
    D3_LED_OFF;
    D4_LED_OFF;
#endif
}


/*****************************************************************************
* SPI Read/Write Functions
*****************************************************************************/

/**
 * Assert the chip select line.
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 */
static void rfm22_assertCs(struct pios_rfm22b_dev *rfm22b_dev)
{
    PIOS_DELAY_WaituS(1);
    if (rfm22b_dev->spi_id != 0) {
        PIOS_SPI_RC_PinSet(rfm22b_dev->spi_id, rfm22b_dev->slave_num, 0);
    }
}

/**
 * Deassert the chip select line.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_deassertCs(struct pios_rfm22b_dev *rfm22b_dev)
{
    if (rfm22b_dev->spi_id != 0) {
        PIOS_SPI_RC_PinSet(rfm22b_dev->spi_id, rfm22b_dev->slave_num, 1);
    }
}

/**
 * Claim the SPI bus.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_claimBus(struct pios_rfm22b_dev *rfm22b_dev)
{
    if (rfm22b_dev->spi_id != 0) {
        PIOS_SPI_ClaimBus(rfm22b_dev->spi_id);
    }
}

/**
 * Release the SPI bus.
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 */
static void rfm22_releaseBus(struct pios_rfm22b_dev *rfm22b_dev)
{
    if (rfm22b_dev->spi_id != 0) {
        PIOS_SPI_ReleaseBus(rfm22b_dev->spi_id);
    }
}

/**
 * Claim the semaphore and write a byte to a register
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 * @param[in] addr The address to write to
 * @param[in] data The datat to write to that address
 */
static void rfm22_write_claim(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr, uint8_t data)
{
    rfm22_claimBus(rfm22b_dev);
    rfm22_assertCs(rfm22b_dev);
    uint8_t buf[2] = { addr | 0x80, data };
    PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, buf, NULL, sizeof(buf), NULL);
    rfm22_deassertCs(rfm22b_dev);
    rfm22_releaseBus(rfm22b_dev);
}

/**
 * Write a byte to a register without claiming the semaphore
 *
 * @param[in] rfm22b_dev  The RFM22B device.
 * @param[in] addr The address to write to
 * @param[in] data The datat to write to that address
 */
static void rfm22_write(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr, uint8_t data)
{
    rfm22_assertCs(rfm22b_dev);
    uint8_t buf[2] = { addr | 0x80, data };
    PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, buf, NULL, sizeof(buf), NULL);
    rfm22_deassertCs(rfm22b_dev);
}

/**
 * Read a byte from an RFM22b register without claiming the bus
 *
 * @param[in] rfm22b_dev  The RFM22B device structure pointer.
 * @param[in] addr The address to read from
 * @return Returns the result of the register read
 */
static uint8_t rfm22_read(struct pios_rfm22b_dev *rfm22b_dev, uint8_t addr)
{
    uint8_t out[2] = { addr &0x7F, 0xFF };
    uint8_t in[2];

    rfm22_assertCs(rfm22b_dev);
    PIOS_SPI_TransferBlock(rfm22b_dev->spi_id, out, in, sizeof(out), NULL);
    rfm22_deassertCs(rfm22b_dev);
    return in[1];
}


static void rfm22_hmac_sha1(const uint8_t *data, size_t len,
                            uint8_t key[SHA1_DIGEST_LENGTH],
                            uint8_t digest[SHA1_DIGEST_LENGTH])
{
    uint8_t ipad[64] = { 0 };
    uint8_t opad[64] = { 0 };
    static SHA1_CTX *ctx;

    ctx = pios_malloc(sizeof(SHA1_CTX));

    memcpy(ipad, key, SHA1_DIGEST_LENGTH);
    memcpy(opad, key, SHA1_DIGEST_LENGTH);

    for (int i = 0; i < 64; i++) {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }

    SHA1Init(ctx);
    SHA1Update(ctx, ipad, sizeof(ipad));
    SHA1Update(ctx, data, len);
    SHA1Final(digest, ctx);

    SHA1Init(ctx);
    SHA1Update(ctx, opad, sizeof(opad));
    SHA1Update(ctx, digest, SHA1_DIGEST_LENGTH);
    SHA1Final(digest, ctx);

    pios_free(ctx);
}

static bool rfm22_gen_channels(uint32_t coordid, enum rfm22b_datarate rate, uint8_t min,
                               uint8_t max, uint8_t channels[MAX_CHANNELS], uint8_t *clen)
{
    // Define first and last channel to be used within min/max values
    // according to the frequency deviation, without up/down overflow.
    uint8_t chan_min_limit = min + channel_limits[rate];
    uint8_t chan_max_limit = max - channel_limits[rate];

    // Define how many channels we can use according to the spacing.
    uint8_t chan_count     = ((chan_max_limit - chan_min_limit) / channel_spacing[rate]) + 1;

    uint32_t data = 0;
    uint8_t cpos  = 0;
    uint8_t key[SHA1_DIGEST_LENGTH] = { 0 };
    uint8_t digest[SHA1_DIGEST_LENGTH];
    uint8_t *all_channels;

    all_channels = pios_malloc(RFM22B_NUM_CHANNELS);

    memcpy(key, &coordid, sizeof(coordid));

    // Fill all_channels[] with usable channels
    for (int i = 0; i < chan_count; i++) {
        all_channels[i] = chan_min_limit + (i * channel_spacing[rate]);
    }

    // DEBUG_PRINTF(3, "\r\nChannel Min: %d Max:%d - Spacing: %d Limits: %d\r\n", min, max, channel_spacing[rate], channel_limits[rate]);
    // DEBUG_PRINTF(3, "Result: Channel count: %d - Usable channels from ch%d to ch%d\r\n", chan_count, all_channels[0], all_channels[chan_count - 1]);

    int j = SHA1_DIGEST_LENGTH;
    for (int i = 0; i < chan_count && i < MAX_CHANNELS; i++) {
        uint8_t rnd;
        uint8_t r;
        uint8_t tmp;

        if (j == SHA1_DIGEST_LENGTH) {
            rfm22_hmac_sha1((uint8_t *)&data, sizeof(data), key, digest);
            j = 0;
            data++;
        }
        rnd = digest[j];
        j++;
        r   = rnd % (chan_count - i) + i;
        tmp = all_channels[i];
        all_channels[i] = all_channels[r];
        all_channels[r] = tmp;
    }

    // DEBUG_PRINTF(3, "Final channel list:");
    for (int i = 0; i < chan_count && cpos < MAX_CHANNELS; i++, cpos++) {
        channels[cpos] = all_channels[i];
        // DEBUG_PRINTF(3, " %d ", all_channels[i]);
    }

    *clen = cpos & 0xfe;

    pios_free(all_channels);

    return *clen > 0;
}

#endif /* PIOS_INCLUDE_RFM22B */

/**
 * @}
 * @}
 */
