/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_HMC5x83 HMC5x83 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 * @file       pios_hmc5x83.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      HMC5x83 Magnetic Sensor Functions from AHRS
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

#include "pios.h"
#include <pios_hmc5x83.h>
#include <pios_mem.h>

#ifdef PIOS_INCLUDE_HMC5X83

#define PIOS_HMC5X83_MAGIC 0x4d783833
/* Global Variables */

/* Local Types */

typedef struct {
    uint32_t magic;
    const struct pios_hmc5x83_cfg *cfg;
    uint32_t port_id;
    uint8_t  slave_num;
    uint8_t  CTRLB;
    uint16_t magCountMax;
    uint16_t magCount;
    volatile bool data_ready;
    int16_t  magData[3];
    bool     hw_error;
    uint32_t lastConfigTime;
} pios_hmc5x83_dev_data_t;

static int32_t PIOS_HMC5x83_Config(pios_hmc5x83_dev_data_t *dev);

// sensor driver interface
bool PIOS_HMC5x83_driver_Test(uintptr_t context);
void PIOS_HMC5x83_driver_Reset(uintptr_t context);
void PIOS_HMC5x83_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
void PIOS_HMC5x83_driver_fetch(void *, uint8_t size, uintptr_t context);
bool PIOS_HMC5x83_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_HMC5x83_Driver = {
    .test      = PIOS_HMC5x83_driver_Test,
    .poll      = PIOS_HMC5x83_driver_poll,
    .fetch     = PIOS_HMC5x83_driver_fetch,
    .reset     = PIOS_HMC5x83_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_HMC5x83_driver_get_scale,
    .is_polled = true,
};

/**
 * Allocate the device setting structure
 * @return pios_hmc5x83_dev_data_t pointer to newly created structure
 */
pios_hmc5x83_dev_data_t *dev_alloc()
{
    pios_hmc5x83_dev_data_t *dev = (pios_hmc5x83_dev_data_t *)pios_malloc(sizeof(pios_hmc5x83_dev_data_t));

    PIOS_DEBUG_Assert(dev);
    memset(dev, 0x00, sizeof(pios_hmc5x83_dev_data_t));
    dev->magic = PIOS_HMC5X83_MAGIC;
    return dev;
}

/**
 * Validate a pios_hmc5x83_dev_t handler and return the related pios_hmc5x83_dev_data_t pointer
 * @param dev device handler
 * @return the device data structure
 */
pios_hmc5x83_dev_data_t *dev_validate(pios_hmc5x83_dev_t dev)
{
    pios_hmc5x83_dev_data_t *dev_data = (pios_hmc5x83_dev_data_t *)dev;

    PIOS_DEBUG_Assert(dev_data->magic == PIOS_HMC5X83_MAGIC);
    return dev_data;
}

/**
 * @brief Initialize the HMC5x83 magnetometer sensor.
 * @return none
 */
pios_hmc5x83_dev_t PIOS_HMC5x83_Init(const struct pios_hmc5x83_cfg *cfg, uint32_t port_id, uint8_t slave_num)
{
    pios_hmc5x83_dev_data_t *dev = dev_alloc();

    dev->cfg       = cfg; // store config before enabling interrupt
    dev->port_id   = port_id;
    dev->slave_num = slave_num;

#ifdef PIOS_HMC5X83_HAS_GPIOS
    if (cfg->exti_cfg) {
        PIOS_EXTI_Init(cfg->exti_cfg);
    } else
#endif /* PIOS_HMC5X83_HAS_GPIOS */
    {
// if PIOS_SENSOR_RATE is defined, there is a sensor loop that is called at that frequency
// and "is data available" can simply return false a few times to save some CPU
#ifdef PIOS_SENSOR_RATE
        // for external mags that have no interrupt line, just poll them with a timer
        // use the configured Output Data Rate to calculate the number of interations (of the main sensor task loop)
        // to return false, before returning true
        uint16_t rate100;
        switch (cfg->M_ODR) {
        case PIOS_HMC5x83_ODR_0_75:
            rate100 = 75;
            break;
        case PIOS_HMC5x83_ODR_1_5:
            rate100 = 150;
            break;
        case PIOS_HMC5x83_ODR_3:
            rate100 = 300;
            break;
        case PIOS_HMC5x83_ODR_7_5:
            rate100 = 750;
            break;
        case PIOS_HMC5x83_ODR_15:
            rate100 = 1500;
            break;
        case PIOS_HMC5x83_ODR_30:
            rate100 = 3000;
            break;
        default:
        case PIOS_HMC5x83_ODR_75:
            rate100 = 7500;
            break;
        }
        // if the application sensor rate is fast enough to warrant skipping some slow hardware sensor reads
        if ((PIOS_SENSOR_RATE * 100.0f / 3.0f) > rate100) {
            // count the number of "return false" up to this number
            dev->magCountMax = ((uint16_t)PIOS_SENSOR_RATE * 100 / rate100) + 1;
        } else {
            // return true every time (do a hardware sensor poll every time)
            dev->magCountMax = 1;
        }
#else /* PIOS_SENSOR_RATE */
        // return true every time (do a hardware sensor poll every time)
        dev->magCountMax = 1;
#endif /* PIOS_SENSOR_RATE */
        // with this counter
        dev->magCount    = 0;
    }

    if (PIOS_HMC5x83_Config(dev) != 0) {
        dev->hw_error = true;
    }

    dev->data_ready = false;
    return (pios_hmc5x83_dev_t)dev;
}

void PIOS_HMC5x83_Register(pios_hmc5x83_dev_t handler, PIOS_SENSORS_TYPE sensortype)
{
    if (handler) {
        PIOS_SENSORS_Register(&PIOS_HMC5x83_Driver, sensortype, handler);
    }
}

/**
 * @brief Initialize the HMC5x83 magnetometer sensor
 * \return none
 * \param[in] pios_hmc5x83_dev_data_t device config to be used.
 * \param[in] PIOS_HMC5x83_ConfigTypeDef struct to be used to configure sensor.
 *
 * CTRL_REGA: Control Register A
 * Read Write
 * Default value: 0x10
 * 7:5  0   These bits must be cleared for correct operation.
 * 4:2 DO2-DO0: Data Output Rate Bits
 *             DO2 |  DO1 |  DO0 |   Minimum Data Output Rate (Hz)
 *            ------------------------------------------------------
 *              0  |  0   |  0   |            0.75
 *              0  |  0   |  1   |            1.5
 *              0  |  1   |  0   |            3
 *              0  |  1   |  1   |            7.5
 *              1  |  0   |  0   |           15 (default)
 *              1  |  0   |  1   |           30
 *              1  |  1   |  0   |           75
 *              1  |  1   |  1   |           Not Used
 * 1:0 MS1-MS0: Measurement Configuration Bits
 *             MS1 | MS0 |   MODE
 *            ------------------------------
 *              0  |  0   |  Normal
 *              0  |  1   |  Positive Bias
 *              1  |  0   |  Negative Bias
 *              1  |  1   |  Not Used
 *
 * CTRL_REGB: Control RegisterB
 * Read Write
 * Default value: 0x20
 * 7:5 GN2-GN0: Gain Configuration Bits.
 *             GN2 |  GN1 |  GN0 |   Mag Input   | Gain       | Output Range
 *                 |      |      |  Range[Ga]    | [LSB/mGa]  |
 *            ------------------------------------------------------
 *              0  |  0   |  0   |  ±0.88Ga      |   1370     | 0xF8000x07FF (-2048:2047)
 *              0  |  0   |  1   |  ±1.3Ga (def) |   1090     | 0xF8000x07FF (-2048:2047)
 *              0  |  1   |  0   |  ±1.9Ga       |   820      | 0xF8000x07FF (-2048:2047)
 *              0  |  1   |  1   |  ±2.5Ga       |   660      | 0xF8000x07FF (-2048:2047)
 *              1  |  0   |  0   |  ±4.0Ga       |   440      | 0xF8000x07FF (-2048:2047)
 *              1  |  0   |  1   |  ±4.7Ga       |   390      | 0xF8000x07FF (-2048:2047)
 *              1  |  1   |  0   |  ±5.6Ga       |   330      | 0xF8000x07FF (-2048:2047)
 *              1  |  1   |  1   |  ±8.1Ga       |   230      | 0xF8000x07FF (-2048:2047)
 *                               |Not recommended|
 *
 * 4:0 CRB4-CRB: 0 This bit must be cleared for correct operation.
 *
 * _MODE_REG: Mode Register
 * Read Write
 * Default value: 0x02
 * 7:2  0   These bits must be cleared for correct operation.
 * 1:0 MD1-MD0: Mode Select Bits
 *             MS1 | MS0 |   MODE
 *            ------------------------------
 *              0  |  0   |  Continuous-Conversion Mode.
 *              0  |  1   |  Single-Conversion Mode
 *              1  |  0   |  Negative Bias
 *              1  |  1   |  Sleep Mode
 */
static int32_t PIOS_HMC5x83_Config(pios_hmc5x83_dev_data_t *dev)
{
    uint8_t CTRLA = 0x00;
    uint8_t MODE  = 0x00;

    dev->lastConfigTime = PIOS_DELAY_GetRaw();

    const struct pios_hmc5x83_cfg *cfg = dev->cfg;

    dev->CTRLB = 0;

    CTRLA |= (uint8_t)(cfg->M_ODR | cfg->Meas_Conf);
    CTRLA |= cfg->TempCompensation ? PIOS_HMC5x83_CTRLA_TEMP : 0;
    dev->CTRLB |= (uint8_t)(cfg->Gain);
    MODE  |= (uint8_t)(cfg->Mode);

    // CRTL_REGA
    if (cfg->Driver->Write((pios_hmc5x83_dev_t)dev, PIOS_HMC5x83_CONFIG_REG_A, CTRLA) != 0) {
        return -1;
    }

    // CRTL_REGB
    if (cfg->Driver->Write((pios_hmc5x83_dev_t)dev, PIOS_HMC5x83_CONFIG_REG_B, dev->CTRLB) != 0) {
        return -1;
    }

    // Mode register
    if (cfg->Driver->Write((pios_hmc5x83_dev_t)dev, PIOS_HMC5x83_MODE_REG, MODE) != 0) {
        return -1;
    }

#ifdef PIOS_INCLUDE_WDG
    // give HMC5x83 on I2C some extra time
    PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

    if (PIOS_HMC5x83_Test((pios_hmc5x83_dev_t)dev) != 0) {
        return -1;
    }

    return 0;
}

static void PIOS_HMC5x83_Orient(enum PIOS_HMC5X83_ORIENTATION orientation, int16_t in[3], int16_t out[3])
{
    switch (orientation) {
    case PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP:
        out[0] = in[2];
        out[1] = in[0];
        out[2] = -in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_SOUTH_EAST_UP:
        out[0] = -in[0];
        out[1] = in[2];
        out[2] = -in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_WEST_SOUTH_UP:
        out[0] = -in[2];
        out[1] = -in[0];
        out[2] = -in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_NORTH_WEST_UP:
        out[0] = in[0];
        out[1] = -in[2];
        out[2] = -in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_EAST_SOUTH_DOWN:
        out[0] = in[2];
        out[1] = -in[0];
        out[2] = in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_SOUTH_WEST_DOWN:
        out[0] = -in[0];
        out[1] = -in[2];
        out[2] = in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_WEST_NORTH_DOWN:
        out[0] = -in[2];
        out[1] = in[0];
        out[2] = in[1];
        break;
    case PIOS_HMC5X83_ORIENTATION_NORTH_EAST_DOWN:
        out[0] = in[0];
        out[1] = in[2];
        out[2] = in[1];
        break;
    }
}

/**
 * @brief Read current X, Z, Y values (in that order)
 * \param[in] dev device handler
 * \param[out] int16_t array of size 3 to store X, Z, and Y magnetometer readings
 * \return 0 for success or -1 for failure
 */
int32_t PIOS_HMC5x83_ReadMag(pios_hmc5x83_dev_t handler, int16_t out[3])
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

    dev->data_ready = false;
    uint8_t buffer[6];
    int16_t temp[3];
    int32_t sensitivity;

    if (dev->cfg->Driver->Read(handler, PIOS_HMC5x83_DATAOUT_XMSB_REG, buffer, 6) != 0) {
        return -1;
    }

    switch (dev->CTRLB & 0xE0) {
    case 0x00:
        sensitivity = PIOS_HMC5x83_Sensitivity_0_88Ga;
        break;
    case 0x20:
        sensitivity = PIOS_HMC5x83_Sensitivity_1_3Ga;
        break;
    case 0x40:
        sensitivity = PIOS_HMC5x83_Sensitivity_1_9Ga;
        break;
    case 0x60:
        sensitivity = PIOS_HMC5x83_Sensitivity_2_5Ga;
        break;
    case 0x80:
        sensitivity = PIOS_HMC5x83_Sensitivity_4_0Ga;
        break;
    case 0xA0:
        sensitivity = PIOS_HMC5x83_Sensitivity_4_7Ga;
        break;
    case 0xC0:
        sensitivity = PIOS_HMC5x83_Sensitivity_5_6Ga;
        break;
    case 0xE0:
        sensitivity = PIOS_HMC5x83_Sensitivity_8_1Ga;
        break;
    default:
        PIOS_Assert(0);
    }
    for (int i = 0; i < 3; i++) {
        int16_t v = ((int16_t)((uint16_t)buffer[2 * i] << 8)
                     + buffer[2 * i + 1]) * 1000 / sensitivity;
        temp[i] = v;
    }

    PIOS_HMC5x83_Orient(dev->cfg->Orientation, temp, out);

    // "This should not be necessary but for some reason it is coming out of continuous conversion mode"
    //
    // By default the chip is in single read mode meaning after reading from it once, it will go idle to save power.
    // Once idle, we have write to it to turn it on before we can read from it again.
    // To conserve current between measurements, the device is placed in a state similar to idle mode, but the
    // Mode Register is not changed to Idle Mode.  That is, MD[n] bits are unchanged.
    dev->cfg->Driver->Write(handler, PIOS_HMC5x83_MODE_REG, PIOS_HMC5x83_MODE_CONTINUOUS);

    return 0;
}


/**
 * @brief Read the identification bytes from the HMC5x83 sensor
 * \param[out] uint8_t array of size 4 to store HMC5x83 ID.
 * \return 0 if successful, -1 if not
 */
uint8_t PIOS_HMC5x83_ReadID(pios_hmc5x83_dev_t handler, uint8_t out[4])
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);
    uint8_t retval = dev->cfg->Driver->Read(handler, PIOS_HMC5x83_DATAOUT_IDA_REG, out, 3);

    out[3] = '\0';
    return retval;
}

/**
 * @brief Tells whether new magnetometer readings are available
 * \return true if new data is available
 * \return false if new data is not available
 */
bool PIOS_HMC5x83_NewDataAvailable(__attribute__((unused)) pios_hmc5x83_dev_t handler)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

#ifdef PIOS_HMC5X83_HAS_GPIOS
    if (dev->cfg->exti_cfg) { // if this device has an interrupt line attached, then wait for interrupt to say data is ready
        return dev->data_ready;
    } else
#endif /* PIOS_HMC5X83_HAS_GPIOS */
    { // else poll to see if data is ready or just say "true" and set polling interval elsewhere
        if (++(dev->magCount) >= dev->magCountMax) {
            dev->magCount = 0;
            return true;
        } else {
            return false;
        }
    }
}

/**
 * @brief Run self-test operation.  Do not call this during operational use!!
 * \return 0 if success, -1 if test failed
 */
int32_t PIOS_HMC5x83_Test(pios_hmc5x83_dev_t handler)
{
    int32_t failed = 0;
    uint8_t registers[3] = { 0, 0, 0 };
    uint8_t status;
    uint8_t ctrl_a_read;
    uint8_t ctrl_b_read;
    uint8_t mode_read;
    int16_t values[3];
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

#ifdef PIOS_INCLUDE_WDG
    // give HMC5x83 on I2C some extra time to allow for reset, etc. if needed
    // this is not in a loop, so it is safe
    PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

    /* Verify that ID matches (HMC5x83 ID is null-terminated ASCII string "H43") */
    char id[4];

    PIOS_HMC5x83_ReadID(handler, (uint8_t *)id);
    if ((id[0] != 'H') || (id[1] != '4') || (id[2] != '3')) { // Expect H43
        return -1;
    }

    /* Backup existing configuration */
    if (dev->cfg->Driver->Read(handler, PIOS_HMC5x83_CONFIG_REG_A, registers, 3) != 0) {
        return -1;
    }

    /* Stop the device and read out last value */
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_MODE_REG, PIOS_HMC5x83_MODE_IDLE) != 0) {
        return -1;
    }
    if (dev->cfg->Driver->Read(handler, PIOS_HMC5x83_DATAOUT_STATUS_REG, &status, 1) != 0) {
        return -1;
    }
    if (PIOS_HMC5x83_ReadMag(handler, values) != 0) {
        return -1;
    }

    /*
     * Put HMC5x83 into self test mode
     * This is done by placing measurement config into positive (0x01) or negative (0x10) bias
     * and then placing the mode register into single-measurement mode.  This causes the HMC5x83
     * to create an artificial magnetic field of ~1.1 Gauss.
     *
     * If gain were PIOS_HMC5x83_GAIN_2_5, for example, X and Y will read around +766 LSB
     * (1.16 Ga * 660 LSB/Ga) and Z would read around +713 LSB (1.08 Ga * 660 LSB/Ga)
     *
     * Changing measurement config back to PIOS_HMC5x83_MEASCONF_NORMAL will leave self-test mode.
     */
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_CONFIG_REG_A, PIOS_HMC5x83_MEASCONF_BIAS_POS | PIOS_HMC5x83_ODR_15) != 0) {
        return -1;
    }
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_CONFIG_REG_B, PIOS_HMC5x83_GAIN_8_1) != 0) {
        return -1;
    }
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_MODE_REG, PIOS_HMC5x83_MODE_SINGLE) != 0) {
        return -1;
    }

    /* Must wait for value to be updated */
    PIOS_DELAY_WaitmS(200);

    if (PIOS_HMC5x83_ReadMag(handler, values) != 0) {
        return -1;
    }

    dev->cfg->Driver->Read(handler, PIOS_HMC5x83_CONFIG_REG_A, &ctrl_a_read, 1);
    dev->cfg->Driver->Read(handler, PIOS_HMC5x83_CONFIG_REG_B, &ctrl_b_read, 1);
    dev->cfg->Driver->Read(handler, PIOS_HMC5x83_MODE_REG, &mode_read, 1);
    dev->cfg->Driver->Read(handler, PIOS_HMC5x83_DATAOUT_STATUS_REG, &status, 1);


    /* Restore backup configuration */
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_CONFIG_REG_A, registers[0]) != 0) {
        return -1;
    }
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_CONFIG_REG_B, registers[1]) != 0) {
        return -1;
    }
    PIOS_DELAY_WaitmS(10);
    if (dev->cfg->Driver->Write(handler, PIOS_HMC5x83_MODE_REG, registers[2]) != 0) {
        return -1;
    }

    return failed;
}

#ifdef PIOS_HMC5X83_HAS_GPIOS
/**
 * @brief IRQ Handler
 */
bool PIOS_HMC5x83_IRQHandler(pios_hmc5x83_dev_t handler)
{
    if (!handler) { // handler is not set on first call
        return false;
    }
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

    dev->data_ready = true;

    return false;
}
#endif /* PIOS_HMC5X83_HAS_GPIOS */

#ifdef PIOS_INCLUDE_SPI
int32_t PIOS_HMC5x83_SPI_Read(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len);
int32_t PIOS_HMC5x83_SPI_Write(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t buffer);

const struct pios_hmc5x83_io_driver PIOS_HMC5x83_SPI_DRIVER = {
    .Read  = PIOS_HMC5x83_SPI_Read,
    .Write = PIOS_HMC5x83_SPI_Write,
};

static int32_t pios_hmc5x83_spi_claim_bus(pios_hmc5x83_dev_data_t *dev)
{
    if (PIOS_SPI_ClaimBus(dev->port_id) < 0) {
        return -1;
    }
    PIOS_SPI_SetClockSpeed(dev->port_id, SPI_BaudRatePrescaler_16);
    PIOS_SPI_RC_PinSet(dev->port_id, dev->slave_num, 0);
    return 0;
}

static void pios_hmc5x83_spi_release_bus(pios_hmc5x83_dev_data_t *dev)
{
    PIOS_SPI_RC_PinSet(dev->port_id, dev->slave_num, 1);
    PIOS_SPI_ReleaseBus(dev->port_id);
}
/**
 * @brief Reads one or more bytes into a buffer
 * \param[in] address HMC5x83 register address (depends on size)
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
int32_t PIOS_HMC5x83_SPI_Read(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

    if (pios_hmc5x83_spi_claim_bus(dev) < 0) {
        return -1;
    }

    memset(buffer, 0xA5, len);
    PIOS_SPI_TransferByte(dev->port_id, address | PIOS_HMC5x83_SPI_AUTOINCR_FLAG | PIOS_HMC5x83_SPI_READ_FLAG);

    // buffer[0] = address | PIOS_HMC5x83_SPI_AUTOINCR_FLAG | PIOS_HMC5x83_SPI_READ_FLAG;
    /* Copy the transfer data to the buffer */
    if (PIOS_SPI_TransferBlock(dev->port_id, NULL, buffer, len, NULL) < 0) {
        pios_hmc5x83_spi_release_bus(dev);
        return -3;
    }
    pios_hmc5x83_spi_release_bus(dev);
    return 0;
}

/**
 * @brief Writes one or more bytes to the HMC5x83
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim spi device
 */
int32_t PIOS_HMC5x83_SPI_Write(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t buffer)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);

    if (pios_hmc5x83_spi_claim_bus(dev) < 0) {
        return -1;
    }
    uint8_t data[] = {
        address | PIOS_HMC5x83_SPI_AUTOINCR_FLAG,
        buffer,
    };

    if (PIOS_SPI_TransferBlock(dev->port_id, data, NULL, sizeof(data), NULL) < 0) {
        pios_hmc5x83_spi_release_bus(dev);
        return -2;
    }

    pios_hmc5x83_spi_release_bus(dev);
    return 0;
}
#endif /* PIOS_INCLUDE_SPI */

#ifdef PIOS_INCLUDE_I2C
int32_t PIOS_HMC5x83_I2C_Read(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len);
int32_t PIOS_HMC5x83_I2C_Write(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t buffer);

const struct pios_hmc5x83_io_driver PIOS_HMC5x83_I2C_DRIVER = {
    .Read  = PIOS_HMC5x83_I2C_Read,
    .Write = PIOS_HMC5x83_I2C_Write,
};

/**
 * @brief Reads one or more bytes into a buffer
 * \param[in] address HMC5x83 register address (depends on size)
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
int32_t PIOS_HMC5x83_I2C_Read(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);
    uint8_t addr_buffer[] = {
        address,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = PIOS_HMC5x83_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(addr_buffer),
            .buf  = addr_buffer,
        }
        ,
        {
            .info = __func__,
            .addr = PIOS_HMC5x83_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    return PIOS_I2C_Transfer(dev->port_id, txn_list, NELEMENTS(txn_list));
}

/**
 * @brief Writes one or more bytes to the HMC5x83
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
int32_t PIOS_HMC5x83_I2C_Write(pios_hmc5x83_dev_t handler, uint8_t address, uint8_t buffer)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate(handler);
    uint8_t data[] = {
        address,
        buffer,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = PIOS_HMC5x83_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(data),
            .buf  = data,
        }
        ,
    };

    ;
    return PIOS_I2C_Transfer(dev->port_id, txn_list, NELEMENTS(txn_list));
}
#endif /* PIOS_INCLUDE_I2C */

/* PIOS sensor driver implementation */
bool PIOS_HMC5x83_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return true; // Do not do tests now, Sensors module takes this rather too seriously.
}

void PIOS_HMC5x83_driver_Reset(__attribute__((unused)) uintptr_t context) {}

void PIOS_HMC5x83_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

void PIOS_HMC5x83_driver_fetch(void *data, uint8_t size, uintptr_t context)
{
    PIOS_Assert(size > 0);
    pios_hmc5x83_dev_data_t *dev = dev_validate((pios_hmc5x83_dev_t)context);

    PIOS_SENSORS_3Axis_SensorsWithTemp *tmp = data;

    tmp->count = 1;
    tmp->sample[0].x = dev->magData[0];
    tmp->sample[0].y = dev->magData[1];
    tmp->sample[0].z = dev->magData[2];
    tmp->temperature = 0;
}


bool PIOS_HMC5x83_driver_poll(uintptr_t context)
{
    pios_hmc5x83_dev_data_t *dev = dev_validate((pios_hmc5x83_dev_t)context);

    if (dev->hw_error) {
        if (PIOS_DELAY_DiffuS(dev->lastConfigTime) < 1000000) {
            return false;
        }

#ifdef PIOS_INCLUDE_WDG
        // give HMC5x83 on I2C some extra time
        PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

        if (PIOS_HMC5x83_Config(dev) == 0) {
            dev->hw_error = false;
        }
    }

    if (dev->hw_error) {
        return false;
    }

    if (!PIOS_HMC5x83_NewDataAvailable((pios_hmc5x83_dev_t)context)) {
        return false;
    }

    if (PIOS_HMC5x83_ReadMag((pios_hmc5x83_dev_t)context, dev->magData) != 0) {
        dev->hw_error = true;
        return false;
    }

    return true;
}

#endif /* PIOS_INCLUDE_HMC5x83 */

/**
 * @}
 * @}
 */
