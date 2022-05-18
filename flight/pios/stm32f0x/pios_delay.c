/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DELAY Delay Functions
 * @brief PiOS Delay functionality
 * @{
 *
 * @file       pios_delay.c
 * @author     Michael Smith Copyright (C) 2011
 * @brief      Delay Functions
 *                 - Provides a micro-second granular delay using the CPU
 *                   cycle counter.
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

#include <pios.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_tim.h>
#ifdef PIOS_INCLUDE_DELAY

/* these should be defined by CMSIS, but they aren't */
#define DELAY_COUNTER (TIM2->CNT)

/**
 * Initialises the Timer used by PIOS_DELAY functions.
 *
 * \return always zero (success)
 */

int32_t PIOS_DELAY_Init(void)
{
    // unfortunately F0 does not allow access to DWT and CoreDebug functionality from CPU side
    // thus we are going to use timer2 for timing measurement

    const TIM_TimeBaseInitTypeDef timerInit = {
        .TIM_Prescaler         = (48000000 / 1000000),
        .TIM_ClockDivision     = TIM_CKD_DIV1,
        .TIM_CounterMode       = TIM_CounterMode_Up,
        .TIM_Period            = 0xFFFFFFFF,
        .TIM_RepetitionCounter = 0x0000,
    };

    // Stop timer
    TIM_Cmd(TIM2, DISABLE);
    // Configure timebase and internal clock
    TIM_TimeBaseInit(TIM2, (TIM_TimeBaseInitTypeDef *)&timerInit);
    TIM_InternalClockConfig(TIM2);
    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);

    return 0;
}

/**
 * Waits for a specific number of uS
 *
 * Example:<BR>
 * \code
 *   // Wait for 500 uS
 *   PIOS_DELAY_Wait_uS(500);
 * \endcode
 * \param[in] uS delay
 * \return < 0 on errors
 */
int32_t PIOS_DELAY_WaituS(uint32_t uS)
{
    uint32_t last_count = DELAY_COUNTER;

    while (DELAY_COUNTER - last_count <= uS) {
        ;
    }

    /* No error */
    return 0;
}

/**
 * Waits for a specific number of mS
 *
 * Example:<BR>
 * \code
 *   // Wait for 500 mS
 *   PIOS_DELAY_Wait_mS(500);
 * \endcode
 * \param[in] mS delay
 * \return < 0 on errors
 */
int32_t PIOS_DELAY_WaitmS(uint32_t mS)
{
    while (mS--) {
        PIOS_DELAY_WaituS(1000);
    }

    /* No error */
    return 0;
}

/**
 * @brief Query the Delay timer for the current uS
 * @return A microsecond value
 */
uint32_t PIOS_DELAY_GetuS(void)
{
    return DELAY_COUNTER;
}

/**
 * @brief Calculate time in microseconds since a previous time
 * @param[in] t previous time
 * @return time in us since previous time t.
 */
uint32_t PIOS_DELAY_GetuSSince(uint32_t t)
{
    return DELAY_COUNTER - t;
}

/**
 * @brief Get the raw delay timer, useful for timing
 * @return Unitless value (uint32 wrap around)
 */
uint32_t PIOS_DELAY_GetRaw()
{
    return DELAY_COUNTER;
}

/**
 * @brief Compare to raw times to and convert to us
 * @return A microsecond value
 */
uint32_t PIOS_DELAY_DiffuS(uint32_t raw)
{
    uint32_t diff = PIOS_DELAY_GetRaw() - raw;

    return diff;
}

#endif /* PIOS_INCLUDE_DELAY */

/**
 * @}
 * @}
 */
