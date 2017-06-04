/**
 ******************************************************************************
 * @file       board_hw_defs.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 *
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

#if defined(PIOS_INCLUDE_LED)

#include <pios_led_priv.h>
static const struct pios_gpio pios_leds[] = {
    [PIOS_LED_HEARTBEAT] = {
        .pin                =             {
            .gpio = GPIOC,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_14,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL,
            },
        },
        .active_low         = false
    },
    [PIOS_LED_ALARM] =     { /* or PC15 ?? */
        .pin                =             {
            .gpio = GPIOA,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_13,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL,
            },
        },
        .active_low         = false
    },
    [PIOS_BUZZER_ALARM] =  { /* (or PB2 ??) */
        .pin                =             {
            .gpio = GPIOC,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL,
            },
        },
        .active_low         = false
    },
};

static const struct pios_gpio_cfg pios_led_cfg = {
    .gpios     = pios_leds,
    .num_gpios = NELEMENTS(pios_leds),
};

const struct pios_gpio_cfg *PIOS_BOARD_HW_DEFS_GetLedCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_led_cfg;
}

#endif /* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_SPI)

#include <pios_spi_priv.h>

/* SPI Flash interface */

static const struct pios_spi_cfg pios_spi2_cfg = {
    .regs = SPI2,
    .init = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS               = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16,       /* 10 Mhz */
    },
    .use_crc = false,
    .remap   = GPIO_AF_5,
    .sclk    = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .miso                      = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .mosi                      = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .slave_count               = 1,
    .ssel                      = {
        {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_12,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};

uint32_t pios_spi2_id;

static const struct pios_spi_cfg pios_spi1_cfg = {
    .regs = SPI1,
    .init = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS               = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16,       /* 10 Mhz */
    },
    .use_crc = false,
    .remap   = GPIO_AF_5,
    .sclk    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .miso                      = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .mosi                      = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .slave_count               = 1,
    .ssel                      = {
        { /* MPU6000 */
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_4,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};

uint32_t pios_spi1_id;

#endif /* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"
#include "pios_flash_jedec_priv.h"
#include "pios_flash_internal_priv.h"

static const struct pios_flash_internal_cfg flash_internal_system_cfg = {};

static const struct flashfs_logfs_cfg flashfs_internal_cfg = {
    .fs_magic      = 0x99abcfef,
    .total_fs_size = EE_BANK_SIZE, /* 32K bytes (16x2KB sectors) */
    .arena_size    = 0x00004000, /* 64 * slot size = 16K bytes = 8 sectors */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = EE_BANK_BASE, /* start after the bootloader */
    .sector_size   = 0x00000800, /* 2K bytes */
    .page_size     = 0x00000800, /* 2K bytes */
};

#endif /* PIOS_INCLUDE_FLASH */

#include "pios_tim_priv.h"

static const TIM_TimeBaseInitTypeDef tim_time_base = {
    .TIM_Prescaler         = (PIOS_MASTER_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
    .timer = TIM1,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_CC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_2_cfg = {
    .timer = TIM2,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM2_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_3_cfg = {
    .timer = TIM3,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM3_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_4_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
    .timer = TIM8,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM8_CC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_15_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_BRK_TIM15_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_16_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_UP_TIM16_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_17_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_TRG_COM_TIM17_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

#include <pios_servo_config.h>

#define GPIO_AF_PA1_TIM2       GPIO_AF_1
#define GPIO_AF_PA0_TIM2       GPIO_AF_1
#define GPIO_AF_PA8_TIM1       GPIO_AF_6
#define GPIO_AF_PA2_TIM2       GPIO_AF_1
#define GPIO_AF_PB6_TIM4       GPIO_AF_2
#define GPIO_AF_PB5_TIM3       GPIO_AF_2
#define GPIO_AF_PB0_TIM3       GPIO_AF_2
#define GPIO_AF_PB1_TIM3       GPIO_AF_2
#define GPIO_AF_PB9_TIM4       GPIO_AF_2
#define GPIO_AF_PB8_TIM4       GPIO_AF_2
#define GPIO_AF_PB7_TIM4       GPIO_AF_2
#define GPIO_AF_PB4_TIM3       GPIO_AF_2
#define GPIO_AF_PB11_TIM2      GPIO_AF_1
#define GPIO_AF_PA15_TIM8      GPIO_AF_2
#define GPIO_AF_PA3_TIM15      GPIO_AF_9
#define GPIO_AF_PA6_TIM3       GPIO_AF_2
#define GPIO_AF_PA7_TIM3       GPIO_AF_2
#define GPIO_AF_PA10_TIM2      GPIO_AF_10

#define GPIO_AF_PA4_TIM3       GPIO_AF_2
#define GPIO_AF_PA7_TIM17      GPIO_AF_1
#define GPIO_AF_PB8_TIM16      GPIO_AF_1

#define GPIO_AF_PB9_TIM8       GPIO_AF_10
#define GPIO_AF_PA2_TIM15      GPIO_AF_9
#define GPIO_AF_PB10_TIM2      GPIO_AF_1

#define TIM1_CH1_DMA_INSTANCE  1
#define TIM1_CH1_DMA_CHANNEL   2

#define TIM16_CH1_DMA_INSTANCE 2
#define TIM16_CH1_DMA_CHANNEL  3

static const struct pios_tim_channel pios_tim_servoport_pins[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM4,  3, B, 8), // bank 1
    TIM_SERVO_CHANNEL_CONFIG(TIM8,  3, B, 9), // bank 2
    TIM_SERVO_CHANNEL_CONFIG(TIM15, 2, A, 3), // bank 3
    TIM_SERVO_CHANNEL_CONFIG(TIM15, 1, A, 2), // bank 3

    // following two are here so we can have two additional PWM outputs instead of UART3
    TIM_SERVO_CHANNEL_CONFIG(TIM2,  4, B, 11), // bank 4 (UART3 RX)
    TIM_SERVO_CHANNEL_CONFIG(TIM2,  3, B, 10), // bank 4 (UART3 TX)
};

#if defined(PIOS_INCLUDE_USART)

#define GPIO_AF_USART1 GPIO_AF_7
#define GPIO_AF_USART2 GPIO_AF_7
#define GPIO_AF_USART3 GPIO_AF_7


#include "pios_usart_priv.h"
#include "pios_usart_config.h"

static const struct pios_usart_cfg pios_usart_cfg[] = {
    USART_CONFIG(USART1, B, 7,  B, 6), /* RX_DEBUG, TX_??? */
    USART_CONFIG(USART2, A, 15, A, 14), /* RX_SBUS, TX_FRSKY_TELEMETRY */
    USART_CONFIG(USART3, B, 11, B, 10),
};


#endif /* PIOS_INCLUDE_USART */

#if defined(PIOS_INCLUDE_COM)

#include "pios_com_priv.h"

#endif /* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler(void);
void RTC_WKUP_IRQHandler() __attribute__((alias("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
    .clksrc    = RCC_RTCCLKSource_HSE_Div32,
    .prescaler = 25 - 1, // 8Mhz / 32 / 16 / 25 => 625Hz
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = RTC_WKUP_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

void PIOS_RTC_IRQ_Handler(void)
{
    PIOS_RTC_irq_handler();
}

#endif /* if defined(PIOS_INCLUDE_RTC) */

#if defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM)
/*
 * Servo outputs
 */
#include <pios_servo_priv.h>

const struct pios_servo_cfg pios_servo_cfg = {
    .tim_oc_init          = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_Pulse        = PIOS_SERVOS_INITIAL_POSITION,
        .TIM_OCPolarity   = TIM_OCPolarity_High,
        .TIM_OCNPolarity  = TIM_OCPolarity_High,
        .TIM_OCIdleState  = TIM_OCIdleState_Reset,
        .TIM_OCNIdleState = TIM_OCNIdleState_Reset,
    },
    .channels     = pios_tim_servoport_pins,
    .num_channels = NELEMENTS(pios_tim_servoport_pins) - 2,
};

const struct pios_servo_cfg pios_servo_uart3_cfg = {
    .tim_oc_init          = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_Pulse        = PIOS_SERVOS_INITIAL_POSITION,
        .TIM_OCPolarity   = TIM_OCPolarity_High,
        .TIM_OCNPolarity  = TIM_OCPolarity_High,
        .TIM_OCIdleState  = TIM_OCIdleState_Reset,
        .TIM_OCNIdleState = TIM_OCNIdleState_Reset,
    },
    .channels     = pios_tim_servoport_pins,
    .num_channels = NELEMENTS(pios_tim_servoport_pins),
};


#endif /* PIOS_INCLUDE_SERVO && PIOS_INCLUDE_TIM */

/*
 * PPM Inputs
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>

static const struct pios_tim_channel pios_tim_ppm = TIM_SERVO_CHANNEL_CONFIG(TIM2, 4, B, 11);

const struct pios_ppm_cfg pios_ppm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    .channels     = &pios_tim_ppm,
    .num_channels = 1,
};
#endif /* PIOS_INCLUDE_PPM */


#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"

#endif /* PIOS_INCLUDE_RCVR */

#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_cfg = {
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = USB_LP_CAN1_RX0_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

const struct pios_usb_cfg *PIOS_BOARD_HW_DEFS_GetUsbCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_usb_main_cfg;
}
#endif /* PIOS_INCLUDE_USB */

#if defined(PIOS_INCLUDE_WS2811)
#include "pios_ws2811_cfg.h"

static const struct pios_ws2811_cfg pios_ws2811_cfg = PIOS_WS2811_CONFIG(TIM1, 1, A, 8);

void DMA1_Channel2_IRQHandler()
{
    PIOS_WS2811_DMA_irq_handler();
}

#endif /* PIOS_INCLUDE_WS2811 */

#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
void PIOS_ADC_DMC_irq_handler(void);
void DMA2_Channel5_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMC_irq_handler")));

struct pios_adc_cfg pios_adc_cfg = {
    .adc_dev = ADC3,
    .dma     = {
        .irq                                       = {
            .flags = (DMA2_FLAG_TC5 | DMA2_FLAG_TE5 | DMA2_FLAG_HT5),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Channel5_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel                               = DMA2_Channel5,
        }
    },
    .half_flag = DMA2_FLAG_HT5,
    .full_flag = DMA2_FLAG_TC5,
};

void PIOS_ADC_DMC_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_ADC_DMA_Handler();
}
const struct pios_adc_cfg *PIOS_BOARD_HW_DEFS_GetAdcCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_adc_cfg;
}
#endif /* if defined(PIOS_INCLUDE_ADC) */

/**
 * Configuration for MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU6000)
#include "pios_mpu6000.h"
static const struct pios_exti_cfg pios_exti_mpu6000_cfg __exti_config = {
    .vector = PIOS_MPU6000_IRQHandler,
    .line   = EXTI_Line13,
    .pin    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_10MHz,
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
            .EXTI_Line    = EXTI_Line13, // matches above GPIO pin
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
    .orientation    = PIOS_MPU6000_BOTTOM_90DEG,
#ifdef PIOS_INCLUDE_SPI
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
#endif /* PIOS_INCLUDE_SPI */
    .max_downsample = 2
};

const struct pios_mpu6000_cfg *PIOS_BOARD_HW_DEFS_GetMPU6000Cfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_mpu6000_cfg;
}
#endif /* PIOS_INCLUDE_MPU6000 */
