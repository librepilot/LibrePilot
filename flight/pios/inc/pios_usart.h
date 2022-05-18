/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USART USART Functions
 * @brief PIOS interface for USART port
 * @{
 *
 * @file       pios_usart.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      USART functions header.
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

#ifndef PIOS_USART_H
#define PIOS_USART_H

/* Global Types */
/* Public Functions */

/* USART Ioctls */

enum PIOS_USART_Inverted {
    PIOS_USART_Inverted_None = 0,
    PIOS_USART_Inverted_Rx   = (1 << 0),
    PIOS_USART_Inverted_Tx   = (1 << 1),
    PIOS_USART_Inverted_RxTx = (PIOS_USART_Inverted_Rx | PIOS_USART_Inverted_Tx)
};

#define PIOS_IOCTL_USART_SET_INVERTED   COM_IOCTL(COM_IOCTL_TYPE_USART, 1, enum PIOS_USART_Inverted)
#define PIOS_IOCTL_USART_SET_SWAPPIN    COM_IOCTL(COM_IOCTL_TYPE_USART, 2, bool)
#define PIOS_IOCTL_USART_SET_HALFDUPLEX COM_IOCTL(COM_IOCTL_TYPE_USART, 3, bool)

#define PIOS_IOCTL_USART_GET_RXGPIO     COM_IOCTL(COM_IOCTL_TYPE_USART, 4, struct stm32_gpio)
#define PIOS_IOCTL_USART_GET_TXGPIO     COM_IOCTL(COM_IOCTL_TYPE_USART, 5, struct stm32_gpio)

/* PIOS_IRQ_PRIO_ values */
#define PIOS_IOCTL_USART_SET_IRQ_PRIO   COM_IOCTL(COM_IOCTL_TYPE_USART, 6, uint8_t)

#define PIOS_IOCTL_USART_GET_DSMBIND    COM_IOCTL(COM_IOCTL_TYPE_USART, 7, struct stm32_gpio)

#define PIOS_IOCTL_USART_LOCK_CONFIG    COM_IOCTL(COM_IOCTL_TYPE_USART, 8, bool)

#endif /* PIOS_USART_H */

/**
 * @}
 * @}
 */
