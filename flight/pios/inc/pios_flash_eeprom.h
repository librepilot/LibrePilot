/**
 ******************************************************************************
 *
 * @file       pios_flash_eeprom.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @brief      API to interface to EEPROM chips using the common flash api
 *             --
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
#ifndef FLIGHT_PIOS_INC_PIOS_FLASH_EEPROM_H_
#define FLIGHT_PIOS_INC_PIOS_FLASH_EEPROM_H_
#include <pios_flash.h>
struct pios_flash_eeprom_cfg {
    uint16_t page_len;
    uint32_t total_size;
};
extern const struct pios_flash_driver pios_EEPROM_flash_driver;
int32_t PIOS_Flash_EEPROM_Init(uintptr_t *flash_id, struct pios_flash_eeprom_cfg *cfg, int32_t i2c_adapter, uint32_t i2c_addr);

#endif /* FLIGHT_PIOS_INC_PIOS_FLASH_EEPROM_H_ */
