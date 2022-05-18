/**
 ******************************************************************************
 *
 * @file       pios_board_io.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      board io setup
 *             --
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
#include <pios_board_io.h>
#include <pios_board_hw.h>
#include <pios_board_info.h>

#include "uavobjectmanager.h"

#include <alarms.h>

#ifdef PIOS_INCLUDE_RCVR
# ifdef PIOS_INCLUDE_HOTT
#  include <pios_hott_priv.h>
# endif
# ifdef PIOS_INCLUDE_DSM
#  include <pios_dsm_priv.h>
# endif
# ifdef PIOS_INCLUDE_SBUS
#  include <pios_sbus_priv.h>
# endif
# ifdef PIOS_INCLUDE_IBUS
#  include <pios_ibus_priv.h>
# endif
# ifdef PIOS_INCLUDE_EXBUS
#  include <pios_exbus_priv.h>
# endif
# ifdef PIOS_INCLUDE_SRXL
#  include <pios_srxl_priv.h>
# endif
# include <pios_rcvr_priv.h>
# ifdef PIOS_INCLUDE_GCSRCVR
#  include <pios_gcsrcvr_priv.h>
# endif
# ifdef PIOS_INCLUDE_OPLINKRCVR
#  include <oplinkreceiver.h>
#  include <pios_oplinkrcvr_priv.h>
# endif
#endif /* PIOS_INCLUDE_RCVR */

#ifdef PIOS_INCLUDE_RFM22B
# include <oplinksettings.h>
# include <oplinkstatus.h>
# ifdef PIOS_INCLUDE_RCVR
#  include <pios_openlrs.h>
#  include <pios_openlrs_rcvr_priv.h>
# endif /* PIOS_INCLUDE_RCVR */
#endif /* PIOS_INCLUDE_RFM22B */

#ifdef PIOS_INCLUDE_RCVR
# include "manualcontrolsettings.h"
uint32_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE]; /* Receivers */
#endif /* PIOS_INCLUDE_RCVR */

#include "hwsettings.h"
#include "gcsreceiver.h"

#ifdef PIOS_INCLUDE_GPS
uint32_t pios_com_gps_id; /* GPS */
#endif /* PIOS_INCLUDE_GPS */

uint32_t pios_com_bridge_id; /* ComUsbBridge */
uint32_t pios_com_telem_rf_id; /* Serial port telemetry */

#ifdef PIOS_INCLUDE_RFM22B
uint32_t pios_rfm22b_id; /* RFM22B handle */
uint32_t pios_com_pri_radio_id; /* oplink primary com stream */
uint32_t pios_com_aux_radio_id; /* oplink aux com stream */
#endif

#ifdef PIOS_INCLUDE_OPENLRS
uint32_t pios_openlrs_id; /* OpenLRS handle */
#endif

uint32_t pios_com_hkosd_id; /* HK OSD ?? */
uint32_t pios_com_msp_id; /* MSP */
uint32_t pios_com_mavlink_id; /* MAVLink */
uint32_t pios_com_frsky_sensorhub_id; /* Frsky Sensorhub */
uint32_t pios_com_vcp_id; /* USB VCP */

#ifdef PIOS_INCLUDE_DEBUG_CONSOLE
uint32_t pios_com_debug_id; /* DebugConsole */
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

#ifdef PIOS_INCLUDE_HOTT_BRIDGE
uint32_t pios_com_hott_id;
#endif

#ifdef PIOS_INCLUDE_USB

uint32_t pios_com_telem_usb_id; /* USB telemetry */

#ifdef PIOS_INCLUDE_USB_RCTX
# include "pios_usb_rctx_priv.h"
uint32_t pios_usb_rctx_id;
#endif

#include "pios_usb_board_data_priv.h"
#include "pios_usb_desc_hid_cdc_priv.h"
#include "pios_usb_desc_hid_only_priv.h"
#include "pios_usbhook.h"


#if defined(PIOS_INCLUDE_COM_MSG)
/* for bootloader? */
#include <pios_com_msg_priv.h>

#endif /* PIOS_INCLUDE_COM_MSG */

#if defined(PIOS_INCLUDE_USB_CDC)
static void PIOS_BOARD_IO_VCP_Init(uint32_t *com_id, uint16_t rx_buf_len, uint16_t tx_buf_len, uint32_t pios_usb_id)
{
#if defined(PIOS_INCLUDE_COM)
    uint32_t pios_usb_cdc_id = 0;
    if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }
    if (PIOS_COM_Init(com_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                      0, rx_buf_len, /* Let Init() allocate this buffer */
                      0, tx_buf_len)) { /* Let Init() allocate this buffer */
        PIOS_Assert(0);
    }
#endif /* PIOS_INCLUDE_COM */
}
#endif

static void PIOS_BOARD_IO_HID_Init(uint32_t *com_id, uint16_t rx_buf_len, uint16_t tx_buf_len, uint32_t pios_usb_id)
{
#ifdef PIOS_INCLUDE_USB_CDC
    const struct pios_usb_hid_cfg *hid_cfg = &pios_usb_hid_cfg;
#else
    const struct pios_usb_hid_cfg *hid_cfg = &pios_usb_hid_only_cfg;
#endif
    uint32_t pios_usb_hid_id;
    if (PIOS_USB_HID_Init(&pios_usb_hid_id, hid_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }

    if (PIOS_COM_Init(com_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
                      0, rx_buf_len, /* Let Init() allocate this buffer */
                      0, tx_buf_len)) { /* Let Init() allocate this buffer */
        PIOS_Assert(0);
    }
}

void PIOS_BOARD_IO_Configure_USB()
{
    uint8_t hwsettings_usb_hidport;
    uint8_t hwsettings_usb_vcpport;

    HwSettingsUSB_HIDPortGet(&hwsettings_usb_hidport);
    HwSettingsUSB_VCPPortGet(&hwsettings_usb_vcpport);

    PIOS_BOARD_IO_Configure_USB_Function((PIOS_BOARD_IO_USB_HID_Function)hwsettings_usb_hidport, (PIOS_BOARD_IO_USB_VCP_Function)hwsettings_usb_vcpport);
}

void PIOS_BOARD_IO_Configure_USB_Function(PIOS_BOARD_IO_USB_HID_Function hid_function, __attribute__((unused)) PIOS_BOARD_IO_USB_VCP_Function vcp_function)
{
    /* Initialize board specific USB data */
    PIOS_USB_BOARD_DATA_Init();

#if defined(PIOS_INCLUDE_USB_CDC)
    if (PIOS_USB_DESC_HID_CDC_Init()) {
        PIOS_Assert(0);
    }
#else
    if (PIOS_USB_DESC_HID_ONLY_Init()) {
        PIOS_Assert(0);
    }
#endif

    uint32_t pios_usb_id;
    PIOS_USB_Init(&pios_usb_id, PIOS_BOARD_HW_DEFS_GetUsbCfg(pios_board_info_blob.board_rev));

#if defined(PIOS_INCLUDE_USB_CDC)
    switch (vcp_function) {
    case PIOS_BOARD_IO_USB_VCP_NONE:
        break;
    case PIOS_BOARD_IO_USB_VCP_TELEMETRY:
        PIOS_BOARD_IO_VCP_Init(&pios_com_telem_usb_id, PIOS_COM_TELEM_USB_RX_BUF_LEN, PIOS_COM_TELEM_USB_TX_BUF_LEN, pios_usb_id);
        break;
    case PIOS_BOARD_IO_USB_VCP_COMBRIDGE:
        PIOS_BOARD_IO_VCP_Init(&pios_com_vcp_id, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, pios_usb_id);
        break;
    case PIOS_BOARD_IO_USB_VCP_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        PIOS_BOARD_IO_VCP_Init(&pios_com_debug_id, 0, PIOS_COM_BRIDGE_TX_BUF_LEN, pios_usb_id);
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
        break;
    case PIOS_BOARD_IO_USB_VCP_MAVLINK:
        PIOS_BOARD_IO_VCP_Init(&pios_com_mavlink_id, PIOS_COM_MAVLINK_RX_BUF_LEN, PIOS_COM_MAVLINK_TX_BUF_LEN, pios_usb_id);
        break;
    }
#endif /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
    /* Configure the usb HID port */

    switch (hid_function) {
    case PIOS_BOARD_IO_USB_HID_NONE:
        break;
    case PIOS_BOARD_IO_USB_HID_TELEMETRY:
        PIOS_BOARD_IO_HID_Init(&pios_com_telem_usb_id, PIOS_COM_TELEM_USB_RX_BUF_LEN, PIOS_COM_TELEM_USB_TX_BUF_LEN, pios_usb_id);
        break;
    case PIOS_BOARD_IO_USB_HID_RCTX:
#if defined(PIOS_INCLUDE_USB_RCTX)
        if (PIOS_USB_RCTX_Init(&pios_usb_rctx_id, &pios_usb_rctx_cfg, pios_usb_id)) {
            PIOS_Assert(0);
        }
#endif /* PIOS_INCLUDE_USB_RCTX */
        break;
    }

#endif /* PIOS_INCLUDE_USB_HID */

#if !defined(STM32F1) && !defined(STM32F3)
    PIOS_USBHOOK_Activate();
#endif
}

#endif /* PIOS_INCLUDE_USB */

#ifdef PIOS_INCLUDE_RCVR
# ifdef PIOS_INCLUDE_HOTT
static int32_t PIOS_HOTT_Init_SUMD(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    return PIOS_HOTT_Init(id, driver, lower_id, PIOS_HOTT_PROTO_SUMD);
}

static int32_t PIOS_HOTT_Init_SUMH(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    return PIOS_HOTT_Init(id, driver, lower_id, PIOS_HOTT_PROTO_SUMH);
}
# endif /* PIOS_INCLUDE_HOTT */
# ifdef PIOS_INCLUDE_DSM
static int32_t PIOS_DSM_Init_Helper(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    // DSM Bind stuff
    uint8_t hwsettings_DSMxBind;

    HwSettingsDSMxBindGet(&hwsettings_DSMxBind);

    return PIOS_DSM_Init(id, driver, lower_id, hwsettings_DSMxBind);
}
# endif
# ifdef PIOS_INCLUDE_SBUS
static int32_t PIOS_SBus_Init_Helper(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    HwSettingsSBusModeOptions hwsettings_SBusMode;

    HwSettingsSBusModeGet(&hwsettings_SBusMode);

    struct pios_sbus_cfg sbus_cfg = {
        .non_inverted = (hwsettings_SBusMode == HWSETTINGS_SBUSMODE_NONINVERTED),
    };

    return PIOS_SBus_Init(id, &sbus_cfg, driver, lower_id);
}
static int32_t PIOS_SBus_Normal_Init_Helper(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    struct pios_sbus_cfg sbus_cfg = {
        .non_inverted = false,
    };

    return PIOS_SBus_Init(id, &sbus_cfg, driver, lower_id);
}
static int32_t PIOS_SBus_Not_Inverted_Init_Helper(uint32_t *id, const struct pios_com_driver *driver, uint32_t lower_id)
{
    struct pios_sbus_cfg sbus_cfg = {
        .non_inverted = true,
    };

    return PIOS_SBus_Init(id, &sbus_cfg, driver, lower_id);
}
# endif /* ifdef PIOS_INCLUDE_SBUS */
#endif /* ifdef PIOS_INCLUDE_RCVR */

struct uart_function {
    uint32_t *com_id;
    uint16_t com_rx_buf_len;
    uint16_t com_tx_buf_len;
#ifdef PIOS_INCLUDE_RCVR
    int32_t  (*rcvr_init)(uint32_t *target, const struct pios_com_driver *driver, uint32_t lower_id);
    const struct pios_rcvr_driver *rcvr_driver;
    ManualControlSettingsChannelGroupsOptions rcvr_group;
#endif
};

static const struct uart_function uart_function_map[] = {
    [PIOS_BOARD_IO_UART_TELEMETRY] =         {
        .com_id         = &pios_com_telem_rf_id,
        .com_rx_buf_len = PIOS_COM_TELEM_RF_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_TELEM_RF_TX_BUF_LEN,
    },

    [PIOS_BOARD_IO_UART_MAVLINK] =           {
        .com_id         = &pios_com_mavlink_id,
        .com_rx_buf_len = PIOS_COM_MAVLINK_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_MAVLINK_TX_BUF_LEN,
    },

    [PIOS_BOARD_IO_UART_MSP] =               {
        .com_id         = &pios_com_msp_id,
        .com_rx_buf_len = PIOS_COM_MSP_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_MSP_TX_BUF_LEN,
    },
#ifdef PIOS_INCLUDE_GPS
    [PIOS_BOARD_IO_UART_GPS] =               {
        .com_id         = &pios_com_gps_id,
        .com_rx_buf_len = PIOS_COM_GPS_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_GPS_TX_BUF_LEN,
    },
#endif
    [PIOS_BOARD_IO_UART_OSDHK] =             {
        .com_id         = &pios_com_hkosd_id,
        .com_rx_buf_len = PIOS_COM_HKOSD_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_HKOSD_TX_BUF_LEN,
    },
#ifdef PIOS_INCLUDE_FRSKY_SENSORHUB
    [PIOS_BOARD_IO_UART_FRSKY_SENSORHUB] =   {
        .com_id         = &pios_com_frsky_sensorhub_id,
        .com_tx_buf_len = PIOS_COM_FRSKY_SENSORHUB_TX_BUF_LEN,
    },
#endif
#ifdef PIOS_INCLUDE_HOTT_BRIDGE
    [PIOS_BOARD_IO_UART_HOTT_BRIDGE] =       {
        .com_id         = &pios_com_hott_id,
        .com_rx_buf_len = PIOS_COM_HOTT_BRIDGE_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_HOTT_BRIDGE_TX_BUF_LEN,
    },
#endif
#ifdef PIOS_INCLUDE_DEBUG_CONSOLE
    [PIOS_BOARD_IO_UART_DEBUGCONSOLE] =      {
        .com_id         = &pios_com_debug_id,
        .com_tx_buf_len = PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN,
    },
#endif
    [PIOS_BOARD_IO_UART_COMBRIDGE] =         {
        .com_id         = &pios_com_bridge_id,
        .com_rx_buf_len = PIOS_COM_BRIDGE_RX_BUF_LEN,
        .com_tx_buf_len = PIOS_COM_BRIDGE_TX_BUF_LEN,
    },
#ifdef PIOS_INCLUDE_RCVR
# ifdef PIOS_INCLUDE_IBUS
    [PIOS_BOARD_IO_UART_IBUS] =              {
        .rcvr_init   = &PIOS_IBUS_Init,
        .rcvr_driver = &pios_ibus_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_IBUS,
    },
# endif /* PIOS_INCLUDE_IBUS */
# ifdef PIOS_INCLUDE_EXBUS
    [PIOS_BOARD_IO_UART_EXBUS] =             {
        .rcvr_init   = &PIOS_EXBUS_Init,
        .rcvr_driver = &pios_exbus_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_EXBUS,
    },
# endif /* PIOS_INCLUDE_EXBUS */
# ifdef PIOS_INCLUDE_SRXL
    [PIOS_BOARD_IO_UART_SRXL] =              {
        .rcvr_init   = &PIOS_SRXL_Init,
        .rcvr_driver = &pios_srxl_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_SRXL,
    },
# endif /* PIOS_INCLUDE_SRXL */
# ifdef PIOS_INCLUDE_HOTT
    [PIOS_BOARD_IO_UART_HOTT_SUMD] =         {
        .rcvr_init   = &PIOS_HOTT_Init_SUMD,
        .rcvr_driver = &pios_hott_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTT,
    },

    [PIOS_BOARD_IO_UART_HOTT_SUMH] =         {
        .rcvr_init   = &PIOS_HOTT_Init_SUMH,
        .rcvr_driver = &pios_hott_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTT,
    },
# endif /* PIOS_INCLUDE_HOTT */
# ifdef PIOS_INCLUDE_DSM
    [PIOS_BOARD_IO_UART_DSM_MAIN] =          {
        .rcvr_init   = &PIOS_DSM_Init_Helper,
        .rcvr_driver = &pios_dsm_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT,
    },

    [PIOS_BOARD_IO_UART_DSM_FLEXI] =         {
        .rcvr_init   = &PIOS_DSM_Init_Helper,
        .rcvr_driver = &pios_dsm_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMFLEXIPORT,
    },

    [PIOS_BOARD_IO_UART_DSM_RCVR] =          {
        .rcvr_init   = &PIOS_DSM_Init_Helper,
        .rcvr_driver = &pios_dsm_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMRCVRPORT,
    },
# endif /* PIOS_INCLUDE_DSM */
# ifdef PIOS_INCLUDE_SBUS
    [PIOS_BOARD_IO_UART_SBUS] =              {
        .rcvr_init   = &PIOS_SBus_Init_Helper,
        .rcvr_driver = &pios_sbus_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS,
    },
    [PIOS_BOARD_IO_UART_SBUS_NORMAL] =       {
        .rcvr_init   = &PIOS_SBus_Normal_Init_Helper,
        .rcvr_driver = &pios_sbus_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS,
    },
    [PIOS_BOARD_IO_UART_SBUS_NOT_INVERTED] = {
        .rcvr_init   = &PIOS_SBus_Not_Inverted_Init_Helper,
        .rcvr_driver = &pios_sbus_rcvr_driver,
        .rcvr_group  = MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS,
    },
# endif /* PIOS_INCLUDE_SBUS */
#endif /* PIOS_INCLUDE_RCVR */
};

void PIOS_BOARD_IO_Configure_UART_COM(const struct pios_usart_cfg *hw_config,
                                      uint16_t rx_buf_len,
                                      uint16_t tx_buf_len,
                                      uint32_t *com_id)
{
    uint32_t usart_id;

    if (PIOS_USART_Init(&usart_id, hw_config)) {
        PIOS_Assert(0);
    }

    if (PIOS_COM_Init(com_id, &pios_usart_com_driver, usart_id,
                      0, rx_buf_len,
                      0, tx_buf_len)) {
        PIOS_Assert(0);
    }
}

void PIOS_BOARD_IO_Configure_UART(const struct pios_usart_cfg *hw_config, PIOS_BOARD_IO_UART_Function function)
{
    if (function >= NELEMENTS(uart_function_map)) {
        return;
    }

    if (uart_function_map[function].com_id) {
        PIOS_BOARD_IO_Configure_UART_COM(hw_config,
                                         uart_function_map[function].com_rx_buf_len,
                                         uart_function_map[function].com_tx_buf_len,
                                         uart_function_map[function].com_id);
    }
#ifdef PIOS_INCLUDE_RCVR
    else if (uart_function_map[function].rcvr_init) {
        uint32_t usart_id;

        if (PIOS_USART_Init(&usart_id, hw_config)) {
            PIOS_Assert(0);
        }

        uint32_t rcvr_driver_id;

        if (uart_function_map[function].rcvr_init(&rcvr_driver_id, &pios_usart_com_driver, usart_id)) {
            PIOS_Assert(0);
        }

        uint32_t rcvr_id;
        if (PIOS_RCVR_Init(&rcvr_id, uart_function_map[function].rcvr_driver, rcvr_driver_id)) {
            PIOS_Assert(0);
        }

        /* lock uart config */
        if (pios_usart_com_driver.ioctl) {
            bool lock = true;
            pios_usart_com_driver.ioctl(usart_id, PIOS_IOCTL_USART_LOCK_CONFIG, &lock);
        }

        pios_rcvr_group_map[uart_function_map[function].rcvr_group] = rcvr_id;
    }
#endif /* PIOS_INCLUDE_RCVR */
}

#ifdef PIOS_INCLUDE_PWM
void PIOS_BOARD_IO_Configure_PWM_RCVR(const struct pios_pwm_cfg *pwm_cfg)
{
    /* Set up the receiver port.  Later this should be optional */
    uint32_t pios_pwm_id;

    PIOS_PWM_Init(&pios_pwm_id, pwm_cfg);

    uint32_t pios_pwm_rcvr_id;
    if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
}
#endif /* PIOS_INCLUDE_PWM */

#ifdef PIOS_INCLUDE_PPM
void PIOS_BOARD_IO_Configure_PPM_RCVR(const struct pios_ppm_cfg *ppm_cfg)
{
    uint32_t pios_ppm_id;

    PIOS_PPM_Init(&pios_ppm_id, ppm_cfg);

    uint32_t pios_ppm_rcvr_id;
    if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
}
#endif /* PIOS_INCLUDE_PPM */

#if defined(PIOS_INCLUDE_GCSRCVR)
void PIOS_BOARD_IO_Configure_GCS_RCVR()
{
    GCSReceiverInitialize();
    uint32_t pios_gcsrcvr_id;
    PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
    uint32_t pios_gcsrcvr_rcvr_id;
    if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
}
#endif /* PIOS_INCLUDE_GCSRCVR */

#if defined(PIOS_INCLUDE_OPLINKRCVR) && defined(PIOS_INCLUDE_RCVR)
void PIOS_BOARD_IO_Configure_OPLink_RCVR()
{
    uint32_t pios_oplinkrcvr_id;

    OPLinkReceiverInitialize();
#if defined(PIOS_INCLUDE_RFM22B)
    PIOS_OPLinkRCVR_Init(&pios_oplinkrcvr_id, pios_rfm22b_id);
#else /* PIOS_INCLUDE_RFM22B */
    PIOS_OPLinkRCVR_Init(&pios_oplinkrcvr_id);
#endif /* PIOS_INCLUDE_RFM22B */
    uint32_t pios_oplinkrcvr_rcvr_id;
    if (PIOS_RCVR_Init(&pios_oplinkrcvr_rcvr_id, &pios_oplinkrcvr_rcvr_driver, pios_oplinkrcvr_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_OPLINK] = pios_oplinkrcvr_rcvr_id;
}
#endif /* PIOS_INCLUDE_OPLINKRCVR && PIOS_INCLUDE_RCVR */

#ifdef PIOS_INCLUDE_RFM22B

void PIOS_BOARD_IO_Configure_RFM22B()
{
#if defined(PIOS_INCLUDE_RFM22B)
    OPLinkStatusInitialize();
#endif /* PIOS_INCLUDE_RFM22B */
#if defined(PIOS_INCLUDE_OPLINKRCVR) && defined(PIOS_INCLUDE_RCVR)
    OPLinkReceiverInitialize();
#endif
    /* Initialize the RFM22B radio COM device. */

    /* Fetch the OPLinkSettings object. */
    OPLinkSettingsData oplinkSettings;
    OPLinkSettingsGet(&oplinkSettings);

    // Initialize out status object.
    OPLinkStatusData oplinkStatus;
    OPLinkStatusGet(&oplinkStatus);
    oplinkStatus.BoardType     = pios_board_info_blob.board_type;
    PIOS_BL_HELPER_FLASH_Read_Description(oplinkStatus.Description, OPLINKSTATUS_DESCRIPTION_NUMELEM);
    PIOS_SYS_SerialNumberGetBinary(oplinkStatus.CPUSerial);
    oplinkStatus.BoardRevision = pios_board_info_blob.board_rev;

    /* Is the radio turned on? */
    bool is_coordinator = (oplinkSettings.Protocol == OPLINKSETTINGS_PROTOCOL_OPLINKCOORDINATOR);
    bool openlrs    = (oplinkSettings.Protocol == OPLINKSETTINGS_PROTOCOL_OPENLRS);
    bool data_mode  = ((oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATA) ||
                       (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATAANDCONTROL));
    bool ppm_mode   = ((oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_CONTROL) ||
                       (oplinkSettings.LinkType == OPLINKSETTINGS_LINKTYPE_DATAANDCONTROL));
    bool is_enabled = ((oplinkSettings.Protocol != OPLINKSETTINGS_PROTOCOL_DISABLED) &&
                       ((oplinkSettings.MaxRFPower != OPLINKSETTINGS_MAXRFPOWER_0) || openlrs));
    if (is_enabled) {
        if (openlrs) {
#if defined(PIOS_INCLUDE_OPENLRS_RCVR) && defined(PIOS_INCLUDE_RCVR)
            const struct pios_openlrs_cfg *openlrs_cfg = PIOS_BOARD_HW_DEFS_GetOpenLRSCfg(pios_board_info_blob.board_rev);

            PIOS_OpenLRS_Init(&pios_openlrs_id, PIOS_SPI_RFM22B_ADAPTER, 0, openlrs_cfg);

            uint32_t openlrsrcvr_id;
            PIOS_OpenLRS_Rcvr_Init(&openlrsrcvr_id, pios_openlrs_id);

            uint32_t openlrsrcvr_rcvr_id;
            if (PIOS_RCVR_Init(&openlrsrcvr_rcvr_id, &pios_openlrs_rcvr_driver, openlrsrcvr_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_OPENLRS] = openlrsrcvr_rcvr_id;
#endif /* PIOS_INCLUDE_OPENLRS_RCVR && PIOS_INCLUDE_RCVR */
        } else {
            /* Configure the RFM22B device. */
            const struct pios_rfm22b_cfg *rfm22b_cfg = PIOS_BOARD_HW_DEFS_GetRfm22bCfg(pios_board_info_blob.board_rev);

            if (PIOS_RFM22B_Init(&pios_rfm22b_id, PIOS_SPI_RFM22B_ADAPTER, rfm22b_cfg->slave_num, rfm22b_cfg, oplinkSettings.RFBand)) {
                PIOS_Assert(0);
            }

            /* Configure the radio com interface */
            if (PIOS_COM_Init(&pios_com_pri_radio_id, &pios_rfm22b_com_driver, pios_rfm22b_id,
                              0, PIOS_COM_PRI_RADIO_RX_BUF_LEN,
                              0, PIOS_COM_PRI_RADIO_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }

            // Initialize the aux radio com interface
            if (PIOS_COM_Init(&pios_com_aux_radio_id, &pios_rfm22b_aux_com_driver, pios_rfm22b_id,
                              0, PIOS_COM_AUX_RADIO_RX_BUF_LEN,
                              0, PIOS_COM_AUX_RADIO_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }

            oplinkStatus.LinkState = OPLINKSTATUS_LINKSTATE_ENABLED;

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

            /* Set the radio configuration parameters. */
            PIOS_RFM22B_SetDeviceID(pios_rfm22b_id, oplinkSettings.CustomDeviceID);
            PIOS_RFM22B_SetCoordinatorID(pios_rfm22b_id, oplinkSettings.CoordID);
            PIOS_RFM22B_SetXtalCap(pios_rfm22b_id, oplinkSettings.RFXtalCap);
            PIOS_RFM22B_SetChannelConfig(pios_rfm22b_id, datarate, oplinkSettings.MinChannel, oplinkSettings.MaxChannel, is_coordinator, data_mode, ppm_mode);

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

            /* Initialize the OPLink Receiver */
#ifdef PIOS_INCLUDE_OPLINKRCVR
            PIOS_BOARD_IO_Configure_OPLink_RCVR();
#endif

            /* Reinitialize the modem. */
            PIOS_RFM22B_Reinit(pios_rfm22b_id);
        }
    } else {
        oplinkStatus.LinkState = OPLINKSTATUS_LINKSTATE_DISABLED;
    }

    OPLinkStatusSet(&oplinkStatus);
}

void PIOS_BOARD_IO_Configure_RadioAuxStream(HwSettingsRadioAuxStreamOptions radioaux)
{
    switch (radioaux) {
    case HWSETTINGS_RADIOAUXSTREAM_DEBUGCONSOLE:
#ifdef PIOS_INCLUDE_DEBUG_CONSOLE
        pios_com_debug_id = pios_com_aux_radio_id;
#endif
        break;
    case HWSETTINGS_RADIOAUXSTREAM_MAVLINK:
        pios_com_mavlink_id = pios_com_aux_radio_id;
        break;
    case HWSETTINGS_RADIOAUXSTREAM_COMBRIDGE:
        pios_com_bridge_id  = pios_com_aux_radio_id;
        break;
    case HWSETTINGS_RADIOAUXSTREAM_DISABLED:
        break;
    }
}
#endif /* ifdef PIOS_INCLUDE_RFM22B */
