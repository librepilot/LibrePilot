/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_ADC ADC Functions
 * @brief STM32F30x ADC PIOS interface
 * @{
 *
 * @file       pios_adc.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      Analog to Digital conversion routines
 * @see        The GNU Public License (GPL) Version 3
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

#ifdef PIOS_INCLUDE_ADC

#include <pios_adc_priv.h>
#include <pios_adc.h>

#if !defined(PIOS_ADC_MAX_SAMPLES)
#define PIOS_ADC_MAX_SAMPLES      0
#endif

#if !defined(PIOS_ADC_MAX_OVERSAMPLING)
#define PIOS_ADC_MAX_OVERSAMPLING 0
#endif

#if !defined(PIOS_ADC_USE_ADC2)
#define PIOS_ADC_USE_ADC2         0
#endif

#if !defined(PIOS_ADC_NUM_CHANNELS)
#define PIOS_ADC_NUM_CHANNELS     0
#endif

struct pios_adc_pin_config {
    GPIO_TypeDef *port;
    uint32_t     pin;
    uint32_t     channel;
    bool initialize;
};

static const struct pios_adc_pin_config config[] = PIOS_DMA_PIN_CONFIG;
#define PIOS_ADC_NUM_PINS        (sizeof(config) / sizeof(config[0]))

#define PIOS_ADC_DMA_BUFFER_SIZE (PIOS_ADC_MAX_SAMPLES * PIOS_ADC_NUM_PINS)

// Private types
enum pios_adc_dev_magic {
    PIOS_ADC_DEV_MAGIC = 0x58375124,
};

struct adc_accumulator {
    uint32_t accumulator;
    uint32_t count;
};

struct pios_adc_dev {
    const struct pios_adc_cfg *cfg;
    ADCCallback  callback_function;
#if defined(PIOS_INCLUDE_FREERTOS)
    xQueueHandle data_queue;
#endif
    enum pios_adc_dev_magic magic;
    volatile uint16_t raw_data_buffer[PIOS_ADC_DMA_BUFFER_SIZE]  __attribute__((aligned(4))); // Double buffer that DMA just used
    struct adc_accumulator  accumulator[PIOS_ADC_NUM_PINS];
};

struct pios_adc_dev *pios_adc_dev;

// Private functions
void PIOS_ADC_downsample_data();
static struct pios_adc_dev *PIOS_ADC_Allocate();
static bool PIOS_ADC_validate(struct pios_adc_dev *);

static void init_pins(struct pios_adc_dev *adc_dev);
static void init_dma(struct pios_adc_dev *adc_dev);
static void init_adc(struct pios_adc_dev *adc_dev);

static void init_pins(__attribute__((unused)) struct pios_adc_dev *adc_dev)
{
    for (uint32_t i = 0; i < PIOS_ADC_NUM_PINS; ++i) {
        if (!config[i].initialize) {
            continue;
        }
        PIOS_ADC_PinSetup(i);
    }
}

static void init_dma(struct pios_adc_dev *adc_dev)
{
    /* Disable interrupts */
    DMA_ITConfig(pios_adc_dev->cfg->dma.rx.channel, pios_adc_dev->cfg->dma.irq.flags, DISABLE);

    /* Configure DMA channel */
    DMA_DeInit(adc_dev->cfg->dma.rx.channel);
    DMA_InitTypeDef DMAInit = adc_dev->cfg->dma.rx.init;
    DMAInit.DMA_PeripheralBaseAddr = (uint32_t)&adc_dev->cfg->adc_dev->DR;

    DMAInit.DMA_MemoryBaseAddr     = (uint32_t)&pios_adc_dev->raw_data_buffer[0];
    DMAInit.DMA_BufferSize         = PIOS_ADC_DMA_BUFFER_SIZE;
    DMAInit.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMAInit.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMAInit.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMAInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMAInit.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    DMAInit.DMA_Mode = DMA_Mode_Circular;
    DMAInit.DMA_M2M  = DMA_M2M_Disable;

    DMA_Init(adc_dev->cfg->dma.rx.channel, &DMAInit); /* channel is actually stream ... */

    /* enable DMA */
    DMA_Cmd(adc_dev->cfg->dma.rx.channel, ENABLE);

    /* Trigger interrupt when for half conversions too to indicate double buffer */
    DMA_ITConfig(adc_dev->cfg->dma.rx.channel, DMA_IT_TC, ENABLE);
    DMA_ITConfig(adc_dev->cfg->dma.rx.channel, DMA_IT_HT, ENABLE);

    /* Configure DMA interrupt */
    NVIC_InitTypeDef NVICInit = adc_dev->cfg->dma.irq.init;
    NVIC_Init(&NVICInit);
}

static void init_adc(struct pios_adc_dev *adc_dev)
{
    ADC_DeInit(adc_dev->cfg->adc_dev);

    if (adc_dev->cfg->adc_dev == ADC1 || adc_dev->cfg->adc_dev == ADC2) {
        RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div32);
    } else {
        RCC_ADCCLKConfig(RCC_ADC34PLLCLK_Div32);
    }

    ADC_VoltageRegulatorCmd(adc_dev->cfg->adc_dev, ENABLE);
    PIOS_DELAY_WaituS(10);
    ADC_SelectCalibrationMode(adc_dev->cfg->adc_dev, ADC_CalibrationMode_Single);
    ADC_StartCalibration(adc_dev->cfg->adc_dev);
    while (ADC_GetCalibrationStatus(adc_dev->cfg->adc_dev) != RESET) {
        ;
    }

    /* Do common ADC init */
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_CommonStructInit(&ADC_CommonInitStructure);

    ADC_CommonInitStructure.ADC_Mode    = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;

    ADC_CommonInitStructure.ADC_Clock   = ADC_Clock_AsynClkMode;
    ADC_CommonInitStructure.ADC_DMAMode = ADC_DMAMode_Circular;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = 0;
    ADC_DMAConfig(adc_dev->cfg->adc_dev, ADC_DMAMode_Circular);
    ADC_CommonInit(adc_dev->cfg->adc_dev, &ADC_CommonInitStructure);

    ADC_InitTypeDef ADC_InitStructure;
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ContinuousConvMode    = ADC_ContinuousConvMode_Enable;
    ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_0;
    ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;

    ADC_InitStructure.ADC_NbrOfRegChannel = ((PIOS_ADC_NUM_PINS) /* >> 1*/);

    ADC_Init(adc_dev->cfg->adc_dev, &ADC_InitStructure);

    /* Enable DMA request */
    ADC_DMACmd(adc_dev->cfg->adc_dev, ENABLE);

    /* Configure input scan */

    for (uint32_t i = 0; i < PIOS_ADC_NUM_PINS; i++) {
        ADC_RegularChannelConfig(adc_dev->cfg->adc_dev,
                                 config[i].channel,
                                 i + 1,
                                 ADC_SampleTime_61Cycles5); /* XXX this is totally arbitrary... */
    }

    ADC_Cmd(adc_dev->cfg->adc_dev, ENABLE);

    while (!ADC_GetFlagStatus(adc_dev->cfg->adc_dev, ADC_FLAG_RDY)) {
        ;
    }

    ADC_StartConversion(adc_dev->cfg->adc_dev);
}

static bool PIOS_ADC_validate(struct pios_adc_dev *dev)
{
    if (dev == NULL) {
        return false;
    }

    return dev->magic == PIOS_ADC_DEV_MAGIC;
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_adc_dev *PIOS_ADC_Allocate()
{
    struct pios_adc_dev *adc_dev;

    adc_dev = (struct pios_adc_dev *)pios_malloc(sizeof(*adc_dev));
    if (!adc_dev) {
        return NULL;
    }

    memset(adc_dev, 0, sizeof(*adc_dev));

    adc_dev->magic = PIOS_ADC_DEV_MAGIC;
    return adc_dev;
}
#else
#error Not implemented
static struct pios_adc_dev *PIOS_ADC_Allocate()
{
    return (struct pios_adc_dev *)NULL;
}
#endif

/**
 * @brief Init the ADC.
 */
int32_t PIOS_ADC_Init(const struct pios_adc_cfg *cfg)
{
    PIOS_Assert(cfg);

    pios_adc_dev = PIOS_ADC_Allocate();
    if (pios_adc_dev == NULL) {
        return -1;
    }

    pios_adc_dev->cfg = cfg;
    pios_adc_dev->callback_function = NULL;

#if defined(PIOS_INCLUDE_FREERTOS)
    pios_adc_dev->data_queue = NULL;
#endif

    init_pins(pios_adc_dev);
    init_dma(pios_adc_dev);
    init_adc(pios_adc_dev);

    return 0;
}

/**
 * @brief Configure the ADC to run at a fixed oversampling
 * @param[in] oversampling the amount of oversampling to run at
 */
void PIOS_ADC_Config(__attribute__((unused)) uint32_t oversampling)
{
    /* we ignore this */
}

/**
 * Returns value of an ADC Pin
 * @param[in] pin number
 * @return ADC pin value averaged over the set of samples since the last reading.
 * @return -1 if pin doesn't exist
 * @return -2 if no data acquired since last read
 */
int32_t last_conv_value;
int32_t PIOS_ADC_PinGet(uint32_t pin)
{
    int32_t result;

    /* Check if pin exists */
    if (pin >= PIOS_ADC_NUM_PINS) {
        return -1;
    }

    if (pios_adc_dev->accumulator[pin].accumulator <= 0) {
        return -2;
    }

    /* return accumulated result and clear accumulator */
    result = pios_adc_dev->accumulator[pin].accumulator / (pios_adc_dev->accumulator[pin].count ? : 1);
    pios_adc_dev->accumulator[pin].accumulator = result;
    pios_adc_dev->accumulator[pin].count = 1;

    return result;
}

float PIOS_ADC_PinGetVolt(uint32_t pin)
{
    return ((float)PIOS_ADC_PinGet(pin)) * PIOS_ADC_VOLTAGE_SCALE;
}

/**
 * @brief Set a callback function that is executed whenever
 * the ADC double buffer swaps
 * @note Not currently supported.
 */
void PIOS_ADC_SetCallback(ADCCallback new_function)
{
    pios_adc_dev->callback_function = new_function;
}

#if defined(PIOS_INCLUDE_FREERTOS)
/**
 * @brief Register a queue to add data to when downsampled
 * @note Not currently supported.
 */
void PIOS_ADC_SetQueue(xQueueHandle data_queue)
{
    pios_adc_dev->data_queue = data_queue;
}
#endif

/**
 * @brief Return the address of the downsampled data buffer
 * @note Not currently supported.
 */
float *PIOS_ADC_GetBuffer(void)
{
    return NULL;
}

/**
 * @brief Return the address of the raw data data buffer
 * @note Not currently supported.
 */
int16_t *PIOS_ADC_GetRawBuffer(void)
{
    return NULL;
}

/**
 * @brief Return the amount of over sampling
 * @note Not currently supported (always returns 1)
 */
uint8_t PIOS_ADC_GetOverSampling(void)
{
    return 1;
}

/**
 * @brief Set the fir coefficients.  Takes as many samples as the
 * current filter order plus one (normalization)
 *
 * @param new_filter Array of adc_oversampling floats plus one for the
 * filter coefficients
 * @note Not currently supported.
 */
void PIOS_ADC_SetFIRCoefficients(__attribute__((unused)) float *new_filter)
{
    // not implemented
}

/**
 * @brief accumulate the data for each of the channels.
 */
void accumulate(struct pios_adc_dev *dev, volatile uint16_t *buffer)
{
    volatile uint16_t *sp = buffer;

    /*
     * Accumulate sampled values.
     */

    int count = (PIOS_ADC_MAX_SAMPLES / 2);

    while (count--) {
        for (uint32_t i = 0; i < PIOS_ADC_NUM_PINS; ++i) {
            dev->accumulator[i].accumulator += *sp++;
            dev->accumulator[i].count++;
            /*
             * If the accumulator reaches half-full, rescale in order to
             * make more space.
             */
            if (dev->accumulator[i].accumulator >= (((uint32_t)1) << 31)) {
                dev->accumulator[i].accumulator /= 2;
                dev->accumulator[i].count /= 2;
            }
        }
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    // XXX should do something with this
    if (pios_adc_dev->data_queue) {
        static portBASE_TYPE xHigherPriorityTaskWoken;
// xQueueSendFromISR(pios_adc_dev->data_queue, pios_adc_dev->downsampled_buffer, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

#endif

// if(pios_adc_dev->callback_function)
// pios_adc_dev->callback_function(pios_adc_dev->downsampled_buffer);
}

/**
 * @brief Interrupt on buffer flip.
 *
 * The hardware is done with the 'other' buffer, so we can pass it to the accumulator.
 */
void PIOS_ADC_DMA_Handler(void)
{
    if (!PIOS_ADC_validate(pios_adc_dev)) {
        return;
    }

    if (DMA_GetFlagStatus(pios_adc_dev->cfg->full_flag)) { // whole double buffer filled
        DMA_ClearFlag(pios_adc_dev->cfg->full_flag);
        accumulate(pios_adc_dev, &pios_adc_dev->raw_data_buffer[PIOS_ADC_DMA_BUFFER_SIZE / 2]);
    } else if (DMA_GetFlagStatus(pios_adc_dev->cfg->half_flag)) {
        DMA_ClearFlag(pios_adc_dev->cfg->half_flag);
        accumulate(pios_adc_dev, &pios_adc_dev->raw_data_buffer[0]);
    } else {
        // This should not happen, probably due to transfer errors
        DMA_ClearFlag(pios_adc_dev->cfg->dma.irq.flags);
    }
}

void PIOS_ADC_PinSetup(uint32_t pin)
{
    if (config[pin].port != NULL && pin < PIOS_ADC_NUM_PINS) {
        /* Setup analog pin */
        GPIO_InitTypeDef GPIO_InitStructure;

        GPIO_StructInit(&GPIO_InitStructure);
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
        GPIO_InitStructure.GPIO_Pin   = config[pin].pin;
        GPIO_Init(config[pin].port, &GPIO_InitStructure);
    }
}
#endif /* PIOS_INCLUDE_ADC */

/**
 * @}
 * @}
 */
