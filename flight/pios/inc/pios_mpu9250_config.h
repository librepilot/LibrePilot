/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU9250 OpenPilot layer configuration utilities
 * @brief provides mpu9250 configuration helpers function
 * @{
 *
 * @file       PIOS_MPU9250_CONFIG.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      MPU9250 UAVO-based configuration functions
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
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

#ifndef PIOS_MPU9250_CONFIG_H
#define PIOS_MPU9250_CONFIG_H

#include "mpugyroaccelsettings.h"
#include "pios_mpu9250.h"

#define PIOS_MPU9250_CONFIG_MAP_GYROSCALE(x) \
    (x == MPUGYROACCELSETTINGS_GYROSCALE_SCALE_250 ? PIOS_MPU9250_SCALE_250_DEG : \
     x == MPUGYROACCELSETTINGS_GYROSCALE_SCALE_500 ? PIOS_MPU9250_SCALE_500_DEG : \
     x == MPUGYROACCELSETTINGS_GYROSCALE_SCALE_1000 ? PIOS_MPU9250_SCALE_1000_DEG : \
     PIOS_MPU9250_SCALE_2000_DEG)

#define PIOS_MPU9250_CONFIG_MAP_ACCELSCALE(x) \
    (x == MPUGYROACCELSETTINGS_ACCELSCALE_SCALE_2G ? PIOS_MPU9250_ACCEL_2G : \
     x == MPUGYROACCELSETTINGS_ACCELSCALE_SCALE_4G ? PIOS_MPU9250_ACCEL_4G : \
     x == MPUGYROACCELSETTINGS_ACCELSCALE_SCALE_16G ? PIOS_MPU9250_ACCEL_16G : \
     PIOS_MPU9250_ACCEL_8G)

#define PIOS_MPU9250_CONFIG_MAP_FILTERSETTING(x) \
    (x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_188_HZ ? PIOS_MPU9250_LOWPASS_188_HZ : \
     x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_98_HZ ? PIOS_MPU9250_LOWPASS_98_HZ : \
     x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_42_HZ ? PIOS_MPU9250_LOWPASS_42_HZ : \
     x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_20_HZ ? PIOS_MPU9250_LOWPASS_20_HZ : \
     x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_10_HZ ? PIOS_MPU9250_LOWPASS_10_HZ : \
     x == MPUGYROACCELSETTINGS_FILTERSETTING_LOWPASS_5_HZ ? PIOS_MPU9250_LOWPASS_5_HZ : \
     PIOS_MPU9250_LOWPASS_256_HZ)
/**
 * @brief Updates MPU9250 config based on MPUGyroAccelSettings UAVO
 * @returns 0 if succeed or -1 otherwise
 */
int32_t PIOS_MPU9250_CONFIG_Configure()
{
    MPUGyroAccelSettingsData mpuSettings;

    MPUGyroAccelSettingsGet(&mpuSettings);
    return PIOS_MPU9250_ConfigureRanges(
        PIOS_MPU9250_CONFIG_MAP_GYROSCALE(mpuSettings.GyroScale),
        PIOS_MPU9250_CONFIG_MAP_ACCELSCALE(mpuSettings.AccelScale),
        PIOS_MPU9250_CONFIG_MAP_FILTERSETTING(mpuSettings.FilterSetting)
        );
}

#endif /* PIOS_MPU9250_CONFIG_H */
