/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SERVO RC Servo Functions
 * @brief Code to do set RC servo output
 * @{
 *
 * @file       pios_servo.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      RC Servo routines (STM32 dependent)
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
/*
 * DShot: Tribute belongs to dRonin, http://dRonin.org/ for sparking the idea of
 * using gpio bitbang as more general solution over using timer dma.
 */

#include "pios.h"

#ifdef PIOS_INCLUDE_SERVO

#include "pios_servo_priv.h"
#include "pios_tim_priv.h"

/* Private Function Prototypes */

#define PIOS_SERVO_GPIO_BANKS 3

static const struct pios_servo_cfg *servo_cfg;

static volatile uint32_t *pios_servo_bsrr[PIOS_SERVO_GPIO_BANKS]; // GPIO banks

struct pios_servo_bank {
    enum pios_servo_bank_mode mode;
    uint16_t    next_update;
    uint16_t    max_pulse;
    TIM_TypeDef *timer;
};

struct pios_servo_pin {
    struct pios_servo_bank *bank;
    uint8_t  bank_nr;
    uint8_t  gpio_bank;
    uint16_t value;
};


static struct pios_servo_bank pios_servo_banks[PIOS_SERVO_BANKS];
static struct pios_servo_pin *pios_servo_pins;


// Dshot timing
static uint32_t pios_dshot_t0h_raw;
static uint32_t pios_dshot_t1h_raw;
static uint32_t pios_dshot_t_raw;

static bool pios_servo_enabled    = false;
static uint32_t pios_servo_active = 0; // No active outputs by default

#define PIOS_SERVO_TIMER_CLOCK 1000000
#define PIOS_SERVO_SAFE_MARGIN 50


#define DSHOT_TIMING_ADJUST    8
#define DSHOT_T0H_DIV          2666
#define DSHOT_T1H_DIV          1333
#define DSHOT_NUM_BITS         16

extern void PIOS_Servo_SetActive(uint32_t active)
{
    bool enabled = pios_servo_enabled;

    if (enabled) {
        PIOS_Servo_Disable();
    }

    pios_servo_active = active;

    if (enabled) {
        PIOS_Servo_Enable();
    }
}

extern void PIOS_Servo_Disable()
{
    if (!servo_cfg) {
        return;
    }
    pios_servo_enabled = false;

    /* NOTE: Following will stop pulses and force low level on output pins.
     * this is ok with ESC and servos, but brushed motors could be in trouble
     * if using inverted setup */

    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        if (!(pios_servo_active & (1L << i))) { // This output is not active
            continue;
        }

        const struct pios_tim_channel *chan = &servo_cfg->channels[i];

        GPIO_InitTypeDef init = chan->pin.init;

#if defined(STM32F40_41xxx) || defined(STM32F446xx) || defined(STM32F411xE) || defined(STM32F3)
        init.GPIO_Mode = GPIO_Mode_OUT;
#elif defined(STM32F10X_MD)
        init.GPIO_Mode = GPIO_Mode_Out_PP;
#else
#error Unsupported MCU
#endif
        GPIO_ResetBits(chan->pin.gpio, chan->pin.init.GPIO_Pin);

        GPIO_Init(chan->pin.gpio, &init);
    }
}

static void PIOS_Servo_SetupBank(uint8_t bank_nr)
{
    struct pios_servo_bank *bank = &pios_servo_banks[bank_nr];

    if (!bank->timer) {
        return;
    }

    // Setup the timer accordingly
    switch (bank->mode) {
    case PIOS_SERVO_BANK_MODE_PWM:
    case PIOS_SERVO_BANK_MODE_SINGLE_PULSE:
        TIM_ARRPreloadConfig(bank->timer, ENABLE);
        TIM_CtrlPWMOutputs(bank->timer, ENABLE);
        TIM_SelectOnePulseMode(bank->timer, TIM_OPMode_Repetitive);
        TIM_Cmd(bank->timer, ENABLE);
        break;
    default:;
        // do not manage timers otherwise
    }

    // Setup GPIO/AF
    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        const struct pios_tim_channel *chan = &servo_cfg->channels[i];

        if (chan->timer != bank->timer) { // Not interested in this bank
            continue;
        }

        if (!(pios_servo_active & (1L << i))) { // This output is not active
            continue;
        }

        GPIO_InitTypeDef init = chan->pin.init;

        switch (bank->mode) {
        case PIOS_SERVO_BANK_MODE_PWM:
        case PIOS_SERVO_BANK_MODE_SINGLE_PULSE:
#if defined(STM32F40_41xxx) || defined(STM32F446xx) || defined(STM32F411xE) || defined(STM32F3)
            GPIO_PinAFConfig(chan->pin.gpio, chan->pin.pin_source, chan->remap);
#elif defined(STM32F10X_MD)
            init.GPIO_Mode = GPIO_Mode_AF_PP;
            if (chan->remap) {
                GPIO_PinRemapConfig(chan->remap, ENABLE);
            }
#else
#error Unsupported MCU
#endif
            GPIO_Init(chan->pin.gpio, &init);

            /* Set up for output compare function */
            switch (chan->timer_chan) {
            case TIM_Channel_1:
                TIM_OC1Init(chan->timer, (TIM_OCInitTypeDef *)&servo_cfg->tim_oc_init);
                TIM_OC1PreloadConfig(chan->timer, TIM_OCPreload_Enable);
                break;
            case TIM_Channel_2:
                TIM_OC2Init(chan->timer, (TIM_OCInitTypeDef *)&servo_cfg->tim_oc_init);
                TIM_OC2PreloadConfig(chan->timer, TIM_OCPreload_Enable);
                break;
            case TIM_Channel_3:
                TIM_OC3Init(chan->timer, (TIM_OCInitTypeDef *)&servo_cfg->tim_oc_init);
                TIM_OC3PreloadConfig(chan->timer, TIM_OCPreload_Enable);
                break;
            case TIM_Channel_4:
                TIM_OC4Init(chan->timer, (TIM_OCInitTypeDef *)&servo_cfg->tim_oc_init);
                TIM_OC4PreloadConfig(chan->timer, TIM_OCPreload_Enable);
                break;
            }
            break;

        case PIOS_SERVO_BANK_MODE_DSHOT:
        {
#if defined(STM32F40_41xxx) || defined(STM32F446xx) || defined(STM32F411xE) || defined(STM32F3)
            init.GPIO_Mode = GPIO_Mode_OUT;
#elif defined(STM32F10X_MD)
            init.GPIO_Mode = GPIO_Mode_Out_PP;
#else
#error Unsupported MCU
#endif
            GPIO_ResetBits(chan->pin.gpio, chan->pin.init.GPIO_Pin);

            GPIO_Init(chan->pin.gpio, &init);
        }
        break;

        default:;
        }
    }
}

extern void PIOS_Servo_Enable()
{
    if (!servo_cfg) {
        return;
    }

    for (uint8_t i = 0; (i < PIOS_SERVO_BANKS); i++) {
        PIOS_Servo_SetupBank(i);
    }

    pios_servo_enabled = true;
}

void PIOS_Servo_DSHot_Rate(uint32_t rate_in_khz)
{
    if (rate_in_khz < 150) {
        rate_in_khz = 150;
    }

    uint32_t raw_hz = PIOS_DELAY_GetRawHz();

    uint32_t tmp    = raw_hz / rate_in_khz;

    pios_dshot_t0h_raw = (tmp / DSHOT_T0H_DIV) - DSHOT_TIMING_ADJUST;
    pios_dshot_t1h_raw = (tmp / DSHOT_T1H_DIV) - DSHOT_TIMING_ADJUST;
    pios_dshot_t_raw   = (tmp / 1000) - DSHOT_TIMING_ADJUST;
}

/**
 * Initialise Servos
 */
int32_t PIOS_Servo_Init(const struct pios_servo_cfg *cfg)
{
    /* Store away the requested configuration */
    servo_cfg = cfg;

    pios_servo_pins = pios_malloc(sizeof(*pios_servo_pins) * cfg->num_channels);
    PIOS_Assert(pios_servo_pins);

    memset(pios_servo_pins, 0, sizeof(*pios_servo_pins) * cfg->num_channels);

    /* set default dshot timing */
    PIOS_Servo_DSHot_Rate(300);


    uint8_t timer_bank = 0;
    uint8_t gpio_bank  = 0;

    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        const struct pios_tim_channel *chan = &servo_cfg->channels[i];
        bool new = true;
        /* See if any previous channels use that same timer */
        for (uint8_t j = 0; (j < i) && new; j++) {
            new &= chan->timer != servo_cfg->channels[j].timer;
        }

        if (new) {
            PIOS_Assert(timer_bank < PIOS_SERVO_BANKS);
            struct pios_servo_bank *bank = &pios_servo_banks[timer_bank];


            for (uint8_t j = i; j < servo_cfg->num_channels; j++) {
                if (servo_cfg->channels[j].timer == chan->timer) {
                    pios_servo_pins[j].bank    = bank;
                    pios_servo_pins[j].bank_nr = timer_bank;
                }
            }

            bank->timer = chan->timer;
            bank->mode  = PIOS_SERVO_BANK_MODE_NONE;

            TIM_Cmd(chan->timer, DISABLE);

            timer_bank++;
        }

        // now map gpio banks
        new = true;
        for (uint8_t j = 0; (j < i) && new; j++) {
            new &= chan->pin.gpio != servo_cfg->channels[j].pin.gpio;
        }

        if (new) {
            PIOS_Assert(gpio_bank < PIOS_SERVO_GPIO_BANKS);

            for (uint8_t j = i; j < servo_cfg->num_channels; j++) {
                if (servo_cfg->channels[j].pin.gpio == chan->pin.gpio) {
                    pios_servo_pins[j].gpio_bank = gpio_bank;
                }
            }
#if defined(STM32F40_41xxx) || defined(STM32F446xx) || defined(STM32F411xE)
            pios_servo_bsrr[gpio_bank] = (uint32_t *)&chan->pin.gpio->BSRRL;
#else
            pios_servo_bsrr[gpio_bank] = &chan->pin.gpio->BSRR;
#endif

            ++gpio_bank;
        }
    }

    static uint32_t dummy_bsrr;

    for (int i = gpio_bank; i < PIOS_SERVO_GPIO_BANKS; ++i) {
        pios_servo_bsrr[gpio_bank] = &dummy_bsrr;
    }

    PIOS_Servo_Enable();

    return 0;
}

void PIOS_Servo_SetBankMode(uint8_t bank, uint8_t mode)
{
    PIOS_Assert(bank < PIOS_SERVO_BANKS);
    pios_servo_banks[bank].mode = mode;

    if (!pios_servo_enabled) {
        return;
    }

    PIOS_Servo_SetupBank(bank);
}

static void PIOS_Servo_DShot_Update()
{
    uint32_t next;
    uint32_t data[PIOS_SERVO_GPIO_BANKS];
    uint16_t pins[PIOS_SERVO_GPIO_BANKS];
    uint16_t buffer[DSHOT_NUM_BITS][PIOS_SERVO_GPIO_BANKS];

    for (uint8_t i = 0; i < PIOS_SERVO_GPIO_BANKS; ++i) {
        pins[i] = 0;
    }

    bool has_dshot = false;

    memset(buffer, 0, sizeof(buffer));

    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        struct pios_servo_pin *pin = &pios_servo_pins[i];

        if (pin->bank->mode != PIOS_SERVO_BANK_MODE_DSHOT) {
            continue;
        }

        if (!(pios_servo_active & (1L << i))) { // This output is not active
            continue;
        }

        has_dshot = true;

        uint16_t payload = pin->value;
        if (payload > 2047) {
            payload = 2047;
        }

        payload <<= 5;

        payload  |= ((payload >> 4) & 0xf) ^
                    ((payload >> 8) & 0xf) ^
                    ((payload >> 12) & 0xf);

        uint16_t gpio_pin = servo_cfg->channels[i].pin.init.GPIO_Pin;

        for (int j = 0; j < DSHOT_NUM_BITS; ++j) {
            if (!(payload & 0x8000)) {
                buffer[j][pin->gpio_bank] |= gpio_pin;
            }
            payload <<= 1;
        }

        pins[pin->gpio_bank] |= gpio_pin;

        pin->value = 0;
    }

    if (!has_dshot) {
        return;
    }

    PIOS_IRQ_Disable();

    uint32_t start = PIOS_DELAY_GetRaw();

    for (int i = 0; i < DSHOT_NUM_BITS; ++i) {
        // single bit:

        COMPILER_BARRIER();
        // 1. write 3x BSRR to set gpio high
        for (int j = 0; j < PIOS_SERVO_GPIO_BANKS; ++j) {
            *(pios_servo_bsrr[j]) = (uint32_t)pins[j];
        }

        // Prep data
        for (int j = 0; j < PIOS_SERVO_GPIO_BANKS; ++j) {
            data[j] = buffer[i][j] << 16;
        }

        // 2. wait until T0H, write 3x BSRR to clear whatever bits are set to 0
        next = start + pios_dshot_t0h_raw;
        while ((next - PIOS_DELAY_GetRaw()) < pios_dshot_t0h_raw) {
            ;
        }

        COMPILER_BARRIER();
        for (int j = 0; j < PIOS_SERVO_GPIO_BANKS; ++j) {
            *(pios_servo_bsrr[j]) = data[j];
        }

        // Prep data
        for (int j = 0; j < PIOS_SERVO_GPIO_BANKS; ++j) {
            data[j] = (uint32_t)pins[j] << 16;
        }

        // 3. wait until T1H, then write 3x BSRR to set all to low
        next = start + pios_dshot_t1h_raw;
        while ((next - PIOS_DELAY_GetRaw()) < pios_dshot_t1h_raw) {
            ;
        }

        COMPILER_BARRIER();
        for (int j = 0; j < PIOS_SERVO_GPIO_BANKS; ++j) {
            *(pios_servo_bsrr[j]) = data[j];
        }

        // 4. wait until Tend
        start += pios_dshot_t_raw;
        while ((start - PIOS_DELAY_GetRaw()) < pios_dshot_t_raw) {
            ;
        }
    }

    PIOS_IRQ_Enable();
}


void PIOS_Servo_Update()
{
    if (!pios_servo_enabled) {
        return;
    }

    for (uint8_t i = 0; (i < PIOS_SERVO_BANKS); i++) {
        struct pios_servo_bank *bank = &pios_servo_banks[i];
        if (bank->timer && (bank->mode == PIOS_SERVO_BANK_MODE_SINGLE_PULSE)) {
            // a pulse to be generated is longer than cycle period. skip this update.
            if (TIM_GetCounter((TIM_TypeDef *)bank->timer) > (uint32_t)(bank->next_update + PIOS_SERVO_SAFE_MARGIN)) {
                TIM_GenerateEvent((TIM_TypeDef *)bank->timer, TIM_EventSource_Update);
                bank->next_update = bank->max_pulse;
            }
        }
        bank->max_pulse = 0;
    }

    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        if ((pios_servo_active & (1L << i))
            && (pios_servo_pins[i].bank->mode == PIOS_SERVO_BANK_MODE_SINGLE_PULSE)) {
            /* Update the position */
            const struct pios_tim_channel *chan = &servo_cfg->channels[i];

            switch (chan->timer_chan) {
            case TIM_Channel_1:
                TIM_SetCompare1(chan->timer, 0);
                break;
            case TIM_Channel_2:
                TIM_SetCompare2(chan->timer, 0);
                break;
            case TIM_Channel_3:
                TIM_SetCompare3(chan->timer, 0);
                break;
            case TIM_Channel_4:
                TIM_SetCompare4(chan->timer, 0);
                break;
            }
        }
    }

    PIOS_Servo_DShot_Update();
}
/**
 * Set the servo update rate (Max 500Hz)
 * \param[in] array of rates in Hz
 * \param[in] array of timer clocks in Hz
 * \param[in] maximum number of banks
 */
void PIOS_Servo_SetHz(const uint16_t *speeds, const uint32_t *clock, uint8_t banks)
{
    PIOS_Assert(banks <= PIOS_SERVO_BANKS);
    if (!servo_cfg) {
        return;
    }

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = servo_cfg->tim_base_init;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;

    for (uint8_t i = 0; i < banks && i < PIOS_SERVO_BANKS; i++) {
        const TIM_TypeDef *timer = pios_servo_banks[i].timer;
        if (timer) {
            uint32_t new_clock = PIOS_SERVO_TIMER_CLOCK;
            if (clock[i]) {
                new_clock = clock[i];
            }

            uint32_t timer_clock;

            // Choose the correct prescaler value for the APB the timer is attached

#if defined(STM32F10X_MD) || defined(STM32F3)
            // F1 & F3 have both timer clock domains running at master clock speed
            timer_clock = PIOS_MASTER_CLOCK;
#elif defined(STM32F40_41xxx) || defined(STM32F446xx) || defined(STM32F411xE)
            if (timer == TIM1 || timer == TIM8 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
                timer_clock = PIOS_PERIPHERAL_APB2_CLOCK;
            } else {
                timer_clock = PIOS_PERIPHERAL_APB1_CLOCK;
            }
#else
#error Unsupported MCU
#endif
            TIM_TimeBaseStructure.TIM_Prescaler = (timer_clock / new_clock) - 1;
            TIM_TimeBaseStructure.TIM_Period    = ((new_clock / speeds[i]) - 1);
            TIM_TimeBaseInit((TIM_TypeDef *)timer, &TIM_TimeBaseStructure);
        }
    }
}

/**
 * Set servo position
 * \param[in] Servo Servo number (0-7)
 * \param[in] Position Servo position in microseconds
 */
void PIOS_Servo_Set(uint8_t servo, uint16_t position)
{
    /* Make sure servo exists */
    if (!pios_servo_enabled || !servo_cfg || servo >= servo_cfg->num_channels) {
        return;
    }


    /* Update the position */

    pios_servo_pins[servo].value = position;

    const struct pios_tim_channel *chan = &servo_cfg->channels[servo];
    struct pios_servo_bank *bank = pios_servo_pins[servo].bank;

    if ((bank->mode == PIOS_SERVO_BANK_MODE_SINGLE_PULSE) || (bank->mode == PIOS_SERVO_BANK_MODE_PWM)) {
        uint16_t val    = position;
        uint16_t margin = chan->timer->ARR / 50; // Leave 2% of period as margin to prevent overlaps
        if (val > (chan->timer->ARR - margin)) {
            val = chan->timer->ARR - margin;
        }

        if (bank->max_pulse < val) {
            bank->max_pulse = val;
        }
        switch (chan->timer_chan) {
        case TIM_Channel_1:
            TIM_SetCompare1(chan->timer, val);
            break;
        case TIM_Channel_2:
            TIM_SetCompare2(chan->timer, val);
            break;
        case TIM_Channel_3:
            TIM_SetCompare3(chan->timer, val);
            break;
        case TIM_Channel_4:
            TIM_SetCompare4(chan->timer, val);
            break;
        }
    }
}

uint8_t PIOS_Servo_GetPinBank(uint8_t pin)
{
    if (pin < servo_cfg->num_channels) {
        return pios_servo_pins[pin].bank_nr;
    } else {
        return 0;
    }
}

const struct pios_servo_cfg *PIOS_Servo_GetConfig()
{
    return servo_cfg;
}

#endif /* PIOS_INCLUDE_SERVO */
