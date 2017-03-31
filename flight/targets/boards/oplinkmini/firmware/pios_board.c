/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 *
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the OpenPilot board.
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

#include "inc/openpilot.h"
#include <pios_board_info.h>
#include <pios_ppm_out.h>
#include <oplinksettings.h>
#include <pios_openlrs.h>
#include <taskinfo.h>
#ifdef PIOS_INCLUDE_SERVO
#include <pios_servo.h>
#endif

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

#define PIOS_COM_TELEM_USB_RX_BUF_LEN 128
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 128

#define PIOS_COM_TELEM_VCP_RX_BUF_LEN 128
#define PIOS_COM_TELEM_VCP_TX_BUF_LEN 128

#define PIOS_COM_RFM22B_RF_RX_BUF_LEN 128
#define PIOS_COM_RFM22B_RF_TX_BUF_LEN 128

#define PIOS_COM_TELEM_RX_BUF_LEN     128
#define PIOS_COM_TELEM_TX_BUF_LEN     128

uint32_t pios_com_hid_id     = 0;
uint32_t pios_com_vcp_id     = 0;
uint32_t pios_com_main_id    = 0;
uint32_t pios_com_flexi_id   = 0;
uint32_t pios_com_bridge_id  = 0;
uint32_t pios_com_gcs_id     = 0;
uint32_t pios_com_gcs_out_id = 0;
#if defined(PIOS_INCLUDE_PPM)
uint32_t pios_ppm_rcvr_id    = 0;
#endif
#if defined(PIOS_INCLUDE_PPM_OUT)
uint32_t pios_ppm_out_id     = 0;
#endif
#if defined(PIOS_INCLUDE_RFM22B)
#include <pios_rfm22b_com.h>
uint32_t pios_rfm22b_id = 0;
uint32_t pios_com_pri_radio_id     = 0;
uint32_t pios_com_aux_radio_id     = 0;
uint32_t pios_com_pri_radio_out_id = 0;
uint32_t pios_com_aux_radio_out_id = 0;
#endif

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id  = 0;

static uint8_t servo_count = 0;

/*
 * Setup a com port based on the passed cfg, driver and buffer sizes.
 * tx size of 0 make the port rx only
 * rx size of 0 make the port tx only
 * having both tx and rx size of 0 is not valid and will fail further down in PIOS_COM_Init()
 */
static void PIOS_Board_configure_com(const struct pios_usart_cfg *usart_port_cfg, uint16_t rx_buf_len, uint16_t tx_buf_len,
                                     const struct pios_com_driver *com_driver, uint32_t *pios_com_id)
{
    uint32_t pios_usart_id;

    if (PIOS_USART_Init(&pios_usart_id, usart_port_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *rx_buffer = 0, *tx_buffer = 0;

    if (rx_buf_len > 0) {
        rx_buffer = (uint8_t *)pios_malloc(rx_buf_len);
        PIOS_Assert(rx_buffer);
    }

    if (tx_buf_len > 0) {
        tx_buffer = (uint8_t *)pios_malloc(tx_buf_len);
        PIOS_Assert(tx_buffer);
    }

    if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                      rx_buffer, rx_buf_len,
                      tx_buffer, tx_buf_len)) {
        PIOS_Assert(0);
    }
}


// Forward definitions
static void PIOS_Board_PPM_callback(uint32_t context, const int16_t *channels);

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
void PIOS_Board_Init(void)
{
    /* Delay system */
    PIOS_DELAY_Init();

#ifdef PIOS_INCLUDE_FLASH_LOGFS_SETTINGS
    uintptr_t flash_id;
    PIOS_Flash_Internal_Init(&flash_id, &flash_internal_cfg);
    PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id);
#endif

    /* Initialize the task monitor */
    if (PIOS_TASK_MONITOR_Initialize(TASKINFO_RUNNING_NUMELEM)) {
        PIOS_Assert(0);
    }

    /* Initialize the delayed callback library */
    PIOS_CALLBACKSCHEDULER_Initialize();

    /* Initialize UAVObject libraries */
    EventDispatcherInitialize();
    UAVObjInitialize();

    /* Set up the SPI interface to the rfm22b */
    if (PIOS_SPI_Init(&pios_spi_rfm22b_id, &pios_spi_rfm22b_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

#ifdef PIOS_INCLUDE_WDG
    /* Initialize watchdog as early as possible to catch faults during init */
    PIOS_WDG_Init();
#endif /* PIOS_INCLUDE_WDG */

#if defined(PIOS_INCLUDE_RTC)
    /* Initialize the real-time clock and its associated tick */
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif /* PIOS_INCLUDE_RTC */

#if defined(PIOS_INCLUDE_LED)
    PIOS_LED_Init(&pios_led_cfg);
#endif /* PIOS_INCLUDE_LED */

    /* IAP System Setup */
    PIOS_IAP_Init();
    // check for safe mode commands from gcs
    if (PIOS_IAP_ReadBootCmd(0) == PIOS_IAP_CLEAR_FLASH_CMD_0 &&
        PIOS_IAP_ReadBootCmd(1) == PIOS_IAP_CLEAR_FLASH_CMD_1 &&
        PIOS_IAP_ReadBootCmd(2) == PIOS_IAP_CLEAR_FLASH_CMD_2) {
        PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
        PIOS_IAP_WriteBootCmd(0, 0);
        PIOS_IAP_WriteBootCmd(1, 0);
        PIOS_IAP_WriteBootCmd(2, 0);
    }

#if defined(PIOS_INCLUDE_RFM22B)
    OPLinkSettingsInitialize();
    OPLinkStatusInitialize();
#endif /* PIOS_INCLUDE_RFM22B */

    /* Retrieve the settings object. */
    OPLinkSettingsData oplinkSettings;
    OPLinkSettingsGet(&oplinkSettings);

    /* Determine the modem protocols */
    bool is_coordinator = (oplinkSettings.Protocol == OPLINKSETTINGS_PROTOCOL_OPLINKCOORDINATOR);
    bool openlrs     = (oplinkSettings.Protocol == OPLINKSETTINGS_PROTOCOL_OPENLRS);
    bool ppm_only    = (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_CONTROL);
    bool data_mode   = ((oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATA) ||
                        (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATAANDCONTROL));
    bool is_enabled  = ((oplinkSettings.Protocol != OPLINKSETTINGS_PROTOCOL_DISABLED) &&
                        ((oplinkSettings.MaxRFPower != OPLINKSETTINGS_MAXRFPOWER_0) || openlrs));
    bool ppm_mode    = ((oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_CONTROL) ||
                        (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATAANDCONTROL));
    bool servo_main  = false;
    bool servo_flexi = false;

#if defined(PIOS_INCLUDE_TIM)
    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
#endif /* PIOS_INCLUDE_TIM */

    /* Initialize board specific USB data */
    PIOS_USB_BOARD_DATA_Init();


#if defined(PIOS_INCLUDE_USB_CDC)
    /* Flags to determine if various USB interfaces are advertised */
    bool usb_cdc_present = false;

    if (PIOS_USB_DESC_HID_CDC_Init()) {
        PIOS_Assert(0);
    }
    usb_cdc_present = true;
#else
    if (PIOS_USB_DESC_HID_ONLY_Init()) {
        PIOS_Assert(0);
    }
#endif

    /*Initialize the USB device */
    uint32_t pios_usb_id;
    PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg);

    /* Configure the USB HID port */
    {
        uint32_t pios_usb_hid_id;
        if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
            PIOS_Assert(0);
        }
        uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
        PIOS_Assert(rx_buffer);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_hid_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
                          rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                          tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }

    /* Configure the USB virtual com port (VCP) */
#if defined(PIOS_INCLUDE_USB_CDC)
    if (usb_cdc_present) {
        uint32_t pios_usb_cdc_id;
        if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
            PIOS_Assert(0);
        }
        uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_VCP_RX_BUF_LEN);
        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_VCP_TX_BUF_LEN);
        PIOS_Assert(rx_buffer);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                          rx_buffer, PIOS_COM_TELEM_VCP_RX_BUF_LEN,
                          tx_buffer, PIOS_COM_TELEM_VCP_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
#endif

    // Configure the main port
    uint32_t mainComSpeed = 0;
    switch (oplinkSettings.MainComSpeed) {
    case OPLINKSETTINGS_MAINCOMSPEED_4800:
        mainComSpeed = 4800;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_9600:
        mainComSpeed = 9600;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_19200:
        mainComSpeed = 19200;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_38400:
        mainComSpeed = 38400;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_57600:
        mainComSpeed = 57600;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_115200:
        mainComSpeed = 115200;
        break;
    case OPLINKSETTINGS_MAINCOMSPEED_DISABLED:
        break;
    }
    switch (oplinkSettings.MainPort) {
    case OPLINKSETTINGS_MAINPORT_TELEMETRY:
    case OPLINKSETTINGS_MAINPORT_SERIAL:
#ifndef PIOS_RFM22B_DEBUG_ON_TELEM
        PIOS_Board_configure_com(&pios_usart_main_cfg,
                                 PIOS_COM_TELEM_RX_BUF_LEN, PIOS_COM_TELEM_TX_BUF_LEN,
                                 &pios_usart_com_driver, &pios_com_main_id);
        PIOS_COM_ChangeBaud(pios_com_main_id, mainComSpeed);
#endif /* !PIOS_RFM22B_DEBUG_ON_TELEM */
        break;
    case OPLINKSETTINGS_MAINPORT_PPM:
#if defined(PIOS_INCLUDE_PPM)
        /* PPM input is configured on the coordinator modem and output on the remote modem. */
        if (is_coordinator) {
            uint32_t pios_ppm_id;
            PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_main_cfg);

            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
        }
        // For some reason, PPM output on the main port doesn't work.
#if defined(PIOS_INCLUDE_PPM_OUT)
        else {
            PIOS_PPM_Out_Init(&pios_ppm_out_id, &pios_main_ppm_out_cfg);
        }
#endif /* PIOS_INCLUDE_PPM_OUT */
#endif /* PIOS_INCLUDE_PPM */
        break;
    case OPLINKSETTINGS_MAINPORT_PWM:
        servo_main = true;
    default:
        break;
    }

    // Configure the flexi port
    uint32_t flexiComSpeed = 0;
    switch (oplinkSettings.FlexiComSpeed) {
    case OPLINKSETTINGS_FLEXICOMSPEED_4800:
        flexiComSpeed = 4800;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_9600:
        flexiComSpeed = 9600;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_19200:
        flexiComSpeed = 19200;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_38400:
        flexiComSpeed = 38400;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_57600:
        flexiComSpeed = 57600;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_115200:
        flexiComSpeed = 115200;
        break;
    case OPLINKSETTINGS_FLEXICOMSPEED_DISABLED:
        break;
    }
    switch (oplinkSettings.FlexiPort) {
    case OPLINKSETTINGS_FLEXIPORT_TELEMETRY:
    case OPLINKSETTINGS_FLEXIPORT_SERIAL:
#ifndef PIOS_RFM22B_DEBUG_ON_TELEM
        PIOS_Board_configure_com(&pios_usart_flexi_cfg,
                                 PIOS_COM_TELEM_RX_BUF_LEN, PIOS_COM_TELEM_TX_BUF_LEN,
                                 &pios_usart_com_driver, &pios_com_flexi_id);
        PIOS_COM_ChangeBaud(pios_com_flexi_id, flexiComSpeed);
#endif /* !PIOS_RFM22B_DEBUG_ON_TELEM */
        break;
    case OPLINKSETTINGS_FLEXIPORT_PPM:
#if defined(PIOS_INCLUDE_PPM)
        /* PPM input is configured on the coordinator modem and output on the remote modem. */
        if (is_coordinator) {
            uint32_t pios_ppm_id;
            PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_flexi_cfg);

            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
        }
        // For some reason, PPM output on the flexi port doesn't work.
#if defined(PIOS_INCLUDE_PPM_OUT)
        else {
            PIOS_PPM_Out_Init(&pios_ppm_out_id, &pios_flexi_ppm_out_cfg);
        }
#endif /* PIOS_INCLUDE_PPM_OUT */
#endif /* PIOS_INCLUDE_PPM */
        break;
    case OPLINKSETTINGS_FLEXIPORT_PWM:
        servo_flexi = true;
    default:
        break;
    }

    /* Configure the PWM servo outputs. */
#if defined(PIOS_INCLUDE_SERVO)
    if (servo_main) {
        if (servo_flexi) {
            servo_count = 4;
            PIOS_Servo_Init(&pios_servo_main_flexi_cfg);
        } else {
            servo_count = 2;
            PIOS_Servo_Init(&pios_servo_main_cfg);
        }
    } else if (servo_flexi) {
        servo_count = 2;
        PIOS_Servo_Init(&pios_servo_flexi_cfg);
    }

    // Set bank modes
    PIOS_Servo_SetBankMode(0, PIOS_SERVO_BANK_MODE_PWM);
    PIOS_Servo_SetBankMode(1, PIOS_SERVO_BANK_MODE_PWM);
#endif

    // Initialize out status object.
    OPLinkStatusData oplinkStatus;
    OPLinkStatusGet(&oplinkStatus);

    // Get our hardware information.
    const struct pios_board_info *bdinfo = &pios_board_info_blob;

    oplinkStatus.BoardType     = bdinfo->board_type;
    PIOS_BL_HELPER_FLASH_Read_Description(oplinkStatus.Description, OPLINKSTATUS_DESCRIPTION_NUMELEM);
    PIOS_SYS_SerialNumberGetBinary(oplinkStatus.CPUSerial);
    oplinkStatus.BoardRevision = bdinfo->board_rev;

    /* Initialize the RFM22B radio COM device. */
    if (is_enabled) {
        if (openlrs) {
#if defined(PIOS_INCLUDE_OPENLRS)
            const struct pios_openlrs_cfg *openlrs_cfg = PIOS_BOARD_HW_DEFS_GetOpenLRSCfg(bdinfo->board_rev);
            uint32_t openlrs_id;

            oplinkStatus.LinkState = OPLINKSTATUS_LINKSTATE_ENABLED;

            PIOS_OpenLRS_Init(&openlrs_id, PIOS_RFM22_SPI_PORT, 0, openlrs_cfg);
            PIOS_OpenLRS_RegisterPPMCallback(openlrs_id, PIOS_Board_PPM_callback, 0);
#endif /* PIOS_INCLUDE_OPENLRS */
        } else {
            oplinkStatus.LinkState = OPLINKSTATUS_LINKSTATE_ENABLED;

            // Configure the RFM22B device
            const struct pios_rfm22b_cfg *rfm22b_cfg = PIOS_BOARD_HW_DEFS_GetRfm22Cfg(bdinfo->board_rev);

            if (PIOS_RFM22B_Init(&pios_rfm22b_id, PIOS_RFM22_SPI_PORT, rfm22b_cfg->slave_num, rfm22b_cfg, oplinkSettings.RFBand)) {
                PIOS_Assert(0);
            }

            // Configure the radio com interface
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_RFM22B_RF_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_RFM22B_RF_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_pri_radio_id, &pios_rfm22b_com_driver, pios_rfm22b_id,
                              rx_buffer, PIOS_COM_RFM22B_RF_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_RFM22B_RF_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }

            // Initialize the aux radio com interface
            uint8_t *auxrx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RX_BUF_LEN);
            uint8_t *auxtx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_TX_BUF_LEN);
            PIOS_Assert(auxrx_buffer);
            PIOS_Assert(auxtx_buffer);
            if (PIOS_COM_Init(&pios_com_aux_radio_id, &pios_rfm22b_aux_com_driver, pios_rfm22b_id,
                              auxrx_buffer, PIOS_COM_TELEM_RX_BUF_LEN,
                              auxtx_buffer, PIOS_COM_TELEM_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }

            // Set the modem (over the air) datarate.
            enum rfm22b_datarate datarate = RFM22_datarate_64000;
            switch (oplinkSettings.AirDataRate) {
            case OPLINKSETTINGS_AIRDATARATE_9600:
                datarate = RFM22_datarate_9600;
                break;
            case OPLINKSETTINGS_AIRDATARATE_19200:
                datarate = RFM22_datarate_19200;
                break;
            case OPLINKSETTINGS_AIRDATARATE_32000:
                datarate = RFM22_datarate_32000;
                break;
            case OPLINKSETTINGS_AIRDATARATE_57600:
                datarate = RFM22_datarate_57600;
                break;
            case OPLINKSETTINGS_AIRDATARATE_64000:
                datarate = RFM22_datarate_64000;
                break;
            case OPLINKSETTINGS_AIRDATARATE_100000:
                datarate = RFM22_datarate_100000;
                break;
            case OPLINKSETTINGS_AIRDATARATE_128000:
                datarate = RFM22_datarate_128000;
                break;
            case OPLINKSETTINGS_AIRDATARATE_192000:
                datarate = RFM22_datarate_192000;
                break;
            case OPLINKSETTINGS_AIRDATARATE_256000:
                datarate = RFM22_datarate_256000;
                break;
            }

            /* Set the modem Tx power level */
            switch (oplinkSettings.MaxRFPower) {
            case OPLINKSETTINGS_MAXRFPOWER_125:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_0);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_16:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_1);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_316:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_2);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_63:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_3);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_126:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_4);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_25:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_5);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_50:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_6);
                break;
            case OPLINKSETTINGS_MAXRFPOWER_100:
                PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_7);
                break;
            default:
                // do nothing
                break;
            }

            // Set the radio configuration parameters.
            PIOS_RFM22B_SetDeviceID(pios_rfm22b_id, oplinkSettings.CustomDeviceID);
            PIOS_RFM22B_SetCoordinatorID(pios_rfm22b_id, oplinkSettings.CoordID);
            PIOS_RFM22B_SetXtalCap(pios_rfm22b_id, oplinkSettings.RFXtalCap);
            PIOS_RFM22B_SetChannelConfig(pios_rfm22b_id, datarate, oplinkSettings.MinChannel, oplinkSettings.MaxChannel, is_coordinator, data_mode, ppm_mode);

            /* Set the PPM callback if we should be receiving PPM. */
            if (ppm_mode || (ppm_only && !is_coordinator)) {
                PIOS_RFM22B_SetPPMCallback(pios_rfm22b_id, PIOS_Board_PPM_callback, 0);
            }
        } // openlrs
    } else {
        oplinkStatus.LinkState = OPLINKSTATUS_LINKSTATE_DISABLED;
    }

    // Configure the primary radio stream destination.
    switch (oplinkSettings.RadioPriStream) {
    case OPLINKSETTINGS_RADIOPRISTREAM_DISABLED:
        break;
    case OPLINKSETTINGS_RADIOPRISTREAM_HID:
        // HID is always connected to GCS
        pios_com_gcs_id     = pios_com_hid_id;
        pios_com_gcs_out_id = pios_com_pri_radio_id;
        break;
    case OPLINKSETTINGS_RADIOPRISTREAM_MAIN:
        // Is the main port configured for telemetry (GCS)?
        if (oplinkSettings.MainPort == OPLINKSETTINGS_MAINPORT_TELEMETRY) {
            pios_com_gcs_id     = pios_com_main_id;
            pios_com_gcs_out_id = pios_com_pri_radio_id;
        } else {
            PIOS_COM_LinkComPair(pios_com_pri_radio_id, pios_com_main_id, false, false);
        }
        break;
    case OPLINKSETTINGS_RADIOPRISTREAM_FLEXI:
        // Is the flexi port configured for telemetry (GCS)?
        if (oplinkSettings.FlexiPort == OPLINKSETTINGS_FLEXIPORT_TELEMETRY) {
            pios_com_gcs_id     = pios_com_flexi_id;
            pios_com_gcs_out_id = pios_com_pri_radio_id;
        } else {
            PIOS_COM_LinkComPair(pios_com_pri_radio_id, pios_com_flexi_id, false, false);
        }
        break;
    case OPLINKSETTINGS_RADIOPRISTREAM_VCP:
        // VCP is never connected to GCS
        PIOS_COM_LinkComPair(pios_com_pri_radio_id, pios_com_vcp_id, false, false);
        break;
    }

    // Configure the Auxiliary radio stream destination.
    switch (oplinkSettings.RadioAuxStream) {
    case OPLINKSETTINGS_RADIOAUXSTREAM_DISABLED:
        break;
    case OPLINKSETTINGS_RADIOAUXSTREAM_HID:
        // HID is always connected to GCS
        pios_com_gcs_id     = pios_com_hid_id;
        pios_com_gcs_out_id = pios_com_aux_radio_id;
        break;
    case OPLINKSETTINGS_RADIOAUXSTREAM_MAIN:
        // Is the main port configured for telemetry (GCS)?
        if (oplinkSettings.MainPort == OPLINKSETTINGS_MAINPORT_TELEMETRY) {
            pios_com_gcs_id     = pios_com_main_id;
            pios_com_gcs_out_id = pios_com_aux_radio_id;
        } else {
            PIOS_COM_LinkComPair(pios_com_aux_radio_id, pios_com_main_id, false, false);
        }
        break;
    case OPLINKSETTINGS_RADIOAUXSTREAM_FLEXI:
        // Is the flexi port configured for telemetry (GCS)?
        if (oplinkSettings.FlexiPort == OPLINKSETTINGS_FLEXIPORT_TELEMETRY) {
            pios_com_gcs_id     = pios_com_flexi_id;
            pios_com_gcs_out_id = pios_com_aux_radio_id;
        } else {
            PIOS_COM_LinkComPair(pios_com_aux_radio_id, pios_com_flexi_id, false, false);
        }
        break;
    case OPLINKSETTINGS_RADIOAUXSTREAM_VCP:
        // VCP is never connected to GCS
        PIOS_COM_LinkComPair(pios_com_aux_radio_id, pios_com_vcp_id, false, false);
        break;
    }

    // Configure the VCP COM bridge
    switch (oplinkSettings.VCPBridge) {
    case OPLINKSETTINGS_VCPBRIDGE_MAIN:
        PIOS_COM_LinkComPair(pios_com_vcp_id, pios_com_main_id, true, true);
        break;
    case OPLINKSETTINGS_VCPBRIDGE_FLEXI:
        PIOS_COM_LinkComPair(pios_com_vcp_id, pios_com_flexi_id, true, true);
        break;
    case OPLINKSETTINGS_VCPBRIDGE_DISABLED:
        break;
    }

    // Update the object
    OPLinkStatusSet(&oplinkStatus);

    /* Remap AFIO pin */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);

#ifdef PIOS_INCLUDE_ADC
    PIOS_ADC_Init();
#endif
}

static void PIOS_Board_PPM_callback(__attribute__((unused)) uint32_t context, const int16_t *channels)
{
#if defined(PIOS_INCLUDE_PPM) && defined(PIOS_INCLUDE_PPM_OUT)
    if (pios_ppm_out_id) {
        for (uint8_t i = 0; i < RFM22B_PPM_NUM_CHANNELS; ++i) {
            if (channels[i] != PIOS_RCVR_INVALID) {
                PIOS_PPM_OUT_Set(PIOS_PPM_OUTPUT, i, channels[i]);
            }
        }
    }
#if defined(PIOS_INCLUDE_SERVO)
    for (uint8_t i = 0; i < servo_count; ++i) {
        uint16_t val = (channels[i] == PIOS_RCVR_INVALID) ? 0 : channels[i];
        PIOS_Servo_Set(i, val);
    }
#endif /* PIOS_INCLUDE_SERVO */
#endif /* PIOS_INCLUDE_PPM && PIOS_INCLUDE_PPM_OUT */
}

/**
 * @}
 */
