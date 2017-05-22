/**
 ******************************************************************************
 *
 * @file       pios_ws2811_cfg.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      A driver for ws2811 rgb led controller.
 *             this is a port of the CleanFlight/BetaFlight implementation.
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
#ifndef PIOS_WS2811_CFG_H_
#define PIOS_WS2811_CFG_H_


struct pios_ws2811_cfg {
    TIM_TypeDef *timer;
    uint8_t     timer_chan;

    struct stm32_gpio pin;
    uint32_t    remap;


    uint16_t    timer_dma_source;

    DMA_Channel_TypeDef *dma_chan;

    uint32_t dma_tcif;

    uint8_t  dma_irqn;
};


#define PIOS_WS2811_CONFIG(_timer, _channel, _gpio, _pin) \
    {                                                     \
        .timer = _timer,                                  \
        .timer_chan = TIM_Channel_##_channel,             \
        .pin   = {                                        \
            .gpio = GPIO##_gpio,                          \
            .init = {                                     \
                .GPIO_Pin   = GPIO_Pin_##_pin,            \
                .GPIO_Speed = GPIO_Speed_2MHz,            \
                .GPIO_Mode  = GPIO_Mode_AF,               \
                .GPIO_OType = GPIO_OType_PP,              \
                .GPIO_PuPd  = GPIO_PuPd_UP                \
            },                                            \
            .pin_source     = GPIO_PinSource##_pin,       \
        },                                                \
        .remap    = GPIO_AF_P##_gpio##_pin##_##_timer, \
        .timer_dma_source = TIM_DMA_CC##_channel, \
        .dma_chan = _EVAL4(DMA, _timer##_CH##_channel##_DMA_INSTANCE, _Channel, _timer##_CH##_channel##_DMA_CHANNEL), \
        .dma_tcif = _EVAL4(DMA, _timer##_CH##_channel##_DMA_INSTANCE, _IT_TC, _timer##_CH##_channel##_DMA_CHANNEL), \
        .dma_irqn = _EVAL5(DMA, _timer##_CH##_channel##_DMA_INSTANCE, _Channel, _timer##_CH##_channel##_DMA_CHANNEL, _IRQn), \
    }

#define DMA0_Channel0      0
#define DMA0_IT_TC0        0
#define DMA0_Channel0_IRQn 0

void PIOS_WS2811_Init(uint32_t *dev_id, const struct pios_ws2811_cfg *ws2811_cfg);

void PIOS_WS2811_DMA_irq_handler();

#endif /* PIOS_WS2811_CFG_H_ */
