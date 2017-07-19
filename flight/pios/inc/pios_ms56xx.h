/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MS5611 MS56XX Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_ms56xx.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      MS56XX functions header.
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

#ifndef PIOS_MS56XX_H
#define PIOS_MS56XX_H

#ifdef PIOS_INCLUDE_MS56XX

#include <pios_sensors.h>

enum pios_ms56xx_version {
    MS56XX_VERSION_5607 = 0,
    MS56XX_VERSION_5611 = 1,
    MS56XX_VERSION_5637 = 2,
};

enum pios_ms56xx_osr {
    MS56XX_OSR_256  = 0,
    MS56XX_OSR_512  = 2,
    MS56XX_OSR_1024 = 4,
    MS56XX_OSR_2048 = 6,
    MS56XX_OSR_4096 = 8,
    // Only supported by the ms5637
    MS56XX_OSR_8192 = 10
};

struct pios_ms56xx_cfg {
    uint8_t address;
    enum pios_ms56xx_version version;
    enum pios_ms56xx_osr oversampling;
};

/* Public Functions */
extern void PIOS_MS56xx_Init(const struct pios_ms56xx_cfg *cfg, int32_t i2c_device);
extern void PIOS_MS56xx_Register();
extern bool PIOS_MS56xx_Read(float *temperature, float *pressure);

#endif /* PIOS_INCLUDE_MS56XX */

#endif /* PIOS_MS56XX_H */

/**
 * @}
 * @}
 */
