/**
 ******************************************************************************
 * @addtogroup Revolution Revolution configuration files
 * @{
 * @brief Configures the revolution board
 * @{
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      Defines board specific static initializers for hardware for the Revolution board.
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
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <taskinfo.h>


/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

/**
 * Sensor configurations
 */

/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, SPEKTRUM1, SPEKTRUM2, SBUS
 * NOTE: No slot in this map for NONE.
 */
uint32_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

#define PIOS_COM_TELEM_RF_RX_BUF_LEN  512
#define PIOS_COM_TELEM_RF_TX_BUF_LEN  512

#define PIOS_COM_GPS_RX_BUF_LEN       32

#define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 65

#define PIOS_COM_BRIDGE_RX_BUF_LEN    65
#define PIOS_COM_BRIDGE_TX_BUF_LEN    12

#define PIOS_COM_AUX_RX_BUF_LEN       512
#define PIOS_COM_AUX_TX_BUF_LEN       512

uint32_t pios_com_aux_id       = 0;
uint32_t pios_com_gps_id       = 0;
uint32_t pios_com_telem_usb_id = 0;
uint32_t pios_com_telem_rf_id  = 0;
uint32_t pios_com_bridge_id    = 0;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id;

/*
 * Setup a com port based on the passed cfg, driver and buffer sizes. tx size = 0 make the port rx only
 */
static void PIOS_Board_configure_com(const struct pios_udp_cfg *usart_port_cfg, uint16_t rx_buf_len, uint16_t tx_buf_len,
                                     const struct pios_com_driver *com_driver, uint32_t *pios_com_id)
{
    uint32_t pios_usart_id;

    if (PIOS_UDP_Init(&pios_usart_id, usart_port_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *rx_buffer = (uint8_t *)pvPortMalloc(rx_buf_len);
    PIOS_Assert(rx_buffer);
    if (tx_buf_len > 0) { // this is the case for rx/tx ports
        uint8_t *tx_buffer = (uint8_t *)pvPortMalloc(tx_buf_len);
        PIOS_Assert(tx_buffer);

        if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                          rx_buffer, rx_buf_len,
                          tx_buffer, tx_buf_len)) {
            PIOS_Assert(0);
        }
    } else { // rx only port
        if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                          rx_buffer, rx_buf_len,
                          NULL, 0)) {
            PIOS_Assert(0);
        }
    }
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
void PIOS_Board_Init(void)
{
    /* Delay system */
    PIOS_DELAY_Init();

    // Initialize logfs for settings.
    // If linking in yaffs for testing, this will be /dev0 with settings stored
    // via the logfs object api in /dev0/logfs/
    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, NULL, NULL, 0)) {
        PIOS_DEBUG_Assert(0);
    }
    // If linking in yaffs for testing, this will re-use the simposix yaffs /dev0 nor
    // simulation, which does not support being instanced twice.
    pios_user_fs_id = pios_uavo_settings_fs_id;

    /* Initialize the task monitor */
    if (PIOS_TASK_MONITOR_Initialize(TASKINFO_RUNNING_NUMELEM)) {
        PIOS_Assert(0);
    }

    /* Initialize the delayed callback library */
    PIOS_CALLBACKSCHEDULER_Initialize();

    /* Initialize UAVObject libraries */
    EventDispatcherInitialize();
    UAVObjInitialize();

    UAVObjectsInitializeAll();

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* Configure IO ports */

    /* Configure Telemetry port */
    HwSettingsRV_TelemetryPortOptions hwsettings_rv_telemetryport;
    HwSettingsRV_TelemetryPortGet(&hwsettings_rv_telemetryport);

    switch (hwsettings_rv_telemetryport) {
    case HWSETTINGS_RV_TELEMETRYPORT_DISABLED:
        break;
    case HWSETTINGS_RV_TELEMETRYPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_udp_telem_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_telem_rf_id);
        break;
    case HWSETTINGS_RV_TELEMETRYPORT_COMAUX:
        PIOS_Board_configure_com(&pios_udp_telem_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_aux_id);
        break;
    default:
        break;
    } /*        hwsettings_rv_telemetryport */

    /* Configure GPS port */
    HwSettingsRV_GPSPortOptions hwsettings_rv_gpsport;
    HwSettingsRV_GPSPortGet(&hwsettings_rv_gpsport);
    switch (hwsettings_rv_gpsport) {
    case HWSETTINGS_RV_GPSPORT_DISABLED:
        break;

    case HWSETTINGS_RV_GPSPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_udp_gps_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_telem_rf_id);
        break;

    case HWSETTINGS_RV_GPSPORT_GPS:
        PIOS_Board_configure_com(&pios_udp_gps_cfg, PIOS_COM_GPS_RX_BUF_LEN, 0, &pios_udp_com_driver, &pios_com_gps_id);
        break;

    case HWSETTINGS_RV_GPSPORT_COMAUX:
        PIOS_Board_configure_com(&pios_udp_gps_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_aux_id);
        break;
    default:
        break;
    } /* hwsettings_rv_gpsport */

    /* Configure AUXPort */
    HwSettingsRV_AuxPortOptions hwsettings_rv_auxport;
    HwSettingsRV_AuxPortGet(&hwsettings_rv_auxport);

    switch (hwsettings_rv_auxport) {
    case HWSETTINGS_RV_AUXPORT_DISABLED:
        break;

    case HWSETTINGS_RV_AUXPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_udp_aux_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_telem_rf_id);
        break;

    case HWSETTINGS_RV_AUXPORT_COMAUX:
        PIOS_Board_configure_com(&pios_udp_aux_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_udp_com_driver, &pios_com_aux_id);
        break;
    default:
        break;
    } /* hwsettings_rv_auxport */
}

/**
 * @}
 * @}
 */
