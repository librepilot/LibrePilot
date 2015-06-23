/**
 ******************************************************************************
 *
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLASHFS_OBJLIST Object list based flash filesystem (low ram)
 * @{
 *
 * @file       pios_flashfs_objlist.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      A file system for storing UAVObject in flash chip
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


#include "openpilot.h"
#include "uavobjectmanager.h"
#ifdef PIOS_INCLUDE_FLASH_OBJLIST
#include <pios_flashfs_objlist.h>

// Private variables
static int32_t numObjects = -1;

// Private structures
// Header for objects in the file system table
struct objectHeader {
    uint32_t objMagic;
    uint32_t objId;
    uint32_t instId;
    uint32_t address;
} __attribute__((packed));;

struct fileHeader {
    uint32_t id;
    uint16_t instId;
    uint16_t size;
} __attribute__((packed));

enum pios_flashfs_dev_magic {
    PIOS_FLASHFS_DEV_MAGIC = 0xF1A58205,
};

struct flashfs_dev {
    uint32_t  magic;
    const struct flashfs_cfg *cfg;
    const struct pios_flash_driver *driver;
    uintptr_t flash_id;
};

// Private functions
static int32_t PIOS_FLASHFS_ClearObjectTableHeader(struct flashfs_dev *dev);
static int32_t PIOS_FLASHFS_GetObjAddress(struct flashfs_dev *dev, uint32_t objId, uint16_t instId);
static int32_t PIOS_FLASHFS_GetNewAddress(struct flashfs_dev *dev, uint32_t objId, uint16_t instId);

#define MAX_BADMAGIC 1000

static struct flashfs_dev *PIOS_FLASHFS_Alloc()
{
    struct flashfs_dev *dev;

    dev = (struct flashfs_dev *)pios_malloc(sizeof(*dev));
    if (!dev) {
        return NULL;
    }

    dev->magic = PIOS_FLASHFS_DEV_MAGIC;
    return dev;
}
int32_t PIOS_FLASHFS_validate(struct flashfs_dev *dev)
{
    return dev && dev->magic == PIOS_FLASHFS_DEV_MAGIC;
}

/**
 * @brief Initialize the flash object setting FS
 * @return 0 if success, -1 if failure
 */

int32_t PIOS_FLASHFS_Init(uintptr_t *fs_id, const struct flashfs_cfg *cfg, const struct pios_flash_driver *driver, uintptr_t flash_id)
{
    PIOS_Assert(cfg);
    PIOS_Assert(fs_id);
    PIOS_Assert(driver);

    // PIOS_Assert((cfg->total_fs_size / cfg->arena_size > 1));

    /* Make sure the underlying flash driver provides the minimal set of required methods */
    PIOS_Assert(driver->start_transaction);
    PIOS_Assert(driver->end_transaction);
    PIOS_Assert(driver->write_data);
    PIOS_Assert(driver->rewrite_data);
    PIOS_Assert(driver->read_data);

    struct flashfs_dev *dev;

    dev = PIOS_FLASHFS_Alloc();
    if (!dev) {
        return -1;
    }

    /* Bind configuration parameters to this filesystem instance */
    dev->cfg      = cfg;  /* filesystem configuration */
    dev->driver   = driver; /* lower-level flash driver */
    dev->flash_id = flash_id; /* lower-level flash device id */

    // Check for valid object table or create one
    uint32_t object_table_magic;
    bool magic_good = false;
    *fs_id = (uintptr_t)dev;

    while (!magic_good) {
        if (dev->driver->read_data(dev->flash_id, 0, (uint8_t *)&object_table_magic, sizeof(object_table_magic)) != 0) {
            return -1;
        }
        if (object_table_magic != dev->cfg->table_magic) {
            if (PIOS_FLASHFS_Format(*fs_id) != 0) {
                return -1;
            }
#if defined(PIOS_LED_HEARTBEAT)
            PIOS_LED_Toggle(PIOS_LED_HEARTBEAT);
#endif /* PIOS_LED_HEARTBEAT */
            magic_good = true;
        } else {
            magic_good = true;
        }
    }

    uint32_t addr = cfg->obj_table_start;
    struct objectHeader header;
    numObjects = 0;

    // Loop through header area while objects detect to count how many saved
    while (addr < cfg->obj_table_end) {
        // Read the instance data
        if (dev->driver->read_data(dev->flash_id, addr, (uint8_t *)&header, sizeof(header)) != 0) {
            return -1;
        }

        // Counting number of valid headers
        if (header.objMagic != cfg->obj_magic) {
            break;
        }

        numObjects++;
        addr += sizeof(header);
    }

    return 0;
}

/**
 * @brief Erase the whole flash chip and create the file system
 * @return 0 if successful, -1 if not
 */
int32_t PIOS_FLASHFS_Format(uintptr_t fs_id)
{
    struct flashfs_dev *dev = (struct flashfs_dev *)fs_id;

    if (!PIOS_FLASHFS_validate(dev)) {
        return -1;
    }

    if (dev->driver->start_transaction(dev->flash_id)) {
        return -1;
    }

    if (PIOS_FLASHFS_ClearObjectTableHeader(dev) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -1;
    }
    dev->driver->end_transaction(dev->flash_id);
    return 0;
}

/**
 * @brief Erase the headers for all objects in the flash chip
 * @return 0 if successful, -1 if not
 */
static int32_t PIOS_FLASHFS_ClearObjectTableHeader(struct flashfs_dev *dev)
{
    uint32_t addr = dev->cfg->obj_table_start;

    while (addr < dev->cfg->obj_table_end) {
        if (dev->driver->erase_sector(dev->flash_id, addr) != 0) {
            return -1;
        }
        addr += dev->cfg->sector_size;
    }

    if (dev->driver->write_data(dev->flash_id, 0, (uint8_t *)&dev->cfg->table_magic, sizeof(dev->cfg->table_magic)) != 0) {
        return -1;
    }

    uint32_t object_table_magic;
    dev->driver->read_data(dev->flash_id, 0, (uint8_t *)&object_table_magic, sizeof(object_table_magic));
    if (object_table_magic != dev->cfg->table_magic) {
        return -1;
    }

    return 0;
}

/**
 * @brief Get the address of an object
 * @param obj UAVObjHandle for that object
 * @parma instId Instance id for that object
 * @return address if successful, -1 if not found, -2 if failure occurred while reading
 */
static int32_t PIOS_FLASHFS_GetObjAddress(struct flashfs_dev *dev, uint32_t objId, uint16_t instId)
{
    uint32_t addr = dev->cfg->obj_table_start;
    struct objectHeader header;

    // Loop through header area while objects detect to count how many saved
    while (addr < dev->cfg->obj_table_end) {
        // Read the instance data
        if (dev->driver->read_data(dev->flash_id, addr, (uint8_t *)&header, sizeof(header)) != 0) {
            return -2;
        }
        if (header.objMagic != dev->cfg->obj_magic) {
            break; // stop searching once hit first non-object header
        } else if (header.objId == objId && header.instId == instId) {
            break;
        }
        addr += sizeof(header);
    }

    if (header.objId == objId && header.instId == instId) {
        return header.address;
    }

    return -1;
}

/**
 * @brief Returns an address for a new object and creates entry into object table
 * @param[in] obj Object handle for object to be saved
 * @param[in] instId The instance id of object to be saved
 * @return 0 if success or error code
 * @retval -1 Object not found
 * @retval -2 No room in object table
 * @retval -3 Unable to write entry into object table
 * @retval -4 FS not initialized
 * @retval -5
 */
static int32_t PIOS_FLASHFS_GetNewAddress(struct flashfs_dev *dev, uint32_t objId, uint16_t instId)
{
    struct objectHeader header;

    if (numObjects < 0) {
        return -4;
    }

    // Don't worry about max size of flash chip here, other code will catch that
    header.objMagic = dev->cfg->obj_magic;
    header.objId    = objId;
    header.instId   = instId;
    header.address  = dev->cfg->obj_table_end + dev->cfg->sector_size * numObjects;

    int32_t addr = dev->cfg->obj_table_start + sizeof(header) * numObjects;

    // No room for this header in object table
    if ((addr + sizeof(header)) > dev->cfg->obj_table_end) {
        return -2;
    }

    // Verify the address is within the chip
    if ((addr + dev->cfg->sector_size) > dev->cfg->chip_size) {
        return -5;
    }

    if (dev->driver->write_data(dev->flash_id, addr, (uint8_t *)&header, sizeof(header)) != 0) {
        return -3;
    }

    // This numObejcts value must stay consistent or there will be a break in the table
    // and later the table will have bad values in it
    numObjects++;
    return header.address;
}


/**
 * @brief Saves one object instance per sector
 * @param[in] obj UAVObjHandle the object to save
 * @param[in] instId The instance of the object to save
 * @return 0 if success or -1 if failure
 * @note This uses one sector on the flash chip per object so that no buffering in ram
 * must be done when erasing the sector before a save
 */
int32_t PIOS_FLASHFS_ObjSave(uintptr_t fs_id, uint32_t obj_id, uint16_t obj_inst_id, uint8_t *obj_data, uint16_t obj_size)
{
    struct flashfs_dev *dev = (struct flashfs_dev *)fs_id;

    if (!PIOS_FLASHFS_validate(dev)) {
        return -1;
    }


    uint8_t crc = 0;

    if (dev->driver->start_transaction(dev->flash_id) != 0) {
        return -1;
    }

    int32_t addr = PIOS_FLASHFS_GetObjAddress(dev, obj_id, obj_inst_id);

    // Object currently not saved
    if (addr < 0) {
        addr = PIOS_FLASHFS_GetNewAddress(dev, obj_id, obj_inst_id);
    }

    // Could not allocate a sector
    if (addr < 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -1;
    }

    struct fileHeader header = {
        .id     = obj_id,
        .instId = obj_inst_id,
        .size   = obj_size,
    };

    // Update CRC
    crc = PIOS_CRC_updateCRC(0, (uint8_t *)&header, sizeof(header));
    crc = PIOS_CRC_updateCRC(crc, obj_data, obj_size);

    if (dev->driver->erase_sector(dev->flash_id, addr) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -2;
    }

    struct pios_flash_chunk chunks[3] = {
        {
            .addr = (uint8_t *)&header,
            .len  = sizeof(header),
        },
        {
            .addr = obj_data,
            .len  = obj_size
        },
        {
            .addr = (uint8_t *)&crc,
            .len  = sizeof(crc)
        }
    };

    if (dev->driver->write_chunks(dev->flash_id, addr, chunks, NELEMENTS(chunks)) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -1;
    }

    if (dev->driver->end_transaction(dev->flash_id) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -1;
    }

    return 0;
}

/**
 * @brief Load one object instance per sector
 * @param[in] obj UAVObjHandle the object to save
 * @param[in] instId The instance of the object to save
 * @return 0 if success or error code
 * @retval -1 if object not in file table
 * @retval -2 if unable to retrieve object header
 * @retval -3 if loaded data instId or objId don't match
 * @retval -4 if unable to retrieve instance data
 * @retval -5 if unable to read CRC
 * @retval -6 if CRC doesn't match
 * @note This uses one sector on the flash chip per object so that no buffering in ram
 * must be done when erasing the sector before a save
 */
int32_t PIOS_FLASHFS_ObjLoad(uintptr_t fs_id, uint32_t obj_id, uint16_t obj_inst_id, uint8_t *obj_data, uint16_t obj_size)
{
    struct flashfs_dev *dev = (struct flashfs_dev *)fs_id;

    if (!PIOS_FLASHFS_validate(dev)) {
        return -1;
    }
    uint8_t crc = 0;
    uint8_t crcFlash = 0;
    const uint8_t crc_read_step = 8;
    uint8_t crc_read_buffer[crc_read_step];

    if (dev->driver->start_transaction(dev->flash_id) != 0) {
        return -1;
    }

    int32_t addr = PIOS_FLASHFS_GetObjAddress(dev, obj_id, obj_inst_id);

    // Object currently not saved
    if (addr < 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -1;
    }

    struct fileHeader header;

    // Load header
    // This information IS redundant with the object table id.  Oh well.  Better safe than sorry.
    if (dev->driver->read_data(dev->flash_id, addr, (uint8_t *)&header, sizeof(header)) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -2;
    }

    // Update CRC
    crc = PIOS_CRC_updateCRC(0, (uint8_t *)&header, sizeof(header));

    if ((header.id != obj_id) || (header.instId != obj_inst_id)) {
        dev->driver->end_transaction(dev->flash_id);
        return -3;
    }

    // To avoid having to allocate the RAM for a copy of the object, we read by chunks
    // and compute the CRC
    for (uint32_t i = 0; i < obj_size; i += crc_read_step) {
        dev->driver->read_data(dev->flash_id, addr + sizeof(header) + i, crc_read_buffer, crc_read_step);
        uint8_t valid_bytes = ((i + crc_read_step) >= obj_size) ? obj_size - i : crc_read_step;
        crc = PIOS_CRC_updateCRC(crc, crc_read_buffer, valid_bytes);
    }

    // Read CRC (written so will work when CRC changes to uint16)
    if (dev->driver->read_data(dev->flash_id, addr + sizeof(header) + obj_size, (uint8_t *)&crcFlash, sizeof(crcFlash)) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -5;
    }

    if (crc != crcFlash) {
        dev->driver->end_transaction(dev->flash_id);
        return -6;
    }

    // Read the instance data
    if (dev->driver->read_data(dev->flash_id, addr + sizeof(header), obj_data, obj_size) != 0) {
        dev->driver->end_transaction(dev->flash_id);
        return -4;
    }

    if (dev->driver->end_transaction(dev->flash_id) != 0) {
        return -1;
    }
    return 0;
}

/**
 * @brief Delete object from flash
 * @param[in] obj UAVObjHandle the object to save
 * @param[in] instId The instance of the object to save
 * @return 0 if success or error code
 * @retval -1 if object not in file table
 * @retval -2 Erase failed
 * @note To avoid buffering the file table (1k ram!) the entry in the file table
 * remains but destination sector is erased.  This will make the load fail as the
 * file header won't match the object.  At next save it goes back there.
 */
int32_t PIOS_FLASHFS_ObjDelete(uintptr_t fs_id, uint32_t obj_id, uint16_t obj_inst_id)
{
    struct flashfs_dev *dev = (struct flashfs_dev *)fs_id;

    if (!PIOS_FLASHFS_validate(dev)) {
        return -1;
    }


    int32_t addr = PIOS_FLASHFS_GetObjAddress(dev, obj_id, obj_inst_id);

    // Object currently not saved
    if (addr < 0) {
        return -1;
    }

    if (dev->driver->erase_sector(dev->flash_id, addr) != 0) {
        return -2;
    }

    return 0;
}
#endif /* PIOS_INCLUDE_FLASH_OBJLIST */
