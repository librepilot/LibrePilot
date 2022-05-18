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
 * @brief Defines board specific static initializers for hardware for the SPRacing F3 board.
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
#include <hwspracingf3settings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>
#include <taskinfo.h>
#include <sanitycheck.h>
#include <actuatorsettings.h>

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

static HwSPRacingF3SettingsData boardHwSettings;

static void hwSPRacingF3SettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    HwSPRacingF3SettingsData currentBoardHwSettings;

    HwSPRacingF3SettingsGet(&currentBoardHwSettings);

    if (memcmp(&currentBoardHwSettings, &boardHwSettings, sizeof(HwSPRacingF3SettingsData)) != 0) {
        ExtendedAlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL, SYSTEMALARMS_EXTENDEDALARMSTATUS_REBOOTREQUIRED, 0);
    }
}

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;

#ifdef PIOS_INCLUDE_WS2811
uint32_t pios_ws2811_id;
#endif

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

void PIOS_Board_Init(void)
{
    const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_LED)
    const struct pios_gpio_cfg *led_cfg  = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
    PIOS_Assert(led_cfg);
    PIOS_LED_Init(led_cfg);
#endif /* PIOS_INCLUDE_LED */

#ifdef PIOS_INCLUDE_INSTRUMENTATION
    PIOS_Instrumentation_Init(PIOS_INSTRUMENTATION_MAX_COUNTERS);
#endif

#if defined(PIOS_INCLUDE_SPI)
    /* Set up the SPI interface to the serial flash */


    if (PIOS_SPI_Init(&pios_spi_flash_accel_id, &pios_spi_flash_accel_cfg)) {
        PIOS_Assert(0);
    }

#endif

    uintptr_t flash_id;

    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_flash_accel_id, 0)) {
        PIOS_DEBUG_Assert(0);
    }
    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_m25p_cfg, &pios_jedec_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

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
        HwSPRacingF3SettingsSetDefaults(HwSPRacingF3SettingsHandle(), 0);

        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }

    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
    PIOS_TIM_InitClock(&tim_15_cfg);
    PIOS_TIM_InitClock(&tim_16_cfg);
    PIOS_TIM_InitClock(&tim_17_cfg);

    HwSPRacingF3SettingsConnectCallback(hwSPRacingF3SettingsUpdatedCb);

    HwSPRacingF3SettingsGet(&boardHwSettings);

    boardHwSettings.UARTPort[0] = HWSPRACINGF3SETTINGS_UARTPORT_TELEMETRY; /* UART1 should always be telemetry */
    boardHwSettings.UARTPort[2] = HWSPRACINGF3SETTINGS_UARTPORT_DISABLED; /* UART3 has conflict with SWD, so we will let it be disabled for a moment */

    static const PIOS_BOARD_IO_UART_Function uart_function_map[] = {
        [HWSPRACINGF3SETTINGS_UARTPORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWSPRACINGF3SETTINGS_UARTPORT_GPS]            = PIOS_BOARD_IO_UART_GPS,
        [HWSPRACINGF3SETTINGS_UARTPORT_SBUS]           = PIOS_BOARD_IO_UART_SBUS,
        [HWSPRACINGF3SETTINGS_UARTPORT_DSM]            = PIOS_BOARD_IO_UART_DSM_MAIN, // single DSM instance? ok.
        [HWSPRACINGF3SETTINGS_UARTPORT_EXBUS]          = PIOS_BOARD_IO_UART_EXBUS,
        [HWSPRACINGF3SETTINGS_UARTPORT_HOTTSUMD]       = PIOS_BOARD_IO_UART_HOTT_SUMD,
        [HWSPRACINGF3SETTINGS_UARTPORT_HOTTSUMH]       = PIOS_BOARD_IO_UART_HOTT_SUMH,
        [HWSPRACINGF3SETTINGS_UARTPORT_SRXL]           = PIOS_BOARD_IO_UART_SRXL,
        [HWSPRACINGF3SETTINGS_UARTPORT_IBUS]           = PIOS_BOARD_IO_UART_IBUS,
        [HWSPRACINGF3SETTINGS_UARTPORT_DEBUGCONSOLE]   = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
        [HWSPRACINGF3SETTINGS_UARTPORT_COMBRIDGE]      = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSPRACINGF3SETTINGS_UARTPORT_MSP]            = PIOS_BOARD_IO_UART_MSP,
        [HWSPRACINGF3SETTINGS_UARTPORT_MAVLINK]        = PIOS_BOARD_IO_UART_MAVLINK,
        [HWSPRACINGF3SETTINGS_UARTPORT_HOTTTELEMETRY]  = PIOS_BOARD_IO_UART_HOTT_BRIDGE,
        [HWSPRACINGF3SETTINGS_UARTPORT_FRSKYSENSORHUB] = PIOS_BOARD_IO_UART_FRSKY_SENSORHUB,
    };

    for (unsigned int i = 0; i < HWSPRACINGF3SETTINGS_UARTPORT_NUMELEM; ++i) {
        if (boardHwSettings.UARTPort[i] < NELEMENTS(uart_function_map)) {
            PIOS_BOARD_IO_Configure_UART(&pios_usart_cfg[i], uart_function_map[boardHwSettings.UARTPort[i]]);
        }
    }

    /* Configure IO ports (IO-1 & IO-2) */

    bool use_ppm = false;

    /* then do PPM */
    if ((boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PPMPWM)
        || (boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PPM)
        || (boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PPMOUTPUTS)) {
#if defined(PIOS_INCLUDE_PPM)
        use_ppm = true;
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
#endif /* PIOS_INCLUDE_PPM */
    }

    if ((boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_OUTPUTS)
        || (boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PPMOUTPUTS)) {
        /* configure remaining inputs as additional outputs */
    } else if ((boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PPMPWM)
               || (boardHwSettings.IOPorts == HWSPRACINGF3SETTINGS_IOPORTS_PWM)) {
#if defined(PIOS_INCLUDE_PWM)
        /* configure remaining inputs as pwm input */

        struct pios_pwm_cfg pwm_cfg = pios_pwm_cfg;

        if (use_ppm) {
            ++(pwm_cfg.channels);
            --(pwm_cfg.num_channels);
        }

        PIOS_BOARD_IO_Configure_PWM_RCVR(&pwm_cfg); // is this ok? pwm_cfg is on stack
#endif /* if defined(PIOS_INCLUDE_PWM) */
    }

#ifdef PIOS_INCLUDE_GCSRCVR
    PIOS_BOARD_IO_Configure_GCS_RCVR();
#endif

#ifdef PIOS_INCLUDE_OPLINKRCVR
    PIOS_BOARD_IO_Configure_OPLink_RCVR();
#endif

#ifndef PIOS_ENABLE_DEBUG_PINS
    // TODO: make use of HWSPRACINGF3SETTINGS_IOPORTS_[PPM]OUTPUTS
    PIOS_Servo_Init(&pios_servo_cfg);
#else
    PIOS_DEBUG_Init(&pios_servo_cfg.channels, pios_servo_cfg.num_channels);
#endif

    switch (boardHwSettings.LEDPort) {
    case HWSPRACINGF3SETTINGS_LEDPORT_WS281X:
#if defined(PIOS_INCLUDE_WS2811)
        PIOS_WS2811_Init(&pios_ws2811_id, &pios_ws2811_cfg);
#endif
        break;
    default:
        break;
    }

#if defined(PIOS_INCLUDE_I2C)
    // init I2C1 for use with the internal mag and baro
    if (PIOS_I2C_Init(&pios_i2c_id, &pios_i2c_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#endif

    PIOS_DELAY_WaitmS(50);

    PIOS_BOARD_Sensors_Configure();

    /* Make sure we have at least one telemetry link configured or else fail initialization */
    PIOS_Assert(pios_com_telem_rf_id);
}


/**
 * @}
 */
