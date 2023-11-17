/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_QMC5883 QMC5883 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 * @file       pios_qmc5883.c
 * @author     The SantyPilot Team, Copyright (C) 2023.
 * @brief      QMC5883 Magnetic Sensor Functions from AHRS
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
#include <pios_qmc5883.h>
#include <pios_mem.h>

#ifdef PIOS_INCLUDE_QMC5883

#define PIOS_QMC5883_MAGIC 0x248C6563 //MD5 QMC5883
/* Global Variables */

/* Local Types */

typedef struct {
    uint32_t magic;
    const struct pios_qmc5883_cfg *cfg;
    uint32_t port_id;
    uint8_t  slave_num;
    uint8_t  CTRLA;
    uint16_t magCountMax;
    uint16_t magCount;
    volatile bool data_ready;
    int16_t  magData[3];
    bool     hw_error;
    uint32_t lastConfigTime;
} pios_qmc5883_dev_data_t;

static int32_t PIOS_QMC5883_Config(pios_qmc5883_dev_data_t *dev);

// sensor driver interface
bool PIOS_QMC5883_driver_Test(uintptr_t context);
void PIOS_QMC5883_driver_Reset(uintptr_t context);
void PIOS_QMC5883_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
void PIOS_QMC5883_driver_fetch(void *, uint8_t size, uintptr_t context);
bool PIOS_QMC5883_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_QMC5883_Driver = {
    .test      = PIOS_QMC5883_driver_Test,
    .poll      = PIOS_QMC5883_driver_poll,
    .fetch     = PIOS_QMC5883_driver_fetch,
    .reset     = PIOS_QMC5883_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_QMC5883_driver_get_scale,
    .is_polled = true,
};

/**
 * Allocate the device setting structure
 * @return pios_qmc5883_dev_data_t pointer to newly created structure
 */
pios_qmc5883_dev_data_t *dev_alloc()
{
    pios_qmc5883_dev_data_t *dev = (pios_qmc5883_dev_data_t *)pios_malloc(sizeof(pios_qmc5883_dev_data_t));

    PIOS_DEBUG_Assert(dev);
    memset(dev, 0x00, sizeof(pios_qmc5883_dev_data_t));
    dev->magic = PIOS_QMC5883_MAGIC;
    return dev;
}

/**
 * Validate a pios_qmc5883_dev_t handler and return the related pios_qmc5883_dev_data_t pointer
 * @param dev device handler
 * @return the device data structure
 */
pios_qmc5883_dev_data_t *dev_validate(pios_qmc5883_dev_t dev)
{
    pios_qmc5883_dev_data_t *dev_data = (pios_qmc5883_dev_data_t *)dev;

    PIOS_DEBUG_Assert(dev_data->magic == PIOS_QMC5883_MAGIC);
    return dev_data;
}

/**
 * @brief Initialize the QMC5883 magnetometer sensor.
 * @return none
 */
pios_qmc5883_dev_t PIOS_QMC5883_Init(const struct pios_qmc5883_cfg *cfg, uint32_t port_id, uint8_t slave_num)
{
    pios_qmc5883_dev_data_t *dev = dev_alloc();

    dev->cfg       = cfg; // store config before enabling interrupt
    dev->port_id   = port_id;
    dev->slave_num = slave_num;

#ifdef PIOS_QMC5883_HAS_GPIOS
    if (cfg->exti_cfg) {
        PIOS_EXTI_Init(cfg->exti_cfg);
    } else
#endif /* PIOS_QMC5883_HAS_GPIOS */
    {
// if PIOS_SENSOR_RATE is defined, there is a sensor loop that is called at that frequency
// and "is data available" can simply return false a few times to save some CPU
#ifdef PIOS_SENSOR_RATE
        // for external mags that have no interrupt line, just poll them with a timer
        // use the configured Output Data Rate to calculate the number of interations (of the main sensor task loop)
        // to return false, before returning true
        uint16_t rate100;
        switch (cfg->CTL_ODR) {
			case PIOS_QMC5883_ODR_10:
				rate100 = 1000;
				break;
			case PIOS_QMC5883_ODR_50:
				rate100 = 5000;
				break;
			case PIOS_QMC5883_ODR_100:
				rate100 = 10000;
				break;
			case PIOS_QMC5883_ODR_200:
				rate100 = 20000;
				break;
			default:
				rate100 = 20000;
				break;
        }
        // if the application sensor rate is fast enough to warrant skipping some slow hardware sensor reads
        if ((PIOS_SENSOR_RATE * 100.0f / 3.0f/*3-AXIS*/) > rate100) {
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

    if (PIOS_QMC5883_Config(dev) != 0) {
        dev->hw_error = true;
    }

    dev->data_ready = false;
    return (pios_qmc5883_dev_t)dev;
}

void PIOS_QMC5883_Register(pios_qmc5883_dev_t handler, PIOS_SENSORS_TYPE sensortype)
{
    if (handler) {
        PIOS_SENSORS_Register(&PIOS_QMC5883_Driver, sensortype, handler);
    }
}

/**
 * @brief Initialize the QMC5883 magnetometer sensor
 * \return none
 * \param[in] pios_qmc5883_dev_data_t device config to be used.
 * \param[in] PIOS_QMC5883_ConfigTypeDef struct to be used to configure sensor.
 *
 * CONFIG_REGA: Control Register A
 * Read Write
 * Default value: 
 *            |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0   | 
 *            |  OSR[1:0] |  RNG[1:0] |  ORD[1:0] |  MODE[1:0] |
 *
 * CONFIG_REGB: Control Register B
 * Read Write
 * SS: Soft Set/Reset
 * PR: Pointer Roll Enable/Disable
 * IP: Interrupt Pin Enable/Disable
 * 
 *            |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0   | 
 *            | SS  |  PR |         RESERVED            |  IP  |
 *
 * DATAOUT_STATUS_REG: Status Register
 * Read
 * DRY: Data Ready
 * OVL: Over Flow
 * DOR: Drop Data
 * 
 *            |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  | 
 *            |         RESERVED            | DOR | OVL | DRY | 
 *
 */
static int32_t PIOS_QMC5883_Config(pios_qmc5883_dev_data_t *dev)
{
    dev->CTRLA = 0;
    uint8_t CTRLB = 0x00;
    dev->lastConfigTime = PIOS_DELAY_GetRaw();
    const struct pios_qmc5883_cfg *cfg = dev->cfg;

    dev->CTRLA |= (uint8_t)(PIOS_QMC5883_MODE_CONTINUOUS |
			           PIOS_QMC5883_ODR_200 << 2 | 
			           PIOS_QMC5883_RANGE_2G << 4 |
					   PIOS_QMC5883_OSR_512 << 6);
    CTRLB |= (uint8_t)(cfg->soft_rst << 7 |
			                cfg->ptr_en << 6 |
							cfg->irt_en); // user configurable

    // CRTL_REGA
    if (cfg->Driver->Write((pios_qmc5883_dev_t)dev, PIOS_QMC5883_CONFIG_REG_A, dev->CTRLA) != 0) {
        return -1;
    }

    // CRTL_REGB
    if (cfg->Driver->Write((pios_qmc5883_dev_t)dev, PIOS_QMC5883_CONFIG_REG_B, CTRLB) != 0) {
        return -1;
    }

    // PERD_REG 
    if (cfg->Driver->Write((pios_qmc5883_dev_t)dev, PIOS_QMC5883_CONFIG_PERD_REG, 0x01) != 0) {
        return -1;
    }
#ifdef PIOS_INCLUDE_WDG
    // give QMC5883 on I2C some extra time
    PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

    if (PIOS_QMC5883_Test((pios_qmc5883_dev_t)dev) != 0) {
        return -1;
    }

    return 0;
}

static void PIOS_QMC5883_Orient(enum PIOS_QMC5883_ORIENTATION orientation, int16_t in[3], int16_t out[3])
{
    switch (orientation) {
    case PIOS_QMC5883_ORIENTATION_EAST_NORTH_UP:
        out[0] = in[2];
        out[1] = in[0];
        out[2] = -in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_SOUTH_EAST_UP:
        out[0] = -in[0];
        out[1] = in[2];
        out[2] = -in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_WEST_SOUTH_UP:
        out[0] = -in[2];
        out[1] = -in[0];
        out[2] = -in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_NORTH_WEST_UP:
        out[0] = in[0];
        out[1] = -in[2];
        out[2] = -in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_EAST_SOUTH_DOWN:
        out[0] = in[2];
        out[1] = -in[0];
        out[2] = in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_SOUTH_WEST_DOWN:
        out[0] = -in[0];
        out[1] = -in[2];
        out[2] = in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_WEST_NORTH_DOWN:
        out[0] = -in[2];
        out[1] = in[0];
        out[2] = in[1];
        break;
    case PIOS_QMC5883_ORIENTATION_NORTH_EAST_DOWN:
        out[0] = in[0];
        out[1] = in[2];
        out[2] = in[1];
        break;
    }
}

/**
 * @brief Read current X, Y, Z, temperature values (in that order)
 * \param[in] dev device handler
 * \param[out] int16_t array of size 3 to store X, Z, and Y magnetometer readings
 * \return 0 for success or -1 for failure
 */
int32_t PIOS_QMC5883_ReadMag(pios_qmc5883_dev_t handler, int16_t out[3])
{
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);

    dev->data_ready = false;
    uint8_t buffer[9];
	// int16_t temp;
    int16_t mag[3];
    // int8_t range;

    if (dev->cfg->Driver->Read(handler, PIOS_QMC5883_DATAOUT_XLSB_REG, buffer, 9) != 0) {
        return -1;
    }

	/*
    switch (dev->CTRLA & 0x30) {
    case 0x00:
        range = 2; // range: -2G ~ 2G
        break;
    case 0x10:
        range = 8;
        break;
    default:
		range = 2;
    }
	*/
    for (int i = 0; i < 3; i++) {
        int16_t v = ((int16_t)((uint16_t)buffer[2 * i + 1] << 8)
                     + buffer[2 * i]);// ->Gauss float / 65536 * 2 * range;
        mag[i] = v;
    }
	// temp = (int16_t)((uint16_t)(buffer[1] << 8) + buffer[0]); // temperature if needed

    PIOS_QMC5883_Orient(dev->cfg->Orientation, mag, out);

    // "This should not be necessary but for some reason it is coming out of continuous conversion mode"
    //
    // By default the chip is in single read mode meaning after reading from it once, it will go idle to save power.
    // Once idle, we have write to it to turn it on before we can read from it again.
    // To conserve current between measurements, the device is placed in a state similar to idle mode, but the
    // Mode Register is not changed to Idle Mode.  That is, MD[n] bits are unchanged.
    /*
	dev->cfg->Driver->Write(handler, PIOS_QMC5883_CONFIG_REG_A,  
			  (uint8_t)(PIOS_QMC5883_MODE_CONTINUOUS |
			           PIOS_QMC5883_ODR_200 << 2 | 
			           PIOS_QMC5883_RANGE_2G << 4 |
					   PIOS_QMC5883_OSR_512 << 6));
					   */

    return 0;
}


/**
 * @brief Read the identification bytes from the QMC5883 sensor
 * \param[out] uint8_t array of size 4 to store QMC5883 ID.
 * \return 0 if successful, -1 if not
 */
uint8_t PIOS_QMC5883_ReadID(pios_qmc5883_dev_t handler, uint8_t out[1])
{
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);
    uint8_t retval = dev->cfg->Driver->Read(handler, PIOS_QMC5883_CHIP_ID_REG, out, 1);

    return retval;
}

/**
 * @brief Tells whether new magnetometer readings are available
 * \return true if new data is available
 * \return false if new data is not available
 */
bool PIOS_QMC5883_NewDataAvailable(__attribute__((unused)) pios_qmc5883_dev_t handler)
{
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);

#ifdef PIOS_QMC5883_HAS_GPIOS
    if (dev->cfg->exti_cfg) { // if this device has an interrupt line attached, then wait for interrupt to say data is ready
		if (dev->data_ready) { // set in irq_handler
		    return true;
		}
		uint8_t tmp[1];
		dev->cfg->Driver->Read(handler, PIOS_QMC5883_DATAOUT_STATUS_REG, tmp, 1); 
		if (tmp != NULL && tmp[0] & 0x01) { // avoid irq stuck by ready data
			return true;
		}
        return false;
    } else
#endif /* PIOS_QMC5883_HAS_GPIOS */
    { // else poll to see if data is ready or just say "true" and set polling interval elsewhere
		// pre-calculated cache to speedup, should query status flag
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
int32_t PIOS_QMC5883_Test(pios_qmc5883_dev_t handler)
{
#ifdef PIOS_INCLUDE_WDG
    // give QMC5883 on I2C some extra time to allow for reset, etc. if needed
    // this is not in a loop, so it is safe
    PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

    /* Verify that ID matches (QMC5883 ID is 0xFF) */
    char id[1];

    PIOS_QMC5883_ReadID(handler, (uint8_t *)id);
	if (id[0] != 0xFF) {
	    return -1;
	}

	// QMC5883L has no self-test mode
    return 0; // success
}

#ifdef PIOS_QMC5883_HAS_GPIOS
/**
 * @brief IRQ Handler
 */
bool PIOS_QMC5883_IRQHandler(pios_qmc5883_dev_t handler)
{
    if (!handler) { // handler is not set on first call
        return false;
    }
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);

    dev->data_ready = true;

    return false;
}
#endif /* PIOS_QMC5883_HAS_GPIOS */

#ifdef PIOS_INCLUDE_SPI
// NOT IMPLEMENTED
#endif /* PIOS_INCLUDE_SPI */

#ifdef PIOS_INCLUDE_I2C
int32_t PIOS_QMC5883_I2C_Read(pios_qmc5883_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len);
int32_t PIOS_QMC5883_I2C_Write(pios_qmc5883_dev_t handler, uint8_t address, uint8_t buffer);

const struct pios_qmc5883_io_driver PIOS_QMC5883_I2C_DRIVER = {
    .Read  = PIOS_QMC5883_I2C_Read,
    .Write = PIOS_QMC5883_I2C_Write,
};

/**
 * @brief Reads one or more bytes into a buffer
 * \param[in] address QMC5883 register address (depends on size)
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
int32_t PIOS_QMC5883_I2C_Read(pios_qmc5883_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len)
{
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);
    uint8_t addr_buffer[] = {
        address,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = PIOS_QMC5883_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(addr_buffer),
            .buf  = addr_buffer,
        }
        ,
        {
            .info = __func__,
            .addr = PIOS_QMC5883_I2C_ADDR,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    return PIOS_I2C_Transfer(dev->port_id, txn_list, NELEMENTS(txn_list));
}

/**
 * @brief Writes one or more bytes to the QMC5883
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
int32_t PIOS_QMC5883_I2C_Write(pios_qmc5883_dev_t handler, uint8_t address, uint8_t buffer)
{
    pios_qmc5883_dev_data_t *dev = dev_validate(handler);
    uint8_t data[] = {
        address,
        buffer,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = PIOS_QMC5883_I2C_ADDR,
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
bool PIOS_QMC5883_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return true; // Do not do tests now, Sensors module takes this rather too seriously.
}

void PIOS_QMC5883_driver_Reset(__attribute__((unused)) uintptr_t context) {}

void PIOS_QMC5883_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

void PIOS_QMC5883_driver_fetch(void *data, uint8_t size, uintptr_t context)
{
    PIOS_Assert(size > 0);
    pios_qmc5883_dev_data_t *dev = dev_validate((pios_qmc5883_dev_t)context);

    PIOS_SENSORS_3Axis_SensorsWithTemp *tmp = data;

    tmp->count = 1;
    tmp->sample[0].x = dev->magData[0];
    tmp->sample[0].y = dev->magData[1];
    tmp->sample[0].z = dev->magData[2];
    tmp->temperature = 0;
}


bool PIOS_QMC5883_driver_poll(uintptr_t context)
{
    pios_qmc5883_dev_data_t *dev = dev_validate((pios_qmc5883_dev_t)context);

    if (dev->hw_error) {
        if (PIOS_DELAY_DiffuS(dev->lastConfigTime) < 1000000) {
            return false;
        }

#ifdef PIOS_INCLUDE_WDG
        // give QMC5883 on I2C some extra time
        PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_WDG */

        if (PIOS_QMC5883_Config(dev) == 0) {
            dev->hw_error = false;
        }
    }

    if (dev->hw_error) {
        return false;
    }

	/* Test Read */
	//int16_t buffer[3] = {0,0,0};
    //if (PIOS_QMC5883_ReadMag((pios_qmc5883_dev_t)context, buffer) != 0) {
    //    return false;
    //}

    if (!PIOS_QMC5883_NewDataAvailable((pios_qmc5883_dev_t)context)) {
        return false;
    }

    if (PIOS_QMC5883_ReadMag((pios_qmc5883_dev_t)context, dev->magData) != 0) {
        dev->hw_error = true;
        return false;
    }

    return true;
}

#endif /* PIOS_INCLUDE_QMC5883 */

/**
 * @}
 * @}
 */
