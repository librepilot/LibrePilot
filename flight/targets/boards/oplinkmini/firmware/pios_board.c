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
#include <oplinkreceiver.h>
#include <pios_openlrs.h>
#include <taskinfo.h>
#ifdef PIOS_INCLUDE_SERVO
#include <pios_servo.h>
#endif
#include <pios_board_io.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

uint32_t pios_com_hid_id     = 0;
// uint32_t pios_com_vcp_id     = 0; /* this is provided by pios_board_io.c */
uint32_t pios_com_main_id    = 0;
uint32_t pios_com_flexi_id   = 0;
uint32_t pios_com_gcs_id     = 0;
uint32_t pios_com_gcs_out_id = 0;
#if defined(PIOS_INCLUDE_PPM_OUT)
uint32_t pios_ppm_out_id     = 0;
bool ppm_rssi = false;
#endif
#if defined(PIOS_INCLUDE_RFM22B)
#include <pios_rfm22b_com.h>
uint32_t pios_com_pri_radio_out_id = 0;
uint32_t pios_com_aux_radio_out_id = 0;
#endif

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id  = 0;

static uint8_t servo_count = 0;

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
    SETTINGS_INITIALISE_ALL;

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

    OPLinkReceiverInitialize();

    /* Retrieve the settings object. */
    OPLinkSettingsData oplinkSettings;
    OPLinkSettingsGet(&oplinkSettings);

    /* Determine the modem protocols */
    bool is_coordinator = (oplinkSettings.Protocol == OPLINKSETTINGS_PROTOCOL_OPLINKCOORDINATOR);
    bool ppm_only    = (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_CONTROL);
    bool ppm_mode    = ((oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_CONTROL) ||
                        (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATAANDCONTROL));
    ppm_rssi = (oplinkSettings.PPMOutRSSI == OPLINKSETTINGS_PPMOUTRSSI_TRUE);
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
        PIOS_BOARD_IO_Configure_UART_COM(&pios_usart_main_cfg,
                                         PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN,
                                         &pios_com_main_id);
        PIOS_COM_ChangeBaud(pios_com_main_id, mainComSpeed);
#endif /* !PIOS_RFM22B_DEBUG_ON_TELEM */
        break;
    case OPLINKSETTINGS_MAINPORT_PPM:
#if defined(PIOS_INCLUDE_PPM)
        /* PPM input is configured on the coordinator modem and output on the remote modem. */
        if (is_coordinator) {
            PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_main_cfg);
        }
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
        PIOS_BOARD_IO_Configure_UART_COM(&pios_usart_flexi_cfg,
                                         PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN,
                                         &pios_com_flexi_id);
        PIOS_COM_ChangeBaud(pios_com_flexi_id, flexiComSpeed);
#endif /* !PIOS_RFM22B_DEBUG_ON_TELEM */
        break;
    case OPLINKSETTINGS_FLEXIPORT_PPM:
#if defined(PIOS_INCLUDE_PPM)
        /* PPM input is configured on the coordinator modem and output on the remote modem. */
        if (is_coordinator) {
            PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_flexi_cfg);
        }
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

    // Set bank modes and activate output. We need to do this here because oplm does not run Actuator module.
    PIOS_Servo_SetBankMode(0, PIOS_SERVO_BANK_MODE_PWM);
    PIOS_Servo_SetBankMode(1, PIOS_SERVO_BANK_MODE_PWM);
    PIOS_Servo_SetActive(0b11);
#endif

    PIOS_BOARD_IO_Configure_RFM22B();

    /* Set the PPM callback if we should be receiving PPM. */
    if (ppm_mode || (ppm_only && !is_coordinator)) {
        PIOS_RFM22B_SetPPMCallback(pios_rfm22b_id, PIOS_Board_PPM_callback, 0);
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
            if ((channels[i] != PIOS_RCVR_INVALID) && (channels[i] != PIOS_RCVR_TIMEOUT)) {
                PIOS_PPM_OUT_Set(PIOS_PPM_OUTPUT, i, channels[i]);
            }
        }
        // Rssi channel output is added after RC channels
        // Output Rssi from 1000µs to 2000µs (-127dBm to -16dBm range)
        if (ppm_rssi) {
            int8_t rssi;
            int16_t ppm_value;
            OPLinkStatusRSSIGet(&rssi);
            ppm_value = 1000 + ((rssi + 127) * 9);
            PIOS_PPM_OUT_Set(PIOS_PPM_OUTPUT, RFM22B_PPM_NUM_CHANNELS, ppm_value);
        }
    } else {
        // If there is no PPM output defined, try sending the control outputs as a UAVObject.
        OPLinkReceiverData recv_data;
        for (uint8_t i = 0; i < OPLINKRECEIVER_CHANNEL_NUMELEM; ++i) {
            if (i < RFM22B_PPM_NUM_CHANNELS) {
                recv_data.Channel[i] = channels[i];
            } else {
                recv_data.Channel[i] = PIOS_RCVR_TIMEOUT;
            }
        }
        // Update the RSSI and quality fields.
        int8_t rssi;
        OPLinkStatusRSSIGet(&rssi);
        recv_data.RSSI = rssi;
        uint16_t quality;
        OPLinkStatusLinkQualityGet(&quality);
        // Link quality is 0-128, so scale it down to 0-100
        recv_data.LinkQuality = quality * 100 / 128;
        OPLinkReceiverSet(&recv_data);
    }
#if defined(PIOS_INCLUDE_SERVO)
    for (uint8_t i = 0; i < servo_count; ++i) {
        uint16_t val = ((channels[i] == PIOS_RCVR_INVALID) || (channels[i] == PIOS_RCVR_TIMEOUT)) ? 0 : channels[i];
        PIOS_Servo_Set(i, val);
    }
#endif /* PIOS_INCLUDE_SERVO */
#endif /* PIOS_INCLUDE_PPM && PIOS_INCLUDE_PPM_OUT */
}

/**
 * @}
 */
