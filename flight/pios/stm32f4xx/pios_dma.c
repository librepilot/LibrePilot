/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_DMA DMA Functions
 * @brief PIOS interface for DMA peripheral 
 * @{
 *
 * @file       pios_dma.c
 * @author     The SantyPilot Team Copyright (c) 2023.
               The LibrePilot Project, http://www.librepilot.org, Copyright (c) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      DMA easy apis
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

#include "pios.h"
#include <pios_dma.h>
#include <pios_stm32.h>

void PIOS_DMA_SetRxBuffer(struct stm32_dma* dma, char* buffer, uint8_t len) {
	dma->rx.init.DMA_Memory0BaseAddr = (uint32_t)buffer;
	dma->rx.init.DMA_BufferSize = len;
	return;
}
