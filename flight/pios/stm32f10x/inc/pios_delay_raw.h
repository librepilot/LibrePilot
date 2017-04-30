/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DELAY Delay Functions
 * @brief PiOS Delay functionality
 * @{
 *
 * @file       pios_delay_raw.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Settings functions header
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
#ifndef PIOS_DELAY_RAW_H
#define PIOS_DELAY_RAW_H

/* these should be defined by CMSIS, but they aren't */
#define DWT_CTRL   (*(volatile uint32_t *)0xe0001000)
#define CYCCNTENA  (1 << 0)
#define DWT_CYCCNT (*(volatile uint32_t *)0xe0001004)

#define PIOS_DELAY_GetRaw() (DWT_CYCCNT)

extern uint32_t PIOS_DELAY_GetRawHz();

#endif /* PIOS_DELAY_RAW_H */
