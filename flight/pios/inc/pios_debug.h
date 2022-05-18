/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @defgroup   PIOS_DEBUG Debugging Functions
 * @brief Debugging functionality
 * @{
 *
 * @file       pios_i2c.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Debug helper functions header.
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

#ifndef PIOS_DEBUG_H
#define PIOS_DEBUG_H

#ifdef PIOS_INCLUDE_DEBUG_CONSOLE
# ifndef PIOS_COM_DEBUG
extern uint32_t pios_com_debug_id;
#  define PIOS_COM_DEBUG (pios_com_debug_id)
# endif
# ifndef DEBUG_LEVEL
#  define DEBUG_LEVEL    0
# endif
# define DEBUG_PRINTF(level, ...) \
    { \
        if ((level <= DEBUG_LEVEL) && (PIOS_COM_DEBUG > 0)) { \
            PIOS_COM_SendFormattedString(PIOS_COM_DEBUG, __VA_ARGS__); \
        } \
    }
#else
#  define DEBUG_PRINTF(level, ...)
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */

extern const char *PIOS_DEBUG_AssertMsg;

#ifdef USE_SIM_POSIX
void PIOS_DEBUG_Init(void);
#else
#include <pios_tim_priv.h>
void PIOS_DEBUG_Init(const struct pios_tim_channel *channels, uint8_t num_channels);
#endif

void PIOS_DEBUG_PinHigh(uint8_t pin);
void PIOS_DEBUG_PinLow(uint8_t pin);
void PIOS_DEBUG_PinValue8Bit(uint8_t value);
void PIOS_DEBUG_PinValue4BitL(uint8_t value);
void PIOS_DEBUG_Panic(const char *msg) __attribute__((noreturn));

#ifdef DEBUG
#define PIOS_DEBUG_Assert(test) if (!(test)) { PIOS_DEBUG_Panic(PIOS_DEBUG_AssertMsg); }
#define PIOS_Assert(test)       PIOS_DEBUG_Assert(test)
#else
#define PIOS_DEBUG_Assert(test)
#define PIOS_Assert(test) \
    if (!(test)) { while (1) {; } \
    }
#endif

/* Static (compile-time) assertion for use in a function.
   If test evaluates to 0 (ie fails) at compile time then compilation will
   fail with the error: "size of unnamed array is negative" */
#define PIOS_STATIC_ASSERT(test) ((void)sizeof(int[1 - 2 * !(test)]))


#endif /* PIOS_DEBUG_H */

/**
 * @}
 * @}
 */
