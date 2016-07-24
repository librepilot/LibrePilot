/**
 ******************************************************************************
 * @file       pios_board.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the revomini board.
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
#include <oplinksettings.h>
#include <oplinkstatus.h>
#include <oplinkreceiver.h>
#include <pios_oplinkrcvr_priv.h>
#include <taskinfo.h>
#include <pios_ws2811.h>
#include <sanitycheck.h>
#include <actuatorsettings.h>
#include <auxmagsettings.h>

#ifdef PIOS_INCLUDE_INSTRUMENTATION
#include <pios_instrumentation.h>
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

static SystemAlarmsExtendedAlarmStatusOptions RevoNanoConfigHook();
static void ActuatorSettingsUpdatedCb(UAVObjEvent *ev);
/**
 * Sensor configurations
 */

#if defined(PIOS_INCLUDE_ADC)

#include "pios_adc_priv.h"
void PIOS_ADC_DMC_irq_handler(void);
void DMA2_Stream4_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMC_irq_handler")));
struct pios_adc_cfg pios_adc_cfg = {
    .adc_dev = ADC1,
    .dma     = {
        .irq                                       = {
            .flags = (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_HTIF4),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Stream4_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel = DMA2_Stream4,
            .init    = {
                .DMA_Channel                       = DMA_Channel_0,
                .DMA_PeripheralBaseAddr            = (uint32_t)&ADC1->DR
            },
        }
    },
    .half_flag = DMA_IT_HTIF4,
    .full_flag = DMA_IT_TCIF4,
};
void PIOS_ADC_DMC_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_ADC_DMA_Handler();
}
#endif /* if defined(PIOS_INCLUDE_ADC) */

#if defined(PIOS_INCLUDE_HMC5X83)
#include "pios_hmc5x83.h"
pios_hmc5x83_dev_t external_mag = 0;

static const struct pios_hmc5x83_cfg pios_hmc5x83_external_cfg = {
#ifdef PIOS_HMC5X83_HAS_GPIOS
    .exti_cfg  = NULL,
#endif
    .M_ODR     = PIOS_HMC5x83_ODR_75, // if you change this for auxmag, change AUX_MAG_SKIP in sensors.c
    .Meas_Conf = PIOS_HMC5x83_MEASCONF_NORMAL,
    .Gain   = PIOS_HMC5x83_GAIN_1_9,
    .Mode   = PIOS_HMC5x83_MODE_CONTINUOUS,
    .TempCompensation = false,
    .Driver = &PIOS_HMC5x83_I2C_DRIVER,
    .Orientation      = PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP, // ENU for GPSV9, WND for typical I2C mag
};
#endif /* PIOS_INCLUDE_HMC5X83 */

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
    .oversampling = MS5611_OSR_4096,
};
#endif /* PIOS_INCLUDE_MS5611 */

/**
 * Configuration for the MPU9250 chip
 */
#if defined(PIOS_INCLUDE_MPU9250)
#include "pios_mpu9250.h"
#include "pios_mpu9250_config.h"
static const struct pios_exti_cfg pios_exti_mpu9250_cfg __exti_config = {
    .vector = PIOS_MPU9250_IRQHandler,
    .line   = EXTI_Line15,
    .pin    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI15_10_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line15, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_mpu9250_cfg pios_mpu9250_cfg = {
    .exti_cfg   = &pios_exti_mpu9250_cfg,
    .Fifo_store = 0,
    // Clock at 8 khz
    .Smpl_rate_div_no_dlp = 0,
    // with dlp on output rate is 1000Hz
    .Smpl_rate_div_dlp    = 0,
    .interrupt_cfg  = PIOS_MPU9250_INT_CLR_ANYRD, // | PIOS_MPU9250_INT_LATCH_EN,
    .interrupt_en   = PIOS_MPU9250_INTEN_DATA_RDY,
    .User_ctl             = PIOS_MPU9250_USERCTL_DIS_I2C | PIOS_MPU9250_USERCTL_I2C_MST_EN,
    .Pwr_mgmt_clk   = PIOS_MPU9250_PWRMGMT_PLL_Z_CLK,
    .accel_range    = PIOS_MPU9250_ACCEL_8G,
    .gyro_range     = PIOS_MPU9250_SCALE_2000_DEG,
    .filter               = PIOS_MPU9250_LOWPASS_256_HZ,
    .orientation    = PIOS_MPU9250_TOP_180DEG,
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
    .max_downsample = 26,
};
#endif /* PIOS_INCLUDE_MPU9250 */


/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, SPEKTRUM1, SPEKTRUM2, SBUS
 * NOTE: No slot in this map for NONE.
 */
uint32_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

#define PIOS_COM_TELEM_RF_RX_BUF_LEN     512
#define PIOS_COM_TELEM_RF_TX_BUF_LEN     512

#define PIOS_COM_GPS_RX_BUF_LEN          128
#define PIOS_COM_GPS_TX_BUF_LEN          32

#define PIOS_COM_TELEM_USB_RX_BUF_LEN    65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN    65

#define PIOS_COM_BRIDGE_RX_BUF_LEN       65
#define PIOS_COM_BRIDGE_TX_BUF_LEN       12

#define PIOS_COM_RFM22B_RF_RX_BUF_LEN    512
#define PIOS_COM_RFM22B_RF_TX_BUF_LEN    512

#define PIOS_COM_HKOSD_RX_BUF_LEN        22
#define PIOS_COM_HKOSD_TX_BUF_LEN        22

#define PIOS_COM_MSP_TX_BUF_LEN          128
#define PIOS_COM_MSP_RX_BUF_LEN          64

#define PIOS_COM_MAVLINK_TX_BUF_LEN      128

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uint32_t pios_com_debug_id;
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

uint32_t pios_com_gps_id       = 0;
uint32_t pios_com_telem_usb_id = 0;
uint32_t pios_com_telem_rf_id  = 0;
uint32_t pios_com_rf_id        = 0;
uint32_t pios_com_bridge_id    = 0;
uint32_t pios_com_overo_id     = 0;
uint32_t pios_com_hkosd_id     = 0;
uint32_t pios_com_msp_id       = 0;
uint32_t pios_com_vcp_id       = 0;
uint32_t pios_com_mavlink_id   = 0;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id;

/*
 * Setup a com port based on the passed cfg, driver and buffer sizes.
 * tx size of -1 make the port rx only
 * rx size of -1 make the port tx only
 * having both tx and rx size of -1 is not valid and will fail further down in PIOS_COM_Init()
 */
static void PIOS_Board_configure_com(const struct pios_usart_cfg *usart_port_cfg, size_t rx_buf_len, size_t tx_buf_len,
                                     const struct pios_com_driver *com_driver, uint32_t *pios_com_id)
{
    uint32_t pios_usart_id;

    if (PIOS_USART_Init(&pios_usart_id, usart_port_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *rx_buffer = 0, *tx_buffer = 0;

    if (rx_buf_len > 0) {
        rx_buffer = (uint8_t *)pios_malloc(rx_buf_len);
        PIOS_Assert(rx_buffer);
    }

    if (tx_buf_len > 0) {
        tx_buffer = (uint8_t *)pios_malloc(tx_buf_len);
        PIOS_Assert(tx_buffer);
    }

    if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                      rx_buffer, rx_buf_len,
                      tx_buffer, tx_buf_len)) {
        PIOS_Assert(0);
    }
}

static void PIOS_Board_configure_dsm(const struct pios_usart_cfg *pios_usart_dsm_cfg, const struct pios_dsm_cfg *pios_dsm_cfg,
                                     const struct pios_com_driver *usart_com_driver,
                                     ManualControlSettingsChannelGroupsOptions channelgroup, uint8_t *bind)
{
    uint32_t pios_usart_dsm_id;

    if (PIOS_USART_Init(&pios_usart_dsm_id, pios_usart_dsm_cfg)) {
        PIOS_Assert(0);
    }

    uint32_t pios_dsm_id;
    if (PIOS_DSM_Init(&pios_dsm_id, pios_dsm_cfg, usart_com_driver,
                      pios_usart_dsm_id, *bind)) {
        PIOS_Assert(0);
    }

    uint32_t pios_dsm_rcvr_id;
    if (PIOS_RCVR_Init(&pios_dsm_rcvr_id, &pios_dsm_rcvr_driver, pios_dsm_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[channelgroup] = pios_dsm_rcvr_id;
}

static void PIOS_Board_configure_pwm(const struct pios_pwm_cfg *pwm_cfg)
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

static void PIOS_Board_configure_ppm(const struct pios_ppm_cfg *ppm_cfg)
{
    uint32_t pios_ppm_id;

    PIOS_PPM_Init(&pios_ppm_id, ppm_cfg);

    uint32_t pios_ppm_rcvr_id;
    if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
}

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
    if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#if false

    /* Set up the SPI interface to the flash and rfm22b */
    if (PIOS_SPI_Init(&pios_spi_telem_flash_id, &pios_spi_telem_flash_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#endif
#ifdef PIOS_INCLUDE_I2C
    if (PIOS_I2C_Init(&pios_i2c_pressure_adapter_id, &pios_i2c_pressure_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#endif
#if defined(PIOS_INCLUDE_FLASH)
    /* Connect flash to the appropriate interface and configure it */

    uintptr_t flash_id = 0;

    // Initialize the external USER flash
    if (PIOS_Flash_EEPROM_Init(&flash_id, &flash_main_chip_cfg, pios_i2c_pressure_adapter_id, 0x50)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_FLASHFS_Init(&pios_uavo_settings_fs_id, &flash_main_fs_cfg, &pios_EEPROM_flash_driver, flash_id)) {
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
    HwSettingsInitialize();
#if defined(PIOS_INCLUDE_RFM22B)
    OPLinkSettingsInitialize();
    OPLinkStatusInitialize();
#endif /* PIOS_INCLUDE_RFM22B */

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
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
    PIOS_USB_Init(&pios_usb_id, PIOS_BOARD_HW_DEFS_GetUsbCfg(bdinfo->board_rev));

#if defined(PIOS_INCLUDE_USB_CDC)

    uint8_t hwsettings_usb_vcpport;
    /* Configure the USB VCP port */
    HwSettingsUSB_VCPPortGet(&hwsettings_usb_vcpport);

    if (!usb_cdc_present) {
        /* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
        hwsettings_usb_vcpport = HWSETTINGS_USB_VCPPORT_DISABLED;
    }
    uint32_t pios_usb_cdc_id;
    if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }

    uint32_t pios_usb_hid_id;
    if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }

    switch (hwsettings_usb_vcpport) {
    case HWSETTINGS_USB_VCPPORT_DISABLED:
        break;
    case HWSETTINGS_USB_VCPPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
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
    }

#endif /* PIOS_INCLUDE_USB_HID */

    if (usb_hid_present || usb_cdc_present) {
        PIOS_USBHOOK_Activate();
    }
#endif /* PIOS_INCLUDE_USB */

    /* Configure IO ports */
    uint8_t hwsettings_DSMxBind;
    HwSettingsDSMxBindGet(&hwsettings_DSMxBind);

    /* Configure main USART port */
    uint8_t hwsettings_mainport;
    HwSettingsRM_MainPortGet(&hwsettings_mainport);
    switch (hwsettings_mainport) {
    case HWSETTINGS_RM_MAINPORT_DISABLED:
        break;
    case HWSETTINGS_RM_MAINPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_usart_main_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
        break;
    case HWSETTINGS_RM_MAINPORT_GPS:
        PIOS_Board_configure_com(&pios_usart_main_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
        break;
    case HWSETTINGS_RM_MAINPORT_SBUS:
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
#endif
        break;
    case HWSETTINGS_RM_MAINPORT_DSM:
    {
        // Force binding to zero on the main port
        hwsettings_DSMxBind = 0;

        // TODO: Define the various Channelgroup for Revo dsm inputs and handle here
        PIOS_Board_configure_dsm(&pios_usart_dsm_main_cfg, &pios_dsm_main_cfg,
                                 &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hwsettings_DSMxBind);
    }
    break;
    case HWSETTINGS_RM_MAINPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            PIOS_Board_configure_com(&pios_usart_main_cfg, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_debug_id);
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
        break;
    case HWSETTINGS_RM_MAINPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_main_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    case HWSETTINGS_RM_MAINPORT_MSP:
        PIOS_Board_configure_com(&pios_usart_main_cfg, PIOS_COM_MSP_RX_BUF_LEN, PIOS_COM_MSP_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_msp_id);
        break;
    case HWSETTINGS_RM_MAINPORT_MAVLINK:
        PIOS_Board_configure_com(&pios_usart_main_cfg, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_mavlink_id);
        break;
    case HWSETTINGS_RM_MAINPORT_OSDHK:
        PIOS_Board_configure_com(&pios_usart_hkosd_main_cfg, PIOS_COM_HKOSD_RX_BUF_LEN, PIOS_COM_HKOSD_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hkosd_id);
        break;
    } /*        hwsettings_rm_mainport */

    if (hwsettings_mainport != HWSETTINGS_RM_MAINPORT_SBUS) {
        GPIO_Init(pios_sbus_cfg.inv.gpio, &pios_sbus_cfg.inv.init);
        GPIO_WriteBit(pios_sbus_cfg.inv.gpio, pios_sbus_cfg.inv.init.GPIO_Pin, pios_sbus_cfg.gpio_inv_disable);
    }

    /* Configure FlexiPort */
    uint8_t hwsettings_flexiport;
    HwSettingsRM_FlexiPortGet(&hwsettings_flexiport);
    switch (hwsettings_flexiport) {
    case HWSETTINGS_RM_FLEXIPORT_DISABLED:
        break;
    case HWSETTINGS_RM_FLEXIPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_I2C:
#if defined(PIOS_INCLUDE_I2C)
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
#if defined(PIOS_INCLUDE_HMC5X83)
        // get auxmag type
        AuxMagSettingsTypeOptions option;
        AuxMagSettingsInitialize();
        AuxMagSettingsTypeGet(&option);
        // if the aux mag type is FlexiPort then set it up
        if (option == AUXMAGSETTINGS_TYPE_FLEXI) {
            // attach the 5x83 mag to the previously inited I2C2
            external_mag = PIOS_HMC5x83_Init(&pios_hmc5x83_external_cfg, pios_i2c_flexiport_adapter_id, 0);
#ifdef PIOS_INCLUDE_WDG
            // give HMC5x83 on I2C some extra time to allow for reset, etc. if needed
            // this is not in a loop, so it is safe
            PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */
            // add this sensor to the sensor task's list
            // be careful that you don't register a slow, unimportant sensor after registering the fastest sensor
            // and before registering some other fast and important sensor
            // as that would cause delay and time jitter for the second fast sensor
            PIOS_HMC5x83_Register(external_mag, PIOS_SENSORS_TYPE_3AXIS_AUXMAG);
            // mag alarm is cleared later, so use I2C
            AlarmsSet(SYSTEMALARMS_ALARM_I2C, (external_mag) ? SYSTEMALARMS_ALARM_OK : SYSTEMALARMS_ALARM_WARNING);
        }
#endif /* PIOS_INCLUDE_HMC5X83 */
#endif /* PIOS_INCLUDE_I2C */
        break;
    case HWSETTINGS_RM_FLEXIPORT_GPS:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_gps_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_DSM:
        // TODO: Define the various Channelgroup for Revo dsm inputs and handle here
        PIOS_Board_configure_dsm(&pios_usart_dsm_flexi_cfg, &pios_dsm_flexi_cfg,
                                 &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMFLEXIPORT, &hwsettings_DSMxBind);
        break;
    case HWSETTINGS_RM_FLEXIPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            PIOS_Board_configure_com(&pios_usart_flexi_cfg, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_debug_id);
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
        break;
    case HWSETTINGS_RM_FLEXIPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_MSP:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_MSP_RX_BUF_LEN, PIOS_COM_MSP_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_msp_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_MAVLINK:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_mavlink_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_OSDHK:
        PIOS_Board_configure_com(&pios_usart_hkosd_flexi_cfg, PIOS_COM_HKOSD_RX_BUF_LEN, PIOS_COM_HKOSD_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hkosd_id);
        break;
    case HWSETTINGS_RM_FLEXIPORT_SRXL:
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
#endif
        break;

    case HWSETTINGS_RM_FLEXIPORT_HOTTSUMD:
    case HWSETTINGS_RM_FLEXIPORT_HOTTSUMH:
#if defined(PIOS_INCLUDE_HOTT)
        {
            uint32_t pios_usart_hott_id;
            if (PIOS_USART_Init(&pios_usart_hott_id, &pios_usart_hott_flexi_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_hott_id;
            if (PIOS_HOTT_Init(&pios_hott_id, &pios_usart_com_driver, pios_usart_hott_id,
                               hwsettings_flexiport == HWSETTINGS_RM_FLEXIPORT_HOTTSUMD ? PIOS_HOTT_PROTO_SUMD : PIOS_HOTT_PROTO_SUMH)) {
                PIOS_Assert(0);
            }

            uint32_t pios_hott_rcvr_id;
            if (PIOS_RCVR_Init(&pios_hott_rcvr_id, &pios_hott_rcvr_driver, pios_hott_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTT] = pios_hott_rcvr_id;
        }
#endif /* PIOS_INCLUDE_HOTT */
        break;

    case HWSETTINGS_RM_FLEXIPORT_EXBUS:
#if defined(PIOS_INCLUDE_EXBUS)
        {
            uint32_t pios_usart_exbus_id;
            if (PIOS_USART_Init(&pios_usart_exbus_id, &pios_usart_exbus_flexi_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_exbus_id;
            if (PIOS_EXBUS_Init(&pios_exbus_id, &pios_usart_com_driver, pios_usart_exbus_id)) {
                PIOS_Assert(0);
            }

            uint32_t pios_exbus_rcvr_id;
            if (PIOS_RCVR_Init(&pios_exbus_rcvr_id, &pios_exbus_rcvr_driver, pios_exbus_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_EXBUS] = pios_exbus_rcvr_id;
        }
#endif /* PIOS_INCLUDE_EXBUS */
        break;
    } /* hwsettings_rm_flexiport */


#if defined(PIOS_INCLUDE_PWM) || defined(PIOS_INCLUDE_PPM)

    const struct pios_servo_cfg *pios_servo_cfg;
    // default to servo outputs only
    pios_servo_cfg = &pios_servo_cfg_out;
#endif

    /* Configure the receiver port*/
    uint8_t hwsettings_rcvrport;
    HwSettingsRM_RcvrPortGet(&hwsettings_rcvrport);
    //
    switch (hwsettings_rcvrport) {
    case HWSETTINGS_RM_RCVRPORT_DISABLED:
        break;
    case HWSETTINGS_RM_RCVRPORT_PWM:
#if defined(PIOS_INCLUDE_PWM)
        /* Set up the receiver port.  Later this should be optional */
        PIOS_Board_configure_pwm(&pios_pwm_cfg);
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_RM_RCVRPORT_PPM:
    case HWSETTINGS_RM_RCVRPORT_PPMOUTPUTS:
    case HWSETTINGS_RM_RCVRPORT_PPMPWM:
#if defined(PIOS_INCLUDE_PPM)
        if (hwsettings_rcvrport == HWSETTINGS_RM_RCVRPORT_PPMOUTPUTS) {
            // configure servo outputs and the remaining 5 inputs as outputs
            pios_servo_cfg = &pios_servo_cfg_out_in_ppm;
        }

        PIOS_Board_configure_ppm(&pios_ppm_cfg);

        break;
#endif /* PIOS_INCLUDE_PPM */
    case HWSETTINGS_RM_RCVRPORT_OUTPUTS:
        // configure only the servo outputs
        pios_servo_cfg = &pios_servo_cfg_out_in;
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

#ifdef PIOS_INCLUDE_WS2811
#include <pios_ws2811.h>
    HwSettingsWS2811LED_OutOptions ws2811_pin_settings;
    HwSettingsWS2811LED_OutGet(&ws2811_pin_settings);

    // No other choices but servo pin 1 on nano
    if (ws2811_pin_settings != HWSETTINGS_WS2811LED_OUT_DISABLED) {
        pios_tim_servoport_all_pins[0] = dummmy_timer; // free timer 1
        PIOS_WS2811_Init(&pios_ws2811_cfg, &pios_ws2811_pin_cfg[0]);
    }
#endif // PIOS_INCLUDE_WS2811


#ifndef PIOS_ENABLE_DEBUG_PINS
    // pios_servo_cfg points to the correct configuration based on input port settings
    PIOS_Servo_Init(pios_servo_cfg);
#else
    PIOS_DEBUG_Init(pios_tim_servoport_all_pins, NELEMENTS(pios_tim_servoport_all_pins));
#endif

    PIOS_DELAY_WaitmS(50);

#if defined(PIOS_INCLUDE_ADC)
    PIOS_ADC_Init(&pios_adc_cfg);
#endif

#if defined(PIOS_INCLUDE_MPU9250)
    PIOS_MPU9250_Init(pios_spi_gyro_id, 0, &pios_mpu9250_cfg);
    PIOS_MPU9250_CONFIG_Configure();
    PIOS_MPU9250_MainRegister();
    PIOS_MPU9250_MagRegister();
#endif

#if defined(PIOS_INCLUDE_MS5611)
    PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_pressure_adapter_id);
    PIOS_MS5611_Register();
#endif

#ifdef PIOS_INCLUDE_ADC
    uint8_t adc_config[HWSETTINGS_ADCROUTING_NUMELEM];
    HwSettingsADCRoutingArrayGet(adc_config);
    for (uint32_t i = 0; i < HWSETTINGS_ADCROUTING_NUMELEM; i++) {
        if (adc_config[i] != HWSETTINGS_ADCROUTING_DISABLED) {
            PIOS_ADC_PinSetup(i);
        }
    }
#endif

    // Attach the board config check hook
    SANITYCHECK_AttachHook(&RevoNanoConfigHook);
    // trigger a config check if actuatorsettings are updated
    ActuatorSettingsInitialize();
    ActuatorSettingsConnectCallback(ActuatorSettingsUpdatedCb);
}

SystemAlarmsExtendedAlarmStatusOptions RevoNanoConfigHook()
{
    // inhibit usage of oneshot for non supported RECEIVER port modes
    uint8_t recmode;

    HwSettingsRM_RcvrPortGet(&recmode);
    uint8_t modes[ACTUATORSETTINGS_BANKMODE_NUMELEM];
    ActuatorSettingsBankModeGet(modes);

    switch ((HwSettingsRM_RcvrPortOptions)recmode) {
    // Those modes allows oneshot usage
    case HWSETTINGS_RM_RCVRPORT_DISABLED:
    case HWSETTINGS_RM_RCVRPORT_PPM:
    case HWSETTINGS_RM_RCVRPORT_PPMOUTPUTS:
    case HWSETTINGS_RM_RCVRPORT_OUTPUTS:
        return SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE;

    // inhibit oneshot for the following modes
    case HWSETTINGS_RM_RCVRPORT_PWM:
        for (uint8_t i = 0; i < ACTUATORSETTINGS_BANKMODE_NUMELEM; i++) {
            if (modes[i] == ACTUATORSETTINGS_BANKMODE_PWMSYNC ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_ONESHOT125 ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_ONESHOT42 ||
                modes[i] == ACTUATORSETTINGS_BANKMODE_MULTISHOT) {
                return SYSTEMALARMS_EXTENDEDALARMSTATUS_UNSUPPORTEDCONFIG_ONESHOT;;
            }
        }

        return SYSTEMALARMS_EXTENDEDALARMSTATUS_NONE;

    default:
        break;
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
 * @}
 */
