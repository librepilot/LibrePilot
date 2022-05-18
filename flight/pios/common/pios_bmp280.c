/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP280 Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_bmp280.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 *             Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @brief      BMP280 Pressure Sensor Routines
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
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
#ifdef PIOS_INCLUDE_BMP280
#include <pios_bmp280.h>
#include <pios_i2c.h>


#define BMP280_I2C_ADDR                               0x76
#define BMP280_ID                                     0xD0
#define BMP280_RESET                                  0xE0
#define BMP280_STATUS                                 0xF3
#define BMP280_CTRL_MEAS                              0xF4
#define BMP280_CONFIG                                 0xF5
#define BMP280_PRESS_MSB                              0xF7
#define BMP280_PRESS_LSB                              0xF8
#define BMP280_PRESS_XLSB                             0xF9
#define BMP280_TEMP_MSB                               0xFA
#define BMP280_TEMP_LSB                               0xFB
#define BMP280_TEMP_XLSB                              0xFC

#define BMP280_CAL_ADDR                               0x88

#define BMP280_P0                                     101.3250f

#define BMP280_MODE_CONTINUOUS                        0x03
#define BMP280_MODE_STANDBY                           0x00
#define BMP280_MODE_FORCED                            0x01

#define BMP280_T_STANDBY                              500

#define BMP280_DEFAULT_CHIP_ID                        0x58
#define BMP280_RESET_MAGIC                            0xB6

#define BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH 24
#define BMP280_DATA_FRAME_SIZE                        6

#define PIOS_BMP280_I2C_RETRIES                       5
#define PIOS_BMP280_I2C_CONFIG_RETRY_DELAY            1000000


enum pios_bmp280_dev_magic {
    PIOS_BMP280_DEV_MAGIC = 0x42323830
};

struct pios_bmp280_dev {
    enum pios_bmp280_dev_magic magic;
    uintptr_t i2c_id;

    bool sensorIsAlive;
    uint32_t  conversionDelayUs;
    uint32_t  configTime;
    uint32_t  conversionStart;

    uint8_t   oversampling;

    // compensation parameters
    uint16_t  digT1;
    int16_t   digT2;
    int16_t   digT3;
    uint16_t  digP1;
    int16_t   digP2;
    int16_t   digP3;
    int16_t   digP4;
    int16_t   digP5;
    int16_t   digP6;
    int16_t   digP7;
    int16_t   digP8;
    int16_t   digP9;

    PIOS_SENSORS_1Axis_SensorsWithTemp results;
};

static int32_t PIOS_BMP280_Read(uintptr_t i2c_id, uint8_t address, uint8_t *buffer, uint8_t len);
static int32_t PIOS_BMP280_Write(uintptr_t i2c_id, uint8_t address, uint8_t buffer);
static int32_t PIOS_BMP280_Configure(struct pios_bmp280_dev *dev);
static int32_t PIOS_BMP280_ReadPTCompensated(struct pios_bmp280_dev *dev,
                                             uint32_t *compensatedPressure,
                                             int32_t *compensatedTemperature);

// sensor driver interface
static bool PIOS_BMP280_driver_Test(uintptr_t context);
static void PIOS_BMP280_driver_Reset(uintptr_t context);
static void PIOS_BMP280_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
static void PIOS_BMP280_driver_fetch(void *, uint8_t size, uintptr_t context);
static bool PIOS_BMP280_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_BMP280_Driver = {
    .test      = PIOS_BMP280_driver_Test,
    .poll      = PIOS_BMP280_driver_poll,
    .fetch     = PIOS_BMP280_driver_fetch,
    .reset     = PIOS_BMP280_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_BMP280_driver_get_scale,
    .is_polled = true,
};

static bool PIOS_BMP280_Validate(struct pios_bmp280_dev *dev)
{
    return dev && (dev->magic == PIOS_BMP280_DEV_MAGIC);
}

/**
 * Initialise the BMP280 sensor
 */

void PIOS_BMP280_Init(const struct pios_bmp280_cfg *cfg, uint32_t i2c_device)
{
    struct pios_bmp280_dev *dev = (struct pios_bmp280_dev *)pios_malloc(sizeof(*dev));

    PIOS_Assert(dev);

    dev->magic  = PIOS_BMP280_DEV_MAGIC;
    dev->i2c_id = i2c_device;
    dev->sensorIsAlive = false;
    dev->oversampling  = cfg->oversampling;

    switch (cfg->oversampling) {
    case BMP280_STANDARD_RESOLUTION:
        dev->conversionDelayUs = 13300 + BMP280_T_STANDBY;
        break;
    case BMP280_HIGH_RESOLUTION:
        dev->conversionDelayUs = 22500 + BMP280_T_STANDBY;
        break;
    default:
    case BMP280_ULTRA_HIGH_RESOLUTION:
        dev->conversionDelayUs = 43200 + BMP280_T_STANDBY;
        break;
    }

    PIOS_BMP280_Configure(dev);

    PIOS_SENSORS_Register(&PIOS_BMP280_Driver, PIOS_SENSORS_TYPE_1AXIS_BARO, (uintptr_t)dev);
}

static int32_t PIOS_BMP280_Configure(struct pios_bmp280_dev *dev)
{
    // read chip id?
    uint8_t chip_id;

    if (dev->sensorIsAlive) {
        return 0;
    }

    if (PIOS_DELAY_DiffuS(dev->configTime) < PIOS_BMP280_I2C_CONFIG_RETRY_DELAY) { // Do not reinitialize too often
        return -1;
    }

    dev->configTime    = PIOS_DELAY_GetRaw();

    dev->sensorIsAlive = (PIOS_BMP280_Read(dev->i2c_id, BMP280_ID, &chip_id, sizeof(chip_id)) == 0);
    if (!dev->sensorIsAlive) {
        return -1;
    }

    if (chip_id != BMP280_DEFAULT_CHIP_ID) {
        return -2;
    }


    uint8_t data[BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH];

    dev->sensorIsAlive = (PIOS_BMP280_Read(dev->i2c_id, BMP280_CAL_ADDR, data, BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH) == 0);

    if (!dev->sensorIsAlive) {
        return -1;
    }

    dev->digT1 = (data[1] << 8) | data[0];
    dev->digT2 = (data[3] << 8) | data[2];
    dev->digT3 = (data[5] << 8) | data[4];
    dev->digP1 = (data[7] << 8) | data[6];
    dev->digP2 = (data[9] << 8) | data[8];
    dev->digP3 = (data[11] << 8) | data[10];
    dev->digP4 = (data[13] << 8) | data[12];
    dev->digP5 = (data[15] << 8) | data[14];
    dev->digP6 = (data[17] << 8) | data[16];
    dev->digP7 = (data[19] << 8) | data[18];
    dev->digP8 = (data[21] << 8) | data[20];
    dev->digP9 = (data[23] << 8) | data[22];

    dev->sensorIsAlive = (PIOS_BMP280_Write(dev->i2c_id, BMP280_RESET, BMP280_RESET_MAGIC) == 0);
    if (!dev->sensorIsAlive) {
        return -1;
    }

    /* start conversion */

    dev->sensorIsAlive   = (PIOS_BMP280_Write(dev->i2c_id, BMP280_CTRL_MEAS, dev->oversampling | BMP280_MODE_CONTINUOUS) == 0);

    dev->conversionStart = PIOS_DELAY_GetRaw();

    return 0;
}

static int32_t PIOS_BMP280_ReadPTCompensated(struct pios_bmp280_dev *dev,
                                             uint32_t *compensatedPressure,
                                             int32_t *compensatedTemperature)
{
    uint8_t data[BMP280_DATA_FRAME_SIZE];

    /* Read and store results */

    if (PIOS_BMP280_Read(dev->i2c_id, BMP280_PRESS_MSB, data, BMP280_DATA_FRAME_SIZE) != 0) {
        return -1;
    }

    static int32_t T = 0;

    int32_t raw_temperature = (int32_t)((((uint32_t)(data[3])) << 12) | (((uint32_t)(data[4])) << 4) | ((uint32_t)data[5] >> 4));

    int32_t varT1, varT2;

    varT1 = ((((raw_temperature >> 3) - ((int32_t)dev->digT1 << 1))) * ((int32_t)dev->digT2)) >> 11;
    varT2 = (((((raw_temperature >> 4) - ((int32_t)dev->digT1)) * ((raw_temperature >> 4) - ((int32_t)dev->digT1))) >> 12) * ((int32_t)dev->digT3)) >> 14;

    /* Filter T ourselves */
    if (!T) {
        T = (varT1 + varT2) * 5;
    } else {
        T = (varT1 + varT2) + (T * 4) / 5; // IIR Gain=5
    }

    *compensatedTemperature = T;

    int32_t raw_pressure = (int32_t)((((uint32_t)(data[0])) << 12) | (((uint32_t)(data[1])) << 4) | ((uint32_t)data[2] >> 4));

    if (raw_pressure == 0x80000) {
        return 1;
    }

    int64_t varP1, varP2, P;

    varP1 = ((int64_t)T / 5) - 128000;
    varP2 = varP1 * varP1 * (int64_t)dev->digP6;
    varP2 = varP2 + ((varP1 * (int64_t)dev->digP5) << 17);
    varP2 = varP2 + (((int64_t)dev->digP4) << 35);
    varP1 = ((varP1 * varP1 * (int64_t)dev->digP3) >> 8) + ((varP1 * (int64_t)dev->digP2) << 12);
    varP1 = (((((int64_t)1) << 47) + varP1)) * ((int64_t)dev->digP1) >> 33;
    if (varP1 == 0) {
        return 1; // avoid exception caused by division by zero
    }
    P     = 1048576 - raw_pressure;
    P     = (((P << 31) - varP2) * 3125) / varP1;
    varP1 = (((int64_t)dev->digP9) * (P >> 13) * (P >> 13)) >> 25;
    varP2 = (((int64_t)dev->digP8) * P) >> 19;
    *compensatedPressure = (uint32_t)((P + varP1 + varP2) >> 8) + (((int64_t)dev->digP7) << 4);

    return 0;
}

/**
 * Reads one or more bytes into a buffer
 * \param[in] the command indicating the address to read
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
static int32_t PIOS_BMP280_Read(uintptr_t i2c_id, uint8_t address, uint8_t *buffer, uint8_t len)
{
    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = BMP280_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 1,
            .buf  = &address,
        }
        ,
        {
            .info = __func__,
            .addr = BMP280_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    for (uint8_t retry = PIOS_BMP280_I2C_RETRIES; retry > 0; --retry) {
        if (PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list)) == 0) {
            return 0;
        }
    }

    return -1;
}

static int32_t PIOS_BMP280_Write(uintptr_t i2c_id, uint8_t address, uint8_t value)
{
    uint8_t data[] = { address, value };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = BMP280_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(data),
            .buf  = data,
        }
    };

    for (uint8_t retry = PIOS_BMP280_I2C_RETRIES; retry > 0; --retry) {
        if (PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list)) == 0) {
            return 0;
        }
    }

    return -1;
}

bool PIOS_BMP280_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return true;
}

static void PIOS_BMP280_driver_Reset(__attribute__((unused)) uintptr_t context)
{}

static void PIOS_BMP280_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

static void PIOS_BMP280_driver_fetch(void *data, __attribute__((unused)) uint8_t size, uintptr_t context)
{
    struct pios_bmp280_dev *dev = (struct pios_bmp280_dev *)context;

    PIOS_Assert(PIOS_BMP280_Validate(dev));
    PIOS_Assert(data);

    memcpy(data, (void *)&dev->results, sizeof(PIOS_SENSORS_1Axis_SensorsWithTemp));
}

static bool PIOS_BMP280_driver_poll(uintptr_t context)
{
    struct pios_bmp280_dev *dev = (struct pios_bmp280_dev *)context;

    PIOS_Assert(PIOS_BMP280_Validate(dev));

    if (!dev->sensorIsAlive) {
        if (PIOS_BMP280_Configure(dev) < 0) {
            return false;
        }
    }

    if (PIOS_DELAY_DiffuS(dev->conversionStart) < dev->conversionDelayUs) {
        return false;
    }

    dev->conversionStart = PIOS_DELAY_GetRaw();

    uint32_t cP = 0;
    int32_t cT  = 0;

    int32_t res = PIOS_BMP280_ReadPTCompensated(dev, &cP, &cT);

    dev->sensorIsAlive = (res >= 0);

    if (res != 0) {
        return false;
    }

    dev->results.temperature = ((float)cT) / 256.0f / 100.0f;
    dev->results.sample = ((float)cP) / 256.0f;

    return true;
}


#endif /* PIOS_INCLUDE_BMP280 */

/**
 * @}
 * @}
 */
