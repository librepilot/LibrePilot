/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USB USB Setup Functions
 * @brief PIOS USB device implementation
 * @{
 *
 * @file       pios_usb.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      USB device functions (STM32 dependent code)
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

#ifdef PIOS_INCLUDE_USB

#include "usb_core.h"
#include "pios_usb_board_data.h"
#include <pios_usb_priv.h>
#include <pios_helpers.h>

/* Rx/Tx status */
static volatile uint8_t transfer_possible = 0;

#ifdef PIOS_INCLUDE_FREERTOS
static void(*disconnection_cb_list[3]) (void);

struct {
    void     (*callback)(bool connected, uint32_t context);
    uint32_t context;
} connectionState_cb_list[3];
#endif

enum pios_usb_dev_magic {
    PIOS_USB_DEV_MAGIC = 0x17365904,
};

struct pios_usb_dev {
    enum pios_usb_dev_magic   magic;
    const struct pios_usb_cfg *cfg;
#ifdef PIOS_INCLUDE_FREERTOS
    xSemaphoreHandle statusCheckSemaphore;
#endif
};
#ifdef PIOS_INCLUDE_FREERTOS
static void raiseDisconnectionCallbacks(void);
static void raiseConnectionStateCallback(bool connected);
#endif
/**
 * @brief Validate the usb device structure
 * @returns true if valid device or false otherwise
 */
static bool PIOS_USB_validate(struct pios_usb_dev *usb_dev)
{
    return usb_dev && (usb_dev->magic == PIOS_USB_DEV_MAGIC);
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_usb_dev *PIOS_USB_alloc(void)
{
    struct pios_usb_dev *usb_dev;

    usb_dev = (struct pios_usb_dev *)pios_malloc(sizeof(*usb_dev));
    if (!usb_dev) {
        return NULL;
    }
    vSemaphoreCreateBinary(usb_dev->statusCheckSemaphore);
    xSemaphoreGive(usb_dev->statusCheckSemaphore);
    usb_dev->magic = PIOS_USB_DEV_MAGIC;
    return usb_dev;
}
#else
static struct pios_usb_dev pios_usb_devs[PIOS_USB_MAX_DEVS];
static uint8_t pios_usb_num_devs;
static struct pios_usb_dev *PIOS_USB_alloc(void)
{
    struct pios_usb_dev *usb_dev;

    if (pios_usb_num_devs >= PIOS_USB_MAX_DEVS) {
        return NULL;
    }

    usb_dev = &pios_usb_devs[pios_usb_num_devs++];
    usb_dev->magic = PIOS_USB_DEV_MAGIC;

    return usb_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */


/**
 * Bind configuration to USB BSP layer
 * \return < 0 if initialisation failed
 */
static uint32_t pios_usb_id;
int32_t PIOS_USB_Init(uint32_t *usb_id, const struct pios_usb_cfg *cfg)
{
    PIOS_Assert(usb_id);
    PIOS_Assert(cfg);

    struct pios_usb_dev *usb_dev;

    usb_dev = (struct pios_usb_dev *)PIOS_USB_alloc();
    if (!usb_dev) {
        goto out_fail;
    }

    /* Bind the configuration to the device instance */
    usb_dev->cfg = cfg;

    /*
     * This is a horrible hack to make this available to
     * the interrupt callbacks.  This should go away ASAP.
     */
    pios_usb_id  = (uint32_t)usb_dev;

    *usb_id = (uint32_t)usb_dev;

    return 0; /* No error */

out_fail:
    return -1;
}

/**
 * This function is called by the USB driver on cable connection/disconnection
 * \param[in] connected connection status (1 if connected)
 * \return < 0 on errors
 * \note Applications shouldn't call this function directly, instead please use \ref PIOS_COM layer functions
 */
int32_t PIOS_USB_ChangeConnectionState(bool connected)
{
#ifdef PIOS_INCLUDE_FREERTOS
    static volatile uint8_t lastStatus = 2; // 2 is "no last status"
#endif
    // In all cases: re-initialise USB HID driver
    if (connected) {
        transfer_possible = 1;

        // TODO: Check SetEPRxValid(ENDP1);

#if defined(USB_LED_ON)
        USB_LED_ON; // turn the USB led on
#endif
    } else {
        // Cable disconnected: disable transfers
        transfer_possible = 0;

#if defined(USB_LED_OFF)
        USB_LED_OFF; // turn the USB led off
#endif
    }

#ifdef PIOS_INCLUDE_FREERTOS
    raiseConnectionStateCallback(connected);
    if (lastStatus != transfer_possible) {
        if (lastStatus == 1) {
            raiseDisconnectionCallbacks();
        }
        lastStatus = transfer_possible;
    }
#endif

    return 0;
}

/**
 * This function returns the connection status of the USB interface
 * \return 1: interface available
 * \return 0: interface not available
 */
uint32_t usb_found;
bool PIOS_USB_CheckAvailable(__attribute__((unused)) uint32_t id)
{
    struct pios_usb_dev *usb_dev = (struct pios_usb_dev *)pios_usb_id;

    if (!PIOS_USB_validate(usb_dev)) {
        return false;
    }

    return transfer_possible;
}

/**
 * This function returns wether a USB cable (5V pin) has been detected
 * \return true: cable connected
 * \return false: cable not detected (no cable or cable with no power)
 */
bool PIOS_USB_CableConnected(__attribute__((unused)) uint8_t id)
{
    struct pios_usb_dev *usb_dev = (struct pios_usb_dev *)pios_usb_id;

    return ((usb_dev->cfg->vsense.gpio->IDR & usb_dev->cfg->vsense.init.GPIO_Pin) != 0) ^ usb_dev->cfg->vsense_active_low;
}

/*
 *
 * Register a physical disconnection callback
 *
 */
#ifdef PIOS_INCLUDE_FREERTOS
void PIOS_USB_RegisterDisconnectionCallback(void (*disconnectionCB)(void))
{
    PIOS_Assert(disconnectionCB);
    for (uint32_t i = 0; i < NELEMENTS(disconnection_cb_list); i++) {
        if (disconnection_cb_list[i] == NULL) {
            disconnection_cb_list[i] = disconnectionCB;
            return;
        }
    }
    PIOS_Assert(0);
}
static void raiseDisconnectionCallbacks(void)
{
    uint32_t i = 0;

    while (i < NELEMENTS(disconnection_cb_list) && disconnection_cb_list[i] != NULL) {
        (disconnection_cb_list[i++])();
    }
}

void PIOS_USB_RegisterConnectionStateCallback(void (*connectionStateCallback)(bool connected, uint32_t context), uint32_t context)
{
    PIOS_Assert(connectionStateCallback);

    for (uint32_t i = 0; i < NELEMENTS(connectionState_cb_list); i++) {
        if (connectionState_cb_list[i].callback == NULL) {
            connectionState_cb_list[i].callback = connectionStateCallback;
            connectionState_cb_list[i].context  = context;
            return;
        }
    }

    PIOS_Assert(0);
}

static void raiseConnectionStateCallback(bool connected)
{
    uint32_t i = 0;

    while (i < NELEMENTS(connectionState_cb_list) && connectionState_cb_list[i].callback != NULL) {
        connectionState_cb_list[i].callback(connected, connectionState_cb_list[i].context);
        i++;
    }
}
#else /* PIOS_INCLUDE_FREERTOS */
void PIOS_USB_RegisterDisconnectionCallback(__attribute__((unused)) void (*disconnectionCB)(void))
{}
void PIOS_USB_RegisterConnectionStateCallback(__attribute__((unused)) void (*connectionStateCallback)(bool connected, uint32_t context), __attribute__((unused)) uint32_t context)
{}
#endif /* PIOS_INCLUDE_FREERTOS */

/*
 *
 * Provide STM32 USB OTG BSP layer API
 *
 */

#include "usb_bsp.h"

void USB_OTG_BSP_Init(__attribute__((unused)) USB_OTG_CORE_HANDLE *pdev)
{
    struct pios_usb_dev *usb_dev = (struct pios_usb_dev *)pios_usb_id;

    bool valid = PIOS_USB_validate(usb_dev);

    PIOS_Assert(valid);

#define FORCE_DISABLE_USB_IRQ 1
#if FORCE_DISABLE_USB_IRQ
    /* Make sure we disable the USB interrupt since it may be left on by bootloader */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure = usb_dev->cfg->irq.init;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif

    /* Configure USB D-/D+ (DM/DP) pins */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);

    /* Configure VBUS sense pin */
    GPIO_Init(usb_dev->cfg->vsense.gpio, &usb_dev->cfg->vsense.init);

    /* Enable USB OTG Clock */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
}

void USB_OTG_BSP_EnableInterrupt(__attribute__((unused)) USB_OTG_CORE_HANDLE *pdev)
{
    struct pios_usb_dev *usb_dev = (struct pios_usb_dev *)pios_usb_id;

    bool valid = PIOS_USB_validate(usb_dev);

    PIOS_Assert(valid);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_Init(&usb_dev->cfg->irq.init);
}

#ifdef USE_HOST_MODE
void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev, uint8_t state)
{}

void USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev)
{}
#endif /* USE_HOST_MODE */

void USB_OTG_BSP_TimeInit(void)
{}

void USB_OTG_BSP_uDelay(const uint32_t usec)
{
    uint32_t count = 0;
    const uint32_t utime = (120 * usec / 7);

    do {
        if (++count > utime) {
            return;
        }
    } while (1);
}

void USB_OTG_BSP_mDelay(const uint32_t msec)
{
    USB_OTG_BSP_uDelay(msec * 1000);
}

void USB_OTG_BSP_TimerIRQ(void)
{}

#endif /* PIOS_INCLUDE_USB */

/**
 * @}
 * @}
 */
