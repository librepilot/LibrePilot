/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MS5611 MS5611 Functions
 * @brief Hardware functions to deal with the altitude pressure sensor
 * @{
 *
 * @file       pios_ms5611.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      MS5611 Pressure Sensor Routines
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
#ifdef PIOS_INCLUDE_MS5611
#include <pios_ms5611.h>
#define POW2(x) (1 << x)

// TODO: Clean this up.  Getting around old constant.
#define PIOS_MS5611_OVERSAMPLING   oversampling

// Option to change the interleave between Temp and Pressure conversions
// Undef for normal operation
#define PIOS_MS5611_SLOW_TEMP_RATE 20
#ifndef PIOS_MS5611_SLOW_TEMP_RATE
#define PIOS_MS5611_SLOW_TEMP_RATE 1
#endif
// Running moving average smoothing factor
#define PIOS_MS5611_TEMP_SMOOTHING 10
//
/* Local Types */
typedef struct {
    uint16_t C[6];
} MS5611CalibDataTypeDef;

typedef enum {
    MS5611_CONVERSION_TYPE_None = 0,
    MS5611_CONVERSION_TYPE_PressureConv,
    MS5611_CONVERSION_TYPE_TemperatureConv
} ConversionTypeTypeDef;

typedef enum {
    MS5611_FSM_INIT = 0,
    MS5611_FSM_TEMPERATURE,
    MS5611_FSM_PRESSURE,
    MS5611_FSM_CALCULATE,
} MS5611_FSM_State;

/* Glocal Variables */
ConversionTypeTypeDef CurrentRead = MS5611_CONVERSION_TYPE_None;

/* Local Variables */
MS5611CalibDataTypeDef CalibData;

/* Straight from the datasheet */
static uint32_t RawTemperature;
static uint32_t RawPressure;
static int64_t Pressure;
static int64_t Temperature;
static int64_t FilteredTemperature = INT32_MIN;
static int32_t lastConversionStart;

static uint32_t conversionDelayMs;
static uint32_t conversionDelayUs;

static int32_t PIOS_MS5611_Read(uint8_t address, uint8_t *buffer, uint8_t len);
static int32_t PIOS_MS5611_WriteCommand(uint8_t command);
static uint32_t PIOS_MS5611_GetDelay();
static uint32_t PIOS_MS5611_GetDelayUs();

// Second order temperature compensation. Temperature offset
static int64_t compensation_t2;

// Move into proper driver structure with cfg stored
static uint32_t oversampling;
static const struct pios_ms5611_cfg *dev_cfg;
static int32_t i2c_id;
static PIOS_SENSORS_1Axis_SensorsWithTemp results;

// sensor driver interface
bool PIOS_MS5611_driver_Test(uintptr_t context);
void PIOS_MS5611_driver_Reset(uintptr_t context);
void PIOS_MS5611_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
void PIOS_MS5611_driver_fetch(void *, uint8_t size, uintptr_t context);
bool PIOS_MS5611_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_MS5611_Driver = {
    .test      = PIOS_MS5611_driver_Test,
    .poll      = PIOS_MS5611_driver_poll,
    .fetch     = PIOS_MS5611_driver_fetch,
    .reset     = PIOS_MS5611_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_MS5611_driver_get_scale,
    .is_polled = true,
};
/**
 * Initialise the MS5611 sensor
 */
int32_t ms5611_read_flag;
void PIOS_MS5611_Init(const struct pios_ms5611_cfg *cfg, int32_t i2c_device)
{
    i2c_id = i2c_device;

    oversampling = cfg->oversampling;
    conversionDelayMs = PIOS_MS5611_GetDelay();
    conversionDelayUs = PIOS_MS5611_GetDelayUs();

    dev_cfg = cfg; // Store cfg before enabling interrupt

    PIOS_MS5611_WriteCommand(MS5611_RESET);
    PIOS_DELAY_WaitmS(20);

    uint8_t data[2];

    // reset temperature compensation values
    compensation_t2 = 0;
    /* Calibration parameters */
    for (int i = 0; i < 6; i++) {
        while (PIOS_MS5611_Read(MS5611_CALIB_ADDR + i * 2, data, 2)) {}
        ;
        CalibData.C[i] = (data[0] << 8) | data[1];
    }
}

/**
 * Start the ADC conversion
 * \param[in] PresOrTemp BMP085_PRES_ADDR or BMP085_TEMP_ADDR
 * \return 0 for success, -1 for failure (conversion completed and not read)
 */
int32_t PIOS_MS5611_StartADC(ConversionTypeTypeDef Type)
{
    /* Start the conversion */
    if (Type == MS5611_CONVERSION_TYPE_TemperatureConv) {
        while (PIOS_MS5611_WriteCommand(MS5611_TEMP_ADDR + oversampling) != 0) {
            continue;
        }
    } else if (Type == MS5611_CONVERSION_TYPE_PressureConv) {
        while (PIOS_MS5611_WriteCommand(MS5611_PRES_ADDR + oversampling) != 0) {
            continue;
        }
    }
    lastConversionStart = PIOS_DELAY_GetRaw();
    CurrentRead = Type;

    return 0;
}

/**
 * @brief Return the delay for the current osr
 */
static uint32_t PIOS_MS5611_GetDelay()
{
    switch (oversampling) {
    case MS5611_OSR_256:
        return 1;

    case MS5611_OSR_512:
        return 2;

    case MS5611_OSR_1024:
        return 3;

    case MS5611_OSR_2048:
        return 5;

    case MS5611_OSR_4096:
        return 10;

    default:
        break;
    }
    return 10;
}

/**
 * @brief Return the delay for the current osr in uS
 */
static uint32_t PIOS_MS5611_GetDelayUs()
{
    switch (oversampling) {
    case MS5611_OSR_256:
        return 600;

    case MS5611_OSR_512:
        return 1170;

    case MS5611_OSR_1024:
        return 2280;

    case MS5611_OSR_2048:
        return 4540;

    case MS5611_OSR_4096:
        return 9040;

    default:
        break;
    }
    return 10;
}

/**
 * Read the ADC conversion value (once ADC conversion has completed)
 * \return 0 if successfully read the ADC, -1 if conversion time has not elapsed, -2 if failure occurred
 */
int32_t PIOS_MS5611_ReadADC(void)
{
    uint8_t Data[3];

    Data[0] = 0;
    Data[1] = 0;
    Data[2] = 0;

    if (CurrentRead == MS5611_CONVERSION_TYPE_None) {
        return -2;
    }
    if (conversionDelayUs > PIOS_DELAY_DiffuS(lastConversionStart)) {
        return -1;
    }
    static int64_t deltaTemp;

    /* Read and store the 16bit result */
    if (CurrentRead == MS5611_CONVERSION_TYPE_TemperatureConv) {
        /* Read the temperature conversion */
        if (PIOS_MS5611_Read(MS5611_ADC_READ, Data, 3) != 0) {
            return -2;
        }

        RawTemperature = (Data[0] << 16) | (Data[1] << 8) | Data[2];
        // Difference between actual and reference temperature
        // dT = D2 - TREF = D2 - C5 * 2^8
        deltaTemp   = ((int32_t)RawTemperature) - (CalibData.C[4] * POW2(8));
        // Actual temperature (-40…85°C with 0.01°C resolution)
        // TEMP = 20°C + dT * TEMPSENS = 2000 + dT * C6 / 2^23
        Temperature = 2000l + ((deltaTemp * CalibData.C[5]) / POW2(23));
        if (FilteredTemperature != INT32_MIN) {
            FilteredTemperature = (FilteredTemperature * (PIOS_MS5611_TEMP_SMOOTHING - 1)
                                   + Temperature) / PIOS_MS5611_TEMP_SMOOTHING;
        } else {
            FilteredTemperature = Temperature;
        }
    } else {
        int64_t Offset;
        int64_t Sens;
        // used for second order temperature compensation
        int64_t Offset2 = 0;
        int64_t Sens2   = 0;

        /* Read the pressure conversion */
        if (PIOS_MS5611_Read(MS5611_ADC_READ, Data, 3) != 0) {
            return -2;
        }
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
        // Offset at actual temperature
        // OFF = OFFT1 + TCO * dT = C2 * 2^16 + (C4 * dT) / 2^7
        Offset   = ((int64_t)CalibData.C[1]) * POW2(16) + (((int64_t)CalibData.C[3]) * deltaTemp) / POW2(7) - Offset2;
        // Sensitivity at actual temperature
        // SENS = SENST1 + TCS * dT = C1 * 2^15 + (C3 * dT) / 2^8
        Sens     = ((int64_t)CalibData.C[0]) * POW2(15) + (((int64_t)CalibData.C[2]) * deltaTemp) / POW2(8) - Sens2;
        // Temperature compensated pressure (10…1200mbar with 0.01mbar resolution)
        // P = D1 * SENS - OFF = (D1 * SENS / 2^21 - OFF) / 2^15
        Pressure = (((((int64_t)RawPressure) * Sens) / POW2(21)) - Offset) / POW2(15);
    }
    return 0;
}

/**
 * Return the most recently computed temperature in kPa
 */
static float PIOS_MS5611_GetTemperature(void)
{
    // Apply the second order low and very low temperature compensation offset
    return ((float)(FilteredTemperature - compensation_t2)) / 100.0f;
}

/**
 * Return the most recently computed pressure in Pa
 */
static float PIOS_MS5611_GetPressure(void)
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
static int32_t PIOS_MS5611_Read(uint8_t address, uint8_t *buffer, uint8_t len)
{
    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = MS5611_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 1,
            .buf  = &address,
        }
        ,
        {
            .info = __func__,
            .addr = MS5611_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    return PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
 * Writes one or more bytes to the MS5611
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
static int32_t PIOS_MS5611_WriteCommand(uint8_t command)
{
    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = MS5611_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 1,
            .buf  = &command,
        }
        ,
    };

    return PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
 * @brief Run self-test operation.
 * \return 0 if self-test succeed, -1 if failed
 */
int32_t PIOS_MS5611_Test()
{
    // TODO: Is there a better way to test this than just checking that pressure/temperature has changed?
    int32_t cur_value = 0;

    cur_value = Temperature;
    PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_TemperatureConv);
    PIOS_DELAY_WaitmS(10);
    PIOS_MS5611_ReadADC();
    if (cur_value == Temperature) {
        return -1;
    }

    cur_value = Pressure;
    PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_PressureConv);
    PIOS_DELAY_WaitmS(10);
    PIOS_MS5611_ReadADC();
    if (cur_value == Pressure) {
        return -1;
    }

    return 0;
}


/* PIOS sensor driver implementation */
void PIOS_MS5611_Register()
{
    PIOS_SENSORS_Register(&PIOS_MS5611_Driver, PIOS_SENSORS_TYPE_1AXIS_BARO, 0);
}

bool PIOS_MS5611_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return true; // !PIOS_MS5611_Test();
}

void PIOS_MS5611_driver_Reset(__attribute__((unused)) uintptr_t context) {}

void PIOS_MS5611_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

void PIOS_MS5611_driver_fetch(void *data, __attribute__((unused)) uint8_t size, __attribute__((unused)) uintptr_t context)
{
    PIOS_Assert(data);
    memcpy(data, (void *)&results, sizeof(PIOS_SENSORS_1Axis_SensorsWithTemp));
}

bool PIOS_MS5611_driver_poll(__attribute__((unused)) uintptr_t context)
{
    static uint8_t temp_press_interleave_count = 1;
    static MS5611_FSM_State next_state = MS5611_FSM_INIT;

    int32_t conversionResult = PIOS_MS5611_ReadADC();

    if (__builtin_expect(conversionResult == -1, 1)) {
        return false; // wait for conversion to complete
    } else if (__builtin_expect(conversionResult == -2, 0)) {
        temp_press_interleave_count = 1;
        next_state = MS5611_FSM_INIT;
    }
    switch (next_state) {
    case MS5611_FSM_INIT:
    case MS5611_FSM_TEMPERATURE:
        PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_TemperatureConv);
        next_state = MS5611_FSM_PRESSURE;
        return false;

    case MS5611_FSM_PRESSURE:
        PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_PressureConv);
        next_state = MS5611_FSM_CALCULATE;
        return false;

    case MS5611_FSM_CALCULATE:
        temp_press_interleave_count--;
        if (!temp_press_interleave_count) {
            temp_press_interleave_count = PIOS_MS5611_SLOW_TEMP_RATE;
            PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_TemperatureConv);
            next_state = MS5611_FSM_PRESSURE;
        } else {
            PIOS_MS5611_StartADC(MS5611_CONVERSION_TYPE_PressureConv);
            next_state = MS5611_FSM_CALCULATE;
        }

        results.temperature = PIOS_MS5611_GetTemperature();
        results.sample = PIOS_MS5611_GetPressure();
        return true;

    default:
        // it should not be there
        PIOS_Assert(0);
    }
    return false;
}


#endif /* PIOS_INCLUDE_MS5611 */

/**
 * @}
 * @}
 */
