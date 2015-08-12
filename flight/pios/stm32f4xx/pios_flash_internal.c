/**
 ******************************************************************************
 * @file       pios_flash_internal.c
 * @author     PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief Provides a flash driver for the STM32 internal flash sectors
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

#include "pios.h"

#ifdef PIOS_INCLUDE_FLASH_INTERNAL

#include "stm32f4xx_flash.h"
#include "pios_flash_internal_priv.h"
#include "pios_flash.h"
#include <stdbool.h>

struct device_flash_sector {
    uint32_t start;
    uint32_t size;
    uint16_t st_sector;
};

static struct device_flash_sector flash_sectors[] = {
    [0] =  {
        .start     = 0x08000000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_0,
    },
    [1] =  {
        .start     = 0x08004000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_1,
    },
    [2] =  {
        .start     = 0x08008000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_2,
    },
    [3] =  {
        .start     = 0x0800C000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_3,
    },
    [4] =  {
        .start     = 0x08010000,
        .size      = 64 * 1024,
        .st_sector = FLASH_Sector_4,
    },
    [5] =  {
        .start     = 0x08020000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_5,
    },
    [6] =  {
        .start     = 0x08040000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_6,
    },
    [7] =  {
        .start     = 0x08060000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_7,
    },
    [8] =  {
        .start     = 0x08080000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_8,
    },
    [9] =  {
        .start     = 0x080A0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_9,
    },
    [10] = {
        .start     = 0x080C0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_10,
    },
    [11] = {
        .start     = 0x080E0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_11,
    },
    [12] = {
        .start     = 0x08100000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_12,
    },
    [13] = {
        .start     = 0x08104000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_13,
    },
    [14] = {
        .start     = 0x08108000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_14,
    },
    [15] = {
        .start     = 0x0810C000,
        .size      = 16 * 1024,
        .st_sector = FLASH_Sector_15,
    },
    [16] = {
        .start     = 0x08110000,
        .size      = 64 * 1024,
        .st_sector = FLASH_Sector_16,
    },
    [17] = {
        .start     = 0x08120000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_17,
    },
    [18] = {
        .start     = 0x08140000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_18,
    },
    [19] = {
        .start     = 0x08160000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_19,
    },
    [20] = {
        .start     = 0x08180000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_20,
    },
    [21] = {
        .start     = 0x081A0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_21,
    },
    [22] = {
        .start     = 0x081C0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_22,
    },
    [23] = {
        .start     = 0x081E0000,
        .size      = 128 * 1024,
        .st_sector = FLASH_Sector_23,
    },
};

static bool PIOS_Flash_Internal_GetSectorInfo(uint32_t address, uint8_t *sector_number, uint32_t *sector_start, uint32_t *sector_size)
{
    for (uint8_t i = 0; i < NELEMENTS(flash_sectors); i++) {
        struct device_flash_sector *sector = &flash_sectors[i];
        if ((address >= sector->start) &&
            (address < (sector->start + sector->size))) {
            /* address lies within this sector */
            *sector_number = sector->st_sector;
            *sector_start  = sector->start;
            *sector_size   = sector->size;
            return true;
        }
    }

    return false;
}

enum pios_internal_flash_dev_magic {
    PIOS_INTERNAL_FLASH_DEV_MAGIC = 0x33445902,
};

struct pios_internal_flash_dev {
    enum pios_internal_flash_dev_magic magic;

#if defined(PIOS_INCLUDE_FREERTOS)
    xSemaphoreHandle transaction_lock;
#endif /* defined(PIOS_INCLUDE_FREERTOS) */
};

static bool PIOS_Flash_Internal_Validate(struct pios_internal_flash_dev *flash_dev)
{
    return flash_dev && (flash_dev->magic == PIOS_INTERNAL_FLASH_DEV_MAGIC);
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_internal_flash_dev *PIOS_Flash_Internal_alloc(void)
{
    struct pios_internal_flash_dev *flash_dev;

    flash_dev = (struct pios_internal_flash_dev *)pios_malloc(sizeof(*flash_dev));
    if (!flash_dev) {
        return NULL;
    }

    flash_dev->magic = PIOS_INTERNAL_FLASH_DEV_MAGIC;

    return flash_dev;
}
#else
static struct pios_internal_flash_dev pios_internal_flash_devs[PIOS_INTERNAL_FLASH_MAX_DEVS];
static uint8_t pios_internal_flash_num_devs;
static struct pios_internal_flash_dev *PIOS_Flash_Internal_alloc(void)
{
    struct pios_internal_flash_dev *flash_dev;

    if (pios_internal_flash_num_devs >= PIOS_INTERNAL_FLASH_MAX_DEVS) {
        return NULL;
    }

    flash_dev = &pios_internal_flash_devs[pios_internal_flash_num_devs++];
    flash_dev->magic = PIOS_INTERNAL_FLASH_DEV_MAGIC;

    return flash_dev;
}

#endif /* defined(PIOS_INCLUDE_FREERTOS) */

int32_t PIOS_Flash_Internal_Init(uintptr_t *flash_id, __attribute__((unused)) const struct pios_flash_internal_cfg *cfg)
{
    struct pios_internal_flash_dev *flash_dev;

    flash_dev = PIOS_Flash_Internal_alloc();
    if (flash_dev == NULL) {
        return -1;
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    flash_dev->transaction_lock = xSemaphoreCreateMutex();
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

    *flash_id = (uintptr_t)flash_dev;

    return 0;
}

/**********************************
 *
 * Provide a PIOS flash driver API
 *
 *********************************/
#include "pios_flash.h"

static int32_t PIOS_Flash_Internal_StartTransaction(uintptr_t flash_id)
{
    struct pios_internal_flash_dev *flash_dev = (struct pios_internal_flash_dev *)flash_id;

    if (!PIOS_Flash_Internal_Validate(flash_dev)) {
        return -1;
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreTake(flash_dev->transaction_lock, portMAX_DELAY) != pdTRUE) {
        return -2;
    }
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

    /* Unlock the internal flash so we can write to it */
    FLASH_Unlock();
    return 0;
}

static int32_t PIOS_Flash_Internal_EndTransaction(uintptr_t flash_id)
{
    struct pios_internal_flash_dev *flash_dev = (struct pios_internal_flash_dev *)flash_id;

    if (!PIOS_Flash_Internal_Validate(flash_dev)) {
        return -1;
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreGive(flash_dev->transaction_lock) != pdTRUE) {
        return -2;
    }
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

    /* Lock the internal flash again so we can no longer write to it */
    FLASH_Lock();

    return 0;
}

static int32_t PIOS_Flash_Internal_EraseSector(uintptr_t flash_id, uint32_t addr)
{
    struct pios_internal_flash_dev *flash_dev = (struct pios_internal_flash_dev *)flash_id;

    if (!PIOS_Flash_Internal_Validate(flash_dev)) {
        return -1;
    }

    uint8_t sector_number;
    uint32_t sector_start;
    uint32_t sector_size;
    if (!PIOS_Flash_Internal_GetSectorInfo(addr,
                                           &sector_number,
                                           &sector_start,
                                           &sector_size)) {
        /* We're asking for an invalid flash address */
        return -2;
    }

    if (FLASH_EraseSector(sector_number, VoltageRange_3) != FLASH_COMPLETE) {
        return -3;
    }

    return 0;
}

static int32_t PIOS_Flash_Internal_WriteData(uintptr_t flash_id, uint32_t addr, uint8_t *data, uint16_t len)
{
    PIOS_Assert(data);

    struct pios_internal_flash_dev *flash_dev = (struct pios_internal_flash_dev *)flash_id;

    if (!PIOS_Flash_Internal_Validate(flash_dev)) {
        return -1;
    }

    uint8_t sector_number;
    uint32_t sector_start;
    uint32_t sector_size;

    /* Ensure that the base address is in a valid sector */
    if (!PIOS_Flash_Internal_GetSectorInfo(addr,
                                           &sector_number,
                                           &sector_start,
                                           &sector_size)) {
        /* We're asking for an invalid flash address */
        return -2;
    }

    /* Ensure that the entire write occurs within the same sector */
    if ((uintptr_t)addr + len > sector_start + sector_size) {
        /* Write crosses the end of the sector */
        return -3;
    }
    FLASH_Status status;
    for (uint16_t i = 0; i < len / 4; i++) {
        uint32_t data_word = *(uint32_t *)(data + i * 4);
        status = FLASH_ProgramWord(addr + i * 4, data_word);
        if (status != FLASH_COMPLETE) {
            return -4;
        }
    }

    /* Write the data */
    for (uint16_t i = len - len % 4; i < len; i++) {
        status = FLASH_ProgramByte(addr + i, data[i]);
        if (status != FLASH_COMPLETE) {
            return -5;
        }
    }

    return 0;
}

static int32_t PIOS_Flash_Internal_ReadData(uintptr_t flash_id, uint32_t addr, uint8_t *data, uint16_t len)
{
    PIOS_Assert(data);

    struct pios_internal_flash_dev *flash_dev = (struct pios_internal_flash_dev *)flash_id;

    if (!PIOS_Flash_Internal_Validate(flash_dev)) {
        return -1;
    }

    uint8_t sector_number;
    uint32_t sector_start;
    uint32_t sector_size;

    /* Ensure that the base address is in a valid sector */
    if (!PIOS_Flash_Internal_GetSectorInfo(addr,
                                           &sector_number,
                                           &sector_start,
                                           &sector_size)) {
        /* We're asking for an invalid flash address */
        return -2;
    }

    /* Ensure that the entire read occurs within the same sector */
    if ((uintptr_t)addr + len > sector_start + sector_size) {
        /* Read crosses the end of the sector */
        return -3;
    }

    /* Read the data into the buffer directly */
    memcpy(data, (void *)addr, len);

    return 0;
}

/* Provide a flash driver to external drivers */
const struct pios_flash_driver pios_internal_flash_driver = {
    .start_transaction = PIOS_Flash_Internal_StartTransaction,
    .end_transaction   = PIOS_Flash_Internal_EndTransaction,
    .erase_sector = PIOS_Flash_Internal_EraseSector,
    .write_data   = PIOS_Flash_Internal_WriteData,
    .read_data    = PIOS_Flash_Internal_ReadData,
};

#endif /* PIOS_INCLUDE_FLASH_INTERNAL */
