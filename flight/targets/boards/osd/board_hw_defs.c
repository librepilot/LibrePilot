/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 *
 * @file       board_hw_defs.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Defines board specific static initializers for hardware for the OpenPilot board.
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

#if defined(PIOS_INCLUDE_LED)

#include <pios_led_priv.h>
static const struct pios_gpio pios_leds[] = {
    [PIOS_LED_HEARTBEAT] = {
        .pin                =             {
            .gpio = GPIOB,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_0,
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
            .gpio = GPIOC,
            .init =             {
                .GPIO_Pin   = GPIO_Pin_5,
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


#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"
#include "pios_flash_internal_priv.h"

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

#include "pios_flash.h"

#endif /* PIOS_INCLUDE_FLASH */


#if defined(PIOS_INCLUDE_SPI)

#include <pios_spi_priv.h>

/* MicroSD Interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
void PIOS_SPI_sdcard_irq_handler(void);
void DMA1_Channel2_IRQHandler() __attribute__((alias("PIOS_SPI_sdcard_irq_handler")));
void DMA1_Channel3_IRQHandler() __attribute__((alias("PIOS_SPI_sdcard_irq_handler")));
static const struct pios_spi_cfg pios_spi_sdcard_cfg = {
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
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16, /* Maximum divider (ie. slowest clock rate) */
    },
    .use_crc = false,
    .dma     = {
        .irq                                       = {
            .flags = (DMA_IT_TCIF3 | DMA_IT_TEIF3 | DMA_IT_HTIF3 | DMA_IT_TCIF3),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Stream3_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
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
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .miso                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_14,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .mosi                                          = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
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

static uint32_t pios_spi_sdcard_id;
void PIOS_SPI_sdcard_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_SPI_IRQ_Handler(pios_spi_sdcard_id);
}

#endif /* if defined(PIOS_INCLUDE_SPI) */


#include <pios_usart_priv.h>

#if defined(PIOS_INCLUDE_GPS)

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
 * AUX USART
 */
static const struct pios_usart_cfg pios_usart_aux_cfg = {
    .regs  = USART2,
    .remap = GPIO_AF_USART2,
    .rx    = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_3,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
    .tx                 = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_2,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode  = GPIO_Mode_AF,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_UP
        },
    },
};

#endif /* PIOS_COM_AUX */

#ifdef PIOS_INCLUDE_COM_TELEM
/*
 * Telemetry on main USART
 */
static const struct pios_usart_cfg pios_usart_telem_main_cfg = {
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

#endif /* PIOS_COM_TELEM */

#if defined(PIOS_INCLUDE_COM)

#include <pios_com_priv.h>

#endif /* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

/*
 * I2C Adapters
 */
void PIOS_I2C_flexiport_adapter_ev_irq_handler(void);
void PIOS_I2C_flexiport_adapter_er_irq_handler(void);
void I2C2_EV_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_ev_irq_handler")));
void I2C2_ER_IRQHandler() __attribute__((alias("PIOS_I2C_flexiport_adapter_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_flexiport_adapter_cfg = {
    .regs     = I2C2,
    .remapSDA = GPIO_AF_I2C2,
    .remapSCL = GPIO_AF_I2C2,
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
#endif /* if defined(PIOS_INCLUDE_I2C) */

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
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_15,
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


#if defined(PIOS_INCLUDE_VIDEO)

static const TIM_TimeBaseInitTypeDef tim_8_time_base = {
    .TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_CLOCK / 1000000) - 1,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = ((1000000 / 50000) - 1),
    .TIM_RepetitionCounter = 0x0000,
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

/**
 * Pios servo configuration structures
 */
#include <pios_servo_priv.h>
static const struct pios_tim_channel pios_tim_servoport_all_pins[] = {
    {
        .timer = TIM8,
        .timer_chan = TIM_Channel_3,
        .pin   = {
            .gpio = GPIOC,
            .init = {
                .GPIO_Pin   = GPIO_Pin_8,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource8,
        },
        .remap = GPIO_AF_TIM8,
    },
    {
        .timer = TIM8,
        .timer_chan = TIM_Channel_4,
        .pin   = {
            .gpio = GPIOC,
            .init = {
                .GPIO_Pin   = GPIO_Pin_9,
                .GPIO_Speed = GPIO_Speed_2MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source     = GPIO_PinSource9,
        },
        .remap = GPIO_AF_TIM8,
    },
};

const struct pios_servo_cfg pios_servo_cfg = {
    .tim_oc_init          = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_Pulse        = 0,
        .TIM_OCPolarity   = TIM_OCPolarity_High,
        .TIM_OCNPolarity  = TIM_OCPolarity_High,
        .TIM_OCIdleState  = TIM_OCIdleState_Reset,
        .TIM_OCNIdleState = TIM_OCNIdleState_Reset,
    },
    .channels     = pios_tim_servoport_all_pins,
    .num_channels = NELEMENTS(pios_tim_servoport_all_pins),
};


#include <pios_video.h>
static const struct pios_exti_cfg pios_exti_hsync_cfg __exti_config = {
    .vector = PIOS_Hsync_ISR,
    .line   = EXTI_Line7,
    .pin    = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_7,
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
            .EXTI_Line    = EXTI_Line7, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            // .EXTI_Trigger = EXTI_Trigger_Rising_Falling,
            .EXTI_Trigger = EXTI_Trigger_Falling,
            // .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};
static const struct pios_exti_cfg pios_exti_vsync_cfg __exti_config = {
    .vector = PIOS_Vsync_ISR,
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
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line5, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Falling,
            .EXTI_LineCmd = ENABLE,
        },
    },
};


static const struct pios_video_cfg pios_video_cfg = {
    .mask                                              = {
        .regs  = SPI3,
        .remap = GPIO_AF_SPI3,
        .init  = {
            .SPI_Mode              = SPI_Mode_Slave,
            .SPI_Direction         = SPI_Direction_1Line_Tx,
            .SPI_DataSize          = SPI_DataSize_8b,
            .SPI_NSS                                   = SPI_NSS_Soft,
            .SPI_FirstBit          = SPI_FirstBit_MSB,
            .SPI_CRCPolynomial     = 7,
            .SPI_CPOL              = SPI_CPOL_Low,
            .SPI_CPHA              = SPI_CPHA_2Edge,
            .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
        },
        .use_crc = false,
        .dma     = {
            .irq                                       = {
                // Note this is the stream ID that triggers interrupts (in this case RX)
                .flags = (DMA_IT_TCIF7),
                .init  = {
                    .NVIC_IRQChannel    = DMA1_Stream7_IRQn,
                    .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
                    .NVIC_IRQChannelSubPriority        = 0,
                    .NVIC_IRQChannelCmd = ENABLE,
                },
            },
            /*.rx = {},*/
            .tx                                        = {
                .channel = DMA1_Stream7,
                .init    = {
                    .DMA_Channel            = DMA_Channel_0,
                    .DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR),
                    .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                    .DMA_BufferSize         = BUFFER_LINE_LENGTH,
                    .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                    .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                    .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
                    .DMA_Mode               = DMA_Mode_Normal,
                    .DMA_Priority           = DMA_Priority_VeryHigh,
                    .DMA_FIFOMode           = DMA_FIFOMode_Enable,
                    .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                    .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
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
        /*.mosi = {},*/
        .slave_count                                   = 1,
    },
    .level                                             = {
        .regs  = SPI1,
        .remap = GPIO_AF_SPI1,
        .init  = {
            .SPI_Mode              = SPI_Mode_Slave,
            .SPI_Direction         = SPI_Direction_1Line_Tx,
            .SPI_DataSize          = SPI_DataSize_8b,
            .SPI_NSS                                   = SPI_NSS_Soft,
            .SPI_FirstBit          = SPI_FirstBit_MSB,
            .SPI_CRCPolynomial     = 7,
            .SPI_CPOL              = SPI_CPOL_Low,
            .SPI_CPHA              = SPI_CPHA_2Edge,
            .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
        },
        .use_crc = false,
        .dma     = {
            .irq                                       = {
                .flags = (DMA_IT_TCIF5),
                .init  = {
                    .NVIC_IRQChannel    = DMA2_Stream5_IRQn,
                    .NVIC_IRQChannelPreemptionPriority = 0,
                    .NVIC_IRQChannelSubPriority        = 0,
                    .NVIC_IRQChannelCmd = ENABLE,
                },
            },
            /*.rx = {},*/
            .tx                                        = {
                .channel = DMA2_Stream5,
                .init    = {
                    .DMA_Channel            = DMA_Channel_3,
                    .DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
                    .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                    .DMA_BufferSize         = BUFFER_LINE_LENGTH,
                    .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                    .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                    .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
                    .DMA_Mode               = DMA_Mode_Normal,
                    .DMA_Priority           = DMA_Priority_VeryHigh,
                    .DMA_FIFOMode           = DMA_FIFOMode_Enable,
                    .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                    .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
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
        /*.mosi = {},*/
        .slave_count                                   = 1,
    },

    .hsync = &pios_exti_hsync_cfg,
    .vsync = &pios_exti_vsync_cfg,

    .pixel_timer                                       = {
        .timer = TIM4,
        .timer_chan                                    = TIM_Channel_1,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_6,
                .GPIO_Speed = GPIO_Speed_100MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source                                = GPIO_PinSource6,
        },
        .remap                                         = GPIO_AF_TIM4,
    },
    .hsync_capture                                     = {
        .timer = TIM4,
        .timer_chan                                    = TIM_Channel_2,
        .pin   = {
            .gpio = GPIOB,
            .init = {
                .GPIO_Pin   = GPIO_Pin_7,
                .GPIO_Speed = GPIO_Speed_100MHz,
                .GPIO_Mode  = GPIO_Mode_AF,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd  = GPIO_PuPd_UP
            },
            .pin_source                                = GPIO_PinSource7,
        },
        .remap                                         = GPIO_AF_TIM4,
    },
    .tim_oc_init                                       = {
        .TIM_OCMode       = TIM_OCMode_PWM1,
        .TIM_OutputState  = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_Pulse        = 1,
        .TIM_OCPolarity   = TIM_OCPolarity_High,
        .TIM_OCNPolarity  = TIM_OCPolarity_High,
        .TIM_OCIdleState  = TIM_OCIdleState_Reset,
        .TIM_OCNIdleState = TIM_OCNIdleState_Reset,
    },
};

#endif /* if defined(PIOS_INCLUDE_VIDEO) */

#if defined(PIOS_INCLUDE_WAVE)
#include <pios_wavplay.h>
#include <pios_tim_priv.h>

static const TIM_TimeBaseInitTypeDef tim_6_time_base = {
    .TIM_Prescaler         = 0,
    .TIM_ClockDivision     = TIM_CKD_DIV1,
    .TIM_CounterMode       = TIM_CounterMode_Up,
    .TIM_Period            = (PIOS_PERIPHERAL_APB1_CLOCK / 44100),
    .TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_6_cfg = {
    .timer = TIM6,
    .time_base_init                            = &tim_6_time_base,
    .irq   = {
        .init                                  = {
            .NVIC_IRQChannel    = TIM6_DAC_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
};

static const struct pios_dac_cfg pios_dac_cfg = {
    .timer                                         = TIM6,
    .time_base_init                                = {
        .TIM_Prescaler         = 0,
        .TIM_ClockDivision     = TIM_CKD_DIV1,
        .TIM_CounterMode       = TIM_CounterMode_Up,
        .TIM_Period                                = (PIOS_PERIPHERAL_APB1_CLOCK / 44100),
        .TIM_RepetitionCounter = 0x0000,
    },
    .irq                                           = {
        .init                                      = {
            .NVIC_IRQChannel    = TIM6_DAC_IRQn,
            .NVIC_IRQChannelPreemptionPriority     = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority            = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .dma                                           = {
        .irq                                       = {
            // Note this is the stream ID that triggers interrupts (in this case RX)
            .flags = (DMA_IT_TCIF5),
            .init  = {
                .NVIC_IRQChannel    = DMA1_Stream5_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },

        .rx                                        = {},
        .tx                                        = {
            .channel = DMA1_Stream5,
            .init    = {
                .DMA_Channel            = DMA_Channel_7,
                .DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR8R1,
                .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
                .DMA_BufferSize         = BUFFERSIZE,
                .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
                .DMA_MemoryInc          = DMA_MemoryInc_Enable,
                .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
                .DMA_Mode               = DMA_Mode_Circular,
                .DMA_Priority           = DMA_Priority_High,
                .DMA_FIFOMode           = DMA_FIFOMode_Disable,
                .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
            },
        },
    },
    .channel  = DAC_Channel_1,
    .dac_init = {
        .DAC_Trigger                               = DAC_Trigger_T6_TRGO,
        .DAC_WaveGeneration                        = DAC_WaveGeneration_None,
        .DAC_OutputBuffer                          = DAC_OutputBuffer_Enable,
    },
    .dac_io                                        = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin   = GPIO_Pin_4,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode  = GPIO_Mode_AN,
            .GPIO_OType = GPIO_OType_PP,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
};
#endif /* if defined(PIOS_INCLUDE_WAVE) */
