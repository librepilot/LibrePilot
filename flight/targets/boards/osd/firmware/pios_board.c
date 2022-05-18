/**
 ******************************************************************************
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the OPOSD board.
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
#include <gcsreceiver.h>
#include <taskinfo.h>

#include <pios_board_io.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

/* Private macro -------------------------------------------------------------*/
#define countof(a) (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/

#if defined(PIOS_INCLUDE_ADC)
#include <pios_adc_priv.h>
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


static void Clock(uint32_t spektrum_id);

#define PIOS_COM_AUX_RX_BUF_LEN 512
#define PIOS_COM_AUX_TX_BUF_LEN 512

uint32_t pios_com_aux_id;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id = 0;

/**
 * TIM3 is triggered by the HSYNC signal into its ETR line and will divide the
 *  APB1_CLOCK to generate a pixel clock that is used by the SPI CLK lines.
 * TIM4 will be synced to it and will divide by that times the pixel width to
 *  fire an IRQ when the last pixel of the line has been output.  Then the timer will
 *  be rearmed and wait for the next HSYNC signal.
 * The critical timing detail is that the task be _DISABLED_ at the end of the line
 *  before an extra pixel is clocked out
 *  or we will need to configure the DMA task per line
 */
#include "pios_tim_priv.h"
#define NTSC_PX_CLOCK 6797088
#define PAL_PX_CLOCK  6750130
#define PX_PERIOD     ((PIOS_PERIPHERAL_APB1_CLOCK / NTSC_PX_CLOCK) + 1)
#define LINE_PERIOD   PX_PERIOD * GRAPHICS_WIDTH

static const TIM_TimeBaseInitTypeDef tim_4_time_base = {
    .TIM_Prescaler         = 0, // PIOS_PERIPHERAL_APB1_CLOCK,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = LINE_PERIOD - 1,
    .TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg pios_tim4_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_4_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    }
};

void PIOS_Board_Init(void)
{
    // Delay system
    PIOS_DELAY_Init();

    PIOS_LED_Init(&pios_led_cfg);

#if defined(PIOS_INCLUDE_SPI)
    /* Set up the SPI interface to the SD card */
    if (PIOS_SPI_Init(&pios_spi_sdcard_id, &pios_spi_sdcard_cfg)) {
        PIOS_Assert(0);
    }

#if defined(PIOS_INCLUDE_SDCARD)
    /* Enable and mount the SDCard */
    PIOS_SDCARD_Init(pios_spi_sdcard_id);
    PIOS_SDCARD_MountFS(0);
#endif
#endif /* PIOS_INCLUDE_SPI */

#ifdef PIOS_INCLUDE_FLASH_LOGFS_SETTINGS
    uintptr_t flash_id;
    PIOS_Flash_Internal_Init(&flash_id, &flash_internal_cfg);
    PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id);
#elif !defined(PIOS_USE_SETTINGS_ON_SDCARD)
#error No setting storage specified. (define PIOS_USE_SETTINGS_ON_SDCARD or INCLUDE_FLASH_SECTOR_SETTINGS)
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

#ifdef PIOS_INCLUDE_WDG
    /* Initialize watchdog as early as possible to catch faults during init */
    PIOS_WDG_Init();
#endif /* PIOS_INCLUDE_WDG */

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* IAP System Setup */
    PIOS_IAP_Init();
    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }


#if defined(PIOS_INCLUDE_RTC)
    /* Initialize the real-time clock and its associated tick */
    PIOS_RTC_Init(&pios_rtc_main_cfg);
    if (!PIOS_RTC_RegisterTickCallback(Clock, 0)) {
        PIOS_DEBUG_Assert(0);
    }
#endif

#if defined(PIOS_INCLUDE_USB)
    PIOS_BOARD_IO_Configure_USB();
#endif

#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_GPS)

    uint32_t pios_usart_gps_id;
    if (PIOS_USART_Init(&pios_usart_gps_id, &pios_usart_gps_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *gps_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_GPS_RX_BUF_LEN);
    PIOS_Assert(gps_rx_buffer);
    if (PIOS_COM_Init(&pios_com_gps_id, &pios_usart_com_driver, pios_usart_gps_id,
                      gps_rx_buffer, PIOS_COM_GPS_RX_BUF_LEN,
                      NULL, 0)) {
        PIOS_Assert(0);
    }

#endif /* PIOS_INCLUDE_GPS */

#if defined(PIOS_INCLUDE_COM_AUX)
    {
        uint32_t pios_usart_aux_id;

        if (PIOS_USART_Init(&pios_usart_aux_id, &pios_usart_aux_cfg)) {
            PIOS_DEBUG_Assert(0);
        }

        uint8_t *aux_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_AUX_RX_BUF_LEN);
        uint8_t *aux_tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_AUX_TX_BUF_LEN);
        PIOS_Assert(aux_rx_buffer);
        PIOS_Assert(aux_tx_buffer);

        if (PIOS_COM_Init(&pios_com_aux_id, &pios_usart_com_driver, pios_usart_aux_id,
                          aux_rx_buffer, PIOS_COM_AUX_RX_BUF_LEN,
                          aux_tx_buffer, PIOS_COM_AUX_TX_BUF_LEN)) {
            PIOS_DEBUG_Assert(0);
        }
    }
#else
    pios_com_aux_id = 0;
#endif /* PIOS_INCLUDE_COM_AUX */

#if defined(PIOS_INCLUDE_COM_TELEM)
    { /* Eventually add switch for this port function */
        uint32_t pios_usart_telem_rf_id;
        if (PIOS_USART_Init(&pios_usart_telem_rf_id, &pios_usart_telem_main_cfg)) {
            PIOS_Assert(0);
        }

        uint8_t *telem_rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
        uint8_t *telem_tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
        PIOS_Assert(telem_rx_buffer);
        PIOS_Assert(telem_tx_buffer);
        if (PIOS_COM_Init(&pios_com_telem_rf_id, &pios_usart_com_driver, pios_usart_telem_rf_id,
                          telem_rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
                          telem_tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }
    }
#else
    pios_com_telem_rf_id = 0;
#endif /* PIOS_INCLUDE_COM_TELEM */

#endif /* PIOS_INCLUDE_COM */


    /* Configure FlexiPort */

/*	uint8_t hwsettings_rv_flexiport;
        HwSettingsRV_FlexiPortGet(&hwsettings_rv_flexiport);

        switch (hwsettings_rv_flexiport) {
                case HWSETTINGS_RV_FLEXIPORT_DISABLED:
                        break;
                case HWSETTINGS_RV_FLEXIPORT_I2C:*/
#if defined(PIOS_INCLUDE_I2C)
    {
        if (PIOS_I2C_Init(&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg)) {
            PIOS_Assert(0);
        }
    }
#endif /* PIOS_INCLUDE_I2C */
/*			break;

                case HWSETTINGS_RV_FLEXIPORT_DSM:
                        //TODO: Define the various Channelgroup for Revo dsm inputs and handle here
                        PIOS_Board_configure_dsm(&pios_usart_dsm_flexi_cfg, &pios_dsm_flexi_cfg,
                                                                                         &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT,&hwsettings_DSMxBind);
                        break;
                case HWSETTINGS_RV_FLEXIPORT_COMAUX:
                        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
                        break;
                case HWSETTINGS_RV_FLEXIPORT_COMBRIDGE:
                        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
                        break;
        }*/
    /* hwsettings_rv_flexiport */


#if defined(PIOS_INCLUDE_WAVE)
    PIOS_WavPlay_Init(&pios_dac_cfg);
#endif

    // ADC system
#if defined(PIOS_INCLUDE_ADC)
    PIOS_ADC_Init(&pios_adc_cfg);
#endif

#if defined(PIOS_INCLUDE_VIDEO)
    PIOS_TIM_InitClock(&tim_8_cfg);
    PIOS_Servo_Init(&pios_servo_cfg);
    // Start the pixel and line clock counter
    // PIOS_TIM_InitClock(&pios_tim4_cfg);
    PIOS_Video_Init(&pios_video_cfg);
#endif
#ifdef PIOS_INCLUDE_ADC
    {
        uint8_t adc_config[HWSETTINGS_ADCROUTING_NUMELEM];
        HwSettingsADCRoutingArrayGet(adc_config);
        for (uint32_t i = 0; i < HWSETTINGS_ADCROUTING_NUMELEM; i++) {
            if (adc_config[i] != HWSETTINGS_ADCROUTING_DISABLED) {
                PIOS_ADC_PinSetup(i);
            }
        }
    }
#endif // PIOS_INCLUDE_ADC
}

uint16_t supv_timer = 0;

static void Clock(__attribute__((unused)) uint32_t spektrum_id)
{
    /* 125hz */
    ++supv_timer;
    if (supv_timer >= 625) {
        supv_timer = 0;
        timex.sec++;
    }
    if (timex.sec >= 60) {
        timex.sec = 0;
        timex.min++;
    }
    if (timex.min >= 60) {
        timex.min = 0;
        timex.hour++;
    }
    if (timex.hour >= 99) {
        timex.hour = 0;
    }
}
