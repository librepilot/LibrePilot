/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU9250 MPU9250 Functions
 * @brief Deals with the hardware interface to the 9 DOF sensor.
 * @{
 *
 * @file       pios_mp9250.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @brief      MPU9250 9-axis gyro, accel and mag chip
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
#include <pios_mpu9250.h>
#ifdef PIOS_INCLUDE_MPU9250
#include <stdint.h>
#include <pios_constants.h>
#include <pios_sensors.h>
/* Global Variables */

enum pios_mpu9250_dev_magic {
    PIOS_MPU9250_DEV_MAGIC = 0x9da9b3ed,
};

struct mpu9250_dev {
    uint32_t spi_id;
    uint32_t slave_num;
    QueueHandle_t queue;
    const struct pios_mpu9250_cfg *cfg;
    enum pios_mpu9250_range gyro_range;
    enum pios_mpu9250_accel_range accel_range;
    enum pios_mpu9250_filter filter;
    enum pios_mpu9250_dev_magic   magic;
    float mag_sens_adj[PIOS_MPU9250_MAG_ASA_NB_BYTE];
};

#ifdef PIOS_MPU9250_ACCEL
#define PIOS_MPU9250_ACCEL_SAMPLES_BYTES (6)
#else
#define PIOS_MPU9250_ACCEL_SAMPLES_BYTES (0)
#endif

#ifdef PIOS_MPU9250_MAG
#define PIOS_MPU9250_MAG_SAMPLES_BYTES   (8)
#else
#define PIOS_MPU9250_MAG_SAMPLES_BYTES   (0)
#endif

#define PIOS_MPU9250_GYRO_SAMPLES_BYTES  (6)
#define PIOS_MPU9250_TEMP_SAMPLES_BYTES  (2)

#define PIOS_MPU9250_SAMPLES_BYTES \
    (PIOS_MPU9250_ACCEL_SAMPLES_BYTES + \
     PIOS_MPU9250_GYRO_SAMPLES_BYTES + \
     PIOS_MPU9250_TEMP_SAMPLES_BYTES + \
     PIOS_MPU9250_MAG_SAMPLES_BYTES)

#ifdef PIOS_MPU9250_ACCEL
#define PIOS_MPU9250_SENSOR_FIRST_REG    PIOS_MPU9250_ACCEL_X_OUT_MSB
#else
#define PIOS_MPU9250_SENSOR_FIRST_REG    PIOS_MPU9250_TEMP_OUT_MSB
#endif

#if defined(PIOS_MPU9250_MAG) && !defined(PIOS_MPU9250_ACCEL)
#error ERROR: PIOS_MPU9250_ACCEL not defined! THIS CONFIGURATION IS NOT SUPPORTED
#endif

typedef union {
    uint8_t buffer[2 + PIOS_MPU9250_SAMPLES_BYTES];
    struct {
        uint8_t dummy;
#ifdef PIOS_MPU9250_ACCEL
        uint8_t Accel_X_h;
        uint8_t Accel_X_l;
        uint8_t Accel_Y_h;
        uint8_t Accel_Y_l;
        uint8_t Accel_Z_h;
        uint8_t Accel_Z_l;
#endif
        uint8_t Temperature_h;
        uint8_t Temperature_l;
        uint8_t Gyro_X_h;
        uint8_t Gyro_X_l;
        uint8_t Gyro_Y_h;
        uint8_t Gyro_Y_l;
        uint8_t Gyro_Z_h;
        uint8_t Gyro_Z_l;
#ifdef PIOS_MPU9250_MAG
        uint8_t st1;
        uint8_t Mag_X_l;
        uint8_t Mag_X_h;
        uint8_t Mag_Y_l;
        uint8_t Mag_Y_h;
        uint8_t Mag_Z_l;
        uint8_t Mag_Z_h;
        uint8_t st2;
#endif
    } data;
} __attribute__((__packed__)) mpu9250_data_t;

#define GET_SENSOR_DATA(mpudataptr, sensor) ((int16_t)((mpudataptr.data.sensor##_h << 8 | mpudataptr.data.sensor##_l)))

static PIOS_SENSORS_3Axis_SensorsWithTemp *queue_data = 0;
static PIOS_SENSORS_3Axis_SensorsWithTemp *mag_data   = 0;
static volatile bool mag_ready = false;
#define SENSOR_COUNT         2
#define SENSOR_DATA_SIZE     (sizeof(PIOS_SENSORS_3Axis_SensorsWithTemp) + sizeof(Vector3i16) * SENSOR_COUNT)
#define MAG_SENSOR_DATA_SIZE (sizeof(PIOS_SENSORS_3Axis_SensorsWithTemp) + sizeof(Vector3i16))
// ! Global structure for this device device
static struct mpu9250_dev *dev;
volatile bool mpu9250_configured = false;
static mpu9250_data_t mpu9250_data;
static int32_t mpu9250_id;

// ! Private functions
static struct mpu9250_dev *PIOS_MPU9250_alloc(const struct pios_mpu9250_cfg *cfg);
static int32_t PIOS_MPU9250_Validate(struct mpu9250_dev *dev);
static void PIOS_MPU9250_Config(struct pios_mpu9250_cfg const *cfg);
static int32_t PIOS_MPU9250_SetReg(uint8_t address, uint8_t buffer);
static int32_t PIOS_MPU9250_GetReg(uint8_t address);
static void PIOS_MPU9250_SetSpeed(const bool fast);
static bool PIOS_MPU9250_HandleData(uint32_t gyro_read_timestamp);
static bool PIOS_MPU9250_ReadSensor(bool *woken);
static int32_t PIOS_MPU9250_Test(void);
#if defined(PIOS_MPU9250_MAG)
static int32_t PIOS_MPU9250_Mag_Test(void);
static int32_t PIOS_MPU9250_Mag_Init(void);
#endif

/* Driver Framework interfaces */
// Gyro/accel interface
bool PIOS_MPU9250_Main_driver_Test(uintptr_t context);
void PIOS_MPU9250_Main_driver_Reset(uintptr_t context);
void PIOS_MPU9250_Main_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
QueueHandle_t PIOS_MPU9250_Main_driver_get_queue(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_MPU9250_Main_Driver = {
    .test      = PIOS_MPU9250_Main_driver_Test,
    .poll      = NULL,
    .fetch     = NULL,
    .reset     = PIOS_MPU9250_Main_driver_Reset,
    .get_queue = PIOS_MPU9250_Main_driver_get_queue,
    .get_scale = PIOS_MPU9250_Main_driver_get_scale,
    .is_polled = false,
};

// mag sensor interface
bool PIOS_MPU9250_Mag_driver_Test(uintptr_t context);
void PIOS_MPU9250_Mag_driver_Reset(uintptr_t context);
void PIOS_MPU9250_Mag_driver_get_scale(float *scales, uint8_t size, uintptr_t context);
void PIOS_MPU9250_Mag_driver_fetch(void *, uint8_t size, uintptr_t context);
bool PIOS_MPU9250_Mag_driver_poll(uintptr_t context);

const PIOS_SENSORS_Driver PIOS_MPU9250_Mag_Driver = {
    .test      = PIOS_MPU9250_Mag_driver_Test,
    .poll      = PIOS_MPU9250_Mag_driver_poll,
    .fetch     = PIOS_MPU9250_Mag_driver_fetch,
    .reset     = PIOS_MPU9250_Mag_driver_Reset,
    .get_queue = NULL,
    .get_scale = PIOS_MPU9250_Mag_driver_get_scale,
    .is_polled = true,
};

void PIOS_MPU9250_MainRegister()
{
    PIOS_SENSORS_Register(&PIOS_MPU9250_Main_Driver, PIOS_SENSORS_TYPE_3AXIS_GYRO_ACCEL, 0);
}

void PIOS_MPU9250_MagRegister()
{
    if (mpu9250_id == PIOS_MPU9250_GYRO_ACC_ID) {
        PIOS_SENSORS_Register(&PIOS_MPU9250_Mag_Driver, PIOS_SENSORS_TYPE_3AXIS_MAG, 0);
    }
}
/**
 * @brief Allocate a new device
 */
static struct mpu9250_dev *PIOS_MPU9250_alloc(const struct pios_mpu9250_cfg *cfg)
{
    struct mpu9250_dev *mpu9250_dev;

    mpu9250_dev = (struct mpu9250_dev *)pios_malloc(sizeof(*mpu9250_dev));
    PIOS_Assert(mpu9250_dev);

    mpu9250_dev->magic = PIOS_MPU9250_DEV_MAGIC;

    mpu9250_dev->queue = xQueueCreate(cfg->max_downsample + 1, SENSOR_DATA_SIZE);
    PIOS_Assert(mpu9250_dev->queue);

    queue_data = (PIOS_SENSORS_3Axis_SensorsWithTemp *)pios_malloc(SENSOR_DATA_SIZE);
    PIOS_Assert(queue_data);

    queue_data->count = SENSOR_COUNT;

    mag_data = (PIOS_SENSORS_3Axis_SensorsWithTemp *)pios_malloc(MAG_SENSOR_DATA_SIZE);
    mag_data->count   = 1;
    PIOS_Assert(mag_data);
    return mpu9250_dev;
}

/**
 * @brief Validate the handle to the spi device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_MPU9250_Validate(struct mpu9250_dev *vdev)
{
    if (vdev == NULL) {
        return -1;
    }
    if (vdev->magic != PIOS_MPU9250_DEV_MAGIC) {
        return -2;
    }
    if (vdev->spi_id == 0) {
        return -3;
    }
    return 0;
}

/**
 * @brief Initialize the MPU9250 3-axis gyro sensor.
 * @return 0 for success, -1 for failure
 */
int32_t PIOS_MPU9250_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_mpu9250_cfg *cfg)
{
    dev = PIOS_MPU9250_alloc(cfg);
    if (dev == NULL) {
        return -1;
    }

    dev->spi_id    = spi_id;
    dev->slave_num = slave_num;
    dev->cfg = cfg;

    /* Configure the MPU9250 Sensor */
    PIOS_MPU9250_Config(cfg);

    /* Set up EXTI line */
    PIOS_EXTI_Init(cfg->exti_cfg);
    return 0;
}

/**
 * @brief Initialize the MPU9250 3-axis gyro sensor
 * \return none
 * \param[in] PIOS_MPU9250_ConfigTypeDef struct to be used to configure sensor.
 *
 */
static void PIOS_MPU9250_Config(struct pios_mpu9250_cfg const *cfg)
{
    uint8_t power;

    while (PIOS_MPU9250_Test() != 0) {
        ;
    }

    // Reset chip
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_PWR_MGMT_REG, PIOS_MPU9250_PWRMGMT_IMU_RST) != 0) {
        ;
    }

    PIOS_DELAY_WaitmS(100);

    // Wake up the chip
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_PWR_MGMT_REG, 0) != 0) {
        ;
    }
    // Reset sensors and fifo
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_USER_CTRL_REG,
                               PIOS_MPU9250_USERCTL_DIS_I2C |
                               PIOS_MPU9250_USERCTL_SIG_COND) != 0) {
        ;
    }
    PIOS_DELAY_WaitmS(100);

    // Power management configuration
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) {
        ;
    }

    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_USER_CTRL_REG, cfg->User_ctl) != 0) {
        ;
    }

    // FIFO storage by default, do not include accelerometer and external sense data.
    power = PIOS_MPU9250_PWRMGMT2_DISABLE_ACCEL;

#if defined(PIOS_MPU9250_ACCEL)

    power &= ~PIOS_MPU9250_PWRMGMT2_DISABLE_ACCEL;
#endif

    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_FIFO_EN_REG, cfg->Fifo_store) != 0) {
        ;
    }
    PIOS_MPU9250_SetReg(PIOS_MPU9250_PWR_MGMT2_REG, power);

#if defined(PIOS_MPU9250_ACCEL)
    PIOS_MPU9250_ConfigureRanges(cfg->gyro_range, cfg->accel_range, cfg->filter);
#endif

    // Interrupt configuration
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_INT_CFG_REG, cfg->interrupt_cfg) != 0) {
        ;
    }

#ifdef PIOS_MPU9250_MAG
    if (mpu9250_id == PIOS_MPU9250_GYRO_ACC_ID) {
        PIOS_MPU9250_Mag_Init();
    }
#endif

    // Interrupt enable
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_INT_EN_REG, cfg->interrupt_en) != 0) {
        ;
    }
    if ((PIOS_MPU9250_GetReg(PIOS_MPU9250_INT_EN_REG)) != cfg->interrupt_en) {
        return;
    }

    PIOS_MPU9250_GetReg(PIOS_MPU9250_INT_STATUS_REG);

    mpu9250_configured = true;
}
/**
 * @brief Configures Gyro, accel and Filter ranges/setings
 * @return 0 if successful, -1 if device has not been initialized
 */
int32_t PIOS_MPU9250_ConfigureRanges(
    enum pios_mpu9250_range gyroRange,
    enum pios_mpu9250_accel_range accelRange,
    enum pios_mpu9250_filter filterSetting)
{
    if (dev == NULL) {
        return -1;
    }

    // update filter settings
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_DLPF_CFG_REG, filterSetting) != 0) {
        ;
    }

    // Sample rate divider, chosen upon digital filtering settings
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_SMPLRT_DIV_REG,
                               filterSetting == PIOS_MPU9250_LOWPASS_256_HZ ?
                               dev->cfg->Smpl_rate_div_no_dlp : dev->cfg->Smpl_rate_div_dlp) != 0) {
        ;
    }

    dev->filter = filterSetting;

    // Gyro range
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_GYRO_CFG_REG, gyroRange) != 0) {
        ;
    }

    dev->gyro_range = gyroRange;
#if defined(PIOS_MPU9250_ACCEL)
    // Set the accel range
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_ACCEL_CFG_REG, accelRange) != 0) {
        ;
    }

    dev->accel_range = accelRange;
#endif
    return 0;
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_MPU9250_ClaimBus(bool fast_spi)
{
    if (PIOS_MPU9250_Validate(dev) != 0) {
        return -1;
    }
    if (PIOS_SPI_ClaimBus(dev->spi_id) != 0) {
        return -2;
    }
    PIOS_MPU9250_SetSpeed(fast_spi);
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);
    return 0;
}


static void PIOS_MPU9250_SetSpeed(const bool fast)
{
    if (fast) {
        PIOS_SPI_SetClockSpeed(dev->spi_id, dev->cfg->fast_prescaler);
    } else {
        PIOS_SPI_SetClockSpeed(dev->spi_id, dev->cfg->std_prescaler);
    }
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 */
static int32_t PIOS_MPU9250_ClaimBusISR(bool *woken, bool fast_spi)
{
    if (PIOS_MPU9250_Validate(dev) != 0) {
        return -1;
    }
    if (PIOS_SPI_ClaimBusISR(dev->spi_id, woken) != 0) {
        return -2;
    }
    PIOS_MPU9250_SetSpeed(fast_spi);
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);
    return 0;
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_MPU9250_ReleaseBus()
{
    if (PIOS_MPU9250_Validate(dev) != 0) {
        return -1;
    }
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);
    return PIOS_SPI_ReleaseBus(dev->spi_id);
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 */
static int32_t PIOS_MPU9250_ReleaseBusISR(bool *woken)
{
    if (PIOS_MPU9250_Validate(dev) != 0) {
        return -1;
    }
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);
    return PIOS_SPI_ReleaseBusISR(dev->spi_id, woken);
}

/**
 * @brief Read a register from MPU9250
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU9250_GetReg(uint8_t reg)
{
    uint8_t data;

    if (PIOS_MPU9250_ClaimBus(false) != 0) {
        return -1;
    }

    PIOS_SPI_TransferByte(dev->spi_id, (0x80 | reg)); // request byte
    data = PIOS_SPI_TransferByte(dev->spi_id, 0); // receive response

    PIOS_MPU9250_ReleaseBus();
    return data;
}

/**
 * @brief Writes one byte to the MPU9250
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * \return 0 if operation was successful
 * \return -1 if unable to claim SPI bus
 * \return -2 if unable to send the command
 * \return -3 if unable to receive the response
 */
static int32_t PIOS_MPU9250_SetReg(uint8_t reg, uint8_t data)
{
    int ret = 0;

    if (PIOS_MPU9250_ClaimBus(false) != 0) {
        return -1;
    }

    PIOS_SPI_TransferByte(dev->spi_id, 0x7f & reg);
    // if (PIOS_SPI_TransferByte(dev->spi_id, 0x7f & reg) != 0) {
    // PIOS_MPU9250_ReleaseBus();
    // return -2;
    // }

    PIOS_SPI_TransferByte(dev->spi_id, data);
    // if (PIOS_SPI_TransferByte(dev->spi_id, data) != 0) {
    // PIOS_MPU9250_ReleaseBus();
    // return -3;
    // }

    PIOS_MPU9250_ReleaseBus();

    return ret;
}


/*
 * @brief Read the identification bytes from the MPU9250 sensor
 * \return ID read from MPU9250 or -1 if failure
 */
int32_t PIOS_MPU9250_ReadID()
{
    int32_t id = PIOS_MPU9250_GetReg(PIOS_MPU9250_WHOAMI);

    if (id < 0) {
        return -1;
    }
    return id;
}

static float PIOS_MPU9250_GetScale()
{
    switch (dev->gyro_range) {
    case PIOS_MPU9250_SCALE_250_DEG:
        return 1.0f / 131.0f;

    case PIOS_MPU9250_SCALE_500_DEG:
        return 1.0f / 65.5f;

    case PIOS_MPU9250_SCALE_1000_DEG:
        return 1.0f / 32.8f;

    case PIOS_MPU9250_SCALE_2000_DEG:
        return 1.0f / 16.4f;
    }
    return 0;
}

static float PIOS_MPU9250_GetAccelScale()
{
    switch (dev->accel_range) {
    case PIOS_MPU9250_ACCEL_2G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 16384.0f;

    case PIOS_MPU9250_ACCEL_4G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 8192.0f;

    case PIOS_MPU9250_ACCEL_8G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 4096.0f;

    case PIOS_MPU9250_ACCEL_16G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 2048.0f;
    }
    return 0;
}

/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test failed
 */
static int32_t PIOS_MPU9250_Test(void)
{
    /* Verify that ID matches */
    mpu9250_id = PIOS_MPU9250_ReadID();

    if (mpu9250_id < 0) {
        return -1;
    }

    if ((mpu9250_id != PIOS_MPU9250_GYRO_ACC_ID) && (mpu9250_id != PIOS_MPU6500_GYRO_ACC_ID)) {
        return -2;
    }

    return 0;
}

#if defined(PIOS_MPU9250_MAG)
/**
 * @brief Read a mag register from MPU9250
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU9250_Mag_GetReg(uint8_t reg)
{
    int32_t data;

    // Set the I2C slave address and read command.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_ADDR, PIOS_MPU9250_MAG_I2C_ADDR |
                               PIOS_MPU9250_MAG_I2C_READ_FLAG) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Set the address of the register to read.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_REG, reg) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Trigger the byte transfer.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_CTRL, PIOS_MPU9250_I2C_SLV_ENABLE) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    PIOS_DELAY_WaitmS(1);

    // Read result.
    data = PIOS_MPU9250_GetReg(PIOS_MPU9250_I2C_SLV4_DI);
    PIOS_DELAY_WaitmS(1);
    return data;
}

/**
 * @brief Writes one byte to the MPU9250
 * \param[in] reg Register address
 * \param[in] data Byte to write
 */
static int32_t PIOS_MPU9250_Mag_SetReg(uint8_t reg, uint8_t data)
{
    // Set the I2C slave address.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_ADDR, PIOS_MPU9250_MAG_I2C_ADDR) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Set the address of the register to write.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_REG, reg) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Set the byte to write.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_DO, data) != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Trigger the byte transfer.
    while (PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_CTRL, PIOS_MPU9250_I2C_SLV_ENABLE) != PIOS_MPU9250_MAG_OK) {
        ;
    }
    PIOS_DELAY_WaitmS(1);
    return PIOS_MPU9250_MAG_OK;
}

/**
 * @rief Get ASAx registers from fuse ROM
 *  Hadj = H*((ASA-128)*0.5/128+1)
 * \return 0 if test succeeded
 * \return non-zero value if test failed
 */
static int32_t PIOS_MPU9250_Mag_Sensitivity(void)
{
    /* Put mag in power down state before changing mode */
    PIOS_MPU9250_Mag_SetReg(PIOS_MPU9250_CNTL1, PIOS_MPU9250_MAG_POWER_DOWN_MODE);
    PIOS_DELAY_WaitmS(1);

    /* Enable fuse ROM for access */
    PIOS_MPU9250_Mag_SetReg(PIOS_MPU9250_CNTL1, PIOS_MPU9250_MAG_FUSE_ROM_MODE);
    PIOS_DELAY_WaitmS(1);

    /* Set addres and read flag */
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_ADDR, PIOS_MPU9250_MAG_I2C_ADDR | PIOS_MPU9250_MAG_I2C_READ_FLAG);

    /* Set the address of the register to read. */
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_REG, PIOS_MPU9250_ASAX);

    /* Trigger the byte transfer. */
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_CTRL, PIOS_MPU9250_I2C_SLV_ENABLE | 0x3);

    PIOS_DELAY_WaitmS(1);

    if (PIOS_MPU9250_ClaimBus(false) != 0) {
        return -1;
    }

    /* Read the mag data from SPI block */

    uint8_t mpu9250_send_buf[4] = { PIOS_MPU9250_EXT_SENS_DATA_00 | 0x80 };

    if (PIOS_SPI_TransferBlock(dev->spi_id, mpu9250_send_buf, mpu9250_send_buf, sizeof(mpu9250_send_buf), 0) == 0) {
        for (int i = 0; i < 3; ++i) {
            dev->mag_sens_adj[i] = 1.0f + ((float)((uint8_t)mpu9250_send_buf[i + 1] - 128)) / 256.0f;
        }
    } else {
        PIOS_MPU9250_ReleaseBus();
        return -1;
    }

    PIOS_MPU9250_ReleaseBus();


    /* Put mag in power down state before changing mode */
    PIOS_MPU9250_Mag_SetReg(PIOS_MPU9250_CNTL1, PIOS_MPU9250_MAG_POWER_DOWN_MODE);

    return PIOS_MPU9250_MAG_OK;
}

/**
 * @brief Read a mag register from MPU9250
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU9250_Mag_Init(void)
{
    // I2C multi-master init.
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_MST_CTRL, PIOS_MPU9250_I2C_MST_P_NSR | PIOS_MPU9250_I2C_MST_CLOCK_400);
    PIOS_DELAY_WaitmS(1);

    // Reset Mag.
    PIOS_MPU9250_Mag_SetReg(PIOS_MPU9250_CNTL2, PIOS_MPU9250_MAG_RESET);
    PIOS_DELAY_WaitmS(1);


    // read fuse ROM to get the sensitivity adjustment values.
    if (PIOS_MPU9250_Mag_Sensitivity() != PIOS_MPU9250_MAG_OK) {
        ;
    }

    // Confirm Mag ID.
    while (false && (PIOS_MPU9250_Mag_Test() != PIOS_MPU9250_MAG_OK)) {
        ;
    }

    // Make sure no other registers will be triggered before entering continuous mode.
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV4_CTRL, 0x0);
    PIOS_DELAY_WaitmS(1);
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_DO, 0x0);
    PIOS_DELAY_WaitmS(1);

    // Making sure register are accessible.
    PIOS_MPU9250_Mag_SetReg(PIOS_MPU9250_CNTL1, PIOS_MPU9250_MAG_OUTPUT_16BITS | PIOS_MPU9250_MAG_CONTINUOUS_MODE2);
    PIOS_DELAY_WaitmS(1);

    // Get ST1, the 6 mag data and ST2.
    // This is to save 2 SPI access.
    // Set the I2C slave address and read command.
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_ADDR, PIOS_MPU9250_MAG_I2C_ADDR | PIOS_MPU9250_MAG_I2C_READ_FLAG);

    // Set the address of the register to read.
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_REG, PIOS_MPU9250_ST1);

    // Trigger the byte transfer.
    PIOS_MPU9250_SetReg(PIOS_MPU9250_I2C_SLV0_CTRL, PIOS_MPU9250_I2C_SLV_ENABLE | 0x8);
    PIOS_DELAY_WaitmS(1);

    return PIOS_MPU9250_MAG_OK;
}

/*
 * @brief Read the mag identification bytes from the MPU9250 sensor
 */
int32_t PIOS_MPU9250_Mag_ReadID()
{
    int32_t mpu9250_mag_id = PIOS_MPU9250_Mag_GetReg(PIOS_MPU9250_WIA);

    if (mpu9250_mag_id < PIOS_MPU9250_MAG_OK) {
        return PIOS_MPU9250_ERR_MAG_READ_ID;
    }
    return mpu9250_mag_id;
}

/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test failed
 */
static int32_t PIOS_MPU9250_Mag_Test(void)
{
    /* Verify that ID matches */
    int32_t mpu9250_mag_id = PIOS_MPU9250_Mag_ReadID();

    if (mpu9250_mag_id < PIOS_MPU9250_MAG_OK) {
        return PIOS_MPU9250_ERR_MAG_READ_ID;
    }

    if (mpu9250_mag_id != PIOS_MPU9250_MAG_ID) {
        return PIOS_MPU9250_ERR_MAG_BAD_ID;
    }

    /* TODO: run self-test */

    return PIOS_MPU9250_MAG_OK;
}


/**
 * @brief Read the mag data.
 * \return true if data has been read from mpu
 * \return false on error
 */
static bool PIOS_MPU9250_ReadMag(bool *woken)
{
    if (PIOS_MPU9250_ClaimBusISR(woken, true) != 0) {
        return false;
    }
    // Trigger the byte transfer.
    PIOS_SPI_TransferByte(dev->spi_id, PIOS_MPU9250_I2C_SLV0_CTRL);
    PIOS_SPI_TransferByte(dev->spi_id, PIOS_MPU9250_I2C_SLV_ENABLE | 0x8);

    PIOS_MPU9250_ReleaseBusISR(woken);

    return true;
}
#endif /* if defined(PIOS_MPU9250_MAG) */

/**
 * @brief EXTI IRQ Handler.  Read all the data from onboard buffer
 * @return a boolean to the EXTI IRQ Handler wrapper indicating if a
 *         higher priority task is now eligible to run
 */
bool PIOS_MPU9250_IRQHandler(void)
{
    uint32_t gyro_read_timestamp = PIOS_DELAY_GetRaw();
    bool woken = false;

    if (!mpu9250_configured) {
        return false;
    }

#if defined(PIOS_MPU9250_MAG)
    PIOS_MPU9250_ReadMag(&woken);
#endif

    if (PIOS_MPU9250_ReadSensor(&woken)) {
        woken |= PIOS_MPU9250_HandleData(gyro_read_timestamp);
    }

    return woken;
}

static bool PIOS_MPU9250_HandleData(uint32_t gyro_read_timestamp)
{
    // Rotate the sensor to OP convention.  The datasheet defines X as towards the right
    // and Y as forward.  OP convention transposes this.  Also the Z is defined negatively
    // to our convention
    if (!queue_data) {
        return false;
    }

#ifdef PIOS_MPU9250_MAG
    bool mag_valid = mpu9250_data.data.st1 & PIOS_MPU9250_MAG_DATA_RDY;
#endif

    // Currently we only support rotations on top so switch X/Y accordingly
    switch (dev->cfg->orientation) {
    case PIOS_MPU9250_TOP_0DEG:
#ifdef PIOS_MPU9250_ACCEL
        queue_data->sample[0].y = GET_SENSOR_DATA(mpu9250_data, Accel_X); // chip X
        queue_data->sample[0].x = GET_SENSOR_DATA(mpu9250_data, Accel_Y); // chip Y
#endif
        queue_data->sample[1].y = GET_SENSOR_DATA(mpu9250_data, Gyro_X); // chip X
        queue_data->sample[1].x = GET_SENSOR_DATA(mpu9250_data, Gyro_Y); // chip Y
#ifdef PIOS_MPU9250_MAG
        if (mag_valid) {
            mag_data->sample[0].y = GET_SENSOR_DATA(mpu9250_data, Mag_Y) * dev->mag_sens_adj[1]; // chip Y
            mag_data->sample[0].x = GET_SENSOR_DATA(mpu9250_data, Mag_X) * dev->mag_sens_adj[0]; // chip X
        }
#endif
        break;
    case PIOS_MPU9250_TOP_90DEG:
        // -1 to bring it back to -32768 +32767 range
#ifdef PIOS_MPU9250_ACCEL
        queue_data->sample[0].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Accel_Y)); // chip Y
        queue_data->sample[0].x = GET_SENSOR_DATA(mpu9250_data, Accel_X); // chip X
#endif
        queue_data->sample[1].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Gyro_Y)); // chip Y
        queue_data->sample[1].x = GET_SENSOR_DATA(mpu9250_data, Gyro_X); // chip X
#ifdef PIOS_MPU9250_MAG
        if (mag_valid) {
            mag_data->sample[0].y = GET_SENSOR_DATA(mpu9250_data, Mag_X) * dev->mag_sens_adj[0]; // chip X
            mag_data->sample[0].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Mag_Y)) * dev->mag_sens_adj[1]; // chip Y
        }

#endif
        break;
    case PIOS_MPU9250_TOP_180DEG:
#ifdef PIOS_MPU9250_ACCEL
        queue_data->sample[0].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Accel_X)); // chip X
        queue_data->sample[0].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Accel_Y)); // chip Y
#endif
        queue_data->sample[1].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Gyro_X)); // chip X
        queue_data->sample[1].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Gyro_Y)); // chip Y
#ifdef PIOS_MPU9250_MAG
        if (mag_valid) {
            mag_data->sample[0].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Mag_Y)) * dev->mag_sens_adj[1]; // chip Y
            mag_data->sample[0].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Mag_X)) * dev->mag_sens_adj[0]; // chip X
        }
#endif
        break;
    case PIOS_MPU9250_TOP_270DEG:
#ifdef PIOS_MPU9250_ACCEL
        queue_data->sample[0].y = GET_SENSOR_DATA(mpu9250_data, Accel_Y); // chip Y
        queue_data->sample[0].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Accel_X)); // chip X
#endif
        queue_data->sample[1].y = GET_SENSOR_DATA(mpu9250_data, Gyro_Y); // chip Y
        queue_data->sample[1].x = -1 - (GET_SENSOR_DATA(mpu9250_data, Gyro_X)); // chip X
#ifdef PIOS_MPU9250_MAG
        if (mag_valid) {
            mag_data->sample[0].y = -1 - (GET_SENSOR_DATA(mpu9250_data, Mag_X)) * dev->mag_sens_adj[0]; // chip X
            mag_data->sample[0].x = GET_SENSOR_DATA(mpu9250_data, Mag_Y) * dev->mag_sens_adj[1]; // chip Y
        }
#endif
        break;
    }
#ifdef PIOS_MPU9250_ACCEL
    queue_data->sample[0].z = -1 - (GET_SENSOR_DATA(mpu9250_data, Accel_Z));
#endif
    queue_data->sample[1].z = -1 - (GET_SENSOR_DATA(mpu9250_data, Gyro_Z));
    const int16_t temp = GET_SENSOR_DATA(mpu9250_data, Temperature);
    queue_data->temperature = 2100 + ((float)(temp - PIOS_MPU9250_TEMP_OFFSET)) * (100.0f / PIOS_MPU9250_TEMP_SENSITIVITY);
    queue_data->timestamp   = gyro_read_timestamp;
    mag_data->temperature   = queue_data->temperature;
#ifdef PIOS_MPU9250_MAG
    if (mag_valid) {
        mag_data->sample[0].z = GET_SENSOR_DATA(mpu9250_data, Mag_Z) * dev->mag_sens_adj[2]; // chip Z
        mag_ready = true;
    }
#endif

    BaseType_t higherPriorityTaskWoken;
    xQueueSendToBackFromISR(dev->queue, queue_data, &higherPriorityTaskWoken);
    return higherPriorityTaskWoken == pdTRUE;
}

static bool PIOS_MPU9250_ReadSensor(bool *woken)
{
    const uint8_t mpu9250_send_buf[1 + PIOS_MPU9250_SAMPLES_BYTES] = { PIOS_MPU9250_SENSOR_FIRST_REG | 0x80 };

    if (PIOS_MPU9250_ClaimBusISR(woken, true) != 0) {
        return false;
    }
    if (PIOS_SPI_TransferBlock(dev->spi_id, &mpu9250_send_buf[0], &mpu9250_data.buffer[0], sizeof(mpu9250_data_t), NULL) < 0) {
        PIOS_MPU9250_ReleaseBusISR(woken);
        return false;
    }
    PIOS_MPU9250_ReleaseBusISR(woken);
    return true;
}

// Sensor driver implementation
bool PIOS_MPU9250_Main_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return !PIOS_MPU9250_Test();
}

void PIOS_MPU9250_Main_driver_Reset(__attribute__((unused)) uintptr_t context)
{
    PIOS_MPU9250_GetReg(PIOS_MPU9250_INT_STATUS_REG);
}

void PIOS_MPU9250_Main_driver_get_scale(float *scales, uint8_t size, __attribute__((unused)) uintptr_t contet)
{
    PIOS_Assert(size >= 2);
    scales[0] = PIOS_MPU9250_GetAccelScale();
    scales[1] = PIOS_MPU9250_GetScale();
}

QueueHandle_t PIOS_MPU9250_Main_driver_get_queue(__attribute__((unused)) uintptr_t context)
{
    return dev->queue;
}


/* PIOS sensor driver implementation */
bool PIOS_MPU9250_Mag_driver_Test(__attribute__((unused)) uintptr_t context)
{
    return !PIOS_MPU9250_Test();
}

void PIOS_MPU9250_Mag_driver_Reset(__attribute__((unused)) uintptr_t context) {}

void PIOS_MPU9250_Mag_driver_get_scale(float *scales, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    PIOS_Assert(size > 0);
    scales[0] = 1;
}

void PIOS_MPU9250_Mag_driver_fetch(void *data, uint8_t size, __attribute__((unused))  uintptr_t context)
{
    mag_ready = false;
    PIOS_Assert(size > 0);
    memcpy(data, mag_data, MAG_SENSOR_DATA_SIZE);
}

bool PIOS_MPU9250_Mag_driver_poll(__attribute__((unused)) uintptr_t context)
{
    return mag_ready;
}

#endif /* PIOS_INCLUDE_MPU9250 */

/**
 * @}
 * @}
 */
