/**
 ******************************************************************************
 *
 * @file       pios_flash_eeprom.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLASH EEPROM Driver API Definition
 * @{
 * @brief EEPROM Driver API Definition
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


#include <pios.h>

#ifdef PIOS_INCLUDE_FLASH_EEPROM

#ifndef PIOS_EEPROM_EMULATED_SECTOR_SIZE
#define PIOS_EEPROM_EMULATED_SECTOR_SIZE 256
#endif

#include <pios_flash_eeprom.h>
#include <pios_math.h>
enum pios_eeprom_dev_magic {
    PIOS_EEPROM_DEV_MAGIC = 0xEE55aa55,
};

// ! Device handle structure
struct flash_eeprom_dev {
    enum pios_eeprom_dev_magic magic;
    uint32_t i2c_adapter;
    uint32_t i2c_address;
    const struct pios_flash_eeprom_cfg *cfg;
#ifdef PIOS_INCLUDE_FREERTOS
    xSemaphoreHandle transaction_lock;
#endif
};

// ! Private functions
static int32_t PIOS_Flash_EEPROM_Validate(struct  flash_eeprom_dev *flash_dev);
static struct flash_eeprom_dev *PIOS_Flash_EEPROM_alloc(void);

static int32_t PIOS_Flash_EEPROM_Busy(struct flash_eeprom_dev *flash_dev);

/**
 * @brief Allocate a new device
 */
static struct flash_eeprom_dev *PIOS_Flash_EEPROM_alloc(void)
{
    struct flash_eeprom_dev *flash_dev;

    flash_dev = (struct flash_eeprom_dev *)pios_malloc(sizeof(*flash_dev));
    if (!flash_dev) {
        return NULL;
    }
    flash_dev->magic = PIOS_EEPROM_DEV_MAGIC;
#ifdef PIOS_INCLUDE_FREERTOS
    flash_dev->transaction_lock = xSemaphoreCreateMutex();
#endif
    return flash_dev;
}

/**
 * @brief Validate the handler to the device
 */
static int32_t PIOS_Flash_EEPROM_Validate(struct flash_eeprom_dev *flash_dev)
{
    if (flash_dev == NULL) {
        return -1;
    }
    if (flash_dev->magic != PIOS_EEPROM_DEV_MAGIC) {
        return -2;
    }
    if (flash_dev->i2c_adapter == 0 || flash_dev->i2c_address == 0) {
        return -3;
    }
    return 0;
}

/**
 * @brief Initialize the flash device and enable write access
 */
int32_t PIOS_Flash_EEPROM_Init(uintptr_t *flash_id, struct pios_flash_eeprom_cfg *cfg, int32_t i2c_adapter, uint32_t i2c_addr)
{
    struct flash_eeprom_dev *flash_dev = PIOS_Flash_EEPROM_alloc();

    if (!flash_dev) {
        return -1;
    }

    flash_dev->i2c_adapter = i2c_adapter;
    flash_dev->i2c_address = i2c_addr;
    flash_dev->cfg = cfg;

    if (!flash_dev->cfg) {
        return -1;
    }

    /* Give back a handle to this flash device */
    *flash_id = (uintptr_t)flash_dev;

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
int32_t PIOS_Flash_EEPROM_ReadSinglePage(struct flash_eeprom_dev *flash_dev, uint32_t address, uint8_t *buffer, uint8_t len)
{
    uint8_t i2c_addr = flash_dev->i2c_address;
    uint32_t bus     = flash_dev->i2c_adapter;

    if (address > 0xFFFF) {
        return -2;
    }

    uint8_t mem_address[] = {
        (address & 0xFF00) >> 8, // MSB
        (address & 0xFF) // LSB
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = i2c_addr,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = 2,
            .buf  = mem_address,
        }
        ,
        {
            .info = __func__,
            .addr = i2c_addr,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    return PIOS_I2C_Transfer(bus, txn_list, NELEMENTS(txn_list));
}


/**
 * Writes one up to a single page (128 bytes) to the EEPROM
 * \param[in] address write start address
 * \param[in] buffer source buffer
 * \param[in] len Buffer length
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
int32_t PIOS_Flash_EEPROM_WriteSinglePage(struct flash_eeprom_dev *flash_dev, const uint32_t address, const uint8_t *buffer, const uint8_t len)
{
    uint8_t i2c_addr = flash_dev->i2c_address;
    uint32_t bus     = flash_dev->i2c_adapter;

    if (address > 0xFFFF) {
        return -2;
    }
    uint16_t address16 = address & 0xFFFF;

    uint8_t tmp[len + 2];
    tmp[0] = (address16 & 0xFF00) >> 8; // MSB
    tmp[1] = (address16 & 0xFF); // LSB


    memcpy(&tmp[2], buffer, len);

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = i2c_addr,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = len + 2,
            .buf  = tmp,
        }
    };

    return PIOS_I2C_Transfer(bus, txn_list, NELEMENTS(txn_list));
}

/**
 * Read blocks of data spliting them into several single page operations to the EEPROM
 * \param[in] address Read start address
 * \param[in] buffer Dest buffer
 * \param[in] len Buffer length
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
int32_t PIOS_Flash_EEPROM_Read(struct flash_eeprom_dev *flash_dev, const uint32_t address, uint8_t *buffer, const uint8_t len)
{
    uint16_t address16 = address & 0xFFFF;

    if (address16 < address) {
        return -2;
    }
    // split the operation into several page operations
    uint8_t bytes_read = 0;

    const uint16_t page_len = flash_dev->cfg->page_len;

    while (bytes_read < len) {
        // all is fine, wait until memory is not busy
        int32_t status;
        while ((status = PIOS_Flash_EEPROM_Busy(flash_dev)) == 1) {
#ifdef PIOS_INCLUDE_FREERTOS
            vTaskDelay(0);
#endif
        }
        // An error occurred while probing the status, return and report
        if (status < 0) {
            return status;
        }
        uint16_t current_block_len = len - bytes_read;
        uint16_t index_within_page = (address16 + bytes_read) % page_len;
        // prevent overflowing the page boundary
        current_block_len = MIN(page_len - index_within_page, current_block_len);
        status      = PIOS_Flash_EEPROM_ReadSinglePage(flash_dev, address + bytes_read, &buffer[bytes_read], current_block_len);
        bytes_read += current_block_len;
        if (status) {
            // error occurred during the write operation
            return status;
        }
    }

    return 0;
}

/**
 * Writes blocks of data spliting them into several single page writes to the EEPROM
 * \param[in] address write start address
 * \param[in] buffer source buffer
 * \param[in] len Buffer length
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 */
int32_t PIOS_Flash_EEPROM_Write(struct flash_eeprom_dev *flash_dev, const uint32_t address, const uint8_t *buffer, const uint8_t len)
{
    uint16_t address16 = address & 0xFFFF;

    if (address16 < address) {
        return -2;
    }
    // split the operation into several page writes
    uint8_t bytes_written  = 0;

    const uint8_t page_len = flash_dev->cfg->page_len;
    while (bytes_written < len) {
        // wait until memory is not busy
        int32_t status;
        while ((status = PIOS_Flash_EEPROM_Busy(flash_dev)) == 1) {
#ifdef PIOS_INCLUDE_FREERTOS
            vTaskDelay(1);
#endif
        }
        // An error occurred while probing the status, return and report
        if (status < 0) {
            return status;
        }
        uint8_t current_block_len  = len - bytes_written;
        uint16_t index_within_page = (address16 + bytes_written) % page_len;
        // prevent overflowing the page boundary
        current_block_len = MIN(page_len - index_within_page, current_block_len);
        status = PIOS_Flash_EEPROM_WriteSinglePage(flash_dev, address + bytes_written, &buffer[bytes_written], current_block_len);
        bytes_written    += current_block_len;
        if (status) {
            // error occurred during the write operation
            return status;
        }
    }
    return 0;
}
/**
 * @brief Returns if the flash chip is busy
 * @returns -1 for failure, 0 for not busy, 1 for busy
 */
static int32_t PIOS_Flash_EEPROM_Busy(struct flash_eeprom_dev *flash_dev)
{
    uint8_t buf;
    int32_t status = PIOS_Flash_EEPROM_ReadSinglePage(flash_dev, 0x0, &buf, 1);

    switch (status) {
    case -3: // NACK
        return 1;

    case 0:
        return 0;

    default:
        return -1;
    }
}

/**********************************
 *
 * Provide a PIOS flash driver API
 *
 *********************************/
#include "pios_flash.h"

#if FLASH_USE_FREERTOS_LOCKS

/**
 * @brief Grab the semaphore to perform a transaction
 * @return 0 for success, -1 for timeout
 */
static int32_t PIOS_Flash_EEPROM_StartTransaction(uintptr_t flash_id)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreTake(flash_dev->transaction_lock, portMAX_DELAY) != pdTRUE) {
        return -2;
    }
#endif

    return 0;
}

/**
 * @brief Release the semaphore to perform a transaction
 * @return 0 for success, -1 for timeout
 */
static int32_t PIOS_Flash_EEPROM_EndTransaction(uintptr_t flash_id)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreGive(flash_dev->transaction_lock) != pdTRUE) {
        return -2;
    }
#endif

    return 0;
}

#else /* FLASH_USE_FREERTOS_LOCKS */

static int32_t PIOS_Flash_EEPROM_StartTransaction(__attribute__((unused)) uintptr_t flash_id)
{
    return 0;
}

static int32_t PIOS_Flash_EEPROM_EndTransaction(__attribute__((unused)) uintptr_t flash_id)
{
    return 0;
}

#endif /* FLASH_USE_FREERTOS_LOCKS */

/**
 * @brief Erase a sector on the flash chip
 * @param[in] add Address of flash to erase
 * @returns 0 if successful
 * @retval -1 if unable to claim bus
 * @retval
 */
static int32_t PIOS_Flash_EEPROM_EraseSector(uintptr_t flash_id, uint32_t addr)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }
    // Rewrite the whole page
    const uint8_t buf[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    uint32_t current = addr;
    while (current - addr < PIOS_EEPROM_EMULATED_SECTOR_SIZE) {
        uint32_t erase_size = (MIN(sizeof(buf), flash_dev->cfg->page_len));
        erase_size = MIN(erase_size, (PIOS_EEPROM_EMULATED_SECTOR_SIZE - current + addr));
        int32_t res = PIOS_Flash_EEPROM_Write(flash_dev, current, buf, erase_size);
        if (!res) {
            return res;
        }
        current += erase_size;
    }
    return 0;
}

/**
 * @brief Write one page of data (up to 256 bytes) aligned to a page start
 * @param[in] addr Address in flash to write to
 * @param[in] data Pointer to data to write to flash
 * @param[in] len Length of data to write (max 256 bytes)
 * @return Zero if success or error code
 * @retval -1 Unable to write to the device
 */
static int32_t PIOS_Flash_EEPROM_WriteData(uintptr_t flash_id, uint32_t addr, uint8_t *data, uint16_t len)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }
    if (PIOS_Flash_EEPROM_Write(flash_dev, addr, data, len) < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Write multiple chunks of data in one transaction
 * @param[in] addr Address in flash to write to
 * @param[in] data Pointer to data to write to flash
 * @param[in] len Length of data to write
 * @return Zero if success or error code
 * @retval -1 Unable to write to the device
 */
static int32_t PIOS_Flash_EEPROM_WriteChunks(uintptr_t flash_id, uint32_t addr, struct pios_flash_chunk chunks[], uint32_t num)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }
    uint32_t memory_displacement = 0;
    for (uint32_t i = 0; i < num; i++) {
        struct pios_flash_chunk *chunk = &chunks[i];
        // no need to check for busy flag as it is done before each write operation by _Write function
        if (PIOS_Flash_EEPROM_Write(flash_dev, addr + memory_displacement, chunk->addr, chunk->len) < 0) {
            return -1;
        }
        memory_displacement += chunk->len;
    }
    return 0;
}

/**
 * @brief Read data from a location in flash memory
 * @param[in] addr Address in flash to write to
 * @param[in] data Pointer to data to write from flash
 * @param[in] len Length of data to write (max 256 bytes)
 * @return Zero if success or error code
 * @retval -1 Unable to read from the device
 */
static int32_t PIOS_Flash_EEPROM_ReadData(uintptr_t flash_id, uint32_t addr, uint8_t *data, uint16_t len)
{
    struct flash_eeprom_dev *flash_dev = (struct flash_eeprom_dev *)flash_id;

    if (PIOS_Flash_EEPROM_Validate(flash_dev) != 0) {
        return -1;
    }
    if (PIOS_Flash_EEPROM_Read(flash_dev, addr, data, len) < 0) {
        return -1;
    }

    return 0;
}

/* Provide a flash driver to external drivers */
const struct pios_flash_driver pios_EEPROM_flash_driver = {
    .start_transaction = PIOS_Flash_EEPROM_StartTransaction,
    .end_transaction   = PIOS_Flash_EEPROM_EndTransaction,
    .erase_chip     = NULL,
    .erase_sector   = PIOS_Flash_EEPROM_EraseSector,
    .write_chunks   = PIOS_Flash_EEPROM_WriteChunks,
    .rewrite_chunks = PIOS_Flash_EEPROM_WriteChunks,
    .write_data     = PIOS_Flash_EEPROM_WriteData,
    .rewrite_data   = PIOS_Flash_EEPROM_WriteData,
    .read_data         = PIOS_Flash_EEPROM_ReadData,
};

#endif /* PIOS_INCLUDE_FLASH_EEPROM*/
