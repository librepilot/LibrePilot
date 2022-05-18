/**
 ****************************************************************************************
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the Sparky2 board.
 ***************************************************************************************/
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
#include <taskinfo.h>
#include <pios_ws2811.h>

#ifdef PIOS_INCLUDE_INSTRUMENTATION
#include <pios_instrumentation.h>
#endif
#ifdef PIOS_INCLUDE_WS2811
#include <pios_ws2811.h>
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
uintptr_t pios_user_fs_id;

#ifdef PIOS_INCLUDE_WS2811
uint32_t pios_ws2811_id;
#endif

static const PIOS_BOARD_IO_UART_Function rcvr_function_map[] = {
    [HWSETTINGS_SPK2_RCVRPORT_SBUS]     = PIOS_BOARD_IO_UART_SBUS,
    [HWSETTINGS_SPK2_RCVRPORT_DSM]      = PIOS_BOARD_IO_UART_DSM_RCVR,
    [HWSETTINGS_SPK2_RCVRPORT_SRXL]     = PIOS_BOARD_IO_UART_SRXL,
    [HWSETTINGS_SPK2_RCVRPORT_IBUS]     = PIOS_BOARD_IO_UART_IBUS,
    [HWSETTINGS_SPK2_RCVRPORT_EXBUS]    = PIOS_BOARD_IO_UART_EXBUS,
    [HWSETTINGS_SPK2_RCVRPORT_HOTTSUMD] = PIOS_BOARD_IO_UART_HOTT_SUMD,
    [HWSETTINGS_SPK2_RCVRPORT_HOTTSUMH] = PIOS_BOARD_IO_UART_HOTT_SUMH,
};

static const PIOS_BOARD_IO_UART_Function main_function_map[] = {
    [HWSETTINGS_SPK2_MAINPORT_TELEMETRY]    = PIOS_BOARD_IO_UART_TELEMETRY,
    [HWSETTINGS_SPK2_MAINPORT_GPS] = PIOS_BOARD_IO_UART_GPS,
    [HWSETTINGS_SPK2_MAINPORT_DSM] = PIOS_BOARD_IO_UART_DSM_MAIN,
    [HWSETTINGS_SPK2_MAINPORT_DEBUGCONSOLE] = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
    [HWSETTINGS_SPK2_MAINPORT_COMBRIDGE]    = PIOS_BOARD_IO_UART_COMBRIDGE,
    [HWSETTINGS_SPK2_MAINPORT_OSDHK]   = PIOS_BOARD_IO_UART_OSDHK,
    [HWSETTINGS_SPK2_MAINPORT_MSP]     = PIOS_BOARD_IO_UART_MSP,
    [HWSETTINGS_SPK2_MAINPORT_MAVLINK] = PIOS_BOARD_IO_UART_MAVLINK,
    [HWSETTINGS_SPK2_MAINPORT_FRSKYSENSORHUB] = PIOS_BOARD_IO_UART_FRSKY_SENSORHUB,
};

static const PIOS_BOARD_IO_UART_Function flexi_function_map[] = {
    [HWSETTINGS_SPK2_FLEXIPORT_TELEMETRY]      = PIOS_BOARD_IO_UART_TELEMETRY,
    [HWSETTINGS_SPK2_FLEXIPORT_GPS] = PIOS_BOARD_IO_UART_GPS,
    [HWSETTINGS_SPK2_FLEXIPORT_DSM] = PIOS_BOARD_IO_UART_DSM_FLEXI,
    [HWSETTINGS_SPK2_FLEXIPORT_EXBUS]          = PIOS_BOARD_IO_UART_EXBUS,
    [HWSETTINGS_SPK2_FLEXIPORT_HOTTSUMD]       = PIOS_BOARD_IO_UART_HOTT_SUMD,
    [HWSETTINGS_SPK2_FLEXIPORT_HOTTSUMH]       = PIOS_BOARD_IO_UART_HOTT_SUMH,
    [HWSETTINGS_SPK2_FLEXIPORT_SRXL] = PIOS_BOARD_IO_UART_SRXL,
    [HWSETTINGS_SPK2_FLEXIPORT_IBUS] = PIOS_BOARD_IO_UART_IBUS,
    [HWSETTINGS_SPK2_FLEXIPORT_DEBUGCONSOLE]   = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
    [HWSETTINGS_SPK2_FLEXIPORT_COMBRIDGE]      = PIOS_BOARD_IO_UART_COMBRIDGE,
    [HWSETTINGS_SPK2_FLEXIPORT_OSDHK]          = PIOS_BOARD_IO_UART_OSDHK,
    [HWSETTINGS_SPK2_FLEXIPORT_MSP] = PIOS_BOARD_IO_UART_MSP,
    [HWSETTINGS_SPK2_FLEXIPORT_MAVLINK]        = PIOS_BOARD_IO_UART_MAVLINK,
    [HWSETTINGS_SPK2_FLEXIPORT_FRSKYSENSORHUB] = PIOS_BOARD_IO_UART_FRSKY_SENSORHUB,
};

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

    /* Set up the SPI interface to the gyro/acelerometer */
    if (PIOS_SPI_Init(&pios_spi_gyro_adapter_id, &pios_spi_gyro_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Set up the SPI interface to the flash and rfm22b */
    if (PIOS_SPI_Init(&pios_spi_telem_flash_adapter_id, &pios_spi_telem_flash_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

#if defined(PIOS_INCLUDE_FLASH)
    /* Connect flash to the appropriate interface and configure it */
    uintptr_t flash_id = 0;

    // Initialize the external USER flash
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_telem_flash_adapter_id, 1)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_external_system_cfg, &pios_jedec_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }
#endif /* if defined(PIOS_INCLUDE_FLASH) */

#if defined(PIOS_INCLUDE_RTC)
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif
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
#ifdef PIOS_INCLUDE_WDG
    PIOS_WDG_Init();
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

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
    PIOS_TIM_InitClock(&tim_5_cfg);
    PIOS_TIM_InitClock(&tim_8_cfg);
    PIOS_TIM_InitClock(&tim_9_cfg);
    PIOS_TIM_InitClock(&tim_10_cfg);
    PIOS_TIM_InitClock(&tim_11_cfg);
    PIOS_TIM_InitClock(&tim_12_cfg);

    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }

    /* Configure IO ports */

    /* Configure FlexiPort */
    uint8_t hwsettings_flexiport;
    HwSettingsSPK2_FlexiPortGet(&hwsettings_flexiport);

    if (hwsettings_flexiport < NELEMENTS(flexi_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_flexi_cfg, flexi_function_map[hwsettings_flexiport]);
    }

#if defined(PIOS_INCLUDE_I2C)
    /* Set up internal I2C bus */
    if (PIOS_I2C_Init(&pios_i2c_pressure_adapter_id, &pios_i2c_pressure_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
    PIOS_DELAY_WaitmS(50);

    if (hwsettings_flexiport == HWSETTINGS_RM_FLEXIPORT_I2C) {
        if (PIOS_I2C_Init(&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg)) {
            PIOS_Assert(0);
        }
        PIOS_DELAY_WaitmS(50);
    }
#endif

    /* Moved this here to allow binding on flexiport */
#if defined(PIOS_INCLUDE_FLASH)
    if (PIOS_FLASHFS_Logfs_Init(&pios_user_fs_id, &flashfs_external_user_cfg, &pios_jedec_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }
#endif /* if defined(PIOS_INCLUDE_FLASH) */

#if defined(PIOS_INCLUDE_USB)
    PIOS_BOARD_IO_Configure_USB();
#endif

    uint8_t hwsettings_mainport;
    HwSettingsSPK2_MainPortGet(&hwsettings_mainport);

    if (hwsettings_mainport < NELEMENTS(main_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_main_cfg, main_function_map[hwsettings_mainport]);
    }

#if defined(PIOS_INCLUDE_RFM22B)
    PIOS_BOARD_IO_Configure_RFM22B();

    uint8_t hwsettings_radioaux;
    HwSettingsRadioAuxStreamGet(&hwsettings_radioaux);

    PIOS_BOARD_IO_Configure_RadioAuxStream(hwsettings_radioaux);
#endif /* PIOS_INCLUDE_RFM22B */

    // Configure the receiver port
    // Sparky2 receiver input on PC7 TIM8 CH2
    // include PPM,S.Bus,DSM,SRXL,IBus,EX.Bus,HoTT SUMD,HoTT SUMH
    /* Configure the receiver port*/

    uint8_t hwsettings_rcvrport;
    HwSettingsSPK2_RcvrPortGet(&hwsettings_rcvrport);

    if (hwsettings_rcvrport < NELEMENTS(rcvr_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_rcvr_cfg, rcvr_function_map[hwsettings_rcvrport]);
    }

#if defined(PIOS_INCLUDE_PPM)
    if (hwsettings_rcvrport == HWSETTINGS_SPK2_RCVRPORT_PPM) {
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
    }
#endif

#ifdef PIOS_INCLUDE_GCSRCVR
    PIOS_BOARD_IO_Configure_GCS_RCVR();
#endif

#ifndef PIOS_ENABLE_DEBUG_PINS
    PIOS_Servo_Init(&pios_servo_cfg_out);
#else
    PIOS_DEBUG_Init(pios_tim_servoport_all_pins, NELEMENTS(pios_tim_servoport_all_pins));
#endif

    PIOS_BOARD_Sensors_Configure();

#ifdef PIOS_INCLUDE_WS2811
    HwSettingsWS2811LED_OutOptions ws2811_pin_settings;
    HwSettingsWS2811LED_OutGet(&ws2811_pin_settings);

    if (ws2811_pin_settings != HWSETTINGS_WS2811LED_OUT_DISABLED && ws2811_pin_settings < NELEMENTS(pios_ws2811_pin_cfg)) {
        PIOS_WS2811_Init(&pios_ws2811_id, &pios_ws2811_cfg, &pios_ws2811_pin_cfg[ws2811_pin_settings]);
    }
#endif // PIOS_INCLUDE_WS2811

#ifdef PIOS_INCLUDE_ADC
    // Disable GPIO_A8 Pullup to prevent wrong results on battery voltage readout
    GPIO_InitTypeDef gpioA8 = {
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode  = GPIO_Mode_IN,
        .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        .GPIO_Pin   = GPIO_Pin_8,
        .GPIO_OType = GPIO_OType_OD,
    };
    GPIO_Init(GPIOA, &gpioA8);
#endif /* PIOS_INCLUDE_ADC */
}

/**
 * @}
 * @}
 */
