/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup LibrePilotSystem LibrePilot System
 * @{
 * @addtogroup LibrePilotCore LibrePilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the tinyFISH board.
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
#include <hwtinyfishsettings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>
#include <taskinfo.h>
#include <sanitycheck.h>
#include <actuatorsettings.h>
#include <auxmagsettings.h>
#include <flightbatterysettings.h>
#include <revosettings.h>
#ifdef PIOS_INCLUDE_INSTRUMENTATION
#include <pios_instrumentation.h>
#endif

#include <pios_board_io.h>
#include <pios_board_sensors.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;

#ifdef PIOS_INCLUDE_WS2811
uint32_t pios_ws2811_id;
#endif

void FlightBatterySettingsDataOverrideDefaults(FlightBatterySettingsData *data)
{
    data->SensorCalibrations.VoltageFactor = 8.8f;
    data->SensorCalibrations.CurrentFactor = 0.07f;
}

void RevoSettingsDataOverrideDefaults(RevoSettingsData *data)
{
    /* This board has no barometer, so adjust default fusion algorithm to one that does not depend on working baro */
    data->FusionAlgorithm = REVOSETTINGS_FUSIONALGORITHM_ACRONOSENSORS;
}

static HwTinyFISHSettingsData boardHwSettings;

static void hwTinyFISHSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    HwTinyFISHSettingsData currentBoardHwSettings;

    HwTinyFISHSettingsGet(&currentBoardHwSettings);

    if (memcmp(&currentBoardHwSettings, &boardHwSettings, sizeof(HwTinyFISHSettingsData)) != 0) {
        ExtendedAlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL, SYSTEMALARMS_EXTENDEDALARMSTATUS_REBOOTREQUIRED, 0);
    }
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

void PIOS_Board_Init(void)
{
#if defined(PIOS_INCLUDE_LED)
    const struct pios_gpio_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(pios_board_info_blob.board_rev);
    PIOS_DEBUG_Assert(led_cfg);
    PIOS_LED_Init(led_cfg);
#endif /* PIOS_INCLUDE_LED */

#ifdef PIOS_INCLUDE_INSTRUMENTATION
    PIOS_Instrumentation_Init(PIOS_INSTRUMENTATION_MAX_COUNTERS);
#endif

#if defined(PIOS_INCLUDE_SPI)
    /* Set up the SPI interface to the mpu6000 */

    if (PIOS_SPI_Init(&pios_spi1_id, &pios_spi1_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_SPI_Init(&pios_spi2_id, &pios_spi2_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#endif

#if defined(PIOS_INCLUDE_FLASH)
    /* Connect flash to the appropriate interface and configure it */
    uintptr_t flash_id;

    // initialize the internal settings storage flash
    if (PIOS_Flash_Internal_Init(&flash_id, &flash_internal_system_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

    /* init SPI flash here */

#endif /* if defined(PIOS_INCLUDE_FLASH) */

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

#if defined(PIOS_INCLUDE_RTC)
    /* Initialize the real-time clock and its associated tick */
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif
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

#ifndef ERASE_FLASH
#ifdef PIOS_INCLUDE_WDG
    /* Initialize watchdog as early as possible to catch faults during init */
    PIOS_WDG_Init();
#endif
#endif

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* Check for repeated boot failures */
    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        HwTinyFISHSettingsSetDefaults(HwTinyFISHSettingsHandle(), 0);

        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }


    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
// PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
    PIOS_TIM_InitClock(&tim_8_cfg);
    PIOS_TIM_InitClock(&tim_15_cfg);
// PIOS_TIM_InitClock(&tim_16_cfg);
// PIOS_TIM_InitClock(&tim_17_cfg);

#if defined(PIOS_INCLUDE_USB)
    PIOS_BOARD_IO_Configure_USB();
#endif

    HwTinyFISHSettingsConnectCallback(hwTinyFISHSettingsUpdatedCb);

    HwTinyFISHSettingsGet(&boardHwSettings);

    static const PIOS_BOARD_IO_UART_Function uart_function_map[] = {
        [HWTINYFISHSETTINGS_UART3PORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWTINYFISHSETTINGS_UART3PORT_GPS]            = PIOS_BOARD_IO_UART_GPS,
        [HWTINYFISHSETTINGS_UART3PORT_SBUS]           = PIOS_BOARD_IO_UART_SBUS,
        [HWTINYFISHSETTINGS_UART3PORT_DSM]            = PIOS_BOARD_IO_UART_DSM_MAIN, // single DSM instance? ok.
        [HWTINYFISHSETTINGS_UART3PORT_EXBUS]          = PIOS_BOARD_IO_UART_EXBUS,
        [HWTINYFISHSETTINGS_UART3PORT_HOTTSUMD]       = PIOS_BOARD_IO_UART_HOTT_SUMD,
        [HWTINYFISHSETTINGS_UART3PORT_HOTTSUMH]       = PIOS_BOARD_IO_UART_HOTT_SUMH,
        [HWTINYFISHSETTINGS_UART3PORT_SRXL]           = PIOS_BOARD_IO_UART_SRXL,
        [HWTINYFISHSETTINGS_UART3PORT_IBUS]           = PIOS_BOARD_IO_UART_IBUS,
        [HWTINYFISHSETTINGS_UART3PORT_DEBUGCONSOLE]   = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
        [HWTINYFISHSETTINGS_UART3PORT_COMBRIDGE]      = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWTINYFISHSETTINGS_UART3PORT_MSP]            = PIOS_BOARD_IO_UART_MSP,
        [HWTINYFISHSETTINGS_UART3PORT_MAVLINK]        = PIOS_BOARD_IO_UART_MAVLINK,
        [HWTINYFISHSETTINGS_UART3PORT_HOTTTELEMETRY]  = PIOS_BOARD_IO_UART_HOTT_BRIDGE,
        [HWTINYFISHSETTINGS_UART3PORT_FRSKYSENSORHUB] = PIOS_BOARD_IO_UART_FRSKY_SENSORHUB,
    };

    if (boardHwSettings.UART3Port < NELEMENTS(uart_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_cfg[2], uart_function_map[boardHwSettings.UART3Port]);
    }

#ifdef PIOS_INCLUDE_PPM
    if (boardHwSettings.UART3Port == HWTINYFISHSETTINGS_UART3PORT_PPM) {
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
    }
#endif
    if (PIOS_COM_DEBUG) {
        PIOS_COM_ChangeBaud(PIOS_COM_DEBUG, 57600);
        DEBUG_PRINTF(0, "\r\n\r\ntinyFISH FC booting\r\n");
    }

    /* Configure SBus as primary usart client.
     * PIOS_BOARD_IO_Configure_UART() will also lock COM port config so that secondary client cannot mess with settings */
    if (boardHwSettings.UART3Port != HWTINYFISHSETTINGS_UART3PORT_SBUS) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_cfg[1], PIOS_BOARD_IO_UART_SBUS_NOT_INVERTED);
    }

    /* Configure Frsky SensorHub as secondary client. */
    if (boardHwSettings.UART3Port != HWTINYFISHSETTINGS_UART3PORT_FRSKYSENSORHUB) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_cfg[1], PIOS_BOARD_IO_UART_FRSKY_SENSORHUB);
    }

    /* and what to do with UART1? There is RX_DEBUG only connected. Maybe make it into configurable ComBridge? */
    if (boardHwSettings.UART1Port == HWTINYFISHSETTINGS_UART1PORT_COMBRIDGE) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_cfg[0], PIOS_BOARD_IO_UART_COMBRIDGE);
    }

#ifdef PIOS_INCLUDE_GCSRCVR
    PIOS_BOARD_IO_Configure_GCS_RCVR();
#endif

#ifdef PIOS_INCLUDE_OPLINKRCVR
    PIOS_BOARD_IO_Configure_OPLink_RCVR();
#endif

#ifdef PIOS_ENABLE_DEBUG_PINS
    PIOS_DEBUG_Init(&pios_servo_cfg.channels, pios_servo_cfg.num_channels);
#else
    if (boardHwSettings.UART3Port == HWTINYFISHSETTINGS_UART3PORT_OUTPUTS) {
        PIOS_Servo_Init(&pios_servo_uart3_cfg);
    } else {
        PIOS_Servo_Init(&pios_servo_cfg);
    }
#endif /* PIOS_ENABLE_DEBUG_PINS */

    switch (boardHwSettings.LEDPort) {
    case HWTINYFISHSETTINGS_LEDPORT_WS281X:
#if defined(PIOS_INCLUDE_WS2811)
        PIOS_WS2811_Init(&pios_ws2811_id, &pios_ws2811_cfg);
#endif
        break;
    default:
        break;
    }

    PIOS_BOARD_Sensors_Configure();

    PIOS_LED_On(PIOS_LED_HEARTBEAT);
}


/**
 * @}
 */
