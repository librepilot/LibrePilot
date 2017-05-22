/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP280 Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_bmp280.h
 * @author     The LibrePilot Team, http://www.librepilot.org Copyright (C) 2017.
 * @brief      BMP280 functions header.
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

#ifndef PIOS_BMP280_H
#define PIOS_BMP280_H
#include <pios_sensors.h>

#define BMP280_OVERSAMP_SKIPPED      (0x00)
#define BMP280_OVERSAMP_1X           (0x01)
#define BMP280_OVERSAMP_2X           (0x02)
#define BMP280_OVERSAMP_4X           (0x03)
#define BMP280_OVERSAMP_8X           (0x04)
#define BMP280_OVERSAMP_16X          (0x05)

#define BMP280_PRESSURE_OSR(x)    (x << 2)
#define BMP280_TEMPERATURE_OSR(x) (x << 5)

#define BMP280_STANDARD_RESOLUTION   (BMP280_PRESSURE_OSR(BMP280_OVERSAMP_4X) | BMP280_TEMPERATURE_OSR(BMP280_OVERSAMP_1X))
#define BMP280_HIGH_RESOLUTION       (BMP280_PRESSURE_OSR(BMP280_OVERSAMP_8X) | BMP280_TEMPERATURE_OSR(BMP280_OVERSAMP_1X))
#define BMP280_ULTRA_HIGH_RESOLUTION (BMP280_PRESSURE_OSR(BMP280_OVERSAMP_16X) | BMP280_TEMPERATURE_OSR(BMP280_OVERSAMP_2X))

struct pios_bmp280_cfg {
    uint8_t oversampling;
};

/* Public Functions */
extern void PIOS_BMP280_Init(const struct pios_bmp280_cfg *cfg, uint32_t i2c_device);

#endif /* PIOS_BMP280_H */

/**
 * @}
 * @}
 */
