/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_DMA DMA Functions
 * @brief PIOS interface for DMA peripheral 
 * @{
 *
 * @file       pios_dma.h
 * @author     The SantyPilot Team, Copyright (C) 2023.
 * @brief      DMA functions header.
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

#ifndef PIOS_DMA_H
#define PIOS_DMA_H

#include <pios.h>
#include <pios_stm32.h>

/* Global Types */
// already defined in pios_stm32.h
/* Public Functions */
void PIOS_DMA_SetRxBuffer(struct stm32_dma* dma, char* buffer, uint8_t len);

#endif /* PIOS_DMA_H */

/**
 * @}
 * @}
 */
