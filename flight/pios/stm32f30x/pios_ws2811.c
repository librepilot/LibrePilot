/**
 ******************************************************************************
 *
 * @file       pios_ws2811.c
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

#include "pios.h"
#include "pios_ws2811.h"
#include "pios_ws2811_cfg.h"

#define WS2811_BITS_PER_LED        24
// for 50us delay
#define WS2811_DELAY_BUFFER_LENGTH 42

#define WS2811_DATA_BUFFER_SIZE    (WS2811_BITS_PER_LED * PIOS_WS2811_NUMLEDS)
// number of bytes needed is #LEDs * 24 bytes + 42 trailing bytes)
#define WS2811_DMA_BUFFER_SIZE     (WS2811_DATA_BUFFER_SIZE + WS2811_DELAY_BUFFER_LENGTH)

#define WS2811_TIMER_HZ            24000000
#define WS2811_TIMER_PERIOD        29
// timer compare value for logical 1
#define BIT_COMPARE_1              17
// timer compare value for logical 0
#define BIT_COMPARE_0              9

#define PIOS_WS2811_MAGIC          0x00281100

struct pios_ws2811_dev {
    uint32_t magic;
    const struct pios_ws2811_cfg *config;
    uint8_t  dma_buffer[WS2811_DMA_BUFFER_SIZE];
    bool     dma_active;
};

struct pios_ws2811_dev *ws2811_dev;

void PIOS_WS2811_Init(uint32_t *dev_id, const struct pios_ws2811_cfg *ws2811_cfg)
{
    if (ws2811_dev) {
        return;
    }

    ws2811_dev = (struct pios_ws2811_dev *)pios_malloc(sizeof(*ws2811_dev));

    PIOS_Assert(ws2811_dev);

    memset(ws2811_dev, 0, sizeof(*ws2811_dev));

    ws2811_dev->magic  = PIOS_WS2811_MAGIC;
    ws2811_dev->config = ws2811_cfg;


    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    GPIO_Init(ws2811_cfg->pin.gpio, (GPIO_InitTypeDef *)&ws2811_cfg->pin.init);

    GPIO_PinAFConfig(ws2811_cfg->pin.gpio, ws2811_cfg->pin.pin_source, ws2811_cfg->remap);

    /* Compute the prescaler value */
    uint16_t prescalerValue = (uint16_t)(SystemCoreClock / WS2811_TIMER_HZ) - 1;
    /* Time base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period        = WS2811_TIMER_PERIOD; // 800kHz
    TIM_TimeBaseStructure.TIM_Prescaler     = prescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(ws2811_cfg->timer, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel1 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;

    switch (ws2811_cfg->timer_chan) {
    case TIM_Channel_1:
        TIM_OC1Init(ws2811_cfg->timer, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
        break;
    case TIM_Channel_2:
        TIM_OC2Init(ws2811_cfg->timer, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
        break;
    case TIM_Channel_3:
        TIM_OC3Init(ws2811_cfg->timer, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
        break;
    case TIM_Channel_4:
        TIM_OC4Init(ws2811_cfg->timer, &TIM_OCInitStructure);
        TIM_OC4PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
        break;
    }


    TIM_CtrlPWMOutputs(ws2811_cfg->timer, ENABLE);

    /* configure DMA */
    // NVIC setup here
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel    = ws2811_cfg->dma_irqn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    DMA_DeInit(ws2811_cfg->dma_chan);

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ws2811_cfg->timer->CCR1 + ws2811_cfg->timer_chan;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)ws2811_dev->dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = WS2811_DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode     = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(ws2811_cfg->dma_chan, &DMA_InitStructure);

    TIM_DMACmd(ws2811_cfg->timer, ws2811_cfg->timer_dma_source, ENABLE);

    DMA_ITConfig(ws2811_cfg->dma_chan, DMA_IT_TC, ENABLE);

    *dev_id = (uint32_t)ws2811_dev;
}

void PIOS_WS2811_DMA_irq_handler()
{
    if (!ws2811_dev) {
        return;
    }

    if (DMA_GetITStatus(ws2811_dev->config->dma_tcif)) {
        ws2811_dev->dma_active = false;
        DMA_Cmd(ws2811_dev->config->dma_chan, DISABLE);

        DMA_ClearITPendingBit(ws2811_dev->config->dma_tcif);
    }
}


/**
 * Set a led color
 * @param c color
 * @param led led number
 * @param update Perform an update after changing led color
 */
void PIOS_WS2811_setColorRGB(Color_t c, uint8_t led, bool update)
{
    if (!ws2811_dev || (led >= PIOS_WS2811_NUMLEDS)) {
        return;
    }

    int offset   = led * WS2811_BITS_PER_LED;

    uint32_t grb = (c.G << 16) | (c.R << 8) | (c.B);

    for (int bit = (WS2811_BITS_PER_LED - 1); bit >= 0; --bit) {
        ws2811_dev->dma_buffer[offset++] = (grb & (1 << bit)) ? BIT_COMPARE_1 : BIT_COMPARE_0;
    }

    if (update) {
        PIOS_WS2811_Update();
    }
}

void PIOS_WS2811_Update()
{
    if (!ws2811_dev || ws2811_dev->dma_active) {
        return;
    }

    ws2811_dev->dma_active = true;

    DMA_SetCurrDataCounter(ws2811_dev->config->dma_chan, WS2811_DMA_BUFFER_SIZE); // load number of bytes to be transferred
    TIM_SetCounter(ws2811_dev->config->timer, 0);
    TIM_Cmd(ws2811_dev->config->timer, ENABLE);

    DMA_Cmd(ws2811_dev->config->dma_chan, ENABLE);
}
