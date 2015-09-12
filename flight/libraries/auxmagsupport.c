/**
 ******************************************************************************
 *
 * @file       auxmagsupport.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @brief      Functions to handle aux mag data and calibration.
 *             --
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

#include <stdint.h>
#include "inc/auxmagsupport.h"
#include "CoordinateConversions.h"
#if defined(PIOS_INCLUDE_HMC5X83)
#include "pios_hmc5x83.h"
#endif

#define assumptions \
    ( \
        ((int)   PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP ==    \
         (int) AUXMAGSETTINGS_ORIENTATION_EAST_NORTH_UP) &&   \
        ((int)   PIOS_HMC5X83_ORIENTATION_SOUTH_EAST_UP ==    \
         (int) AUXMAGSETTINGS_ORIENTATION_SOUTH_EAST_UP) &&   \
        ((int)   PIOS_HMC5X83_ORIENTATION_WEST_SOUTH_UP ==    \
         (int) AUXMAGSETTINGS_ORIENTATION_WEST_SOUTH_UP) &&   \
        ((int)   PIOS_HMC5X83_ORIENTATION_NORTH_WEST_UP ==    \
         (int) AUXMAGSETTINGS_ORIENTATION_NORTH_WEST_UP) &&   \
        ((int)   PIOS_HMC5X83_ORIENTATION_EAST_SOUTH_DOWN ==  \
         (int) AUXMAGSETTINGS_ORIENTATION_EAST_SOUTH_DOWN) && \
        ((int)   PIOS_HMC5X83_ORIENTATION_SOUTH_WEST_DOWN ==  \
         (int) AUXMAGSETTINGS_ORIENTATION_SOUTH_WEST_DOWN) && \
        ((int)   PIOS_HMC5X83_ORIENTATION_WEST_NORTH_DOWN ==  \
         (int) AUXMAGSETTINGS_ORIENTATION_WEST_NORTH_DOWN) && \
        ((int)   PIOS_HMC5X83_ORIENTATION_NORTH_EAST_DOWN ==  \
         (int) AUXMAGSETTINGS_ORIENTATION_NORTH_EAST_DOWN) && \
        ((int)   PIOS_HMC5X83_ORIENTATION_UNCHANGED ==        \
         (int) AUXMAGSETTINGS_ORIENTATION_UNCHANGED) )

static float mag_bias[3] = { 0, 0, 0 };
static float mag_transform[3][3] = {
    { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }
};

AuxMagSettingsTypeOptions option;

void auxmagsupport_reload_settings()
{
    AuxMagSettingsTypeGet(&option);
    AuxMagSettingsmag_transformArrayGet((float *)mag_transform);
    AuxMagSettingsOrientationOptions orientation;
    AuxMagSettingsOrientationGet(&orientation);
    PIOS_STATIC_ASSERT(assumptions);
    PIOS_HMC5x83_Ext_Orientation_Set((enum PIOS_HMC5X83_ORIENTATION) orientation);
    AuxMagSettingsmag_biasArrayGet(mag_bias);
}

void auxmagsupport_publish_samples(float mags[3], uint8_t status)
{
    float mag_out[3];

    mags[0] -= mag_bias[0];
    mags[1] -= mag_bias[1];
    mags[2] -= mag_bias[2];
    rot_mult(mag_transform, mags, mag_out);

    AuxMagSensorData data;
    data.x = mag_out[0];
    data.y = mag_out[1];
    data.z = mag_out[2];
    data.Status = status;
    AuxMagSensorSet(&data);
}

AuxMagSettingsTypeOptions auxmagsupport_get_type()
{
    return option;
}
