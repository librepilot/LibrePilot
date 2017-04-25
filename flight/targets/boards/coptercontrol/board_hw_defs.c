/**
 ******************************************************************************
 * @file       board_hw_defs.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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

#define BOARD_REVISION_CC   1
#define BOARD_REVISION_CC3D 2

#if defined(PIOS_INCLUDE_LED)

#include <pios_led_priv.h>
static const struct pios_gpio pios_leds_cc[] = {
    [PIOS_LED_HEARTBEAT] = {
        .pin                =             {
            .gpio = GPIOA,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_6,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
            },
        },
        .active_low         = true
    },
};

static const struct pios_gpio_cfg pios_led_cfg_cc = {
    .gpios     = pios_leds_cc,
    .num_gpios = NELEMENTS(pios_leds_cc),
};

static const struct pios_gpio pios_leds_cc3d[] = {
    [PIOS_LED_HEARTBEAT] = {
        .pin                =             {
            .gpio = GPIOB,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_3,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
                .GPIO_Speed = GPIO_Speed_50MHz,
            },
        },
        .remap              = GPIO_Remap_SWJ_JTAGDisable,
        .active_low         = true
    },
};

static const struct pios_gpio_cfg pios_led_cfg_cc3d = {
    .gpios     = pios_leds_cc3d,
    .num_gpios = NELEMENTS(pios_leds_cc3d),
};

const struct pios_gpio_cfg *PIOS_BOARD_HW_DEFS_GetLedCfg(uint32_t board_revision)
{
    switch (board_revision) {
    case BOARD_REVISION_CC:         return &pios_led_cfg_cc;

    case BOARD_REVISION_CC3D:       return &pios_led_cfg_cc3d;

    default:                        return NULL;
    }
}

#endif /* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_SPI)

#include <pios_spi_priv.h>

/* Gyro interface */
void PIOS_SPI_gyro_irq_handler(void);
void DMA1_Channel2_IRQHandler() __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
void DMA1_Channel3_IRQHandler() __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
static const struct pios_spi_cfg pios_spi_gyro_cfg = {
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
    .sclk                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .miso                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_IPU,
        },
    },
    .mosi                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .slave_count                                   = 1,
    .ssel                                          = {
        {
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_4,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
            },
        }
    },
};

uint32_t pios_spi_gyro_adapter_id;
void PIOS_SPI_gyro_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_gyro_adapter_id);
}


/* Flash/Accel Interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
void PIOS_SPI_flash_accel_irq_handler(void);
void DMA1_Channel4_IRQHandler() __attribute__((alias("PIOS_SPI_flash_accel_irq_handler")));
void DMA1_Channel5_IRQHandler() __attribute__((alias("PIOS_SPI_flash_accel_irq_handler")));
static const struct pios_spi_cfg pios_spi_flash_accel_cfg_cc3d = {
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
    .sclk                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .slave_count                                   = 2,
    .ssel                                          = {
        {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_12,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
            }
        },{
            .gpio = GPIOC,
            .init = {
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
            },
        }
    },
};

static const struct pios_spi_cfg pios_spi_flash_accel_cfg_cc = {
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
    .sclk                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
    .slave_count                                   = 2,
    .ssel                                          = {
        {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_12,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
            }
        },{
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_7,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_Out_PP,
            },
        }
    },
};

uint32_t pios_spi_flash_accel_adapter_id;
void PIOS_SPI_flash_accel_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_flash_accel_adapter_id);
}

#endif /* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"
#include "pios_flash_jedec_priv.h"

static const struct flashfs_logfs_cfg flashfs_w25x_cfg = {
    .fs_magic      = 0x99abcdef,
    .total_fs_size = 0x00080000, /* 512K bytes (128 sectors = entire chip) */
    .arena_size    = 0x00010000, /* 256 * slot size */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = 0,          /* start at the beginning of the chip */
    .sector_size   = 0x00001000, /* 4K bytes */
    .page_size     = 0x00000100, /* 256 bytes */
};


static const struct flashfs_logfs_cfg flashfs_m25p_cfg = {
    .fs_magic      = 0x99abceef,
    .total_fs_size = 0x00200000, /* 2M bytes (32 sectors = entire chip) */
    .arena_size    = 0x00010000, /* 256 * slot size */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = 0,          /* start at the beginning of the chip */
    .sector_size   = 0x00010000, /* 64K bytes */
    .page_size     = 0x00000100, /* 256 bytes */
};

#include "pios_flash.h"

#endif /* PIOS_INCLUDE_FLASH */

/*
 * ADC system
 */
#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
extern void PIOS_ADC_handler(void);
void DMA1_Channel1_IRQHandler() __attribute__((alias("PIOS_ADC_handler")));
// Remap the ADC DMA handler to this one
static const struct pios_adc_cfg pios_adc_cfg = {
    .dma                                           = {
        .ahb_clk = RCC_AHBPeriph_DMA1,
        .irq     = {
            .flags = (DMA1_FLAG_TC1 | DMA1_FLAG_TE1 | DMA1_FLAG_HT1 | DMA1_FLAG_GL1),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Channel1_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel = DMA1_Channel1,
            .init    = {
                .DMA_PeripheralBaseAddr            = (uint32_t)&ADC1->DR,
                .DMA_DIR                           = DMA_DIR_PeripheralSRC,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
                .DMA_Mode     = DMA_Mode_Circular,
                .DMA_Priority = DMA_Priority_High,
                .DMA_M2M                           = DMA_M2M_Disable,
            },
        }
    },
    .half_flag = DMA1_IT_HT1,
    .full_flag = DMA1_IT_TC1,
};

void PIOS_ADC_handler()
{
    PIOS_ADC_DMA_Handler();
}

const struct pios_adc_cfg *PIOS_BOARD_HW_DEFS_GetAdcCfg(uint32_t board_revision)
{
    return (board_revision == BOARD_REVISION_CC) ? &pios_adc_cfg : 0;
}
#endif /* if defined(PIOS_INCLUDE_ADC) */

#include "pios_tim_priv.h"

static const TIM_TimeBaseInitTypeDef tim_1_2_3_4_time_base = {
    .TIM_Prescaler         = (PIOS_MASTER_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
    .timer = TIM1,
    .time_base_init                            = &tim_1_2_3_4_time_base,
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
    .time_base_init                            = &tim_1_2_3_4_time_base,
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
    .time_base_init                            = &tim_1_2_3_4_time_base,
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
    .time_base_init                            = &tim_1_2_3_4_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

#include "pios_servo_config.h"

static const struct pios_tim_channel pios_tim_rcvrport_all_channels[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 1, B, 6, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 2, B, 5, GPIO_PartialRemap_TIM3),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 3, B, 0, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 4, B, 1, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 1, A, 0, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 2, A, 1, 0),
};

static const struct pios_tim_channel pios_tim_servoport_all_pins[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 4, B, 9, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 3, B, 8, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 2, B, 7, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 1, A, 8, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 1, B, 4, GPIO_PartialRemap_TIM3),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 3, A, 2, 0),
};


static const struct pios_tim_channel pios_tim_servoport_rcvrport_pins[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 4, B, 9, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 3, B, 8, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 2, B, 7, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 1, A, 8, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 1, B, 4, GPIO_PartialRemap_TIM3),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 3, A, 2, 0),

    // Receiver port pins
    // S3-S6 inputs are used as outputs in this case
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 3, B, 0, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 4, B, 1, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 1, A, 0, 0),
    TIM_SERVO_CHANNEL_CONFIG(TIM2, 2, A, 1, 0),
};

static const struct pios_tim_channel pios_tim_ppm_flexi_port = TIM_SERVO_CHANNEL_CONFIG(TIM2, 4, B, 11, GPIO_PartialRemap2_TIM2);

#if defined(PIOS_INCLUDE_USART)

#include "pios_usart_priv.h"

static const struct pios_usart_cfg pios_usart_main_cfg = {
    .regs = USART1,
    .rx   = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_IPU,
        },
    },
    .tx                 = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_9,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
};

static const struct pios_usart_cfg pios_usart_flexi_cfg = {
    .regs = USART3,
    .rx   = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_IPU,
        },
    },
    .tx                 = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF_PP,
        },
    },
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
void RTC_IRQHandler() __attribute__((alias("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
    .clksrc    = RCC_RTCCLKSource_HSE_Div128,
    .prescaler = 100,
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = RTC_IRQn,
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
    .channels     = pios_tim_servoport_all_pins,
    .num_channels = NELEMENTS(pios_tim_servoport_all_pins),
};

const struct pios_servo_cfg pios_servo_rcvr_cfg = {
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
    .channels     = pios_tim_servoport_rcvrport_pins,
    .num_channels = NELEMENTS(pios_tim_servoport_rcvrport_pins),
};

#endif /* PIOS_INCLUDE_SERVO && PIOS_INCLUDE_TIM */

/*
 * PPM Inputs
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>

const struct pios_ppm_cfg pios_ppm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    /* Use only the first channel for ppm */
    .channels     = &pios_tim_rcvrport_all_channels[0],
    .num_channels = 1,
};

const struct pios_ppm_cfg pios_ppm_pin8_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    /* Use only the first channel for ppm */
    .channels     = &pios_tim_rcvrport_all_channels[5],
    .num_channels = 1,
};

#endif /* PIOS_INCLUDE_PPM */

#if defined(PIOS_INCLUDE_PPM_FLEXI)
#include <pios_ppm_priv.h>

const struct pios_ppm_cfg pios_ppm_flexi_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    .channels     = &pios_tim_ppm_flexi_port,
    .num_channels = 1,
};

#endif /* PIOS_INCLUDE_PPM_FLEXI */

/*
 * PWM Inputs
 */
#if defined(PIOS_INCLUDE_PWM)
#include <pios_pwm_priv.h>

const struct pios_pwm_cfg pios_pwm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    .channels     = pios_tim_rcvrport_all_channels,
    .num_channels = NELEMENTS(pios_tim_rcvrport_all_channels),
};

const struct pios_pwm_cfg pios_pwm_with_ppm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
    },
    /* Leave the first channel for PPM use and use the rest for PWM */
    .channels     = &pios_tim_rcvrport_all_channels[1],
    .num_channels = NELEMENTS(pios_tim_rcvrport_all_channels) - 1,
};

#endif /* if defined(PIOS_INCLUDE_PWM) */


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

void PIOS_I2C_flexi_adapter_ev_irq_handler(void);
void PIOS_I2C_flexi_adapter_er_irq_handler(void);
void I2C2_EV_IRQHandler() __attribute__((alias("PIOS_I2C_flexi_adapter_ev_irq_handler")));
void I2C2_ER_IRQHandler() __attribute__((alias("PIOS_I2C_flexi_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_flexi_adapter_cfg = {
    .regs = I2C2,
    .init = {
        .I2C_Mode = I2C_Mode_I2C,
        .I2C_OwnAddress1                       = 0,
        .I2C_Ack  = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress               = I2C_AcknowledgedAddress_7bit,
        .I2C_DutyCycle                         = I2C_DutyCycle_2,
        .I2C_ClockSpeed                        = 400000,                      /* bits/s */
    },
    .transfer_timeout_ms                       = 50,
    .scl                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_10MHz,
            .GPIO_Mode  = GPIO_Mode_AF_OD,
        },
    },
    .sda                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_10MHz,
            .GPIO_Mode  = GPIO_Mode_AF_OD,
        },
    },
    .event                                     = {
        .flags = 0,               /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,               /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

uint32_t pios_i2c_flexi_adapter_id;
void PIOS_I2C_flexi_adapter_ev_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_EV_IRQ_Handler(pios_i2c_flexi_adapter_id);
}

void PIOS_I2C_flexi_adapter_er_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_ER_IRQ_Handler(pios_i2c_flexi_adapter_id);
}

#endif /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_GCSRCVR)
#include "pios_gcsrcvr_priv.h"
#endif /* PIOS_INCLUDE_GCSRCVR */

#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"

#endif /* PIOS_INCLUDE_RCVR */

#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_cfg_cc = {
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = USB_LP_CAN1_RX0_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .vsense                                    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_10MHz,
            .GPIO_Mode  = GPIO_Mode_AF_OD,
        },
    },
    .vsense_active_low                         = false
};

static const struct pios_usb_cfg pios_usb_main_cfg_cc3d = {
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = USB_LP_CAN1_RX0_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .vsense                                    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_10MHz,
            .GPIO_Mode  = GPIO_Mode_AF_OD,
        },
    },
    .vsense_active_low                         = false
};

const struct pios_usb_cfg *PIOS_BOARD_HW_DEFS_GetUsbCfg(uint32_t board_revision)
{
    switch (board_revision) {
    case BOARD_REVISION_CC:
        return &pios_usb_main_cfg_cc;

        break;
    case BOARD_REVISION_CC3D:
        return &pios_usb_main_cfg_cc3d;

        break;
    default:
        PIOS_DEBUG_Assert(0);
    }
    return NULL;
}

#endif /* PIOS_INCLUDE_USB */

/**
 * Configuration for MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU6000)
#include "pios_mpu6000.h"
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

const struct pios_mpu6000_cfg *PIOS_BOARD_HW_DEFS_GetMPU6000Cfg(__attribute__((unused)) uint32_t board_revision)
{
    return (board_revision == BOARD_REVISION_CC3D) ? &pios_mpu6000_cfg : 0;
}
#endif /* PIOS_INCLUDE_MPU6000 */

#ifdef PIOS_INCLUDE_ADXL345
bool PIOS_BOARD_HW_DEFS_GetADXL345Cfg(uint32_t board_revision)
{
    return board_revision == BOARD_REVISION_CC;
}
#endif
