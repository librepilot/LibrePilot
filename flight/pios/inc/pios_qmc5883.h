/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_QMC5883 QMC5883 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 *
 * @file       pios_qmc5883.h
 * @author     The SantyPilot Team, copyright (C) 2023.
 * @brief      QMC5883 functions header.
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

#ifndef PIOS_QMC5883_H
#define PIOS_QMC5883_H
#include <stdint.h>
#include <pios_sensors.h>
/* QMC5883 Addresses */
#define PIOS_QMC5883_I2C_ADDR           0x0D
#define PIOS_QMC5883_I2C_READ_ADDR      0x1B
#define PIOS_QMC5883_I2C_WRITE_ADDR     0x1A

#define PIOS_QMC5883_SPI_READ_FLAG      0x80 // TBD: NO TEST
#define PIOS_QMC5883_SPI_AUTOINCR_FLAG  0x40 // TBD: NO TEST

#define PIOS_QMC5883_DATAOUT_XLSB_REG   0x00
#define PIOS_QMC5883_DATAOUT_XMSB_REG   0x01
#define PIOS_QMC5883_DATAOUT_YLSB_REG   0x02
#define PIOS_QMC5883_DATAOUT_YMSB_REG   0x03
#define PIOS_QMC5883_DATAOUT_ZLSB_REG   0x04
#define PIOS_QMC5883_DATAOUT_ZMSB_REG   0x05
#define PIOS_QMC5883_DATAOUT_STATUS_REG 0x06
#define PIOS_QMC5883_DATAOUT_TEMP_REG_L   0x07
#define PIOS_QMC5883_DATAOUT_TEMP_REG_H   0x08

#define PIOS_QMC5883_CONFIG_REG_A       0x09
#define PIOS_QMC5883_CONFIG_REG_B       0x0A
#define PIOS_QMC5883_CONFIG_PERD_REG    0x0B
#define PIOS_QMC5883_CHIP_ID_REG        0x0D // 0x0C Reserved

#define PIOS_QMC5883_DATAOUT_REG      0x00
#define PIOS_QMC5883_DATAOUT_X_REG    0x00
#define PIOS_QMC5883_DATAOUT_Y_REG    0x02
#define PIOS_QMC5883_DATAOUT_Z_REG    0x04

/* Modes */
#define PIOS_QMC5883_MODE_STANDBY       0x00
#define PIOS_QMC5883_MODE_CONTINUOUS     0x01
#define PIOS_QMC5883_MODE_RESERVE_1     0x02
#define PIOS_QMC5883_MODE_RESERVE_2     0x03

/* Output Data Rate */
#define PIOS_QMC5883_ODR_10             0x00 // 10Hz
#define PIOS_QMC5883_ODR_50             0x01 // 50Hz
#define PIOS_QMC5883_ODR_100            0x02 // 100Hz
#define PIOS_QMC5883_ODR_200            0x03 // 200Hz

/* Full Scale */
#define PIOS_QMC5883_RANGE_2G           0x00
#define PIOS_QMC5883_RANGE_8G           0x01
#define PIOS_QMC5883_RANGE_RESERVE_1    0x02
#define PIOS_QMC5883_RANGE_RESERVE_2    0x03

/* Over Sample Ratio */
#define PIOS_QMC5883_OSR_512            0x00
#define PIOS_QMC5883_OSR_256            0x01
#define PIOS_QMC5883_OSR_128            0x02
#define PIOS_QMC5883_OSR_64             0x03

/* CTRL Reg 2 */
/* Interrupt Pin Enable/Disable bit 0 */
#define PIOS_QMC5883_IRQ_PIN_EN         0x00
#define PIOS_QMC5883_IRQ_PIN_DIS        0x01

/* Pointer Roll Enable/Disable bit 6 */
#define PIOS_QMC5883_ROL_PNT_DIS        0x00
#define PIOS_QMC5883_ROL_PNT_EN         0x01

/* Set/Reset Period bit 7 */
#define PIOS_QMC5883_SOFT_SET           0x00
#define PIOS_QMC5883_SOFT_RESET         0x01

/* Status */
#define PIOS_QMC5883_STATUS_DRDY        0x01 // 1: New Data Ready
#define PIOS_QMC5883_STATUS_OVL         0x02 // 1: Overflow
#define PIOS_QMC5883_STATUS_DOR         0x04 // 1: Skip Data Read

typedef uintptr_t pios_qmc5883_dev_t;

struct pios_qmc5883_io_driver {
    int32_t (*Write)(pios_qmc5883_dev_t handler, uint8_t address, uint8_t buffer);
    int32_t (*Read)(pios_qmc5883_dev_t handler, uint8_t address, uint8_t *buffer, uint8_t len);
};

#ifdef PIOS_INCLUDE_SPI // TBD: NO TEST
extern const struct pios_qmc5883_io_driver PIOS_QMC5883_SPI_DRIVER;
#endif

#ifdef PIOS_INCLUDE_I2C
extern const struct pios_qmc5883_io_driver PIOS_QMC5883_I2C_DRIVER;
#endif
// xyz axis orientation
enum PIOS_QMC5883_ORIENTATION {
    PIOS_QMC5883_ORIENTATION_EAST_NORTH_UP,
    PIOS_QMC5883_ORIENTATION_SOUTH_EAST_UP,
    PIOS_QMC5883_ORIENTATION_WEST_SOUTH_UP,
    PIOS_QMC5883_ORIENTATION_NORTH_WEST_UP,
    PIOS_QMC5883_ORIENTATION_EAST_SOUTH_DOWN,
    PIOS_QMC5883_ORIENTATION_SOUTH_WEST_DOWN,
    PIOS_QMC5883_ORIENTATION_WEST_NORTH_DOWN,
    PIOS_QMC5883_ORIENTATION_NORTH_EAST_DOWN,
};


struct pios_qmc5883_cfg {
#ifdef PIOS_QMC5883_HAS_GPIOS
    const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */
#endif
    uint8_t CTL_ODR; // OUTPUT DATA RATE --> here below the relative define (See datasheet page 11 for more details) */
	uint8_t CTL_MOD;
	uint8_t CTL_RNG;
	uint8_t CTL_OSR;
	uint8_t soft_rst;
	uint8_t ptr_en;
	uint8_t irt_en;
    enum PIOS_QMC5883_ORIENTATION Orientation;
    const struct pios_qmc5883_io_driver *Driver;
};

/* Public Functions */
extern pios_qmc5883_dev_t PIOS_QMC5883_Init(const struct pios_qmc5883_cfg *cfg, uint32_t port_id, uint8_t device_num);
extern void PIOS_QMC5883_Register(pios_qmc5883_dev_t handler, PIOS_SENSORS_TYPE sensortype);

extern bool PIOS_QMC5883_NewDataAvailable(pios_qmc5883_dev_t handler);
extern int32_t PIOS_QMC5883_ReadMag(pios_qmc5883_dev_t handler, int16_t out[3]);
extern uint8_t PIOS_QMC5883_ReadID(pios_qmc5883_dev_t handler, uint8_t out[4]);
extern int32_t PIOS_QMC5883_Test(pios_qmc5883_dev_t handler);
extern bool PIOS_QMC5883_IRQHandler(pios_qmc5883_dev_t handler);

extern const PIOS_SENSORS_Driver PIOS_QMC5883_Driver;

#endif /* PIOS_QMC5883_H */

/**
 * @}
 * @}
 */
