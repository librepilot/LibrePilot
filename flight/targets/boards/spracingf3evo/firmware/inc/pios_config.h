/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @file       pios_config.h
 * @author     LibrePilot, https://bitbucket.org/librepilot, Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010-2013.
 * @brief      PiOS configuration header, the compile time config file for the PIOS.
 *             Defines which PiOS libraries and features are included in the firmware.
 * @see        The GNU Public License (GPL) Version 3
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

#ifndef PIOS_CONFIG_H
#define PIOS_CONFIG_H

/*
 * Below is a complete list of PIOS configurable options.
 * Please do not remove or rearrange them. Only comment out
 * unused options in the list. See main pios.h header for more
 * details.
 */

/* #define PIOS_INCLUDE_DEBUG_CONSOLE */
/* #define DEBUG_LEVEL 0 */
/* #define PIOS_ENABLE_DEBUG_PINS */

/* PIOS FreeRTOS support */
#define PIOS_INCLUDE_FREERTOS


/* PIOS CallbackScheduler support */
#define PIOS_INCLUDE_CALLBACKSCHEDULER

/* PIOS bootloader helper */
#define PIOS_INCLUDE_BL_HELPER
/* #define PIOS_INCLUDE_BL_HELPER_WRITE_SUPPORT */

/* PIOS system functions */
#define PIOS_INCLUDE_DELAY
#define PIOS_INCLUDE_INITCALL
#define PIOS_INCLUDE_SYS
#define PIOS_INCLUDE_TASK_MONITOR
// #define PIOS_INCLUDE_INSTRUMENTATION
#define PIOS_INSTRUMENTATION_MAX_COUNTERS 5

/* PIOS hardware peripherals */
#define PIOS_INCLUDE_IRQ
#define PIOS_INCLUDE_RTC
#define PIOS_INCLUDE_TIM
#define PIOS_INCLUDE_USART
#define PIOS_INCLUDE_ADC
#define PIOS_INCLUDE_I2C
#define PIOS_INCLUDE_SPI
#define PIOS_INCLUDE_GPIO
#define PIOS_INCLUDE_EXTI
#define PIOS_INCLUDE_WDG

/* PIOS USB functions */
#define PIOS_INCLUDE_USB
#define PIOS_INCLUDE_USB_HID
#define PIOS_INCLUDE_USB_CDC
/* #define PIOS_INCLUDE_USB_RCTX */

/* PIOS sensor interfaces */
/* #define PIOS_INCLUDE_ADXL345 */
/* #define PIOS_INCLUDE_BMA180 */
/* #define PIOS_INCLUDE_L3GD20 */
/* #define PIOS_INCLUDE_MPU6000 */
/* #define PIOS_MPU6000_ACCEL */
/* #define PIOS_INCLUDE_HMC5843 */
#define PIOS_INCLUDE_HMC5X83
/* #define PIOS_HMC5883_HAS_GPIOS */
#define PIOS_INCLUDE_MPU9250
#define PIOS_MPU9250_ACCEL
#define PIOS_MPU9250_MAG
#define PIOS_INCLUDE_BMP280
/* #define PIOS_INCLUDE_BMP085 */
/* #define PIOS_INCLUDE_MS56XX */
/* #define PIOS_INCLUDE_MPXV */
/* #define PIOS_INCLUDE_ETASV3 */
/* #define PIOS_INCLUDE_HCSR04 */

#define PIOS_SENSOR_RATE 500.0f

#define PIOS_INCLUDE_WS2811

/* PIOS receiver drivers */
#define PIOS_INCLUDE_PWM
#define PIOS_INCLUDE_PPM
#define PIOS_INCLUDE_PPM_FLEXI
#define PIOS_INCLUDE_DSM
#define PIOS_INCLUDE_SBUS
#define PIOS_INCLUDE_EXBUS
#define PIOS_INCLUDE_SRXL
#define PIOS_INCLUDE_HOTT
#define PIOS_INCLUDE_IBUS
#define PIOS_INCLUDE_GCSRCVR
#define PIOS_INCLUDE_OPLINKRCVR

/* PIOS abstract receiver interface */
#define PIOS_INCLUDE_RCVR

/* PIOS common peripherals */
#define PIOS_INCLUDE_LED
#define PIOS_INCLUDE_IAP
#define PIOS_INCLUDE_SERVO
/* #define PIOS_INCLUDE_I2C_ESC */
/* #define PIOS_INCLUDE_OVERO */
/* #define PIOS_OVERO_SPI */
/* #define PIOS_INCLUDE_SDCARD */
/* #define LOG_FILENAME "startup.log" */
#define PIOS_INCLUDE_FLASH
#define PIOS_INCLUDE_FLASH_LOGFS_SETTINGS
#define FLASH_FREERTOS
/* #define PIOS_INCLUDE_FLASH_EEPROM */
#define PIOS_INCLUDE_FLASH_INTERNAL

/* PIOS radio modules */
/* #define PIOS_INCLUDE_RFM22B */
/* #define PIOS_INCLUDE_RFM22B_COM */
/* #define PIOS_INCLUDE_PPM_OUT */
/* #define PIOS_RFM22B_DEBUG_ON_TELEM */

/* PIOS misc peripherals */
/* #define PIOS_INCLUDE_VIDEO */
/* #define PIOS_INCLUDE_WAVE */
/* #define PIOS_INCLUDE_UDP */

/* PIOS abstract comms interface with options */
#define PIOS_INCLUDE_COM
/* #define PIOS_INCLUDE_COM_MSG */
#define PIOS_INCLUDE_TELEMETRY_RF
/* #define PIOS_INCLUDE_COM_TELEM */
/* #define PIOS_INCLUDE_COM_FLEXI */
/* #define PIOS_INCLUDE_COM_AUX */
/* #define PIOS_TELEM_PRIORITY_QUEUE */
#define PIOS_INCLUDE_GPS
/* #define PIOS_GPS_MINIMAL */
#define PIOS_INCLUDE_GPS_NMEA_PARSER
#define PIOS_INCLUDE_GPS_UBX_PARSER
#define PIOS_INCLUDE_GPS_DJI_PARSER
#define PIOS_GPS_SETS_HOMELOCATION

/* Stabilization options */
#define PIOS_QUATERNION_STABILIZATION
/* #define PIOS_EXCLUDE_ADVANCED_FEATURES */
/* Performance counters */
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD  1995998

/* Alarm Thresholds */
#define HEAP_LIMIT_WARNING              220
#define HEAP_LIMIT_CRITICAL             40
#define IRQSTACK_LIMIT_WARNING          100
#define IRQSTACK_LIMIT_CRITICAL         60
#define CPULOAD_LIMIT_WARNING           85
#define CPULOAD_LIMIT_CRITICAL          95

/* Task stack sizes */
#define PIOS_ACTUATOR_STACK_SIZE        800
#define PIOS_MANUAL_STACK_SIZE          935
#define PIOS_RECEIVER_STACK_SIZE        840
#define PIOS_SYSTEM_STACK_SIZE          1536
/* #define PIOS_STABILIZATION_STACK_SIZE   400 */

#define PIOS_TELEM_STACK_SIZE           800
#define PIOS_EVENTDISPATCHER_STACK_SIZE 256

/* This can't be too high to stop eventdispatcher thread overflowing */
#define PIOS_EVENTDISAPTCHER_QUEUE      10

/* Revolution series */
#define REVOLUTION

#endif /* PIOS_CONFIG_H */
/**
 * @}
 * @}
 */
