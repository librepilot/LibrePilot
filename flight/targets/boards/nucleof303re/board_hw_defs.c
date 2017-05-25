/**
 ******************************************************************************
 * @file       board_hw_defs.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 *             PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 *
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the NucleoF303RE board.
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
            .gpio = GPIOA,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_5,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL,
            },
        },
        .active_low         = true
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

/* Gyro interface */

void PIOS_SPI_MPU9250_irq_handler(void);
void DMA1_Channel2_IRQHandler() __attribute__((alias("PIOS_SPI_MPU9250_irq_handler")));
void DMA1_Channel3_IRQHandler() __attribute__((alias("PIOS_SPI_MPU9250_irq_handler")));

static const struct pios_spi_cfg pios_spi_mpu9250_cfg = {
    .regs = SPI1,
    .init = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS                                   = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16, /* 10 Mhz */
    },
    .use_crc = false,
    .dma     = {
        .ahb_clk = RCC_AHBPeriph_DMA1,

        .irq     = {
            .flags = (DMA1_FLAG_TC2 | DMA1_FLAG_TE2 | DMA1_FLAG_HT2 | DMA1_FLAG_GL2),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Channel2_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA1_Channel2,
            .init    = {
                .DMA_PeripheralBaseAddr            = (uint32_t)&(SPI1->DR),
                .DMA_DIR                           = DMA_DIR_PeripheralSRC,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode     = DMA_Mode_Normal,
                .DMA_Priority = DMA_Priority_Medium,
                .DMA_M2M                           = DMA_M2M_Disable,
            },
        },
        .tx                                        = {
            .channel = DMA1_Channel3,
            .init    = {
                .DMA_PeripheralBaseAddr            = (uint32_t)&(SPI1->DR),
                .DMA_DIR                           = DMA_DIR_PeripheralDST,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode     = DMA_Mode_Normal,
                .DMA_Priority = DMA_Priority_Medium,
                .DMA_M2M                           = DMA_M2M_Disable,
            },
        },
    },
    .remap = GPIO_AF_5,
    .sclk  = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_3,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_4,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .slave_count                                   = 1,
    .ssel                                          = {
        {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_9,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};

static uint32_t pios_spi_mpu9250_id;
void PIOS_SPI_MPU9250_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_mpu9250_id);
}


/* SDCARD Interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
void PIOS_SPI_SDCARD_irq_handler(void);
void DMA1_Channel4_IRQHandler() __attribute__((alias("PIOS_SPI_SDCARD_irq_handler")));
void DMA1_Channel5_IRQHandler() __attribute__((alias("PIOS_SPI_SDCARD_irq_handler")));
static const struct pios_spi_cfg pios_spi_sdcard_cfg = {
    .regs = SPI2,
    .init = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS                                   = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8,
    },
    .use_crc = false,
    .dma     = {
        .ahb_clk = RCC_AHBPeriph_DMA1,

        .irq     = {
            .flags = (DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Channel4_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA1_Channel4,
            .init    = {
                .DMA_PeripheralBaseAddr            = (uint32_t)&(SPI2->DR),
                .DMA_DIR                           = DMA_DIR_PeripheralSRC,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode     = DMA_Mode_Normal,
                .DMA_Priority = DMA_Priority_High,
                .DMA_M2M                           = DMA_M2M_Disable,
            },
        },
        .tx                                        = {
            .channel = DMA1_Channel5,
            .init    = {
                .DMA_PeripheralBaseAddr            = (uint32_t)&(SPI2->DR),
                .DMA_DIR                           = DMA_DIR_PeripheralDST,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode     = DMA_Mode_Normal,
                .DMA_Priority = DMA_Priority_High,
                .DMA_M2M                           = DMA_M2M_Disable,
            },
        },
    },
    .remap = GPIO_AF_5,
    .sclk  = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF, /* NOTE: was GPIO_Mode_IN_FLOATING */
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .slave_count                                   = 1,
    .ssel                                          = {
        {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_12,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP,
            }
        },
    },
};

static uint32_t pios_spi_sdcard_id;
void PIOS_SPI_SDCARD_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_sdcard_id);
}

#endif /* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"
#include "pios_flash_jedec_priv.h"
#include "pios_flash_internal_priv.h"

static const struct pios_flash_internal_cfg flash_internal_system_cfg = {};

static const struct flashfs_logfs_cfg flashfs_internal_cfg = {
    .fs_magic      = 0x99abcfef,
    .total_fs_size = EE_BANK_SIZE, /* 32K bytes (2x16KB sectors) */
    .arena_size    = 0x00004000, /* 64 * slot size = 16K bytes = 1 sector */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = EE_BANK_BASE, /* start after the bootloader */
    .sector_size   = 0x00004000, /* 16K bytes */
    .page_size     = 0x00004000, /* 16K bytes */
};

// static const struct flashfs_logfs_cfg flashfs_internal_user_cfg = {
// .fs_magic      = 0x99abcfef,
// .total_fs_size = USER_EE_BANK_SIZE, /* 128K bytes (2x16KB sectors) */
// .arena_size    = 0x00020000, /* 64 * slot size = 16K bytes = 1 sector */
// .slot_size     = 0x00000100, /* 256 bytes */
//
// .start_offset  = USER_EE_BANK_BASE, /* start after the bootloader */
// .sector_size   = 0x00020000, /* 128K bytes */
// .page_size     = 0x00020000, /* 128K bytes */
// };
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

#define GPIO_AF_PA1_TIM2  GPIO_AF_1
#define GPIO_AF_PA0_TIM2  GPIO_AF_1
#define GPIO_AF_PA8_TIM1  GPIO_AF_6
#define GPIO_AF_PA2_TIM2  GPIO_AF_1
#define GPIO_AF_PB6_TIM4  GPIO_AF_2
#define GPIO_AF_PB5_TIM3  GPIO_AF_2
#define GPIO_AF_PB0_TIM3  GPIO_AF_2
#define GPIO_AF_PB1_TIM3  GPIO_AF_2
#define GPIO_AF_PB9_TIM4  GPIO_AF_2
#define GPIO_AF_PB8_TIM4  GPIO_AF_2
#define GPIO_AF_PB7_TIM4  GPIO_AF_2
#define GPIO_AF_PB4_TIM3  GPIO_AF_2
#define GPIO_AF_PB11_TIM2 GPIO_AF_1
#define GPIO_AF_PA15_TIM8 GPIO_AF_2
#define GPIO_AF_PA3_TIM15 GPIO_AF_9
#define GPIO_AF_PA6_TIM3  GPIO_AF_2
#define GPIO_AF_PA7_TIM3  GPIO_AF_2

static const struct pios_tim_channel pios_tim_servoport_pins[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM2,  1, A, 0), // bank 1
    TIM_SERVO_CHANNEL_CONFIG(TIM2,  2, A, 1), // bank 1
    TIM_SERVO_CHANNEL_CONFIG(TIM2,  3, A, 2), // bank 1
    TIM_SERVO_CHANNEL_CONFIG(TIM15, 2, A, 3), // bank 2
    TIM_SERVO_CHANNEL_CONFIG(TIM3,  1, A, 6), // bank 3
    TIM_SERVO_CHANNEL_CONFIG(TIM3,  2, A, 7), // bank 3
    TIM_SERVO_CHANNEL_CONFIG(TIM3,  3, B, 0), // bank 3
    TIM_SERVO_CHANNEL_CONFIG(TIM3,  4, B, 1), // bank 3
};

static const struct pios_tim_channel pios_tim_ppm_flexi_port = TIM_SERVO_CHANNEL_CONFIG(TIM2, 4, B, 11);


#if defined(PIOS_INCLUDE_USART)

#define GPIO_AF_USART1 GPIO_AF_7
#define GPIO_AF_USART2 GPIO_AF_7
#define GPIO_AF_USART3 GPIO_AF_7


#include "pios_usart_priv.h"
#include "pios_usart_config.h"

static const struct pios_usart_cfg pios_usart_cfg[] = {
    USART_CONFIG(USART1, A, 10, A, 9),
    USART_CONFIG(USART2, A, 15, A, 14),
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
    .num_channels = NELEMENTS(pios_tim_servoport_pins),
};

#endif /* PIOS_INCLUDE_SERVO && PIOS_INCLUDE_TIM */

/*
 * PPM Inputs
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>

static const struct pios_tim_channel pios_tim_ppm_channel = TIM_SERVO_CHANNEL_CONFIG(TIM8, 1, A, 15);

const struct pios_ppm_cfg pios_ppm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    /* Use only the first channel for ppm */
    .channels     = &pios_tim_ppm_channel,
    .num_channels = 1,
};
#endif /* PIOS_INCLUDE_PPM */

/*
 * SONAR Inputs
 */
#if defined(PIOS_INCLUDE_HCSR04)
#include <pios_hcsr04_priv.h>

static const struct pios_tim_channel pios_tim_hcsr04_port_all_channels[] = {
    {
        .timer = TIM3,
        .timer_chan = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_5,
                .GPIO_Mode  = GPIO_Mode_IPD,
                .GPIO_Speed = GPIO_Speed_2MHz,
            },
        },
        .remap = GPIO_PartialRemap_TIM3,
    },
};

const struct pios_hcsr04_cfg pios_hcsr04_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    .channels     = pios_tim_hcsr04_port_all_channels,
    .num_channels = NELEMENTS(pios_tim_hcsr04_port_all_channels),
    .trigger             = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Mode  = GPIO_Mode_Out_PP,
            .GPIO_Speed = GPIO_Speed_2MHz,
        },
    },
};
#endif /* if defined(PIOS_INCLUDE_HCSR04) */

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

/*
 * I2C Adapters
 */

void PIOS_I2C_ev_irq_handler(void);
void PIOS_I2C_er_irq_handler(void);
void I2C1_EV_EXTI23_IRQHandler() __attribute__((alias("PIOS_I2C_ev_irq_handler")));
void I2C1_ER_IRQHandler() __attribute__((alias("PIOS_I2C_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_cfg = {
    .regs     = I2C1,
    .remapSCL = GPIO_AF_4,
    .remapSDA = GPIO_AF_4,
    .init     = {
        .I2C_Mode = I2C_Mode_I2C,
        .I2C_OwnAddress1                       = 0,
        .I2C_Ack  = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress               = I2C_AcknowledgedAddress_7bit,
        .I2C_DigitalFilter = 0x00,
        .I2C_AnalogFilter  = I2C_AnalogFilter_Enable,
// .I2C_Timing                            = 0x70310309,                  // 50kHz I2C @ 8MHz input -> PRESC=0x7, SCLDEL=0x3, SDADEL=0x1, SCLH=0x03, SCLL=0x09
        .I2C_Timing                            = 0x00E0257A,                  // 400 Khz, 72Mhz Clock, Analog Filter Delay ON, Rise 100, Fall 10.
    },
    .transfer_timeout_ms                       = 50,
    .scl                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP,
        },
        .pin_source                            = GPIO_PinSource6,
    },
    .sda                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP,
        },
        .pin_source                            = GPIO_PinSource7,
    },
    .event                                     = {
        .flags = 0,               /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,               /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

uint32_t pios_i2c_id;
void PIOS_I2C_ev_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_EV_IRQ_Handler(pios_i2c_id);
}

void PIOS_I2C_er_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_ER_IRQ_Handler(pios_i2c_id);
}


#endif /* PIOS_INCLUDE_I2C */


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

#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
void PIOS_ADC_DMC_irq_handler(void);
void DMA2_Channel1_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMC_irq_handler")));

struct pios_adc_cfg pios_adc_cfg = {
    .adc_dev = ADC2,
    .dma     = {
        .irq                                       = {
            .flags = (DMA2_FLAG_TC1 | DMA2_FLAG_TE1 | DMA2_FLAG_HT1),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Channel1_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel                               = DMA2_Channel1,
        }
    },
    .half_flag = DMA2_FLAG_HT1,
    .full_flag = DMA2_FLAG_TC1,
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

const struct pios_hmc5x83_cfg *PIOS_BOARD_HW_DEFS_GetExternalHMC5x83Cfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_hmc5x83_external_cfg;
}
#endif /* PIOS_INCLUDE_HMC5X83 */

#ifdef PIOS_INCLUDE_BMP280
#include "pios_bmp280.h"
static const struct pios_bmp280_cfg bmp280_config = {
    .oversampling = BMP280_ULTRA_HIGH_RESOLUTION
};
#endif

/**
 * Configuration for the MPU9250 chip
 */
#if defined(PIOS_INCLUDE_MPU9250)
#include "pios_mpu9250.h"
#include "pios_mpu9250_config.h"
static const struct pios_exti_cfg pios_exti_mpu9250_cfg __exti_config = {
    .vector = PIOS_MPU9250_IRQHandler,
    .line   = EXTI_Line13,
    .pin    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
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
    .orientation    = PIOS_MPU9250_TOP_270DEG,
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
    .max_downsample = 26,
};
#endif /* PIOS_INCLUDE_MPU9250 */
