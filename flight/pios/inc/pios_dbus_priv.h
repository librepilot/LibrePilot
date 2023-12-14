/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DBus DJI DBus receiver functions
 * @brief PIOS interface to read and write from DBus port
 * @{
 *
 * @file       pios_dbus_priv.h
 * @author     The SantyPilot Team, Copyright (C) 2023.
 * @brief      DJI DBus Private structures.
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

#ifndef PIOS_DBUS_PRIV_H
#define PIOS_DBUS_PRIV_H

#include <pios.h>
#include <pios_stm32.h>
#include <pios_usart_priv.h>

extern const struct pios_rcvr_driver pios_dbus_rcvr_driver;

extern int32_t PIOS_DBus_Init(uint32_t *dbus_id,
                              const struct pios_com_driver *driver,
                              uint32_t lower_id);

#endif /* PIOS_DBUS_PRIV_H */

/**
 * @}
 * @}
 */
