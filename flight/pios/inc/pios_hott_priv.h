/**
 ******************************************************************************
 * @file       pios_hott_private.h
 * @author     The LibrePilot Project, http://www.librepilot.org, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_HOTT Graupner HoTT receiver functions
 * @{
 * @brief Graupner HoTT receiver functions for SUMD/H
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

#ifndef PIOS_HOTT_PRIV_H
#define PIOS_HOTT_PRIV_H

#include <pios.h>
#include <pios_usart_priv.h>

/* HOTT protocol variations */
enum pios_hott_proto {
    PIOS_HOTT_PROTO_SUMD,
    PIOS_HOTT_PROTO_SUMH,
};

/* HOTT receiver instance configuration */
extern const struct pios_rcvr_driver pios_hott_rcvr_driver;

extern int32_t PIOS_HOTT_Init(uint32_t *hott_id,
                              const struct pios_com_driver *driver,
                              uint32_t lower_id,
                              enum pios_hott_proto proto);

#endif /* PIOS_HOTT_PRIV_H */

/**
 * @}
 * @}
 */
