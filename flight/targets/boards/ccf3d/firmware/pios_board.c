/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup LibrePilotSystem LibrePilot System
 * @{
 * @addtogroup LibrePilotCore LibrePilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the CCF3D board.
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
#include <gcsreceiver.h>
#include <taskinfo.h>
#include <sanitycheck.h>
#include <actuatorsettings.h>
#include <auxmagsettings.h>

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


static SystemAlarmsExtendedAlarmStatusOptions CopterControlConfigHook();
static void ActuatorSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev);

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
int32_t init_test;
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
    if (PIOS_SPI_Init(&pios_spi_flash_accel_id, &pios_spi_flash_accel_cfg_cc3d)) {
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
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }

    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);

#if defined(PIOS_INCLUDE_USB)
    PIOS_BOARD_IO_Configure_USB();
#endif

    /* Configure the main IO port */
    static const PIOS_BOARD_IO_UART_Function usart_main_function_map[] = {
        [HWSETTINGS_CC_MAINPORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWSETTINGS_CC_MAINPORT_GPS]          = PIOS_BOARD_IO_UART_GPS,
        [HWSETTINGS_CC_MAINPORT_SBUS]         = PIOS_BOARD_IO_UART_SBUS,
        [HWSETTINGS_CC_MAINPORT_DSM]          = PIOS_BOARD_IO_UART_DSM_MAIN,
        [HWSETTINGS_CC_MAINPORT_DEBUGCONSOLE] = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
        [HWSETTINGS_CC_MAINPORT_COMBRIDGE]    = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_CC_MAINPORT_OSDHK]        = PIOS_BOARD_IO_UART_OSDHK,
        [HWSETTINGS_CC_MAINPORT_MSP]          = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_CC_MAINPORT_MAVLINK]      = PIOS_BOARD_IO_UART_MAVLINK,
    };

    uint8_t hwsettings_cc_mainport;
    HwSettingsCC_MainPortGet(&hwsettings_cc_mainport);

    if (hwsettings_cc_mainport < NELEMENTS(usart_main_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_main_cfg, usart_main_function_map[hwsettings_cc_mainport]);
    }


    /* Configure the flexi port */
    static const PIOS_BOARD_IO_UART_Function usart_flexi_function_map[] = {
        [HWSETTINGS_CC_FLEXIPORT_TELEMETRY]    = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWSETTINGS_CC_FLEXIPORT_GPS] = PIOS_BOARD_IO_UART_GPS,
        [HWSETTINGS_CC_FLEXIPORT_DSM] = PIOS_BOARD_IO_UART_DSM_FLEXI,
        [HWSETTINGS_CC_FLEXIPORT_EXBUS]        = PIOS_BOARD_IO_UART_EXBUS,
        [HWSETTINGS_CC_FLEXIPORT_HOTTSUMD]     = PIOS_BOARD_IO_UART_HOTT_SUMD,
        [HWSETTINGS_CC_FLEXIPORT_HOTTSUMH]     = PIOS_BOARD_IO_UART_HOTT_SUMH,
        [HWSETTINGS_CC_FLEXIPORT_SRXL] = PIOS_BOARD_IO_UART_SRXL,
        [HWSETTINGS_CC_FLEXIPORT_IBUS] = PIOS_BOARD_IO_UART_IBUS,
        [HWSETTINGS_CC_FLEXIPORT_DEBUGCONSOLE] = PIOS_BOARD_IO_UART_DEBUGCONSOLE,
        [HWSETTINGS_CC_FLEXIPORT_COMBRIDGE]    = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_CC_FLEXIPORT_OSDHK]        = PIOS_BOARD_IO_UART_OSDHK,
        [HWSETTINGS_CC_FLEXIPORT_MSP] = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_CC_FLEXIPORT_MAVLINK]      = PIOS_BOARD_IO_UART_MAVLINK,
    };

    uint8_t hwsettings_cc_flexiport;
    HwSettingsCC_FlexiPortGet(&hwsettings_cc_flexiport);

    if (hwsettings_cc_flexiport < NELEMENTS(usart_flexi_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_flexi_cfg, usart_flexi_function_map[hwsettings_cc_flexiport]);
    }

    if (hwsettings_cc_flexiport == HWSETTINGS_CC_FLEXIPORT_PPM) {
#if defined(PIOS_INCLUDE_PPM_FLEXI)
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_flexi_cfg);
#endif /* PIOS_INCLUDE_PPM_FLEXI */
    } else if (hwsettings_cc_flexiport == HWSETTINGS_CC_FLEXIPORT_I2C) {
        // F303CC has no I2C on CC3D flexi.
    }

    /* Configure the rcvr port */
    uint8_t hwsettings_rcvrport;
    HwSettingsCC_RcvrPortGet(&hwsettings_rcvrport);

    switch ((HwSettingsCC_RcvrPortOptions)hwsettings_rcvrport) {
    case HWSETTINGS_CC_RCVRPORT_DISABLEDONESHOT:
#if defined(PIOS_INCLUDE_HCSR04)
        {
            uint32_t pios_hcsr04_id;
            PIOS_HCSR04_Init(&pios_hcsr04_id, &pios_hcsr04_cfg);
        }
#endif
        break;
    case HWSETTINGS_CC_RCVRPORT_PWMNOONESHOT:
#if defined(PIOS_INCLUDE_PWM)
        PIOS_BOARD_IO_Configure_PWM_RCVR(&pios_pwm_cfg);
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT:
#if defined(PIOS_INCLUDE_PPM)
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_pin8_cfg);
#endif /* PIOS_INCLUDE_PPM */
        break;
    case HWSETTINGS_CC_RCVRPORT_PPMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMOUTPUTSNOONESHOT:
#if defined(PIOS_INCLUDE_PPM)
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
#endif /* PIOS_INCLUDE_PPM */
        break;
    case HWSETTINGS_CC_RCVRPORT_PPMPWMNOONESHOT:
        /* This is a combination of PPM and PWM inputs */
#if defined(PIOS_INCLUDE_PPM)
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
#endif /* PIOS_INCLUDE_PPM */
#if defined(PIOS_INCLUDE_PWM)
        PIOS_BOARD_IO_Configure_PWM_RCVR(&pios_pwm_with_ppm_cfg);
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_CC_RCVRPORT_OUTPUTSONESHOT:
        break;
    }

#ifdef PIOS_INCLUDE_GCSRCVR
    PIOS_BOARD_IO_Configure_GCS_RCVR();
#endif

#ifdef PIOS_INCLUDE_OPLINKRCVR
    PIOS_BOARD_IO_Configure_OPLink_RCVR();
#endif

#ifndef PIOS_ENABLE_DEBUG_PINS
    switch ((HwSettingsCC_RcvrPortOptions)hwsettings_rcvrport) {
    case HWSETTINGS_CC_RCVRPORT_DISABLEDONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PWMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMPWMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT:
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    case HWSETTINGS_CC_RCVRPORT_PPMOUTPUTSNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_OUTPUTSONESHOT:
        PIOS_Servo_Init(&pios_servo_rcvr_cfg);
        break;
    }
#else
    PIOS_DEBUG_Init(pios_tim_servoport_all_pins, NELEMENTS(pios_tim_servoport_all_pins));
#endif /* PIOS_ENABLE_DEBUG_PINS */

    PIOS_BOARD_Sensors_Configure();

    /* Make sure we have at least one telemetry link configured or else fail initialization */
    PIOS_Assert(pios_com_telem_rf_id || pios_com_telem_usb_id);

    // Attach the board config check hook
    SANITYCHECK_AttachHook(&CopterControlConfigHook);
    // trigger a config check if actuatorsettings are updated
    ActuatorSettingsConnectCallback(ActuatorSettingsUpdatedCb);
}

SystemAlarmsExtendedAlarmStatusOptions CopterControlConfigHook()
{
    // inhibit usage of oneshot for non supported RECEIVER port modes
    uint8_t recmode;

    HwSettingsCC_RcvrPortGet(&recmode);
    uint8_t flexiMode;
    uint8_t modes[ACTUATORSETTINGS_BANKMODE_NUMELEM];
    ActuatorSettingsBankModeGet(modes);
    HwSettingsCC_FlexiPortGet(&flexiMode);

    switch ((HwSettingsCC_RcvrPortOptions)recmode) {
    // Those modes allows oneshot usage
    case HWSETTINGS_CC_RCVRPORT_DISABLEDONESHOT:
    case HWSETTINGS_CC_RCVRPORT_OUTPUTSONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT:
        if ((recmode == HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT ||
             flexiMode == HWSETTINGS_CC_FLEXIPORT_PPM) &&
            (modes[3] == ACTUATORSETTINGS_BANKMODE_PWMSYNC ||
             modes[3] == ACTUATORSETTINGS_BANKMODE_ONESHOT125 ||
             modes[3] == ACTUATORSETTINGS_BANKMODE_ONESHOT42 ||
             modes[3] == ACTUATORSETTINGS_BANKMODE_MULTISHOT)) {
            return SYSTEMALARMS_EXTENDEDALARMSTATUS_UNSUPPORTEDCONFIG_ONESHOT;
        } else {
            return SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE;
        }

    // inhibit oneshot for the following modes
    case HWSETTINGS_CC_RCVRPORT_PPMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMOUTPUTSNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMPWMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PWMNOONESHOT:
        for (uint8_t i = 0; i < ACTUATORSETTINGS_BANKMODE_NUMELEM; i++) {
            if (modes[i] == ACTUATORSETTINGS_BANKMODE_PWMSYNC ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_ONESHOT125 ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_ONESHOT42 ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_MULTISHOT) {
                return SYSTEMALARMS_EXTENDEDALARMSTATUS_UNSUPPORTEDCONFIG_ONESHOT;;
            }

            return SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE;
        }
    }
    return SYSTEMALARMS_EXTENDEDALARMSTATUS_UNSUPPORTEDCONFIG_ONESHOT;;
}
// trigger a configuration check if ActuatorSettings are changed.
void ActuatorSettingsUpdatedCb(__attribute__((unused)) UAVObjEvent *ev)
{
    configuration_check();
}

/**
 * @}
 */
