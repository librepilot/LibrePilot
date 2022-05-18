/**
 ******************************************************************************
 *
 * @file       pios_board_sensors.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      board sensors setup
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

#include "pios_board_info.h"
#include "pios_board_sensors.h"
#include "pios_board_hw.h"

#include "uavobjectmanager.h"
#include "auxmagsettings.h"
#include "hwsettings.h"
#include <alarms.h>

#ifdef PIOS_INCLUDE_MPU6000
# include <pios_mpu6000.h>
# include <pios_mpu6000_config.h>
#endif

# include <pios_ms56xx.h>

#ifdef PIOS_INCLUDE_BMP280
# include <pios_bmp280.h>
#endif

#ifdef PIOS_INCLUDE_ADXL345
# include <pios_adxl345.h>
#endif

#ifdef PIOS_INCLUDE_MPU9250
# include <pios_mpu9250.h>
# include <pios_mpu9250_config.h>
#endif

#ifdef PIOS_INCLUDE_HMC5X83
# include "pios_hmc5x83.h"
# ifdef PIOS_HMC5X83_HAS_GPIOS
pios_hmc5x83_dev_t pios_hmc5x83_internal_id;
# endif
#endif

#ifdef PIOS_INCLUDE_L3GD20
# include "pios_l3gd20.h"
#endif

#ifdef PIOS_INCLUDE_BMA180
# include "pios_bma180.h"
#endif

#ifdef PIOS_INCLUDE_ADC
# include "pios_adc_priv.h"
#endif

void PIOS_BOARD_Sensors_Configure()
{
#ifdef PIOS_INCLUDE_MPU6000
    const struct pios_mpu6000_cfg *mpu6000_cfg = PIOS_BOARD_HW_DEFS_GetMPU6000Cfg(pios_board_info_blob.board_rev);
    if (mpu6000_cfg) {
#ifdef PIOS_SPI_MPU6000_ADAPTER
        PIOS_MPU6000_Init(PIOS_SPI_MPU6000_ADAPTER, 0, mpu6000_cfg);
#elif defined(PIOS_I2C_MPU6000_ADAPTER)
        PIOS_MPU6000_Init(PIOS_I2C_MPU6000_ADAPTER, 0, mpu6000_cfg);
#else
#error PIOS_INCLUDE_MPU6000 requires one of PIOS_SPI_MPU6000_ADAPTER or PIOS_I2C_MPU6000_ADAPTER
#endif
        PIOS_MPU6000_CONFIG_Configure();
#ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
        PIOS_MPU6000_Register();
#endif
    }
#endif /* PIOS_INCLUDE_MPU6000 */

#ifdef PIOS_INCLUDE_MPU9250
    const struct pios_mpu9250_cfg *mpu9250_cfg = PIOS_BOARD_HW_DEFS_GetMPU9250Cfg(pios_board_info_blob.board_rev);
    if (mpu9250_cfg) {
        PIOS_MPU9250_Init(PIOS_SPI_MPU9250_ADAPTER, 0, mpu9250_cfg);
        PIOS_MPU9250_CONFIG_Configure();
        PIOS_MPU9250_MainRegister();
        PIOS_MPU9250_MagRegister();
    }
#endif /* PIOS_INCLUDE_MPU9250 */

#ifdef PIOS_INCLUDE_ADXL345
    if (PIOS_BOARD_HW_DEFS_GetADXL345Cfg(pios_board_info_blob.board_rev)) {
        PIOS_ADXL345_Init(PIOS_SPI_ADXL345_ADAPTER, 0);
    }
#endif
#if defined(PIOS_INCLUDE_L3GD20)
    const struct pios_l3gd20_cfg *l3gd20_cfg = PIOS_BOARD_HW_DEFS_GetL3GD20Cfg(pios_board_info_blob.board_rev);
    if (l3gd20_cfg) {
        PIOS_Assert(0); // L3gd20 has not been ported to Sensor framework!!!
        PIOS_L3GD20_Init(PIOS_SPI_L3GD20_ADAPTER, 0, l3gd20_cfg);
        PIOS_Assert(PIOS_L3GD20_Test() == 0);
    }
#endif
#if defined(PIOS_INCLUDE_BMA180)
    const struct pios_bma180_cfg *bma180_cfg = PIOS_BOARD_HW_DEFS_GetBMA180Cfg(pios_board_info_blob.board_rev);
    if (bma180_cfg) {
        PIOS_Assert(0); // BMA180 has not been ported to Sensor framework!!!
        PIOS_BMA180_Init(PIOS_SPI_BMA180_ADAPTER, 0, bma180_cfg);
        PIOS_Assert(PIOS_BMA180_Test() == 0);
    }
#endif

    // internal HMC5x83 mag
# ifdef PIOS_INCLUDE_HMC5X83_INTERNAL
    const struct pios_hmc5x83_cfg *hmc5x83_internal_cfg = PIOS_BOARD_HW_DEFS_GetInternalHMC5x83Cfg(pios_board_info_blob.board_rev);

    if (hmc5x83_internal_cfg) {
        // attach the 5x83 mag to internal i2c bus

        pios_hmc5x83_dev_t internal_mag = PIOS_HMC5x83_Init(hmc5x83_internal_cfg, PIOS_I2C_HMC5X83_INTERNAL_ADAPTER, 0);
#  ifdef PIOS_INCLUDE_WDG
        // give HMC5x83 on I2C some extra time to allow for reset, etc. if needed
        // this is not in a loop, so it is safe
        PIOS_WDG_Clear();
#  endif /* PIOS_INCLUDE_WDG */

#ifdef PIOS_HMC5X83_HAS_GPIOS
        pios_hmc5x83_internal_id = internal_mag;
#endif
        // add this sensor to the sensor task's list
        PIOS_HMC5x83_Register(internal_mag, PIOS_SENSORS_TYPE_3AXIS_MAG);
    }

# endif /* PIOS_INCLUDE_HMC5X83_INTERNAL */

# ifdef PIOS_INCLUDE_HMC5X83

    AuxMagSettingsTypeOptions option;
    AuxMagSettingsTypeGet(&option);

    const struct pios_hmc5x83_cfg *hmc5x83_external_cfg = PIOS_BOARD_HW_DEFS_GetExternalHMC5x83Cfg(pios_board_info_blob.board_rev);

    if (hmc5x83_external_cfg) {
        uint32_t i2c_id = 0;

        if (option == AUXMAGSETTINGS_TYPE_FLEXI) {
            // i2c_external
#ifdef PIOS_I2C_FLEXI_ADAPTER
            i2c_id = PIOS_I2C_FLEXI_ADAPTER;
#endif
        } else if (option == AUXMAGSETTINGS_TYPE_I2C) {
            // i2c_internal (or Sparky2/F3 dedicated I2C port)
#ifdef PIOS_I2C_EXTERNAL_ADAPTER
            i2c_id = PIOS_I2C_EXTERNAL_ADAPTER;
#endif
        }

        if (i2c_id) {
            uint32_t external_mag = PIOS_HMC5x83_Init(hmc5x83_external_cfg, i2c_id, 0);
#  ifdef PIOS_INCLUDE_WDG
            // give HMC5x83 on I2C some extra time to allow for reset, etc. if needed
            // this is not in a loop, so it is safe
            PIOS_WDG_Clear();
#  endif /* PIOS_INCLUDE_WDG */
            // add this sensor to the sensor task's list
            // be careful that you don't register a slow, unimportant sensor after registering the fastest sensor
            // and before registering some other fast and important sensor
            // as that would cause delay and time jitter for the second fast sensor
            PIOS_HMC5x83_Register(external_mag, PIOS_SENSORS_TYPE_3AXIS_AUXMAG);
            // mag alarm is cleared later, so use I2C
            AlarmsSet(SYSTEMALARMS_ALARM_I2C, (external_mag) ? SYSTEMALARMS_ALARM_OK : SYSTEMALARMS_ALARM_WARNING);
        }
    }
# endif /* PIOS_INCLUDE_HMC5X83 */

    // internal ms5611 baro
#ifdef PIOS_INCLUDE_MS56XX
    const struct pios_ms56xx_cfg *ms56xx_cfg = PIOS_BOARD_HW_DEFS_GetMS56xxCfg(pios_board_info_blob.board_rev);
    if (ms56xx_cfg) {
        PIOS_MS56xx_Init(ms56xx_cfg, PIOS_I2C_MS56XX_INTERNAL_ADAPTER);
        PIOS_MS56xx_Register();
    }
#endif /* PIOS_INCLUDE_MS56XX */

#ifdef PIOS_INCLUDE_BMP280
    const struct pios_bmp280_cfg *bmp280_cfg = PIOS_BOARD_HW_DEFS_GetBMP280Cfg(pios_board_info_blob.board_rev);
    if (bmp280_cfg) {
        PIOS_BMP280_Init(bmp280_cfg, PIOS_I2C_BMP280_INTERNAL_ADAPTER);
    }
#endif

#ifdef PIOS_INCLUDE_ADC
    const struct pios_adc_cfg *adc_cfg = PIOS_BOARD_HW_DEFS_GetAdcCfg(pios_board_info_blob.board_rev);
    if (adc_cfg) {
        PIOS_ADC_Init(adc_cfg);
# ifndef PIOS_EXCLUDE_ADVANCED_FEATURES
        uint8_t adc_config[HWSETTINGS_ADCROUTING_NUMELEM];
        HwSettingsADCRoutingArrayGet(adc_config);
        for (uint32_t i = 0; i < HWSETTINGS_ADCROUTING_NUMELEM; i++) {
            if (adc_config[i] != HWSETTINGS_ADCROUTING_DISABLED) {
                PIOS_ADC_PinSetup(i);
            }
        }
# endif
    }
#endif /* PIOS_INCLUDE_ADC */

    // external ETASV3 Eagletree Airspeed v3
    // external MS4525D PixHawk Airpeed based on MS4525DO
    // bmp085/bmp180 baro
    // hmc5843 mag
    // i2c esc (?)
    // UBX DCC
}
