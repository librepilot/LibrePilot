/**
 ******************************************************************************
 * @file       board_hw_defs.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the Revolution board.
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
            .gpio = GPIOE,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_2,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        },
        .active_low         = true
    },
    [PIOS_LED_ALARM] =     {
        .pin                =             {
            .gpio = GPIOE,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_3,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
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
/* SPI1 Interface
 *    - Used for BMA180 accelerometer
 */
void PIOS_SPI_accel_irq_handler(void);
void DMA2_Stream0_IRQHandler(void) __attribute__((alias("PIOS_SPI_accel_irq_handler")));
void DMA2_Stream3_IRQHandler(void) __attribute__((alias("PIOS_SPI_accel_irq_handler")));
static const struct pios_spi_cfg pios_spi_accel_cfg = {
    .regs  = SPI1,
    .remap = GPIO_AF_SPI1,
    .init  = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS                                   = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16,
    },
    .use_crc = false,
    .dma     = {
        .irq                                       = {
            .flags = (DMA_IT_TCIF0 | DMA_IT_TEIF0 | DMA_IT_HTIF0),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Stream0_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA2_Stream0,
            .init    = {
                .DMA_Channel            = DMA_Channel_3,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
                .DMA_DIR                = DMA_DIR_PeripheralToMemory,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                /* .DMA_FIFOThreshold */
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
        .tx                                        = {
            .channel = DMA2_Stream3,
            .init    = {
                .DMA_Channel            = DMA_Channel_3,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
                .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_High,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                /* .DMA_FIFOThreshold */
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
    },
    .sclk                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .miso                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .mosi                                          = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .slave_count                                   = 2,
    .ssel                                          = {
        {
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_4,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            }
        },
        {
            .gpio = GPIOC,
            .init = {
                .GPIO_Pin   = GPIO_Pin_5,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            }
        },
    }
};

uint32_t pios_spi_accel_id;
void PIOS_SPI_accel_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_accel_id);
}


/* SPI2 Interface
 *      - Used for gyro communications
 */
void PIOS_SPI_GYRO_irq_handler(void);
void DMA1_Stream3_IRQHandler(void) __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
static const struct pios_spi_cfg pios_spi_gyro_cfg = {
    .regs  = SPI2,
    .remap = GPIO_AF_SPI2,
    .init  = {
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
        .irq                                       = {
            // Note this is the stream ID that triggers interrupts (in this case RX)
            .flags = (DMA_IT_TCIF3 | DMA_IT_TEIF3 | DMA_IT_HTIF3),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Stream3_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA1_Stream3,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR),
                .DMA_DIR                = DMA_DIR_PeripheralToMemory,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                // TODO: Enable FIFO
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
        .tx                                        = {
            .channel = DMA1_Stream4,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR),
                .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
    },
    .sclk                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
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
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};

uint32_t pios_spi_gyro_id;
void PIOS_SPI_gyro_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_gyro_id);
}


#if !defined(PIOS_FLASH_ON_ACCEL)
/* SPI3 Interface
 *      - Used for flash communications
 */
void PIOS_SPI_flash_irq_handler(void);
void DMA1_Stream0_IRQHandler(void) __attribute__((alias("PIOS_SPI_flash_irq_handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((alias("PIOS_SPI_flash_irq_handler")));
static const struct pios_spi_cfg pios_spi_flash_cfg = {
    .regs  = SPI3,
    .remap = GPIO_AF_SPI3,
    .init  = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS                                   = SPI_NSS_Soft,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
    },
    .use_crc = false,
    .dma     = {
        .irq                                       = {
            // Note this is the stream ID that triggers interrupts (in this case RX)
            .flags = (DMA_IT_TCIF0 | DMA_IT_TEIF0 | DMA_IT_HTIF0),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Stream0_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA1_Stream0,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR),
                .DMA_DIR                = DMA_DIR_PeripheralToMemory,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                // TODO: Enable FIFO
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
        .tx                                        = {
            .channel = DMA1_Stream5,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR),
                .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
    },
    .sclk                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .miso                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .mosi                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_12,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .slave_count                                   = 1,
    .ssel                                          = {
        {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_2,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};

uint32_t pios_spi_flash_id;
void PIOS_SPI_flash_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_flash_id);
}
#endif /* PIOS_FLASH_ON_ACCEL */

#endif /* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"
#include "pios_flash_jedec_priv.h"
#include "pios_flash_internal_priv.h"

static const struct flashfs_logfs_cfg flashfs_external_cfg = {
    .fs_magic      = 0x99abceef,
    .total_fs_size = 0x00200000, /* 2M bytes (32 sectors = entire chip) */
    .arena_size    = 0x00010000, /* 256 * slot size */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = 0,      /* start at the beginning of the chip */
    .sector_size   = 0x00010000, /* 64K bytes */
    .page_size     = 0x00000100, /* 256 bytes */
};


static const struct pios_flash_internal_cfg flash_internal_cfg = {};

static const struct flashfs_logfs_cfg flashfs_internal_cfg = {
    .fs_magic      = 0x99abcfef,
    .total_fs_size = EE_BANK_SIZE, /* 32K bytes (2x16KB sectors) */
    .arena_size    = 0x00004000, /* 64 * slot size = 16K bytes = 1 sector */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = EE_BANK_BASE, /* start after the bootloader */
    .sector_size   = 0x00004000, /* 16K bytes */
    .page_size     = 0x00004000, /* 16K bytes */
};

#endif /* PIOS_INCLUDE_FLASH */

#if defined(PIOS_OVERO_SPI)
/* SPI3 Interface
 *      - Used for flash communications
 */
#include <pios_overo_priv.h>
void PIOS_OVERO_irq_handler(void);
void DMA1_Stream7_IRQHandler(void) __attribute__((alias("PIOS_OVERO_irq_handler")));
static const struct pios_overo_cfg pios_overo_cfg = {
    .regs  = SPI3,
    .remap = GPIO_AF_SPI3,
    .init  = {
        .SPI_Mode              = SPI_Mode_Slave,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS                                   = SPI_NSS_Hard,
        .SPI_FirstBit          = SPI_FirstBit_MSB,
        .SPI_CRCPolynomial     = 7,
        .SPI_CPOL              = SPI_CPOL_High,
        .SPI_CPHA              = SPI_CPHA_2Edge,
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
    },
    .use_crc = false,
    .dma     = {
        .irq                                       = {
            // Note this is the stream ID that triggers interrupts (in this case TX)
            .flags = (DMA_IT_TCIF7),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Stream7_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {
            .channel = DMA1_Stream0,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR),
                .DMA_DIR                = DMA_DIR_PeripheralToMemory,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Circular,
                .DMA_Priority           = DMA_Priority_Medium,
                // TODO: Enable FIFO
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
        .tx                                        = {
            .channel = DMA1_Stream7,
            .init    = {
                .DMA_Channel            = DMA_Channel_0,
                .DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR),
                .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Circular,
                .DMA_Priority           = DMA_Priority_Medium,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
    },
    .sclk                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .miso                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .mosi                                          = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_12,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
    .slave_count                                   = 1,
    .ssel                                          = {
        {
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
        }
    },
};
uint32_t pios_overo_id = 0;
void PIOS_OVERO_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_OVERO_DMA_irq_handler(pios_overo_id);
}
#else /* if defined(PIOS_OVERO_SPI) */

#endif /* PIOS_OVERO_SPI */

#include <pios_usart_priv.h>

#ifdef PIOS_INCLUDE_COM_TELEM
/*
 * Telemetry on main USART
 */
static const struct pios_usart_cfg pios_usart_telem_cfg = {
    .regs  = USART2,
    .remap = GPIO_AF_USART2,
    .rx    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_COM_TELEM */

#ifdef PIOS_INCLUDE_GPS
/*
 * GPS USART
 */
static const struct pios_usart_cfg pios_usart_gps_cfg = {
    .regs  = USART1,
    .remap = GPIO_AF_USART1,
    .rx    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_9,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_INCLUDE_GPS */

#ifdef PIOS_INCLUDE_COM_AUX
/*
 * AUX USART (UART label on rev2)
 */
static const struct pios_usart_cfg pios_usart_aux_cfg = {
    .regs  = USART6,
    .remap = GPIO_AF_USART6,
    .rx    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_COM_AUX */

#ifdef PIOS_INCLUDE_COM_AUXSBUS
/*
 * AUX USART SBUS ( UART/ S Bus label on rev2)
 */

static const struct pios_usart_cfg pios_usart_auxsbus_cfg = {
    .regs  = UART4,
    .remap = GPIO_AF_UART4,
    .rx    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_1,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_0,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_INCLUDE_COM_AUXSBUS */

#ifdef PIOS_INCLUDE_COM_FLEXI
/*
 * FLEXI PORT
 */
static const struct pios_usart_cfg pios_usart_flexi_cfg = {
    .regs  = USART3,
    .remap = GPIO_AF_USART3,
    .rx    = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_INCLUDE_COM_FLEXI */

#if defined(PIOS_INCLUDE_COM)

#include <pios_com_priv.h>

#endif /* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

/*
 * I2C Adapters
 */
void PIOS_I2C_mag_adapter_ev_irq_handler(void);
void PIOS_I2C_mag_adapter_er_irq_handler(void);
void I2C1_EV_IRQHandler()
__attribute__((alias("PIOS_I2C_mag_adapter_ev_irq_handler")));
void I2C1_ER_IRQHandler()
__attribute__((alias("PIOS_I2C_mag_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_mag_adapter_cfg = {
    .regs     = I2C1,
    .remapSCL = GPIO_AF_I2C1,
    .remapSDA = GPIO_AF_I2C1,
    .init     = {
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
            .GPIO_Pin   = GPIO_Pin_6,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .sda                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .event                                     = {
        .flags = 0,     /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,     /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

uint32_t pios_i2c_mag_adapter_id;
void PIOS_I2C_mag_adapter_ev_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_EV_IRQ_Handler(pios_i2c_mag_adapter_id);
}

void PIOS_I2C_mag_adapter_er_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_ER_IRQ_Handler(pios_i2c_mag_adapter_id);
}


void PIOS_I2C_flexiport_adapter_ev_irq_handler(void);
void PIOS_I2C_flexiport_adapter_er_irq_handler(void);
void I2C2_EV_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_ev_irq_handler")));
void I2C2_ER_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_flexiport_adapter_cfg = {
    .regs     = I2C2,
    .remapSCL = GPIO_AF_I2C2,
    .remapSDA = GPIO_AF_I2C2,
    .init     = {
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
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_UP,
        },
    },
    .sda                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_UP,
        },
    },
    .event                                     = {
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

uint32_t pios_i2c_flexiport_adapter_id;
void PIOS_I2C_flexiport_adapter_ev_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_EV_IRQ_Handler(pios_i2c_flexiport_adapter_id);
}

void PIOS_I2C_flexiport_adapter_er_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_ER_IRQ_Handler(pios_i2c_flexiport_adapter_id);
}


void PIOS_I2C_pressure_adapter_ev_irq_handler(void);
void PIOS_I2C_pressure_adapter_er_irq_handler(void);
void I2C3_EV_IRQHandler() __attribute__((alias("PIOS_I2C_pressure_adapter_ev_irq_handler")));
void I2C3_ER_IRQHandler() __attribute__((alias("PIOS_I2C_pressure_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_pressure_adapter_cfg = {
    .regs     = I2C3,
    .remapSCL = GPIO_AF_I2C3,
    .remapSDA = GPIO_AF_I2C3,
    .init     = {
        .I2C_Mode = I2C_Mode_I2C,
        .I2C_OwnAddress1                       = 0,
        .I2C_Ack  = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress               = I2C_AcknowledgedAddress_7bit,
        .I2C_DutyCycle                         = I2C_DutyCycle_2,
        .I2C_ClockSpeed                        = 400000,                      /* bits/s */
    },
    .transfer_timeout_ms                       = 50,
    .scl                                       = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_8,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .sda                                       = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_9,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .event                                     = {
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C3_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C3_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

uint32_t pios_i2c_pressure_adapter_id;
void PIOS_I2C_pressure_adapter_ev_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_EV_IRQ_Handler(pios_i2c_pressure_adapter_id);
}

void PIOS_I2C_pressure_adapter_er_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_I2C_ER_IRQ_Handler(pios_i2c_pressure_adapter_id);
}
#endif /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler(void);
void RTC_WKUP_IRQHandler() __attribute__((alias("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
    .clksrc    = RCC_RTCCLKSource_HSE_Div16, // Divide 8 Mhz crystal down to 1
    // For some reason it's acting like crystal is 16 Mhz.  This clock is then divided
    // by another 16 to give a nominal 62.5 khz clock
    .prescaler = 100, // Every 100 cycles gives 625 Hz
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

#include "pios_tim_priv.h"

static const TIM_TimeBaseInitTypeDef tim_2_3_5_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB1_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};

static const TIM_TimeBaseInitTypeDef tim_9_10_11_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};

// Set up timers that only have inputs on APB2
static const TIM_TimeBaseInitTypeDef tim_1_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = 0xFFFF,
    .TIM_RepetitionCounter = 0x0000,
};

static const TIM_TimeBaseInitTypeDef tim_8_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = 0xFFFF,
    .TIM_RepetitionCounter = 0x0000,
};

// Set up timers that only have inputs on APB1
static const TIM_TimeBaseInitTypeDef tim_4_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB1_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = 0xFFFF,
    .TIM_RepetitionCounter = 0x0000,
};


static const struct pios_tim_clock_cfg tim_2_cfg = {
    .timer = TIM2,
    .time_base_init                            = &tim_2_3_5_time_base,
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
    .time_base_init                            = &tim_2_3_5_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM3_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};


static const struct pios_tim_clock_cfg tim_5_cfg = {
    .timer = TIM5,
    .time_base_init                            = &tim_2_3_5_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
    .timer = TIM8,
    .time_base_init                            = &tim_8_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM8_CC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_9_cfg = {
    .timer = TIM9,
    .time_base_init                            = &tim_9_10_11_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_BRK_TIM9_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_10_cfg = {
    .timer = TIM10,
    .time_base_init                            = &tim_9_10_11_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_UP_TIM10_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_11_cfg = {
    .timer = TIM11,
    .time_base_init                            = &tim_9_10_11_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_TRG_COM_TIM11_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
    .timer = TIM1,
    .time_base_init                            = &tim_1_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_CC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_4_cfg = {
    .timer = TIM4,
    .time_base_init                            = &tim_4_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

/**
 * Pios servo configuration structures
 */
#include <pios_servo_priv.h>
static const struct pios_tim_channel pios_tim_servoport_all_pins[] = {
    {
        .timer = TIM9,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_5,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource5,
        },
        .remap = GPIO_AF_TIM9,
    },
    {
        .timer = TIM9,
        .timer_chan = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_6,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource6,
        },
        .remap = GPIO_AF_TIM9,
    },
    {
        .timer = TIM11,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_9,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource9,
        },
        .remap = GPIO_AF_TIM11,
    },
    {
        .timer = TIM10,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_8,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource8,
        },
        .remap = GPIO_AF_TIM10,
    },
    {
        .timer = TIM5,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_2,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource2,
        },
        .remap = GPIO_AF_TIM5,
    },
    {
        .timer = TIM5,
        .timer_chan = TIM_Channel_4,
        .pin   = {
            .gpio = GPIOA,
            .init = {
                .GPIO_Pin   = GPIO_Pin_3,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource3,
        },
        .remap = GPIO_AF_TIM5,
    },
    {
        .timer = TIM3,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_0,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource0,
        },
        .remap = GPIO_AF_TIM3,
    },
    {
        .timer = TIM3,
        .timer_chan = TIM_Channel_4,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_1,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource1,
        },
        .remap = GPIO_AF_TIM3,
    },
    // PB3 - TIM2 CH2 LED1
    {
        .timer = TIM2,
        .timer_chan = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_3,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource3,
        },
        .remap = GPIO_AF_TIM2,
    },
    // PB4 - TIM3 CH1 LED2
    {
        .timer = TIM3,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_4,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource4,
        },
        .remap = GPIO_AF_TIM3,
    },
};

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


/*
 * PWM Inputs
 */
#if defined(PIOS_INCLUDE_PWM) || defined(PIOS_INCLUDE_PPM)
#include <pios_pwm_priv.h>
static const struct pios_tim_channel pios_tim_rcvrport_all_channels[] = {
    {
        .timer = TIM4,
        .timer_chan = TIM_Channel_4,
        .pin   = {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource15,
        },
        .remap = GPIO_AF_TIM4,
    },
    {
        .timer = TIM4,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_14,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource14,
        },
        .remap = GPIO_AF_TIM4,
    },
    {
        .timer = TIM4,
        .timer_chan = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_13,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource13,
        },
        .remap = GPIO_AF_TIM4,
    },
    {
        .timer = TIM4,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_12,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource12,
        },
        .remap = GPIO_AF_TIM4,
    },
    {
        .timer = TIM1,
        .timer_chan = TIM_Channel_4,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_14,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource14,
        },
        .remap = GPIO_AF_TIM1,
    },
    {
        .timer = TIM1,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_13,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource13,
        },
        .remap = GPIO_AF_TIM1,
    },
    {
        .timer = TIM1,
        .timer_chan = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_11,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource11,
        },
        .remap = GPIO_AF_TIM1,
    },
    {
        .timer = TIM1,
        .timer_chan = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOE,
            .init = {
                .GPIO_Pin   = GPIO_Pin_9,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource9,
        },
        .remap = GPIO_AF_TIM1,
    },
};

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
#endif /* if defined(PIOS_INCLUDE_PWM) || defined(PIOS_INCLUDE_PPM) */

/*
 * PPM Input
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>
static const struct pios_ppm_cfg pios_ppm_cfg = {
    .tim_ic_init         = {
        .TIM_ICPolarity  = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0,
        .TIM_Channel     = TIM_Channel_2,
    },
    /* Use only the first channel for ppm */
    .channels     = &pios_tim_rcvrport_all_channels[0],
    .num_channels = 1,
};

#endif // PPM

#if defined(PIOS_INCLUDE_GCSRCVR)
#include "pios_gcsrcvr_priv.h"
#endif /* PIOS_INCLUDE_GCSRCVR */

#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"
#endif /* PIOS_INCLUDE_RCVR */

/*
 * SONAR Inputs
 */
#if defined(PIOS_INCLUDE_HCSR04)
#include <pios_hcsr04_priv.h>

static const struct pios_tim_channel pios_tim_hcsr04_port_all_channels[] = {
    {
        .timer = TIM8,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOC,
            .init = {
                .GPIO_Pin   = GPIO_Pin_8,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_PuPd  = GPIO_PuPd_DOWN
            },
            .pin_source     = GPIO_PinSource8,
        },
        .remap = GPIO_AF_TIM8,
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
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP,
            .GPIO_Speed = GPIO_Speed_2MHz,
        },
    },
};
#endif /* if defined(PIOS_INCLUDE_HCSR04) */

#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_cfg = {
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = OTG_FS_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .vsense                                    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Speed = GPIO_Speed_25MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
        },
    },
    .vsense_active_low                         = false
};
const struct pios_usb_cfg *PIOS_BOARD_HW_DEFS_GetUsbCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_usb_main_cfg;
}
#endif /* PIOS_INCLUDE_USB */


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
const struct pios_adc_cfg *PIOS_BOARD_HW_DEFS_GetAdcCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_adc_cfg;
}
#endif /* if defined(PIOS_INCLUDE_ADC) */

#if defined(PIOS_INCLUDE_HMC5X83)
#include "pios_hmc5x83.h"

#ifdef PIOS_HMC5X83_HAS_GPIOS
bool pios_board_internal_mag_handler()
{
    return PIOS_HMC5x83_IRQHandler(pios_hmc5x83_internal_id);
}
static const struct pios_exti_cfg pios_exti_hmc5x83_cfg __exti_config = {
    .vector = pios_board_internal_mag_handler,
    .line   = EXTI_Line5,
    .pin    = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line5, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};
#endif /* PIOS_HMC5X83_HAS_GPIOS */

static const struct pios_hmc5x83_cfg pios_hmc5x83_internal_cfg = {
#ifdef PIOS_HMC5X83_HAS_GPIOS
    .exti_cfg  = &pios_exti_hmc5x83_cfg,
#endif
    .M_ODR     = PIOS_HMC5x83_ODR_75,
    .Meas_Conf = PIOS_HMC5x83_MEASCONF_NORMAL,
    .Gain   = PIOS_HMC5x83_GAIN_1_9,
    .Mode   = PIOS_HMC5x83_MODE_CONTINUOUS,
    .TempCompensation = false,
    .Driver = &PIOS_HMC5x83_I2C_DRIVER,
    .Orientation      = PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP,
};

const struct pios_hmc5x83_cfg *PIOS_BOARD_HW_DEFS_GetInternalHMC5x83Cfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_hmc5x83_internal_cfg;
}

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
    .Orientation      = PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP,
};

const struct pios_hmc5x83_cfg *PIOS_BOARD_HW_DEFS_GetExternalHMC5x83Cfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_hmc5x83_external_cfg;
}
#endif /* PIOS_INCLUDE_HMC5X83 */

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS56XX)
#include "pios_ms56xx.h"
static const struct pios_ms56xx_cfg pios_ms56xx_cfg = {
    .address = 0x77,
    .version = MS56XX_VERSION_5611,
    .oversampling = MS56XX_OSR_4096,
};
const struct pios_ms56xx_cfg *PIOS_BOARD_HW_DEFS_GetMS56xxCfg(__attribute__((unused)) uint32_t board_revision)
{
    return &pios_ms56xx_cfg;
}
#endif /* PIOS_INCLUDE_MS56XX */

/**
 * Configuration for the BMA180 chip
 */
#if defined(PIOS_INCLUDE_BMA180)
#include "pios_bma180.h"
static const struct pios_exti_cfg pios_exti_bma180_cfg __exti_config = {
    .vector = PIOS_BMA180_IRQHandler,
    .line   = EXTI_Line4,
    .pin    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_4,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line4, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};
static const struct pios_bma180_cfg pios_bma180_cfg = {
    .exti_cfg  = &pios_exti_bma180_cfg,
    .bandwidth = BMA_BW_600HZ,
    .range     = BMA_RANGE_8G,
};
const struct pios_bma180_cfg *PIOS_BOARD_HW_DEFS_GetBMA180Cfg(uint32_t board_revision)
{
    return (board_revision == 0x01) ? &pios_bma180_cfg : 0;
}
#endif /* PIOS_INCLUDE_BMA180 */

/**
 * Configuration for the MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU6000)
#include "pios_mpu6000.h"
static const struct pios_exti_cfg pios_exti_mpu6000_cfg __exti_config = {
    .vector = PIOS_MPU6000_IRQHandler,
    .line   = EXTI_Line8,
    .pin    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_8,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line8, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_mpu6000_cfg pios_mpu6000_cfg = {
    .exti_cfg   = &pios_exti_mpu6000_cfg,
    .Fifo_store = PIOS_MPU6000_FIFO_TEMP_OUT | PIOS_MPU6000_FIFO_GYRO_X_OUT | PIOS_MPU6000_FIFO_GYRO_Y_OUT | PIOS_MPU6000_FIFO_GYRO_Z_OUT,
    // Clock at 8 khz
    .Smpl_rate_div_no_dlp = 0,
    // with dlp on output rate is 1000Hz
    .Smpl_rate_div_dlp    = 0,
    .interrupt_cfg  = PIOS_MPU6000_INT_CLR_ANYRD,
    .interrupt_en   = PIOS_MPU6000_INTEN_DATA_RDY,
    .User_ctl             = PIOS_MPU6000_USERCTL_DIS_I2C,
    .Pwr_mgmt_clk   = PIOS_MPU6000_PWRMGMT_PLL_X_CLK,
    .accel_range    = PIOS_MPU6000_ACCEL_8G,
    .gyro_range     = PIOS_MPU6000_SCALE_2000_DEG,
    .filter               = PIOS_MPU6000_LOWPASS_256_HZ,
    .orientation    = PIOS_MPU6000_TOP_0DEG,
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
    .max_downsample = 16,
};

const struct pios_mpu6000_cfg *PIOS_BOARD_HW_DEFS_GetMPU6000Cfg(uint32_t board_revision)
{
    return (board_revision == 0x02) ? &pios_mpu6000_cfg : 0;
}
#endif /* PIOS_INCLUDE_MPU6000 */

/**
 * Configuration for L3GD20 chip
 */
#if defined(PIOS_INCLUDE_L3GD20)
#include "pios_l3gd20.h"
static const struct pios_exti_cfg pios_exti_l3gd20_cfg __exti_config = {
    .vector = PIOS_L3GD20_IRQHandler,
    .line   = EXTI_Line8,
    .pin    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_8,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line8, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_l3gd20_cfg pios_l3gd20_cfg = {
    .exti_cfg = &pios_exti_l3gd20_cfg,
    .range    = PIOS_L3GD20_SCALE_500_DEG,
};

const struct pios_l3gd20_cfg *PIOS_BOARD_HW_DEFS_GetL3GD20Cfg(uint32_t board_revision)
{
    return (board_revision == 0x01) ? &pios_l3gd20_cfg : 0;
}
#endif /* PIOS_INCLUDE_L3GD20 */
