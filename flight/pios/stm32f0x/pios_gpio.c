/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_GPIO GPIO Functions
 * @brief STM32 Hardware GPIO handling code
 * @{
 *
 * @file       pios_gpio.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      GPIO functions, init, toggle, on & off.
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

#include "pios.h"

#ifdef PIOS_INCLUDE_GPIO

#include <pios_gpio_priv.h>

/**
 * Initialises all the GPIO's
 */
int32_t PIOS_GPIO_Init(uint32_t *gpios_dev_id, const struct pios_gpio_cfg *cfg)
{
    PIOS_Assert(cfg);
    *gpios_dev_id = (uint32_t)cfg;

    for (uint8_t i = 0; i < cfg->num_gpios; i++) {
        const struct pios_gpio *gpio = &(cfg->gpios[i]);

        if (gpio->remap) {
            GPIO_PinAFConfig(gpio->pin.gpio, gpio->pin.init.GPIO_Pin, gpio->remap);
        }

        GPIO_Init(gpio->pin.gpio, (GPIO_InitTypeDef *)&gpio->pin.init);

        PIOS_GPIO_Off(*gpios_dev_id, i);
    }

    return 0;
}

/**
 * Turn on GPIO
 * \param[in] GPIO GPIO id
 */
void PIOS_GPIO_On(uint32_t gpios_dev_id, uint8_t gpio_id)
{
    const struct pios_gpio_cfg *gpio_cfg = (const struct pios_gpio_cfg *)gpios_dev_id;

    PIOS_Assert(gpio_cfg);

    if (gpio_id >= gpio_cfg->num_gpios) {
        /* GPIO index out of range */
        return;
    }

    const struct pios_gpio *gpio = &(gpio_cfg->gpios[gpio_id]);

    if (gpio->active_low) {
        gpio->pin.gpio->BRR = gpio->pin.init.GPIO_Pin;
    } else {
        gpio->pin.gpio->BSRR = gpio->pin.init.GPIO_Pin;
    }
}

/**
 * Turn off GPIO
 * \param[in] GPIO GPIO id
 */
void PIOS_GPIO_Off(uint32_t gpios_dev_id, uint8_t gpio_id)
{
    const struct pios_gpio_cfg *gpio_cfg = (const struct pios_gpio_cfg *)gpios_dev_id;

    PIOS_Assert(gpio_cfg);

    if (gpio_id >= gpio_cfg->num_gpios) {
        /* GPIO index out of range */
        return;
    }

    const struct pios_gpio *gpio = &(gpio_cfg->gpios[gpio_id]);

    if (gpio->active_low) {
        gpio->pin.gpio->BSRR = gpio->pin.init.GPIO_Pin;
    } else {
        gpio->pin.gpio->BRR = gpio->pin.init.GPIO_Pin;
    }
}

/**
 * Toggle GPIO on/off
 * \param[in] GPIO GPIO id
 */
void PIOS_GPIO_Toggle(uint32_t gpios_dev_id, uint8_t gpio_id)
{
    const struct pios_gpio_cfg *gpio_cfg = (const struct pios_gpio_cfg *)gpios_dev_id;

    PIOS_Assert(gpio_cfg);

    if (gpio_id >= gpio_cfg->num_gpios) {
        /* GPIO index out of range */
        return;
    }

    const struct pios_gpio *gpio = &(gpio_cfg->gpios[gpio_id]);

    if (((gpio->pin.gpio->ODR & gpio->pin.init.GPIO_Pin) != 0) ^ gpio->active_low) {
        PIOS_GPIO_Off(gpios_dev_id, gpio_id);
    } else {
        PIOS_GPIO_On(gpios_dev_id, gpio_id);
    }
}

#endif /* PIOS_INCLUDE_GPIO */

/**
 * @}
 * @}
 */
