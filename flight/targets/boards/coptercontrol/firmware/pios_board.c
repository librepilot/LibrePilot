/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 *
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the CopterControl board.
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

#ifdef PIOS_INCLUDE_INSTRUMENTATION
#include <pios_instrumentation.h>
#endif
#if defined(PIOS_INCLUDE_ADXL345)
#include <pios_adxl345.h>
#endif

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, DSMMAINPORT, DSMFLEXIPORT, SBUS
 * NOTE: No slot in this map for NONE.
 */
uint32_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

static SystemAlarmsExtendedAlarmStatusOptions CopterControlConfigHook();
static void ActuatorSettingsUpdatedCb(UAVObjEvent *ev);

#define PIOS_COM_TELEM_RF_RX_BUF_LEN     32
#define PIOS_COM_TELEM_RF_TX_BUF_LEN     12

#define PIOS_COM_GPS_RX_BUF_LEN          32

#define PIOS_COM_TELEM_USB_RX_BUF_LEN    65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN    65

#define PIOS_COM_BRIDGE_RX_BUF_LEN       65
#define PIOS_COM_BRIDGE_TX_BUF_LEN       12

#define PIOS_COM_HKOSD_TX_BUF_LEN        22

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uint32_t pios_com_debug_id;
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

uint32_t pios_com_telem_rf_id;
uint32_t pios_com_telem_usb_id;
uint32_t pios_com_vcp_id;
uint32_t pios_com_gps_id;
uint32_t pios_com_bridge_id;
uint32_t pios_com_hkosd_id;

uint32_t pios_usb_rctx_id;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;
/**
 * Configuration for MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU6000)
#include "pios_mpu6000.h"
#include "pios_mpu6000_config.h"
static const struct pios_exti_cfg pios_exti_mpu6000_cfg __exti_config = {
    .vector = PIOS_MPU6000_IRQHandler,
    .line   = EXTI_Line3,
    .pin    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_3,
            .GPIO_Speed = GPIO_Speed_10MHz,
            .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI3_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line3, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_mpu6000_cfg pios_mpu6000_cfg = {
    .exti_cfg   = &pios_exti_mpu6000_cfg,
    .Fifo_store = PIOS_MPU6000_FIFO_TEMP_OUT | PIOS_MPU6000_FIFO_GYRO_X_OUT | PIOS_MPU6000_FIFO_GYRO_Y_OUT | PIOS_MPU6000_FIFO_GYRO_Z_OUT,
    // Clock at 8 khz, downsampled by 8 for 1000 Hz
    .Smpl_rate_div_no_dlp = 7,
    // Clock at 1 khz, downsampled by 1 for 1000 Hz
    .Smpl_rate_div_dlp    = 0,
    .interrupt_cfg  = PIOS_MPU6000_INT_CLR_ANYRD,
    .interrupt_en   = PIOS_MPU6000_INTEN_DATA_RDY,
    .User_ctl             = PIOS_MPU6000_USERCTL_DIS_I2C,
    .Pwr_mgmt_clk   = PIOS_MPU6000_PWRMGMT_PLL_X_CLK,
    .accel_range    = PIOS_MPU6000_ACCEL_8G,
    .gyro_range     = PIOS_MPU6000_SCALE_2000_DEG,
    .filter               = PIOS_MPU6000_LOWPASS_256_HZ,
    .orientation    = PIOS_MPU6000_TOP_180DEG,
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
    .max_downsample = 2
};
#endif /* PIOS_INCLUDE_MPU6000 */

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
int32_t init_test;
void PIOS_Board_Init(void)
{
    /* Delay system */
    PIOS_DELAY_Init();

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

    switch (bdinfo->board_rev) {
    case BOARD_REVISION_CC:
        if (PIOS_SPI_Init(&pios_spi_flash_accel_id, &pios_spi_flash_accel_cfg_cc)) {
            PIOS_Assert(0);
        }
        break;
    case BOARD_REVISION_CC3D:
        if (PIOS_SPI_Init(&pios_spi_flash_accel_id, &pios_spi_flash_accel_cfg_cc3d)) {
            PIOS_Assert(0);
        }
        break;
    default:
        PIOS_Assert(0);
    }

#endif

    uintptr_t flash_id;
    switch (bdinfo->board_rev) {
    case BOARD_REVISION_CC:
        if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_flash_accel_id, 1)) {
            PIOS_DEBUG_Assert(0);
        }
        if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_w25x_cfg, &pios_jedec_flash_driver, flash_id)) {
            PIOS_DEBUG_Assert(0);
        }
        break;
    case BOARD_REVISION_CC3D:
        if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_flash_accel_id, 0)) {
            PIOS_DEBUG_Assert(0);
        }
        if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_m25p_cfg, &pios_jedec_flash_driver, flash_id)) {
            PIOS_DEBUG_Assert(0);
        }
        break;
    default:
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

    HwSettingsInitialize();

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
    /* Initialize board specific USB data */
    PIOS_USB_BOARD_DATA_Init();


    /* Flags to determine if various USB interfaces are advertised */
    bool usb_hid_present = false;
    bool usb_cdc_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
    if (PIOS_USB_DESC_HID_CDC_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
    usb_cdc_present = true;
#else
    if (PIOS_USB_DESC_HID_ONLY_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
#endif

    uint32_t pios_usb_id;

    switch (bdinfo->board_rev) {
    case BOARD_REVISION_CC:
        PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg_cc);
        break;
    case BOARD_REVISION_CC3D:
        PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg_cc3d);
        break;
    default:
        PIOS_Assert(0);
    }

#if defined(PIOS_INCLUDE_USB_CDC)

    uint8_t hwsettings_usb_vcpport;
    /* Configure the USB VCP port */
    HwSettingsUSB_VCPPortGet(&hwsettings_usb_vcpport);

    if (!usb_cdc_present) {
        /* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
        hwsettings_usb_vcpport = HWSETTINGS_USB_VCPPORT_DISABLED;
    }

    switch (hwsettings_usb_vcpport) {
    case HWSETTINGS_USB_VCPPORT_DISABLED:
        break;
    case HWSETTINGS_USB_VCPPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_cdc_id;
            if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_VCPPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_cdc_id;
            if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_VCPPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            uint32_t pios_usb_cdc_id;
            if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_debug_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              NULL, 0,
                              tx_buffer, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
#endif /* PIOS_INCLUDE_COM */
        break;
    }
#endif /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
    /* Configure the usb HID port */
    uint8_t hwsettings_usb_hidport;
    HwSettingsUSB_HIDPortGet(&hwsettings_usb_hidport);

    if (!usb_hid_present) {
        /* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
        hwsettings_usb_hidport = HWSETTINGS_USB_HIDPORT_DISABLED;
    }

    switch (hwsettings_usb_hidport) {
    case HWSETTINGS_USB_HIDPORT_DISABLED:
        break;
    case HWSETTINGS_USB_HIDPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint32_t pios_usb_hid_id;
            if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_HIDPORT_RCTRANSMITTER:
#if defined(PIOS_INCLUDE_USB_RCTX)
        {
            if (PIOS_USB_RCTX_Init(&pios_usb_rctx_id, &pios_usb_rctx_cfg, pios_usb_id)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_USB_RCTX */
        break;
    }

#endif /* PIOS_INCLUDE_USB_HID */

#endif /* PIOS_INCLUDE_USB */

    /* Configure the main IO port */
    uint8_t hwsettings_DSMxBind;
    HwSettingsDSMxBindGet(&hwsettings_DSMxBind);
    uint8_t hwsettings_cc_mainport;
    HwSettingsCC_MainPortGet(&hwsettings_cc_mainport);

    switch (hwsettings_cc_mainport) {
    case HWSETTINGS_CC_MAINPORT_DISABLED:
        break;
    case HWSETTINGS_CC_MAINPORT_TELEMETRY:
#if defined(PIOS_INCLUDE_TELEMETRY_RF)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_main_cfg)) {
                PIOS_Assert(0);
            }

            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_rf_id, &pios_usart_com_driver, pios_usart_generic_id,
                              rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_TELEMETRY_RF */
        break;
    case HWSETTINGS_CC_MAINPORT_SBUS:
#if defined(PIOS_INCLUDE_SBUS)
        {
            uint32_t pios_usart_sbus_id;
            if (PIOS_USART_Init(&pios_usart_sbus_id, &pios_usart_sbus_main_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_sbus_id;
            if (PIOS_SBus_Init(&pios_sbus_id, &pios_sbus_cfg, &pios_usart_com_driver, pios_usart_sbus_id)) {
                PIOS_Assert(0);
            }

            uint32_t pios_sbus_rcvr_id;
            if (PIOS_RCVR_Init(&pios_sbus_rcvr_id, &pios_sbus_rcvr_driver, pios_sbus_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS] = pios_sbus_rcvr_id;
        }
#endif /* PIOS_INCLUDE_SBUS */
        break;
    case HWSETTINGS_CC_MAINPORT_GPS:
#if defined(PIOS_INCLUDE_GPS)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_main_cfg)) {
                PIOS_Assert(0);
            }

            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_GPS_RX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            if (PIOS_COM_Init(&pios_com_gps_id, &pios_usart_com_driver, pios_usart_generic_id,
                              rx_buffer, PIOS_COM_GPS_RX_BUF_LEN,
                              NULL, 0)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_GPS */
        break;
    case HWSETTINGS_CC_MAINPORT_DSM:
#if defined(PIOS_INCLUDE_DSM)
        {
            uint32_t pios_usart_dsm_id;
            if (PIOS_USART_Init(&pios_usart_dsm_id, &pios_usart_dsm_main_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_dsm_id;
            if (PIOS_DSM_Init(&pios_dsm_id,
                              &pios_dsm_main_cfg,
                              &pios_usart_com_driver,
                              pios_usart_dsm_id,
                              0)) {
                PIOS_Assert(0);
            }

            uint32_t pios_dsm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_dsm_rcvr_id, &pios_dsm_rcvr_driver, pios_dsm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT] = pios_dsm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_DSM */
        break;
    case HWSETTINGS_CC_MAINPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_main_cfg)) {
                PIOS_Assert(0);
            }

            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_debug_id, &pios_usart_com_driver, pios_usart_generic_id,
                              NULL, 0,
                              tx_buffer, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_CC_MAINPORT_COMBRIDGE:
    {
        uint32_t pios_usart_generic_id;
        if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_main_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
        PIOS_Assert(rx_buffer);
        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_bridge_id, &pios_usart_com_driver, pios_usart_generic_id,
                          rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
                          tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
    break;
    case HWSETTINGS_CC_MAINPORT_OSDHK:
    {
        uint32_t pios_usart_hkosd_id;
        if (PIOS_USART_Init(&pios_usart_hkosd_id, &pios_usart_hkosd_main_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_HKOSD_TX_BUF_LEN);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_hkosd_id, &pios_usart_com_driver, pios_usart_hkosd_id,
                          NULL, 0,
                          tx_buffer, PIOS_COM_HKOSD_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
    break;
    }

    /* Configure the flexi port */
    uint8_t hwsettings_cc_flexiport;
    HwSettingsCC_FlexiPortGet(&hwsettings_cc_flexiport);

    switch (hwsettings_cc_flexiport) {
    case HWSETTINGS_CC_FLEXIPORT_DISABLED:
        break;
    case HWSETTINGS_CC_FLEXIPORT_TELEMETRY:
#if defined(PIOS_INCLUDE_TELEMETRY_RF)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_flexi_cfg)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_rf_id, &pios_usart_com_driver, pios_usart_generic_id,
                              rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_TELEMETRY_RF */
        break;
    case HWSETTINGS_CC_FLEXIPORT_COMBRIDGE:
    {
        uint32_t pios_usart_generic_id;
        if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_flexi_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
        PIOS_Assert(rx_buffer);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_bridge_id, &pios_usart_com_driver, pios_usart_generic_id,
                          rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
                          tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
    break;
    case HWSETTINGS_CC_FLEXIPORT_GPS:
#if defined(PIOS_INCLUDE_GPS)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_flexi_cfg)) {
                PIOS_Assert(0);
            }
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_GPS_RX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            if (PIOS_COM_Init(&pios_com_gps_id, &pios_usart_com_driver, pios_usart_generic_id,
                              rx_buffer, PIOS_COM_GPS_RX_BUF_LEN,
                              NULL, 0)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_GPS */
        break;
    case HWSETTINGS_CC_FLEXIPORT_PPM:
#if defined(PIOS_INCLUDE_PPM_FLEXI)
        {
            uint32_t pios_ppm_id;
            PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_flexi_cfg);

            uint32_t pios_ppm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PPM_FLEXI */
        break;
    case HWSETTINGS_CC_FLEXIPORT_DSM:
#if defined(PIOS_INCLUDE_DSM)
        {
            uint32_t pios_usart_dsm_id;
            if (PIOS_USART_Init(&pios_usart_dsm_id, &pios_usart_dsm_flexi_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_dsm_id;
            if (PIOS_DSM_Init(&pios_dsm_id,
                              &pios_dsm_flexi_cfg,
                              &pios_usart_com_driver,
                              pios_usart_dsm_id,
                              hwsettings_DSMxBind)) {
                PIOS_Assert(0);
            }

            uint32_t pios_dsm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_dsm_rcvr_id, &pios_dsm_rcvr_driver, pios_dsm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMFLEXIPORT] = pios_dsm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_DSM */
        break;
    case HWSETTINGS_CC_FLEXIPORT_SRXL:
#if defined(PIOS_INCLUDE_SRXL)
        {
            uint32_t pios_usart_srxl_id;
            if (PIOS_USART_Init(&pios_usart_srxl_id, &pios_usart_srxl_flexi_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_srxl_id;
            if (PIOS_SRXL_Init(&pios_srxl_id, &pios_usart_com_driver, pios_usart_srxl_id)) {
                PIOS_Assert(0);
            }

            uint32_t pios_srxl_rcvr_id;
            if (PIOS_RCVR_Init(&pios_srxl_rcvr_id, &pios_srxl_rcvr_driver, pios_srxl_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_SRXL] = pios_srxl_rcvr_id;
        }
#endif /* PIOS_INCLUDE_SRXL */
        break;

    case HWSETTINGS_CC_FLEXIPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            uint32_t pios_usart_generic_id;
            if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_flexi_cfg)) {
                PIOS_Assert(0);
            }

            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_debug_id, &pios_usart_com_driver, pios_usart_generic_id,
                              NULL, 0,
                              tx_buffer, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_CC_FLEXIPORT_I2C:
#if defined(PIOS_INCLUDE_I2C)
        {
            if (PIOS_I2C_Init(&pios_i2c_flexi_adapter_id, &pios_i2c_flexi_adapter_cfg)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_I2C */
        break;
    case HWSETTINGS_CC_FLEXIPORT_OSDHK:
    {
        uint32_t pios_usart_hkosd_id;
        if (PIOS_USART_Init(&pios_usart_hkosd_id, &pios_usart_hkosd_flexi_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_HKOSD_TX_BUF_LEN);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_hkosd_id, &pios_usart_com_driver, pios_usart_hkosd_id,
                          NULL, 0,
                          tx_buffer, PIOS_COM_HKOSD_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
    break;
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
        {
            uint32_t pios_pwm_id;
            PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_cfg);

            uint32_t pios_pwm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_CC_RCVRPORT_PPMNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPMOUTPUTSNOONESHOT:
    case HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT:
#if defined(PIOS_INCLUDE_PPM)
        {
            uint32_t pios_ppm_id;
            if (hwsettings_rcvrport == HWSETTINGS_CC_RCVRPORT_PPM_PIN8ONESHOT) {
                PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_pin8_cfg);
            } else {
                PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_cfg);
            }

            uint32_t pios_ppm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PPM */
        break;
    case HWSETTINGS_CC_RCVRPORT_PPMPWMNOONESHOT:
        /* This is a combination of PPM and PWM inputs */
#if defined(PIOS_INCLUDE_PPM)
        {
            uint32_t pios_ppm_id;
            PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_cfg);

            uint32_t pios_ppm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PPM */
#if defined(PIOS_INCLUDE_PWM)
        {
            uint32_t pios_pwm_id;
            PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_with_ppm_cfg);

            uint32_t pios_pwm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_CC_RCVRPORT_OUTPUTSONESHOT:
        break;
    }

#if defined(PIOS_INCLUDE_GCSRCVR)
    GCSReceiverInitialize();
    uint32_t pios_gcsrcvr_id;
    PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
    uint32_t pios_gcsrcvr_rcvr_id;
    if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
#endif /* PIOS_INCLUDE_GCSRCVR */

    /* Remap AFIO pin for PB4 (Servo 5 Out)*/
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);

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

    switch (bdinfo->board_rev) {
    case BOARD_REVISION_CC:
        // Revision 1 with invensense gyros, start the ADC
#if defined(PIOS_INCLUDE_ADC)
        PIOS_ADC_Init(&pios_adc_cfg);
#endif
#if defined(PIOS_INCLUDE_ADXL345)
        PIOS_ADXL345_Init(pios_spi_flash_accel_id, 0);
#endif
        break;
    case BOARD_REVISION_CC3D:
        // Revision 2 with MPU6000 gyros, start a SPI interface and connect to it
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

#if defined(PIOS_INCLUDE_MPU6000)
        // Set up the SPI interface to the serial flash
        if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
            PIOS_Assert(0);
        }
        PIOS_MPU6000_Init(pios_spi_gyro_id, 0, &pios_mpu6000_cfg);
        PIOS_MPU6000_CONFIG_Configure();
        init_test = !PIOS_MPU6000_Driver.test(0);
#endif /* PIOS_INCLUDE_MPU6000 */

        break;
    default:
        PIOS_Assert(0);
    }

    /* Make sure we have at least one telemetry link configured or else fail initialization */
    PIOS_Assert(pios_com_telem_rf_id || pios_com_telem_usb_id);

    // Attach the board config check hook
    SANITYCHECK_AttachHook(&CopterControlConfigHook);
    // trigger a config check if actuatorsettings are updated
    ActuatorSettingsInitialize();
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
             modes[3] == ACTUATORSETTINGS_BANKMODE_ONESHOT125)) {
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
                modes[i] == ACTUATORSETTINGS_BANKMODE_ONESHOT125) {
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
