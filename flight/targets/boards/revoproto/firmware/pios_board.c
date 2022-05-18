/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the revolution board.
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

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{
    /* Delay system */
    PIOS_DELAY_Init();

    PIOS_LED_Init(&pios_led_cfg);

    /* Connect flash to the appropriate interface and configure it */
    uintptr_t flash_id;

    // initialize the internal settings storage flash
    if (PIOS_Flash_Internal_Init(&flash_id, &flash_internal_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Set up the SPI interface to the accelerometer*/
    if (PIOS_SPI_Init(&pios_spi_accel_id, &pios_spi_accel_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Set up the SPI interface to the gyro */
    if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#if !defined(PIOS_FLASH_ON_ACCEL)
    /* Set up the SPI interface to the flash */
    if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Connect flash to the appropriate interface and configure it */
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_flash_id, 0)) {
        PIOS_DEBUG_Assert(0);
    }
#else
    /* Connect flash to the appropriate interface and configure it */
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_accel_id, 1)) {
        PIOS_DEBUG_Assert(0);
    }
#endif
    if (PIOS_FLASHFS_Logfs_Init(&pios_user_fs_id, &flashfs_external_cfg, &pios_jedec_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

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
    PIOS_TIM_InitClock(&tim_9_cfg);
    PIOS_TIM_InitClock(&tim_10_cfg);
    PIOS_TIM_InitClock(&tim_11_cfg);

    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }


    // PIOS_IAP_Init();

    /* Configure USB */
#if defined(PIOS_INCLUDE_USB)
    PIOS_BOARD_IO_Configure_USB();
#endif
    /* Configure IO ports */

    /* Configure Telemetry port */
    static const PIOS_BOARD_IO_UART_Function usart_telem_function_map[] = {
        [HWSETTINGS_RV_TELEMETRYPORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
// [HWSETTINGS_RV_TELEMETRYPORT_COMAUX] = ?? &pios_com_aux_id // UNUSED
        [HWSETTINGS_RV_TELEMETRYPORT_COMBRIDGE] = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_RV_TELEMETRYPORT_MSP] = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_RV_TELEMETRYPORT_MAVLINK]   = PIOS_BOARD_IO_UART_MAVLINK
    };

    uint8_t hwsettings_rv_telemetryport;
    HwSettingsRV_TelemetryPortGet(&hwsettings_rv_telemetryport);

    if (hwsettings_rv_telemetryport < NELEMENTS(usart_telem_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_telem_cfg, usart_telem_function_map[hwsettings_rv_telemetryport]);
    }


    /* Configure GPS port */
    static const PIOS_BOARD_IO_UART_Function usart_gps_function_map[] = {
        [HWSETTINGS_RV_GPSPORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWSETTINGS_RV_GPSPORT_GPS] = PIOS_BOARD_IO_UART_GPS,
// [HWSETTINGS_RV_GPSPORT_COMAUX] =
        [HWSETTINGS_RV_GPSPORT_COMBRIDGE] = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_RV_GPSPORT_MSP] = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_RV_GPSPORT_MAVLINK]   = PIOS_BOARD_IO_UART_MAVLINK,
    };

    uint8_t hwsettings_rv_gpsport;
    HwSettingsRV_GPSPortGet(&hwsettings_rv_gpsport);

    if (hwsettings_rv_gpsport < NELEMENTS(usart_gps_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_gps_cfg, usart_gps_function_map[hwsettings_rv_gpsport]);
    }

    /* Configure AUXPort */
    static const PIOS_BOARD_IO_UART_Function usart_aux_function_map[] = {
        [HWSETTINGS_RV_AUXPORT_TELEMETRY] = PIOS_BOARD_IO_UART_TELEMETRY,
        [HWSETTINGS_RV_AUXPORT_DSM] = PIOS_BOARD_IO_UART_DSM_MAIN,
        // [HWSETTINGS_RV_AUXPORT_COMAUX] =
        [HWSETTINGS_RV_AUXPORT_COMBRIDGE] = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_RV_AUXPORT_MSP] = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_RV_AUXPORT_MAVLINK]   = PIOS_BOARD_IO_UART_MAVLINK,
        [HWSETTINGS_RV_AUXPORT_OSDHK]     = PIOS_BOARD_IO_UART_OSDHK,
    };

    uint8_t hwsettings_rv_auxport;
    HwSettingsRV_AuxPortGet(&hwsettings_rv_auxport);

    if (hwsettings_rv_auxport < NELEMENTS(usart_aux_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_aux_cfg, usart_aux_function_map[hwsettings_rv_auxport]);
    }

    /* Configure AUXSbusPort */
    static const PIOS_BOARD_IO_UART_Function usart_auxsbus_function_map[] = {
        [HWSETTINGS_RV_AUXSBUSPORT_SBUS]      = PIOS_BOARD_IO_UART_SBUS,
        [HWSETTINGS_RV_AUXSBUSPORT_DSM]       = PIOS_BOARD_IO_UART_DSM_MAIN, /* MAIN group, same as usart_aux! */
        // [HWSETTINGS_RV_AUXSBUSPORT_COMAUX] =
        [HWSETTINGS_RV_AUXSBUSPORT_COMBRIDGE] = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_RV_AUXSBUSPORT_MSP]       = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_RV_AUXSBUSPORT_MAVLINK]   = PIOS_BOARD_IO_UART_MAVLINK,
        [HWSETTINGS_RV_AUXSBUSPORT_OSDHK]     = PIOS_BOARD_IO_UART_OSDHK,
    };

    uint8_t hwsettings_rv_auxsbusport;
    HwSettingsRV_AuxSBusPortGet(&hwsettings_rv_auxsbusport);

    if (hwsettings_rv_auxsbusport < NELEMENTS(usart_auxsbus_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_auxsbus_cfg, usart_auxsbus_function_map[hwsettings_rv_auxsbusport]);
    }

    /* Configure FlexiPort */
    static const PIOS_BOARD_IO_UART_Function usart_flexi_function_map[] = {
        [HWSETTINGS_RV_FLEXIPORT_DSM]       = PIOS_BOARD_IO_UART_DSM_FLEXI,
        // [HWSETTINGS_RV_FLEXIPORT_COMAUX] =
        [HWSETTINGS_RV_FLEXIPORT_COMBRIDGE] = PIOS_BOARD_IO_UART_COMBRIDGE,
        [HWSETTINGS_RV_FLEXIPORT_MSP]       = PIOS_BOARD_IO_UART_MSP,
        [HWSETTINGS_RV_FLEXIPORT_MAVLINK]   = PIOS_BOARD_IO_UART_MAVLINK,
    };

    uint8_t hwsettings_rv_flexiport;
    HwSettingsRV_FlexiPortGet(&hwsettings_rv_flexiport);

    if (hwsettings_rv_flexiport < NELEMENTS(usart_flexi_function_map)) {
        PIOS_BOARD_IO_Configure_UART(&pios_usart_flexi_cfg, usart_flexi_function_map[hwsettings_rv_flexiport]);
    }

#if defined(PIOS_INCLUDE_I2C)
    if (hwsettings_rv_flexiport == HWSETTINGS_RV_FLEXIPORT_I2C) {
        if (PIOS_I2C_Init(&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg)) {
            PIOS_Assert(0);
        }
        PIOS_DELAY_WaitmS(50); // this was after the other PIOS_I2C_Init(), so I copied it here too
#ifdef PIOS_INCLUDE_WDG
        // give HMC5x83 on I2C some extra time to allow for reset, etc. if needed
        // this is not in a loop, so it is safe
        // leave this here even if PIOS_INCLUDE_HMC5X83 is undefined
        // to avoid making something else fail when HMC5X83 is removed
        PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */
    }
#endif /* PIOS_INCLUDE_I2C */

    /* Configure the receiver port*/
    uint8_t hwsettings_rcvrport;
    HwSettingsRV_RcvrPortGet(&hwsettings_rcvrport);
    //
    switch (hwsettings_rcvrport) {
    case HWSETTINGS_RV_RCVRPORT_DISABLED:
        break;
    case HWSETTINGS_RV_RCVRPORT_PWM:
#if defined(PIOS_INCLUDE_PWM)
        PIOS_BOARD_IO_Configure_PWM_RCVR(&pios_pwm_cfg);
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_RV_RCVRPORT_PPM:
    case HWSETTINGS_RV_RCVRPORT_PPMOUTPUTS:
#if defined(PIOS_INCLUDE_PPM)
        PIOS_BOARD_IO_Configure_PPM_RCVR(&pios_ppm_cfg);
#endif /* PIOS_INCLUDE_PPM */
    case HWSETTINGS_RV_RCVRPORT_OUTPUTS:

        break;
    }

#if defined(PIOS_OVERO_SPI)
    /* Set up the SPI based PIOS_COM interface to the overo */
    {
        HwSettingsData hwSettings;
        HwSettingsGet(&hwSettings);
        if (hwSettings.OptionalModules.Overo == HWSETTINGS_OPTIONALMODULES_ENABLED) {
            if (PIOS_OVERO_Init(&pios_overo_id, &pios_overo_cfg)) {
                PIOS_DEBUG_Assert(0);
            }
            const uint32_t PACKET_SIZE = 1024;
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PACKET_SIZE);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PACKET_SIZE);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_overo_id, &pios_overo_com_driver, pios_overo_id,
                              rx_buffer, PACKET_SIZE,
                              tx_buffer, PACKET_SIZE)) {
                PIOS_Assert(0);
            }
        }
    }

#endif /* if defined(PIOS_OVERO_SPI) */

#if defined(PIOS_INCLUDE_HCSR04)
    {
        PIOS_TIM_InitClock(&tim_8_cfg);
        uint32_t pios_hcsr04_id;
        PIOS_HCSR04_Init(&pios_hcsr04_id, &pios_hcsr04_cfg);
    }
#endif

#ifdef PIOS_INCLUDE_GCSRCVR
    PIOS_BOARD_IO_Configure_GCS_RCVR();
#endif

#ifdef PIOS_INCLUDE_OPLINKRCVR
    PIOS_BOARD_IO_Configure_OPLink_RCVR();
#endif

#ifndef PIOS_ENABLE_DEBUG_PINS
    switch (hwsettings_rcvrport) {
    case HWSETTINGS_RV_RCVRPORT_DISABLED:
    case HWSETTINGS_RV_RCVRPORT_PWM:
    case HWSETTINGS_RV_RCVRPORT_PPM:
        /* Set up the servo outputs */
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    case HWSETTINGS_RV_RCVRPORT_PPMOUTPUTS:
    case HWSETTINGS_RV_RCVRPORT_OUTPUTS:
        // PIOS_Servo_Init(&pios_servo_rcvr_cfg);
        // TODO: Prepare the configurations on board_hw_defs and handle here:
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    }
#else
    PIOS_DEBUG_Init(pios_tim_servoport_all_pins, NELEMENTS(pios_tim_servoport_all_pins));
#endif

    // init I2C1 for use with the internal mag
    if (PIOS_I2C_Init(&pios_i2c_mag_adapter_id, &pios_i2c_mag_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    // init I2C1 for use with the internal baro
    if (PIOS_I2C_Init(&pios_i2c_pressure_adapter_id, &pios_i2c_pressure_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    PIOS_DELAY_WaitmS(50);

    PIOS_BOARD_Sensors_Configure();
}

/**
 * @}
 * @}
 */
