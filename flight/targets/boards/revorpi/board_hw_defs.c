/**
 ******************************************************************************
 * @file       board_hw_defs.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
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
#include <pios.h>

#define MAIN_USART_REGS                  UART4
#define MAIN_USART_REMAP                 GPIO_AF_UART4
#define MAIN_USART_IRQ                   UART4_IRQn
#define MAIN_USART_RX_GPIO               GPIOA
#define MAIN_USART_RX_PIN                GPIO_Pin_1
#define MAIN_USART_TX_GPIO               GPIOA
#define MAIN_USART_TX_PIN                GPIO_Pin_0
//SBUS USART
#define SBUS_USART_REGS                  USART3
#define SBUS_USART_REMAP                 GPIO_AF_USART3
#define SBUS_USART_IRQ                   USART3_IRQn
#define SBUS_USART_RX_GPIO               GPIOD
#define SBUS_USART_RX_PIN                GPIO_Pin_9
#define SBUS_USART_TX_GPIO               GPIOD
#define SBUS_USART_TX_PIN                GPIO_Pin_8
// Inverter for SBUS handling
#define SBUS_USART_INVERTER_GPIO         GPIOD
#define SBUS_USART_INVERTER_PIN          GPIO_Pin_11
#define SBUS_USART_INVERTER_CLOCK_FUNC   RCC_AHB1PeriphClockCmd
#define SBUS_USART_INVERTER_CLOCK_PERIPH RCC_AHB1Periph_GPIOD

#define FLEXI_USART_REGS                 USART1
#define FLEXI_USART_REMAP                GPIO_AF_USART1
#define FLEXI_USART_IRQ                  USART1_IRQn
#define FLEXI_USART_RX_GPIO              GPIOB
#define FLEXI_USART_RX_PIN               GPIO_Pin_7
#define FLEXI_USART_TX_GPIO              GPIOB
#define FLEXI_USART_TX_PIN               GPIO_Pin_6
// ReceiverPort pin 3
#define FLEXI_USART_DTR_GPIO             GPIOE
#define FLEXI_USART_DTR_PIN              GPIO_Pin_12


#if defined(PIOS_INCLUDE_LED)

#include <pios_led_priv.h>
static const struct pios_gpio pios_leds[] = {
    [PIOS_LED_HEARTBEAT] = {
        .pin                =             {
            .gpio = GPIOD,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_3,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_OD,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL
            },
        },
        .active_low         = true
    },
    [PIOS_LED_ALARM] =     {
        .pin                =             {
            .gpio = GPIOE,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_OD,
                .GPIO_PuPd  = GPIO_PuPd_NOPULL
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

/*
 * SPI1 Interface
 * Used for MPU9250 gyro and accelerometer
 */
void PIOS_SPI_gyro_irq_handler(void);
void DMA1_Stream0_IRQHandler(void) __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
void DMA1_Stream7_IRQHandler(void) __attribute__((alias("PIOS_SPI_gyro_irq_handler")));
static const struct pios_spi_cfg pios_spi_gyro_baro_cfg = {
    .regs  = SPI3,
    .remap = GPIO_AF_SPI3,
    .init  = {
        .SPI_Mode              = SPI_Mode_Master,
        .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
        .SPI_DataSize          = SPI_DataSize_8b,
        .SPI_NSS               = SPI_NSS_Soft,
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
                .DMA_Mode               = DMA_Mode_Normal,
                .DMA_Priority           = DMA_Priority_Medium,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                /* .DMA_FIFOThreshold */
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
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_3,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_4,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
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
                .GPIO_Pin   = GPIO_Pin_15,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            }
        },
        {
            .gpio = GPIOD,
            .init = {
                .GPIO_Pin   = GPIO_Pin_7,
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode  = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            }
        }
    }
};

static uint32_t pios_spi_gyro_baro_id;
void PIOS_SPI_gyro_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_gyro_baro_id);
}

#endif /* PIOS_INCLUDE_SPI */

#include <pios_usart_priv.h>

#ifdef PIOS_INCLUDE_COM_TELEM

/*
 * MAIN USART
 */
static const struct pios_usart_cfg pios_usart_main_cfg = {
    .regs  = MAIN_USART_REGS,
    .remap = MAIN_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 57600,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx | USART_Mode_Tx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = MAIN_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = MAIN_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = MAIN_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};
#endif /* PIOS_INCLUDE_COM_TELEM */

#ifdef PIOS_INCLUDE_DSM

#include "pios_dsm_priv.h"
static const struct pios_usart_cfg pios_usart_dsm_main_cfg = {
    .regs  = MAIN_USART_REGS,
    .remap = MAIN_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 115200,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = MAIN_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = MAIN_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = MAIN_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

// Because of the inverter on the main port this will not
// work.  Notice the mode is set to IN to maintain API
// compatibility but protect the pins
static const struct pios_dsm_cfg pios_dsm_main_cfg = {
    .bind               = {
        .gpio = MAIN_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
};

#endif /* PIOS_INCLUDE_DSM */


#include <pios_sbus_priv.h>
#if defined(PIOS_INCLUDE_SBUS)
/*
 * S.Bus USART
 */
#include <pios_sbus_priv.h>

static const struct pios_usart_cfg pios_usart_sbus_main_cfg = {
    .regs  = SBUS_USART_REGS,
    .remap = SBUS_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 99999,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_Even,
        .USART_StopBits   = USART_StopBits_2,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = SBUS_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = SBUS_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = SBUS_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = SBUS_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = SBUS_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
};

#endif /* PIOS_INCLUDE_SBUS */

// Need this defined regardless to be able to turn it off
static const struct pios_sbus_cfg pios_sbus_cfg = {
    /* Inverter configuration */
    .inv                = {
        .gpio = SBUS_USART_INVERTER_GPIO,
        .init = {
            .GPIO_Pin   = SBUS_USART_INVERTER_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .gpio_inv_enable  = Bit_SET,
    .gpio_inv_disable = Bit_RESET,
    .gpio_clk_func    = SBUS_USART_INVERTER_CLOCK_FUNC,
    .gpio_clk_periph  = SBUS_USART_INVERTER_CLOCK_PERIPH,
};


#ifdef PIOS_INCLUDE_COM_FLEXI
/*
 * FLEXI PORT
 */
static const struct pios_usart_cfg pios_usart_flexi_cfg = {
    .regs  = FLEXI_USART_REGS,
    .remap = FLEXI_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 57600,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             =
            USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx | USART_Mode_Tx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = FLEXI_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = FLEXI_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = FLEXI_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .dtr                                       = {
        .gpio = FLEXI_USART_DTR_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_DTR_PIN,
            .GPIO_Speed = GPIO_Speed_25MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
        },
    },
};

#endif /* PIOS_INCLUDE_COM_FLEXI */

#ifdef PIOS_INCLUDE_DSM

#include "pios_dsm_priv.h"
static const struct pios_usart_cfg pios_usart_dsm_flexi_cfg = {
    .regs  = FLEXI_USART_REGS,
    .remap = FLEXI_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 115200,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = FLEXI_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = FLEXI_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = FLEXI_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

static const struct pios_dsm_cfg pios_dsm_flexi_cfg = {
    .bind               = {
        .gpio = FLEXI_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL
        },
    },
};

#endif /* PIOS_INCLUDE_DSM */

#if defined(PIOS_INCLUDE_SRXL)
/*
 * SRXL USART
 */
#include <pios_srxl_priv.h>

static const struct pios_usart_cfg pios_usart_srxl_flexi_cfg = {
    .regs  = FLEXI_USART_REGS,
    .remap = FLEXI_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 115200,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = FLEXI_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = FLEXI_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = FLEXI_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_INCLUDE_SRXL */
/*
 * HK OSD
 */
static const struct pios_usart_cfg pios_usart_hkosd_main_cfg = {
    .regs  = MAIN_USART_REGS,
    .remap = MAIN_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 57600,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx | USART_Mode_Tx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = MAIN_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = MAIN_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = MAIN_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = MAIN_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

static const struct pios_usart_cfg pios_usart_hkosd_flexi_cfg = {
    .regs  = FLEXI_USART_REGS,
    .remap = FLEXI_USART_REMAP,
    .init  = {
        .USART_BaudRate   = 57600,
        .USART_WordLength = USART_WordLength_8b,
        .USART_Parity     = USART_Parity_No,
        .USART_StopBits   = USART_StopBits_1,
        .USART_HardwareFlowControl             = USART_HardwareFlowControl_None,
        .USART_Mode                            = USART_Mode_Rx | USART_Mode_Tx,
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = FLEXI_USART_IRQ,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .rx                                        = {
        .gpio = FLEXI_USART_RX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_RX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                                        = {
        .gpio = FLEXI_USART_TX_GPIO,
        .init = {
            .GPIO_Pin   = FLEXI_USART_TX_PIN,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#if defined(PIOS_INCLUDE_COM)

#include <pios_com_priv.h>

#endif /* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

/*
 * I2C Adapters
 */
void PIOS_I2C_pressure_adapter_ev_irq_handler(void);
void PIOS_I2C_pressure_adapter_er_irq_handler(void);
void I2C2_EV_IRQHandler()
__attribute__((alias("PIOS_I2C_pressure_adapter_ev_irq_handler")));
void I2C2_ER_IRQHandler()
__attribute__((alias("PIOS_I2C_pressure_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_pressure_adapter_cfg = {
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
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .sda                                       = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_11,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .event                                     = {
        .flags = 0,     /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,     /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C2_ER_IRQn,
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


void PIOS_I2C_flexiport_adapter_ev_irq_handler(void);
void PIOS_I2C_flexiport_adapter_er_irq_handler(void);
void I2C1_EV_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_ev_irq_handler")));
void I2C1_ER_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_flexiport_adapter_cfg = {
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
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .error                                     = {
        .flags = 0,           /* FIXME: check this */
        .init  = {
            .NVIC_IRQChannel    = I2C1_ER_IRQn,
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


#endif /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler(void);
void RTC_WKUP_IRQHandler() __attribute__((alias("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
    .clksrc    = RCC_RTCCLKSource_HSE_Div8, // Divide 8 Mhz crystal down to 1
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

static const TIM_TimeBaseInitTypeDef tim_apb1_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB1_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};
static const TIM_TimeBaseInitTypeDef tim_apb2_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
    .TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
    .timer = TIM1,
    .time_base_init                            = &tim_apb2_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_UP_TIM10_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
    .timer = TIM8,
    .time_base_init                            = &tim_apb2_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM8_CC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_2_cfg = {
    .timer = TIM2,
    .time_base_init                            = &tim_apb1_time_base,
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
    .time_base_init                            = &tim_apb1_time_base,
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
    .time_base_init                            = &tim_apb1_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_5_cfg = {
    .timer = TIM5,
    .time_base_init                            = &tim_apb1_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_tim_clock_cfg tim_9_cfg = {
    .timer = TIM9,
    .time_base_init                            = &tim_apb2_time_base,
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
    .time_base_init                            = &tim_apb2_time_base,
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
    .time_base_init                            = &tim_apb2_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM1_TRG_COM_TIM11_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};



/**
 * Pios servo configuration structures
 *      I/O |   PIN     |    Channel    |   Alternate function
 *      I1  |   PD12    |   TIM4_CH1    |   X
 *      i2  |   PD13    |   TIM4_CH2    |   X
 *      I3  |   PD14    |   TIM4_CH3    |   X
 *      I4  |   PD15    |   TIM4_CH4    |   X
 *      I5  |   PC6     |   TIM8_CH1    |   X
 *      I6  |   PC7     |   TIM8_CH2    |   X
 *------------------------------------
 *      o1  |   PB0     |   TIM3_CH3    |   X
 *      o2  |   PB1     |   TIM3_CH4    |   X
 *      o3  |   PE9     |   TIM1_CH1    |   X
 *      o4  |   PE11    |   TIM1_CH2    |   X
 *      o5  |   PE13    |   TIM1_CH3    |   X
 *      o6  |   PE14    |   TIM1_CH4    |   X
 */
#include <pios_servo_priv.h>
#include <pios_servo_config.h>
static const struct pios_tim_channel dummmy_timer =
    TIM_SERVO_CHANNEL_CONFIG(TIM9, 1, E, 5); // dummy unused timer + gpio. Hack to free tim1

static struct pios_tim_channel pios_tim_servoport_all_pins[] = {
    // TIMER, CHANNEL, GPIO, PIN
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 1, D, 12),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 2, D, 13),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 3, D, 14),
    TIM_SERVO_CHANNEL_CONFIG(TIM4, 4, D, 15),
    TIM_SERVO_CHANNEL_CONFIG(TIM8, 1, C, 6 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM8, 2, C, 7 ),
    // PWM pins on FlexiIO(receiver) port
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 3, B, 0 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 4, B, 1 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 1, E, 9 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 2, E, 11),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 3, E, 13),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 4, E, 14),
};

#define PIOS_SERVOPORT_ALL_PINS_PWMOUT        6
#define PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN     (NELEMENTS(pios_tim_servoport_all_pins))
#define PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN_PPM (PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN - 1)


const struct pios_servo_cfg pios_servo_cfg_out = {
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
    .num_channels = PIOS_SERVOPORT_ALL_PINS_PWMOUT,
};
// All servo outputs, servo input ch1 ppm, ch2-6 outputs
const struct pios_servo_cfg pios_servo_cfg_out_in_ppm = {
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
    .num_channels = PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN_PPM,
};
// All servo outputs, servo inputs ch1-6 Outputs
const struct pios_servo_cfg pios_servo_cfg_out_in = {
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
    .num_channels = PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN,
};


/*
 * PWM Inputs
 * TIM1, TIM3
 */
#if defined(PIOS_INCLUDE_PWM) || defined(PIOS_INCLUDE_PPM)
#include <pios_pwm_priv.h>
static const struct pios_tim_channel pios_tim_rcvrport_all_channels[] = {
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 3, B, 0 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM3, 4, B, 1 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 1, E, 9 ),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 2, E, 11),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 3, E, 13),
    TIM_SERVO_CHANNEL_CONFIG(TIM1, 4, E, 14),
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
        .TIM_Channel     = TIM_Channel_1,
    },
    /* Use only the second channel for ppm */
    .channels     = &pios_tim_servoport_all_pins[PIOS_SERVOPORT_ALL_PINS_PWMOUT_IN - 1],
    .num_channels = 1,
};

#endif // PPM

#if defined(PIOS_INCLUDE_GCSRCVR)
#include "pios_gcsrcvr_priv.h"
#endif /* PIOS_INCLUDE_GCSRCVR */

#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"
#endif /* PIOS_INCLUDE_RCVR */

#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_revr_cfg = {
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = OTG_FS_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .vsense                                    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_25MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_UP,
        },
    },
    .vsense_active_low                         = false
};


const struct pios_usb_cfg *PIOS_BOARD_HW_DEFS_GetUsbCfg(uint32_t board_revision)
{
    switch (board_revision) {
    default:
        return &pios_usb_main_revr_cfg;

        break;
    }
    return NULL;
}

#include "pios_usb_board_data_priv.h"
#include "pios_usb_desc_hid_cdc_priv.h"
#include "pios_usb_desc_hid_only_priv.h"
#include "pios_usbhook.h"

#endif /* PIOS_INCLUDE_USB */

#if defined(PIOS_INCLUDE_COM_MSG)

#include <pios_com_msg_priv.h>

#endif /* PIOS_INCLUDE_COM_MSG */

#if defined(PIOS_INCLUDE_USB_HID) && !defined(PIOS_INCLUDE_USB_CDC)
#include <pios_usb_hid_priv.h>

const struct pios_usb_hid_cfg pios_usb_hid_cfg = {
    .data_if    = 0,
    .data_rx_ep = 1,
    .data_tx_ep = 1,
};
#endif /* PIOS_INCLUDE_USB_HID && !PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID) && defined(PIOS_INCLUDE_USB_CDC)
#include <pios_usb_cdc_priv.h>

const struct pios_usb_cdc_cfg pios_usb_cdc_cfg = {
    .ctrl_if    = 0,
    .ctrl_tx_ep = 2,

    .data_if    = 1,
    .data_rx_ep = 3,
    .data_tx_ep = 3,
};

#include <pios_usb_hid_priv.h>

const struct pios_usb_hid_cfg pios_usb_hid_cfg = {
    .data_if    = 2,
    .data_rx_ep = 1,
    .data_tx_ep = 1,
};
#endif /* PIOS_INCLUDE_USB_HID && PIOS_INCLUDE_USB_CDC */

#ifdef PIOS_INCLUDE_WS2811
#include <pios_ws2811_cfg.h>
#include <hwsettings.h>
#define PIOS_WS2811_TIM_DIVIDER (PIOS_PERIPHERAL_APB2_CLOCK / (800000 * PIOS_WS2811_TIM_PERIOD))

void DMA2_Stream1_IRQHandler(void) __attribute__((alias("PIOS_WS2811_irq_handler")));
// list of pin configurable as ws281x outputs.
// this will not clash with PWM in or servo output as
// pins will be reconfigured as _OUT so the alternate function is disabled.
const struct pios_ws2811_pin_cfg pios_ws2811_pin_cfg[] = {
    [HWSETTINGS_WS2811LED_OUT_SERVOOUT1] = {
        .gpio     = GPIOA,
        .gpioInit =                        {
            .GPIO_Pin   = GPIO_Pin_10,
            .GPIO_Speed = GPIO_Speed_25MHz,
            .GPIO_Mode  = GPIO_Mode_OUT,
            .GPIO_OType = GPIO_OType_PP,
        },
    },
};

const struct pios_ws2811_cfg pios_ws2811_cfg = {
    .timer     = TIM1,
    .timerInit = {
        .TIM_Prescaler         = PIOS_WS2811_TIM_DIVIDER - 1,
        .TIM_ClockDivision     = TIM_CKD_DIV1,
        .TIM_CounterMode       = TIM_CounterMode_Up,
        // period (1.25 uS per period
        .TIM_Period                            = PIOS_WS2811_TIM_PERIOD,
        .TIM_RepetitionCounter = 0x0000,
    },

    .timerCh1     = 1,
    .streamCh1    = DMA2_Stream1,
    .timerCh2     = 3,
    .streamCh2    = DMA2_Stream6,
    .streamUpdate = DMA2_Stream5,

    // DMA streamCh1, triggered by timerCh1 pwm signal.
    // if FrameBuffer indicates, reset output value early to indicate "0" bit to ws2812
    .dmaInitCh1 = PIOS_WS2811_DMA_CH1_CONFIG(DMA_Channel_6),
    .dmaItCh1   = DMA_IT_TEIF1 | DMA_IT_TCIF1,

    // DMA streamCh2, triggered by timerCh2 pwm signal.
    // Reset output value late to indicate "1" bit to ws2812.
    .dmaInitCh2 = PIOS_WS2811_DMA_CH2_CONFIG(DMA_Channel_6),
    .dmaItCh2   = DMA_IT_TEIF6 | DMA_IT_TCIF6,

    // DMA streamUpdate Triggered by timer update event
    // Outputs a high logic level at beginning of a cycle
    .dmaInitUpdate = PIOS_WS2811_DMA_UPDATE_CONFIG(DMA_Channel_6),
    .dmaItUpdate   = DMA_IT_TEIF5 | DMA_IT_TCIF5,
    .dmaSource     = TIM_DMA_CC1 | TIM_DMA_CC3 | TIM_DMA_Update,

    // DMAInitCh1 interrupt vector, used to block timer at end of framebuffer transfer
    .irq                                       = {
        .flags = (DMA_IT_TCIF1),
        .init  = {
            .NVIC_IRQChannel    = DMA2_Stream1_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

void PIOS_WS2811_irq_handler(void)
{
    PIOS_WS2811_DMA_irq_handler();
}
#endif // PIOS_INCLUDE_WS2811

#ifdef PIOS_INCLUDE_FLASH_OBJLIST
#include <pios_flashfs_objlist.h>
struct flashfs_cfg flash_main_fs_cfg = {
    .table_magic     = 0x01020304,
    .obj_magic       = 0x19293949,
    .obj_table_start = 0x00000010,
    .obj_table_end   = 0x1000,
    .sector_size     = 0x100,
    .chip_size       = 0x10000
};
#endif /* PIOS_INCLUDE_FLASH_OBJLIST */

#ifdef PIOS_INCLUDE_FLASH_EEPROM
#include <pios_flash_eeprom.h>
struct pios_flash_eeprom_cfg flash_main_chip_cfg = {
    .page_len   = 128,
    .total_size = 0x10000,
};
// .i2c_address = 0x50,
#endif /*  PIOS_INCLUDE_FLASH_EEPROM */
