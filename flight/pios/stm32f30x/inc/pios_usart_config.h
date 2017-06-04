/**
 ******************************************************************************
 *
 * @file       pios_usart_config.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017
 * @brief      Architecture specific macros and definitions
 *             --
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

#ifndef PIOS_USART_CONFIG_H_
#define PIOS_USART_CONFIG_H_

/**
 * Generic USART configuration structure for an STM32F30x port.
 */
#define USART_CONFIG(_usart, _rx_gpio, _rx_pin, _tx_gpio, _tx_pin) \
    {                                                                       \
        .regs  = _usart,                                                    \
        .remap = GPIO_AF_##_usart,                                        \
        .rx    = {                                                           \
            .gpio = GPIO##_rx_gpio,                                               \
            .init = {                                                       \
                .GPIO_Pin   = GPIO_Pin_##_rx_pin,                                      \
                .GPIO_Mode  = GPIO_Mode_AF,                                 \
                .GPIO_Speed = GPIO_Speed_50MHz,                             \
                .GPIO_OType = GPIO_OType_PP,                                \
                .GPIO_PuPd  = GPIO_PuPd_UP,                                 \
            },                                                              \
            .pin_source     = GPIO_PinSource##_rx_pin,       \
        },                                                                  \
        .tx = {                                                           \
            .gpio = GPIO##_tx_gpio,                                               \
            .init = {                                                       \
                .GPIO_Pin   = GPIO_Pin_##_tx_pin,                                      \
                .GPIO_Mode  = GPIO_Mode_AF,                                 \
                .GPIO_Speed = GPIO_Speed_50MHz,                             \
                .GPIO_OType = GPIO_OType_PP,                                \
                .GPIO_PuPd  = GPIO_PuPd_UP,                                 \
            },                                                              \
            .pin_source     = GPIO_PinSource##_tx_pin,       \
        },                                                                  \
    }

#endif /* PIOS_USART_CONFIG_H_ */
