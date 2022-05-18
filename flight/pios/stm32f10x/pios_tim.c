#include "pios.h"

#ifdef PIOS_INCLUDE_TIM

#include "pios_tim_priv.h"

enum pios_tim_dev_magic {
    PIOS_TIM_DEV_MAGIC = 0x87654098,
};

struct pios_tim_dev {
    enum pios_tim_dev_magic magic;

    const struct pios_tim_channel   *channels;
    uint8_t num_channels;

    const struct pios_tim_callbacks *callbacks;
    uint32_t context;
};

#if 0
static bool PIOS_TIM_validate(struct pios_tim_dev *tim_dev)
{
    return tim_dev->magic == PIOS_TIM_DEV_MAGIC;
}
#endif

#if defined(PIOS_INCLUDE_FREERTOS) && 0
static struct pios_tim_dev *PIOS_TIM_alloc(void)
{
    struct pios_tim_dev *tim_dev;

    tim_dev = (struct pios_tim_dev *)pios_malloc(sizeof(*tim_dev));
    if (!tim_dev) {
        return NULL;
    }

    tim_dev->magic = PIOS_TIM_DEV_MAGIC;
    return tim_dev;
}
#else
static struct pios_tim_dev pios_tim_devs[PIOS_TIM_MAX_DEVS];
static uint8_t pios_tim_num_devs;
static struct pios_tim_dev *PIOS_TIM_alloc(void)
{
    struct pios_tim_dev *tim_dev;

    if (pios_tim_num_devs >= PIOS_TIM_MAX_DEVS) {
        return NULL;
    }

    tim_dev = &pios_tim_devs[pios_tim_num_devs++];
    tim_dev->magic = PIOS_TIM_DEV_MAGIC;

    return tim_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) && 0 */


int32_t PIOS_TIM_InitClock(const struct pios_tim_clock_cfg *cfg)
{
    PIOS_DEBUG_Assert(cfg);

    /* Enable appropriate clock to timer module */
    switch ((uint32_t)cfg->timer) {
    case (uint32_t)TIM1:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
        break;
    case (uint32_t)TIM2:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        break;
    case (uint32_t)TIM3:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
        break;
    case (uint32_t)TIM4:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        break;
#ifdef STM32F10X_HD
    case (uint32_t)TIM5:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
        break;
    case (uint32_t)TIM6:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
        break;
    case (uint32_t)TIM7:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
        break;
    case (uint32_t)TIM8:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
        break;
#endif
    }

    /* Configure the dividers for this timer */
    TIM_TimeBaseInit(cfg->timer, cfg->time_base_init);

    /* Configure internal timer clocks */
    TIM_InternalClockConfig(cfg->timer);

    /* Enable timers */
    TIM_Cmd(cfg->timer, ENABLE);

    /* Enable Interrupts */
    NVIC_Init(&cfg->irq.init);

    /* Advanced TIM1 has additional IRQ */
    if (cfg->timer == TIM1) {
        NVIC_InitTypeDef init = cfg->irq.init;
        init.NVIC_IRQChannel = TIM1_UP_IRQn;
        NVIC_Init(&init);
    }

    return 0;
}

int32_t PIOS_TIM_InitChannels(uint32_t *tim_id, const struct pios_tim_channel *channels, uint8_t num_channels, const struct pios_tim_callbacks *callbacks, uint32_t context)
{
    PIOS_Assert(channels);
    PIOS_Assert(num_channels);

    struct pios_tim_dev *tim_dev;
    tim_dev = (struct pios_tim_dev *)PIOS_TIM_alloc();
    if (!tim_dev) {
        goto out_fail;
    }

    /* Bind the configuration to the device instance */
    tim_dev->channels     = channels;
    tim_dev->num_channels = num_channels;
    tim_dev->callbacks    = callbacks;
    tim_dev->context = context;

    /* Configure the pins */
    for (uint8_t i = 0; i < num_channels; i++) {
        const struct pios_tim_channel *chan = &(channels[i]);

        /* Enable the peripheral clock for the GPIO */
        switch ((uint32_t)chan->pin.gpio) {
        case (uint32_t)GPIOA:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
            break;
        case (uint32_t)GPIOB:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
            break;
        case (uint32_t)GPIOC:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
            break;
        default:
            PIOS_Assert(0);
            break;
        }
        GPIO_Init(chan->pin.gpio, &chan->pin.init);

        if (chan->remap) {
            GPIO_PinRemapConfig(chan->remap, ENABLE);
        }
    }

    *tim_id = (uint32_t)tim_dev;

    return 0;

out_fail:
    return -1;
}

static void PIOS_TIM_generic_irq_handler(TIM_TypeDef *timer)
{
    /* Check for an overflow event on this timer */
    bool overflow_event     = 0;
    uint16_t overflow_count = false;

    if (TIM_GetITStatus(timer, TIM_IT_Update) == SET) {
        TIM_ClearITPendingBit(timer, TIM_IT_Update);
        overflow_count = timer->ARR;
        overflow_event = true;
    }

    /* Iterate over all registered clients of the TIM layer to find channels on this timer */
    for (uint8_t i = 0; i < pios_tim_num_devs; i++) {
        const struct pios_tim_dev *tim_dev = &pios_tim_devs[i];

        if (!tim_dev->channels || tim_dev->num_channels == 0) {
            /* No channels to process on this client */
            continue;
        }

        for (uint8_t j = 0; j < tim_dev->num_channels; j++) {
            const struct pios_tim_channel *chan = &tim_dev->channels[j];

            if (chan->timer != timer) {
                /* channel is not on this timer */
                continue;
            }

            /* Figure out which interrupt bit we should be looking at */
            uint16_t timer_it;
            switch (chan->timer_chan) {
            case TIM_Channel_1:
                timer_it = TIM_IT_CC1;
                break;
            case TIM_Channel_2:
                timer_it = TIM_IT_CC2;
                break;
            case TIM_Channel_3:
                timer_it = TIM_IT_CC3;
                break;
            case TIM_Channel_4:
                timer_it = TIM_IT_CC4;
                break;
            default:
                PIOS_Assert(0);
                break;
            }

            bool edge_event;
            uint16_t edge_count;
            if (TIM_GetITStatus(chan->timer, timer_it) == SET) {
                TIM_ClearITPendingBit(chan->timer, timer_it);

                /* Read the current counter */
                switch (chan->timer_chan) {
                case TIM_Channel_1:
                    edge_count = TIM_GetCapture1(chan->timer);
                    break;
                case TIM_Channel_2:
                    edge_count = TIM_GetCapture2(chan->timer);
                    break;
                case TIM_Channel_3:
                    edge_count = TIM_GetCapture3(chan->timer);
                    break;
                case TIM_Channel_4:
                    edge_count = TIM_GetCapture4(chan->timer);
                    break;
                default:
                    PIOS_Assert(0);
                    break;
                }
                edge_event = true;
            } else {
                edge_event = false;
                edge_count = 0;
            }

            if (!tim_dev->callbacks) {
                /* No callbacks registered, we're done with this channel */
                continue;
            }

            /* Generate the appropriate callbacks */
            if (overflow_event & edge_event) {
                /*
                 * When both edge and overflow happen in the same interrupt, we
                 * need a heuristic to determine the order of the edge and overflow
                 * events so that the callbacks happen in the right order.  If we
                 * get the order wrong, our pulse width calculations could be off by up
                 * to ARR ticks.  That could be bad.
                 *
                 * Heuristic: If the edge_count is < 32 ticks above zero then we assume the
                 *            edge happened just after the overflow.
                 */

                if (edge_count < 32) {
                    /* Call the overflow callback first */
                    if (tim_dev->callbacks->overflow) {
                        (*tim_dev->callbacks->overflow)((uint32_t)tim_dev,
                                                        tim_dev->context,
                                                        j,
                                                        overflow_count);
                    }
                    /* Call the edge callback second */
                    if (tim_dev->callbacks->edge) {
                        (*tim_dev->callbacks->edge)((uint32_t)tim_dev,
                                                    tim_dev->context,
                                                    j,
                                                    edge_count);
                    }
                } else {
                    /* Call the edge callback first */
                    if (tim_dev->callbacks->edge) {
                        (*tim_dev->callbacks->edge)((uint32_t)tim_dev,
                                                    tim_dev->context,
                                                    j,
                                                    edge_count);
                    }
                    /* Call the overflow callback second */
                    if (tim_dev->callbacks->overflow) {
                        (*tim_dev->callbacks->overflow)((uint32_t)tim_dev,
                                                        tim_dev->context,
                                                        j,
                                                        overflow_count);
                    }
                }
            } else if (overflow_event && tim_dev->callbacks->overflow) {
                (*tim_dev->callbacks->overflow)((uint32_t)tim_dev,
                                                tim_dev->context,
                                                j,
                                                overflow_count);
            } else if (edge_event && tim_dev->callbacks->edge) {
                (*tim_dev->callbacks->edge)((uint32_t)tim_dev,
                                            tim_dev->context,
                                            j,
                                            edge_count);
            }
        }
    }
}

/* Bind Interrupt Handlers
 *
 * Map all valid TIM IRQs to the common interrupt handler
 * and give it enough context to properly demux the various timers
 */
void TIM1_UP_IRQHandler(void) __attribute__((alias("PIOS_TIM_1_UP_irq_handler")));
static void PIOS_TIM_1_UP_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM1);
}

void TIM1_CC_IRQHandler(void) __attribute__((alias("PIOS_TIM_1_CC_irq_handler")));
static void PIOS_TIM_1_CC_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM1);
}

void TIM2_IRQHandler(void) __attribute__((alias("PIOS_TIM_2_irq_handler")));
static void PIOS_TIM_2_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM2);
}

void TIM3_IRQHandler(void) __attribute__((alias("PIOS_TIM_3_irq_handler")));
static void PIOS_TIM_3_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM3);
}

void TIM4_IRQHandler(void) __attribute__((alias("PIOS_TIM_4_irq_handler")));
static void PIOS_TIM_4_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM4);
}

void TIM5_IRQHandler(void) __attribute__((alias("PIOS_TIM_5_irq_handler")));
static void PIOS_TIM_5_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM5);
}

void TIM6_IRQHandler(void) __attribute__((alias("PIOS_TIM_6_irq_handler")));
static void PIOS_TIM_6_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM6);
}

void TIM7_IRQHandler(void) __attribute__((alias("PIOS_TIM_7_irq_handler")));
static void PIOS_TIM_7_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM7);
}

void TIM8_UP_IRQHandler(void) __attribute__((alias("PIOS_TIM_8_UP_irq_handler")));
static void PIOS_TIM_8_UP_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM8);
}

void TIM8_CC_IRQHandler(void) __attribute__((alias("PIOS_TIM_8_CC_irq_handler")));
static void PIOS_TIM_8_CC_irq_handler(void)
{
    PIOS_TIM_generic_irq_handler(TIM8);
}

#endif /* PIOS_INCLUDE_TIM */
