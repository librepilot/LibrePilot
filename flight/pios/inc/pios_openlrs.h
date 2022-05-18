/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_RFM22B Radio Functions
 * @brief PIOS OpenLRS interface for for the RFM22B radio
 * @{
 *
 * @file       pios_openlrs.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Implements an OpenLRS driver for the RFM22B
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

#ifndef PIOS_OPENLRS_H
#define PIOS_OPENLRS_H

/* Global Types */
struct pios_openlrs_cfg {
    const struct pios_spi_cfg  *spi_cfg; /* Pointer to SPI interface configuration */
    const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */
    enum gpio_direction gpio_direction; /* Definition comes from pios_rfm22b.h */
};

typedef void (*PIOS_OpenLRS_PPMReceivedCallback)(uint32_t context, const int16_t *channels);

extern int32_t PIOS_OpenLRS_Init(uint32_t *openlrs_id, uint32_t spi_id,
                                 uint32_t slave_num, const struct pios_openlrs_cfg *cfg);

extern void PIOS_OpenLRS_RegisterPPMCallback(uint32_t openlrs_id, PIOS_OpenLRS_PPMReceivedCallback callback, uint32_t context);
extern uint8_t PIOS_OpenLRS_RSSI_Get(void);
#endif /* PIOS_OPENLRS_H */
/**
 * @}
 * @}
 */
