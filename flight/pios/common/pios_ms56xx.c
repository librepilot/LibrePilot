/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MS56XX MS56XX Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_ms56xx.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      MS56XX Pressure Sensor Routines
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
#ifdef PIOS_INCLUDE_MS56XX
#include <pios_ms56xx.h>
#define POW2(x) (1 << x)

// Command addresses
#define MS56XX_RESET               0x1E
#define MS56XX_CALIB_ADDR          0xA2 /* First sample is factory stuff */
#define MS56XX_CALIB_LEN           16
#define MS56XX_ADC_READ            0x00
#define MS56XX_PRES_ADDR           0x40
#define MS56XX_TEMP_ADDR           0x50

// Option to change the interleave between Temp and Pressure conversions
// Undef for normal operation
#define PIOS_MS56XX_SLOW_TEMP_RATE 20
#ifndef PIOS_MS56XX_SLOW_TEMP_RATE
#define PIOS_MS56XX_SLOW_TEMP_RATE 1
#endif
// Running moving average smoothing factor
#define PIOS_MS56XX_TEMP_SMOOTHING 10

#define PIOS_MS56XX_INIT_DELAY_US  1000000
#define PIOS_MS56XX_RESET_DELAY_US 20000

/* Local Types */
typedef struct {
    uint16_t C[6];
} MS56XXCalibDataTypeDef;

typedef enum {
    MS56XX_CONVERSION_TYPE_None = 0,
    MS56XX_CONVERSION_TYPE_PressureConv,
    MS56XX_CONVERSION_TYPE_TemperatureConv
} ConversionTypeTypeDef;

typedef enum {
    MS56XX_FSM_INIT = 0,
    MS56XX_FSM_CALIBRATION,
    MS56XX_FSM_TEMPERATURE,
    MS56XX_FSM_PRESSURE,
    MS56XX_FSM_CALCULATE,
} MS56XX_FSM_State;

static ConversionTypeTypeDef CurrentRead = MS56XX_CONVERSION_TYPE_None;

static MS56XXCalibDataTypeDef CalibData;

static uint8_t temp_press_interleave_count = 1;

/* Straight from the datasheet */
static uint32_t RawTemperature;
static uint32_t RawPressure;
static int64_t Pressure;
static int64_t Temperature;
static int64_t FilteredTemperature = INT32_MIN;

static uint32_t lastCommandTimeRaw;
static uint32_t commandDelayUs;

static uint32_t conversionDelayUs;

// Second order temperature compensation. Temperature offset
static int64_t compensation_t2;

// Move into proper driver structure with cfg stored
static enum pios_ms56xx_version version;
static uint8_t ms56xx_address;
static uint32_t oversampling;
static int32_t i2c_id;
static PIOS_SENSORS_1Axis_SensorsWithTemp results;
static bool hw_error = false;

// private functions
static int32_t PIOS_MS56xx_Read_I2C(uint8_t address, uint8_t *buffer, uint8_t len);
static int32_t PIOS_MS56xx_WriteCommand(uint8_t command, uint32_t delayuS);
static uint32_t PIOS_MS56xx_GetDelayUs(void);
static void PIOS_MS56xx_ReadCalibrationData(void);

// sensor driver interface
bool PIOS_MS56xx_driver_Test(uintptr_t context);
void PIOS_MS56xx_driver_Reset(uintptr_t context);
void PIOS_MS56xx_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
void PIOS_MS56xx_driver_fetch(void *, uint8_t size, uintptr_t context);
bool PIOS_MS56xx_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_MS56xx_Driver = {
    .test      = PIOS_MS56xx_driver_Test,
    .poll      = PIOS_MS56xx_driver_poll,
    .fetch     = PIOS_MS56xx_driver_fetch,
    .reset     = PIOS_MS56xx_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_MS56xx_driver_get_scale,
    .is_polled = true,
};
/**
 * Initialise the MS56XX sensor
 */
void PIOS_MS56xx_Init(const struct pios_ms56xx_cfg *cfg, int32_t i2c_device)
{
    i2c_id  = i2c_device;

    ms56xx_address = cfg->address;
    version = cfg->version;
    oversampling = cfg->oversampling;
    conversionDelayUs = PIOS_MS56xx_GetDelayUs();
}
/**
 * Start the ADC conversion
 * \param[in] PresOrTemp BMP085_PRES_ADDR or BMP085_TEMP_ADDR
 * \return 0 for success, -1 for failure (conversion completed and not read), -2 if failure occurred
 */
int32_t PIOS_MS56xx_StartADC(ConversionTypeTypeDef Type)
{
    /* Start the conversion */

    if (Type == MS56XX_CONVERSION_TYPE_TemperatureConv) {
        if (PIOS_MS56xx_WriteCommand(MS56XX_TEMP_ADDR + oversampling, conversionDelayUs) != 0) {
            return -2;
        }
    } else if (Type == MS56XX_CONVERSION_TYPE_PressureConv) {
        if (PIOS_MS56xx_WriteCommand(MS56XX_PRES_ADDR + oversampling, conversionDelayUs) != 0) {
            return -2;
        }
    }

    CurrentRead = Type;

    return 0;
}

/**
 * @brief Return the delay for the current osr in uS
 */
static uint32_t PIOS_MS56xx_GetDelayUs()
{
    switch (oversampling) {
    case MS56XX_OSR_256:
        return 600;

    case MS56XX_OSR_512:
        return 1170;

    case MS56XX_OSR_1024:
        return 2280;

    case MS56XX_OSR_2048:
        return 4540;

    case MS56XX_OSR_4096:
        return 9040;

    case MS56XX_OSR_8192:
        return 18080;

    default:
        break;
    }
    return 10;
}

/**
 * Read the ADC conversion value (once ADC conversion has completed)
 * \return 0 if successfully read the ADC, -1 if conversion time has not elapsed, -2 if failure occurred
 */
int32_t PIOS_MS56xx_ReadADC(void)
{
    uint8_t Data[3];

    Data[0] = 0;
    Data[1] = 0;
    Data[2] = 0;

    if (CurrentRead == MS56XX_CONVERSION_TYPE_None) {
        return -2;
    }

    static int64_t deltaTemp;

    if (PIOS_MS56xx_Read_I2C(MS56XX_ADC_READ, Data, 3) != 0) {
        return -2;
    }

    /* Read and store the 16bit result */
    if (CurrentRead == MS56XX_CONVERSION_TYPE_TemperatureConv) {
        RawTemperature = (Data[0] << 16) | (Data[1] << 8) | Data[2];
        // Difference between actual and reference temperature
        // dT = D2 - TREF = D2 - C5 * 2^8
        deltaTemp   = ((int32_t)RawTemperature) - (CalibData.C[4] * POW2(8));
        // Actual temperature (-40…85°C with 0.01°C resolution)
        // TEMP = 20°C + dT * TEMPSENS = 2000 + dT * C6 / 2^23
        Temperature = 2000l + ((deltaTemp * CalibData.C[5]) / POW2(23));
        if (FilteredTemperature != INT32_MIN) {
            FilteredTemperature = (FilteredTemperature * (PIOS_MS56XX_TEMP_SMOOTHING - 1)
                                   + Temperature) / PIOS_MS56XX_TEMP_SMOOTHING;
        } else {
            FilteredTemperature = Temperature;
        }
    } else {
        int64_t Offset;
        int64_t Sens;
        // used for second order temperature compensation
        int64_t Offset2 = 0;
        int64_t Sens2   = 0;

        // check if temperature is less than 20°C
        if (FilteredTemperature < 2000) {
            // Apply compensation
            // T2 = dT^2 / 2^31
            // OFF2 = 5 ⋅ (TEMP – 2000)^2/2
            // SENS2 = 5 ⋅ (TEMP – 2000)^2/2^2

            int64_t tcorr = (FilteredTemperature - 2000) * (FilteredTemperature - 2000);
            Offset2 = (5 * tcorr) / 2;
            Sens2   = (5 * tcorr) / 4;
            compensation_t2 = (deltaTemp * deltaTemp) >> 31;
            // Apply the "Very low temperature compensation" when temp is less than -15°C
            if (FilteredTemperature < -1500) {
                // OFF2 = OFF2 + 7 ⋅ (TEMP + 1500)^2
                // SENS2 = SENS2 + 11 ⋅ (TEMP + 1500)^2 / 2
                int64_t tcorr2 = (FilteredTemperature + 1500) * (FilteredTemperature + 1500);
                Offset2 += 7 * tcorr2;
                Sens2   += (11 * tcorr2) / 2;
            }
        } else {
            compensation_t2 = 0;
            Offset2 = 0;
            Sens2 = 0;
        }
        RawPressure = ((Data[0] << 16) | (Data[1] << 8) | Data[2]);

        // Offset and sensitivity at actual temperature
        if (version == MS56XX_VERSION_5611) {
            // OFF = OFFT1 + TCO * dT = C2 * 2^16 + (C4 * dT) / 2^7
            Offset = ((int64_t)CalibData.C[1]) * POW2(16) + (((int64_t)CalibData.C[3]) * deltaTemp) / POW2(7) - Offset2;
            // SENS = SENST1 + TCS * dT = C1 * 2^15 + (C3 * dT) / 2^8
            Sens   = ((int64_t)CalibData.C[0]) * POW2(15) + (((int64_t)CalibData.C[2]) * deltaTemp) / POW2(8) - Sens2;
        } else {
            // OFF = OFFT1 + TCO * dT = C2 * 2^17 + (C4 * dT) / 2^6
            Offset = ((int64_t)CalibData.C[1]) * POW2(17) + (((int64_t)CalibData.C[3]) * deltaTemp) / POW2(6) - Offset2;
            // SENS = SENST1 + TCS * dT = C1 * 2^16 + (C3 * dT) / 2^7
            Sens   = ((int64_t)CalibData.C[0]) * POW2(16) + (((int64_t)CalibData.C[2]) * deltaTemp) / POW2(7) - Sens2;
        }

        // Temperature compensated pressure (10…1200mbar with 0.01mbar resolution)
        // P = D1 * SENS - OFF = (D1 * SENS / 2^21 - OFF) / 2^15
        Pressure = (((((int64_t)RawPressure) * Sens) / POW2(21)) - Offset) / POW2(15);
    }

    CurrentRead = MS56XX_CONVERSION_TYPE_None;

    return 0;
}

/**
 * Return the most recently computed temperature in kPa
 */
static float PIOS_MS56xx_GetTemperature(void)
{
    // Apply the second order low and very low temperature compensation offset
    return ((float)(FilteredTemperature - compensation_t2)) / 100.0f;
}

/**
 * Return the most recently computed pressure in Pa
 */
static float PIOS_MS56xx_GetPressure(void)
{
    return (float)Pressure;
}

/**
 * Reads one or more bytes into a buffer
 * \param[in] the command indicating the address to read
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
static int32_t PIOS_MS56xx_Read_I2C(uint8_t address, uint8_t *buffer, uint8_t len)
{
    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = ms56xx_address,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 1,
            .buf  = &address,
        }
        ,
        {
            .info = __func__,
            .addr = ms56xx_address,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    enum pios_i2c_transfer_result i2c_result = PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));

    if (i2c_result == PIOS_I2C_TRANSFER_OK) {
        return 0;
    }

    if (i2c_result != PIOS_I2C_TRANSFER_BUSY) {
        hw_error = true;
    }

    return -1;
}

/**
 * Writes one or more bytes to the MS56XX
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
static int32_t PIOS_MS56xx_WriteCommand(uint8_t command, uint32_t delayuS)
{
    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = ms56xx_address,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 1,
            .buf  = &command,
        }
        ,
    };

    lastCommandTimeRaw = PIOS_DELAY_GetRaw();
    commandDelayUs     = delayuS;

    enum pios_i2c_transfer_result i2c_result = PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));

    if (i2c_result == PIOS_I2C_TRANSFER_OK) {
        return 0;
    }

    if (i2c_result != PIOS_I2C_TRANSFER_BUSY) {
        hw_error = true;
    }

    return -1;
}

static void PIOS_MS56xx_ReadCalibrationData()
{
    uint8_t data[2];

    /* reset temperature compensation values */
    compensation_t2 = 0;
    /* Calibration parameters */
    for (int i = 0; i < 6; i++) {
        if (PIOS_MS56xx_Read_I2C(MS56XX_CALIB_ADDR + i * 2, data, 2) != 0) {
            break;
        }
        CalibData.C[i] = (data[0] << 8) | data[1];
    }
}

static void PIOS_MS56xx_Reset()
{
    temp_press_interleave_count = 1;
    hw_error = false;

    PIOS_MS56xx_WriteCommand(MS56XX_RESET, PIOS_MS56XX_RESET_DELAY_US);
}


/**
 * @brief Run self-test operation.
 * \return 0 if self-test succeed, -1 if failed
 */
int32_t PIOS_MS56xx_Test()
{
    // TODO: Is there a better way to test this than just checking that pressure/temperature has changed?
    int32_t cur_value = 0;

    cur_value = Temperature;
    PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_TemperatureConv);
    PIOS_DELAY_WaitmS(10);
    PIOS_MS56xx_ReadADC();
    if (cur_value == Temperature) {
        return -1;
    }

    cur_value = Pressure;
    PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_PressureConv);
    PIOS_DELAY_WaitmS(10);
    PIOS_MS56xx_ReadADC();
    if (cur_value == Pressure) {
        return -1;
    }

    return 0;
}


/* PIOS sensor driver implementation */
void PIOS_MS56xx_Register()
{
    PIOS_SENSORS_Register(&PIOS_MS56xx_Driver, PIOS_SENSORS_TYPE_1AXIS_BARO, 0);
}

bool PIOS_MS56xx_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return true; // !PIOS_MS56xx_Test();
}

void PIOS_MS56xx_driver_Reset(__attribute__((unused)) uintptr_t context) {}

void PIOS_MS56xx_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

void PIOS_MS56xx_driver_fetch(void *data, __attribute__((unused)) uint8_t size, __attribute__((unused)) uintptr_t context)
{
    PIOS_Assert(data);
    memcpy(data, (void *)&results, sizeof(PIOS_SENSORS_1Axis_SensorsWithTemp));
}

bool PIOS_MS56xx_driver_poll(__attribute__((unused)) uintptr_t context)
{
    static MS56XX_FSM_State next_state = MS56XX_FSM_INIT;

    if (PIOS_DELAY_DiffuS(lastCommandTimeRaw) < commandDelayUs) {
        return false;
    }

    commandDelayUs = 0;

    PIOS_MS56xx_ReadADC();

    switch (next_state) {
    case MS56XX_FSM_INIT:
        PIOS_MS56xx_Reset();
        next_state = MS56XX_FSM_CALIBRATION;
        break;
    case MS56XX_FSM_CALIBRATION:
        PIOS_MS56xx_ReadCalibrationData();
    /* fall through to MS56XX_FSM_TEMPERATURE */

    case MS56XX_FSM_TEMPERATURE:
        PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_TemperatureConv);
        next_state = MS56XX_FSM_PRESSURE;
        break;

    case MS56XX_FSM_PRESSURE:
        PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_PressureConv);
        next_state = MS56XX_FSM_CALCULATE;
        break;

    case MS56XX_FSM_CALCULATE:
        temp_press_interleave_count--;
        if (!temp_press_interleave_count) {
            temp_press_interleave_count = PIOS_MS56XX_SLOW_TEMP_RATE;
            PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_TemperatureConv);
            next_state = MS56XX_FSM_PRESSURE;
        } else {
            PIOS_MS56xx_StartADC(MS56XX_CONVERSION_TYPE_PressureConv);
            next_state = MS56XX_FSM_CALCULATE;
        }

        results.temperature = PIOS_MS56xx_GetTemperature();
        results.sample = PIOS_MS56xx_GetPressure();
        return true;

    default:
        // it should not be there
        PIOS_Assert(0);
    }

    if (hw_error) {
        lastCommandTimeRaw = PIOS_DELAY_GetRaw();
        commandDelayUs     = (next_state == MS56XX_FSM_CALIBRATION) ? PIOS_MS56XX_INIT_DELAY_US : 0;
        CurrentRead = MS56XX_CONVERSION_TYPE_None;
        next_state = MS56XX_FSM_INIT;
    }

    return false;
}

/* Poll the pressure sensor and return the temperature and pressure. */
bool PIOS_MS56xx_Read(float *temperature, float *pressure)
{
    if (PIOS_MS56xx_driver_poll(0)) {
        *temperature = results.temperature;
        *pressure    = results.sample;
        return true;
    }
    return false;
}

#endif /* PIOS_INCLUDE_MS56XX */

/**
 * @}
 * @}
 */
