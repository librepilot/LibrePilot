/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the CopterControl board.
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

#include "inc/openpilot.h"
#include <pios_board_info.h>
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

#define PIOS_COM_MAIN_RX_BUF_LEN 32
#define PIOS_COM_MAIN_TX_BUF_LEN 128

uint32_t pios_com_main_id;
uintptr_t flash_id;
/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
int32_t init_test;
void PIOS_Board_Init(void)
{
    /* Delay system */
    PIOS_DELAY_Init();

    const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_LED)
    const struct pios_gpio_cfg *led_cfg  = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
    PIOS_Assert(led_cfg);
    PIOS_LED_Init(led_cfg);
#endif /* PIOS_INCLUDE_LED */

    /* Initialize watchdog as early as possible to catch faults during init */
#ifdef PIOS_INCLUDE_WDG
    PIOS_WDG_Init();
#endif
#if defined(PIOS_INCLUDE_SPI)
    /* Set up the SPI interface to the serial flash */
    if (PIOS_SPI_Init(&pios_spi_mag_flash_id, &pios_spi_mag_flash_cfg)) {
        PIOS_Assert(0);
    }

#ifdef PIOS_INCLUDE_HMC5X83
    PIOS_SPI_SetClockSpeed(pios_spi_mag_flash_id, SPI_BaudRatePrescaler_16);
    onboard_mag = PIOS_HMC5x83_Init(&pios_mag_cfg, pios_spi_mag_flash_id, 0);
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_mag_flash_id, 1)) {
        PIOS_DEBUG_Assert(0);
    }
#endif

#endif

#if defined(PIOS_INCLUDE_RTC)
    /* Initialize the real-time clock and its associated tick */
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif
    PIOS_IAP_Init();

    /* Check for repeated boot failures */
    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
    }

#if defined(PIOS_INCLUDE_COM)
    {
        uint32_t pios_usart_generic_id;
        if (PIOS_USART_Init(&pios_usart_generic_id, &pios_usart_generic_main_cfg)) {
            PIOS_Assert(0);
        }
        uint8_t *rx_buffer = (uint8_t *)pvPortMalloc(PIOS_COM_MAIN_RX_BUF_LEN);
        PIOS_Assert(rx_buffer);

        uint8_t *tx_buffer = (uint8_t *)pvPortMalloc(PIOS_COM_MAIN_TX_BUF_LEN);
        PIOS_Assert(tx_buffer);
        if (PIOS_COM_Init(&pios_com_main_id, &pios_usart_com_driver, pios_usart_generic_id,
                          rx_buffer, PIOS_COM_MAIN_RX_BUF_LEN,
                          tx_buffer, PIOS_COM_MAIN_TX_BUF_LEN)) {
            PIOS_Assert(0);
        }

        PIOS_COM_ChangeBaud(pios_com_main_id, 57600);
    }
#endif

#if defined(PIOS_INCLUDE_I2C)
    {
        if (PIOS_I2C_Init(&pios_i2c_gps_id, &pios_i2c_gps_cfg)) {
            flash_id = -1;
        }
    }
#endif /* PIOS_INCLUDE_I2C */
}

/**
 * @}
 */
