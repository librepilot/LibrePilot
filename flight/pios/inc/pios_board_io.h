/**
 ******************************************************************************
 *
 * @file       pios_board_io.h
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
#ifndef PIOS_BOARD_IO_H
#define PIOS_BOARD_IO_H

#include "pios.h"


#ifdef PIOS_INCLUDE_USB
#include <pios_usb_priv.h>
#endif

#include <pios_usart_priv.h>

#ifdef PIOS_INCLUDE_PWM
#include <pios_pwm_priv.h>
#endif

#ifdef PIOS_INCLUDE_PPM
#include <pios_ppm_priv.h>
#endif

#ifdef PIOS_INCLUDE_OPENLRS
#include <pios_openlrs_priv.h>
#endif

#ifdef PIOS_INCLUDE_RCVR
extern uint32_t pios_rcvr_group_map[]; /* Receivers */
#endif

/* GPS */
#ifdef PIOS_INCLUDE_GPS
extern uint32_t pios_com_gps_id;
# define PIOS_COM_GPS             (pios_com_gps_id)
# ifndef PIOS_COM_GPS_RX_BUF_LEN
#  define PIOS_COM_GPS_RX_BUF_LEN 128
# endif
# ifndef PIOS_COM_GPS_TX_BUF_LEN
#  define PIOS_COM_GPS_TX_BUF_LEN 32
# endif
#endif /* PIOS_INCLUDE_GPS */


/* ComUsbBridge */
extern uint32_t pios_com_bridge_id;
#define PIOS_COM_BRIDGE             (pios_com_bridge_id)
#ifndef PIOS_COM_BRIDGE_RX_BUF_LEN
# define PIOS_COM_BRIDGE_RX_BUF_LEN 65
#endif
#ifndef PIOS_COM_BRIDGE_TX_BUF_LEN
# define PIOS_COM_BRIDGE_TX_BUF_LEN 12
#endif


/* USB telemetry */
extern uint32_t pios_com_telem_usb_id;
#define PIOS_COM_TELEM_USB             (pios_com_telem_usb_id)
#ifndef PIOS_COM_TELEM_USB_RX_BUF_LEN
# define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
#endif
#ifndef PIOS_COM_TELEM_USB_TX_BUF_LEN
#define PIOS_COM_TELEM_USB_TX_BUF_LEN  65
#endif

/* Serial port telemetry */
extern uint32_t pios_com_telem_rf_id;
#define PIOS_COM_TELEM_RF              (pios_com_telem_rf_id)
#ifndef PIOS_COM_TELEM_RF_RX_BUF_LEN
# define PIOS_COM_TELEM_RF_RX_BUF_LEN  512
#endif
#ifndef PIOS_COM_TELEM_RF_TX_BUF_LEN
# define PIOS_COM_TELEM_RF_TX_BUF_LEN  512
#endif


/* RFM22B telemetry */
#ifdef PIOS_INCLUDE_RFM22B
extern uint32_t pios_rfm22b_id; /* RFM22B handle */
extern uint32_t pios_com_rf_id; /* oplink telemetry */
# define PIOS_COM_RF                    (pios_com_rf_id)
# ifndef PIOS_COM_RFM22B_RF_RX_BUF_LEN
#  define PIOS_COM_RFM22B_RF_RX_BUF_LEN 512
# endif
# ifndef PIOS_COM_RFM22B_RF_TX_BUF_LEN
# define PIOS_COM_RFM22B_RF_TX_BUF_LEN  512
# endif
#endif


/* HK OSD ?? */
extern uint32_t pios_com_hkosd_id;
#define PIOS_COM_OSDHK                    (pios_com_hkosd_id)
#ifndef PIOS_COM_HKOSD_RX_BUF_LEN
# define PIOS_COM_HKOSD_RX_BUF_LEN        22
#endif
#ifndef PIOS_COM_HKOSD_TX_BUF_LEN
# define PIOS_COM_HKOSD_TX_BUF_LEN        22
#endif

/* MSP */
extern uint32_t pios_com_msp_id;
#define PIOS_COM_MSP                      (pios_com_msp_id)
#ifndef PIOS_COM_MSP_TX_BUF_LEN
# define PIOS_COM_MSP_TX_BUF_LEN          128
#endif
#ifndef PIOS_COM_MSP_RX_BUF_LEN
# define PIOS_COM_MSP_RX_BUF_LEN          64
#endif

/* MAVLink */
extern uint32_t pios_com_mavlink_id;
#define PIOS_COM_MAVLINK                  (pios_com_mavlink_id)
#ifndef PIOS_COM_MAVLINK_TX_BUF_LEN
# define PIOS_COM_MAVLINK_TX_BUF_LEN      128
#endif
#ifndef PIOS_COM_MAVLINK_RX_BUF_LEN
# define PIOS_COM_MAVLINK_RX_BUF_LEN      128
#endif

/* HoTT Telemetry */
#ifdef PIOS_INCLUDE_HOTT_BRIDGE
# ifndef PIOS_COM_HOTT_BRIDGE_RX_BUF_LEN
#  define PIOS_COM_HOTT_BRIDGE_RX_BUF_LEN 512
# endif
# ifndef PIOS_COM_HOTT_BRIDGE_TX_BUF_LEN
#  define PIOS_COM_HOTT_BRIDGE_TX_BUF_LEN 512
# endif
extern uint32_t pios_com_hott_id;
# define PIOS_COM_HOTT (pios_com_hott_id)
#endif

/* USB VCP */
extern uint32_t pios_com_vcp_id;
#define PIOS_COM_VCP                      (pios_com_vcp_id)


#ifdef PIOS_INCLUDE_DEBUG_CONSOLE
extern uint32_t pios_com_debug_id;
#define PIOS_COM_DEBUG                    (pios_com_debug_id)
#ifndef PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN
# define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
#endif
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

#ifdef PIOS_INCLUDE_USB_RCTX
extern uint32_t pios_usb_rctx_id;
#endif /* PIOS_INCLUDE_USB_RCTX */

#if defined(PIOS_INCLUDE_HMC5X83) && defined(PIOS_HMC5X83_HAS_GPIOS)
#include <pios_hmc5x83.h>
extern pios_hmc5x83_dev_t pios_hmc5x83_internal_id;
#endif

typedef enum {
    PIOS_BOARD_IO_UART_NONE,
    PIOS_BOARD_IO_UART_TELEMETRY, /* com */
    PIOS_BOARD_IO_UART_MAVLINK, /* com */
    PIOS_BOARD_IO_UART_MSP, /* com */
    PIOS_BOARD_IO_UART_GPS, /* com */
    PIOS_BOARD_IO_UART_COMBRIDGE, /* com */
    PIOS_BOARD_IO_UART_DEBUGCONSOLE, /* com */
    PIOS_BOARD_IO_UART_OSDHK, /* com */
    PIOS_BOARD_IO_UART_SBUS, /* rcvr */
    PIOS_BOARD_IO_UART_DSM_MAIN, /* rcvr  */
    PIOS_BOARD_IO_UART_DSM_FLEXI, /* rcvr  */
    PIOS_BOARD_IO_UART_DSM_RCVR, /* rcvr  */
    PIOS_BOARD_IO_UART_HOTT_SUMD, /* rcvr  */
    PIOS_BOARD_IO_UART_HOTT_SUMH, /* rcvr  */
    PIOS_BOARD_IO_UART_SRXL, /* rcvr */
    PIOS_BOARD_IO_UART_IBUS, /* rcvr */
    PIOS_BOARD_IO_UART_EXBUS, /* rcvr */
//    PIOS_BOARD_IO_UART_FRSKY_SPORT_TELEMETRY, /* com */
    PIOS_BOARD_IO_UART_HOTT_BRIDGE, /* com */
} PIOS_BOARD_IO_UART_Function;

typedef enum {
    PIOS_BOARD_IO_RADIOAUX_NONE,
    PIOS_BOARD_IO_RADIOAUX_MAVLINK,
    PIOS_BOARD_IO_RADIOAUX_COMBRIDGE,
    PIOS_BOARD_IO_RADIOAUX_DEBUGCONSOLE,
//    PIOS_BOARD_IO_RADIOAUX_FRSKY_SPORT_TELEMETRY,
} PIOS_BOARD_IO_RADIOAUX_Function;

#ifdef PIOS_INCLUDE_USB
void PIOS_BOARD_IO_Configure_USB();
//# if defined(PIOS_INCLUDE_USB_HID)
//#  include <pios_usb_hid_priv.h>
//extern const struct pios_usb_hid_cfg pios_usb_hid_cfg;
//# endif /* PIOS_INCLUDE_USB_HID */
#endif /* PIOS_INCLUDE_USB */
#ifdef PIOS_INCLUDE_PWM
void PIOS_BOARD_IO_Configure_PWM(const struct pios_pwm_cfg *pwm_cfg);
#endif
#ifdef PIOS_INCLUDE_PPM
void PIOS_BOARD_IO_Configure_PPM(const struct pios_ppm_cfg *ppm_cfg);
#endif

void PIOS_BOARD_IO_Configure_UART(const struct pios_usart_cfg *usart_cfg, PIOS_BOARD_IO_UART_Function function);

#ifdef PIOS_INCLUDE_RFM22B
void PIOS_BOARD_IO_Configure_RFM22B(uint32_t spi_id, PIOS_BOARD_IO_RADIOAUX_Function function);
#endif

#ifdef PIOS_INCLUDE_I2C
void PIOS_BOARD_IO_Configure_I2C(uint32_t i2c_internal_id, uint32_t i2c_external_id);
#endif

#ifdef PIOS_INCLUDE_GCSRCVR
void PIOS_BOARD_IO_Configure_GCSRCVR();
#endif

#ifdef PIOS_INCLUDE_WS2811
void PIOS_BOARD_IO_Configure_WS2811();
#endif

#ifdef PIOS_INCLUDE_ADC
void PIOS_BOARD_IO_Configure_ADC();
#endif


#endif /* PIOS_BOARD_IO_H */
