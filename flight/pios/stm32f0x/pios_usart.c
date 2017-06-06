/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USART USART Functions
 * @brief PIOS interface for USART port
 * @{
 *
 * @file       pios_usart.c
 * @author     The LibrePilot Project, http://www.librepilot.org, Copyright (c) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      USART commands. Inits USARTs, controls USARTs & Interupt handlers. (STM32 dependent)
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

#ifdef PIOS_INCLUDE_USART

#include <pios_usart_priv.h>

#define PIOS_UART_TX_BUFFER 10
/* Provide a COM driver */
static void PIOS_USART_ChangeBaud(uint32_t usart_id, uint32_t baud);
#ifndef BOOTLOADER
static void PIOS_USART_ChangeConfig(uint32_t usart_id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate);
#endif
static void PIOS_USART_RegisterRxCallback(uint32_t usart_id, pios_com_callback rx_in_cb, uint32_t context);
static void PIOS_USART_RegisterTxCallback(uint32_t usart_id, pios_com_callback tx_out_cb, uint32_t context);
static void PIOS_USART_TxStart(uint32_t usart_id, uint16_t tx_bytes_avail);
static void PIOS_USART_RxStart(uint32_t usart_id, uint16_t rx_bytes_avail);

const struct pios_com_driver pios_usart_com_driver = {
    .set_baud   = PIOS_USART_ChangeBaud,
#ifndef BOOTLOADER
    .set_config = PIOS_USART_ChangeConfig,
#endif
    .tx_start   = PIOS_USART_TxStart,
    .rx_start   = PIOS_USART_RxStart,
    .bind_tx_cb = PIOS_USART_RegisterTxCallback,
    .bind_rx_cb = PIOS_USART_RegisterRxCallback,
};

enum pios_usart_dev_magic {
    PIOS_USART_DEV_MAGIC = 0x11223344,
};

struct pios_usart_dev {
    enum pios_usart_dev_magic   magic;
    const struct pios_usart_cfg *cfg;
    USART_InitTypeDef init;
    pios_com_callback rx_in_cb;
    uint32_t rx_in_context;
    pios_com_callback tx_out_cb;
    uint32_t tx_out_context;

    uint32_t rx_dropped;

    uint8_t  tx_buffer[PIOS_UART_TX_BUFFER];
    uint8_t  tx_len;
    uint8_t  tx_pos;
    uint8_t  irq_channel;
};

static struct pios_usart_dev *PIOS_USART_validate(uint32_t usart_id)
{
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = (usart_dev->magic == PIOS_USART_DEV_MAGIC);

    PIOS_Assert(valid);
    return usart_dev;
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_usart_dev *PIOS_USART_alloc(void)
{
    struct pios_usart_dev *usart_dev;

    usart_dev = (struct pios_usart_dev *)pvPortMalloc(sizeof(struct pios_usart_dev));
    if (!usart_dev) {
        return NULL;
    }

    memset(usart_dev, 0, sizeof(struct pios_usart_dev));
    usart_dev->magic = PIOS_USART_DEV_MAGIC;
    return usart_dev;
}
#else
static struct pios_usart_dev pios_usart_devs[PIOS_USART_MAX_DEVS];
static uint8_t pios_usart_num_devs;
static struct pios_usart_dev *PIOS_USART_alloc(void)
{
    struct pios_usart_dev *usart_dev;

    if (pios_usart_num_devs >= PIOS_USART_MAX_DEVS) {
        return NULL;
    }

    usart_dev = &pios_usart_devs[pios_usart_num_devs++];

    memset(usart_dev, 0, sizeof(struct pios_usart_dev));
    usart_dev->magic = PIOS_USART_DEV_MAGIC;

    return usart_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */

/* Bind Interrupt Handlers
 *
 * Map all valid USART IRQs to the common interrupt handler
 * and provide storage for a 32-bit device id IRQ to map
 * each physical IRQ to a specific registered device instance.
 */
static void PIOS_USART_generic_irq_handler(uint32_t usart_id);

static uint32_t PIOS_USART_1_id;
void USART1_IRQHandler(void) __attribute__((alias("PIOS_USART_1_irq_handler")));
static void PIOS_USART_1_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_1_id);
}

static uint32_t PIOS_USART_2_id;
void USART2_IRQHandler(void) __attribute__((alias("PIOS_USART_2_irq_handler")));
static void PIOS_USART_2_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_2_id);
}
#if defined(STM32F072)
static uint32_t PIOS_USART_3_id;
void USART3_IRQHandler(void) __attribute__((alias("PIOS_USART_3_irq_handler")));
static void PIOS_USART_3_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_3_id);
}
#endif

static int32_t PIOS_USART_SetIrqPrio(struct pios_usart_dev *usart_dev, uint8_t irq_prio)
{
    NVIC_InitTypeDef init = {
        .NVIC_IRQChannel    = usart_dev->irq_channel,
        .NVIC_IRQChannelPriority = irq_prio,
        .NVIC_IRQChannelCmd = ENABLE,
    };

    NVIC_Init(&init);

    return 0;
}

/**
 * Initialise a single USART device
 */
int32_t PIOS_USART_Init(uint32_t *usart_id, const struct pios_usart_cfg *cfg)
{
    PIOS_DEBUG_Assert(usart_id);
    PIOS_DEBUG_Assert(cfg);

    uint32_t *local_id;
    uint8_t irq_channel;

    /* Enable USART clock */
    switch ((uint32_t)cfg->regs) {
    case (uint32_t)USART1:
        local_id    = &PIOS_USART_1_id;
        irq_channel = USART1_IRQn;
        break;
    case (uint32_t)USART2:
        local_id    = &PIOS_USART_2_id;
        irq_channel = USART2_IRQn;
        break;
#if defined(STM32F072)
    case (uint32_t)USART3:
        local_id    = &PIOS_USART_3_id;
        irq_channel = USART3_4_IRQn;
        break;
#endif
    default:
        goto out_fail;
    }

    struct pios_usart_dev *usart_dev;

    usart_dev = (struct pios_usart_dev *)PIOS_USART_alloc();
    if (!usart_dev) {
        return -1;
    }

    /* Bind the configuration to the device instance */
    usart_dev->cfg = cfg;
    usart_dev->irq_channel = irq_channel;

    /* Initialize the comm parameter structure */
    USART_StructInit(&usart_dev->init); // 9600 8n1

    /* We will set modes later, depending on installed callbacks */
    usart_dev->init.USART_Mode = 0;

    *usart_id = (uint32_t)usart_dev;
    *local_id = (uint32_t)usart_dev;

    PIOS_USART_SetIrqPrio(usart_dev, PIOS_IRQ_PRIO_MID);

    /* Disable overrun detection */
    USART_OverrunDetectionConfig(usart_dev->cfg->regs, USART_OVRDetection_Disable);

    return 0;

out_fail:
    return -1;
}

static void PIOS_USART_Setup(struct pios_usart_dev *usart_dev)
{
    /* Configure RX GPIO */
    if ((usart_dev->init.USART_Mode & USART_Mode_Rx) && (usart_dev->cfg->rx.gpio)) {
        if (usart_dev->cfg->remap) {
            GPIO_PinAFConfig(usart_dev->cfg->rx.gpio,
                             __builtin_ctz(usart_dev->cfg->rx.init.GPIO_Pin),
                             usart_dev->cfg->remap);
        }

        GPIO_Init(usart_dev->cfg->rx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->rx.init);

        USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
    }

    /* Configure TX GPIO */
    if ((usart_dev->init.USART_Mode & USART_Mode_Tx) && usart_dev->cfg->tx.gpio) {
        if (usart_dev->cfg->remap) {
            GPIO_PinAFConfig(usart_dev->cfg->tx.gpio,
                             __builtin_ctz(usart_dev->cfg->tx.init.GPIO_Pin),
                             usart_dev->cfg->remap);
        }

        GPIO_Init(usart_dev->cfg->tx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->tx.init);
    }

    /* Write new configuration */
    { // fix parity stuff
        USART_InitTypeDef init = usart_dev->init;

        if ((init.USART_Parity != USART_Parity_No) && (init.USART_WordLength == USART_WordLength_8b)) {
            init.USART_WordLength = USART_WordLength_9b;
        }

        USART_Init(usart_dev->cfg->regs, &init);
    }

    /*
     * Re enable USART.
     */
    USART_Cmd(usart_dev->cfg->regs, ENABLE);
}

static void PIOS_USART_RxStart(uint32_t usart_id, __attribute__((unused)) uint16_t rx_bytes_avail)
{
    const struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
}
static void PIOS_USART_TxStart(uint32_t usart_id, __attribute__((unused)) uint16_t tx_bytes_avail)
{
    const struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, ENABLE);
}

/**
 * Changes the baud rate of the USART peripheral without re-initialising.
 * \param[in] usart_id USART name (GPS, TELEM, AUX)
 * \param[in] baud Requested baud rate
 */
static void PIOS_USART_ChangeBaud(uint32_t usart_id, uint32_t baud)
{
    struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    /* Use our working copy of the usart init structure */
    usart_dev->init.USART_BaudRate = baud;

    PIOS_USART_Setup(usart_dev);
}
#ifndef BOOTLOADER
/**
 * Changes configuration of the USART peripheral without re-initialising.
 * \param[in] usart_id USART name (GPS, TELEM, AUX)
 * \param[in] word_len Requested word length
 * \param[in] stop_bits Requested stop bits
 * \param[in] parity Requested parity
 *
 */
static void PIOS_USART_ChangeConfig(uint32_t usart_id,
                                    enum PIOS_COM_Word_Length word_len,
                                    enum PIOS_COM_Parity parity,
                                    enum PIOS_COM_StopBits stop_bits,
                                    uint32_t baud_rate)
{
    struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    switch (word_len) {
    case PIOS_COM_Word_length_8b:
        usart_dev->init.USART_WordLength = USART_WordLength_8b;
        break;
    case PIOS_COM_Word_length_9b:
        usart_dev->init.USART_WordLength = USART_WordLength_9b;
        break;
    default:
        break;
    }

    switch (stop_bits) {
    case PIOS_COM_StopBits_1:
        usart_dev->init.USART_StopBits = USART_StopBits_1;
        break;
    case PIOS_COM_StopBits_1_5:
        usart_dev->init.USART_StopBits = USART_StopBits_1_5;
        break;
    case PIOS_COM_StopBits_2:
        usart_dev->init.USART_StopBits = USART_StopBits_2;
        break;
    default:
        break;
    }

    switch (parity) {
    case PIOS_COM_Parity_No:
        usart_dev->init.USART_Parity = USART_Parity_No;
        break;
    case PIOS_COM_Parity_Even:
        usart_dev->init.USART_Parity = USART_Parity_Even;
        break;
    case PIOS_COM_Parity_Odd:
        usart_dev->init.USART_Parity = USART_Parity_Odd;
        break;
    default:
        break;
    }

    if (baud_rate) {
        usart_dev->init.USART_BaudRate = baud_rate;
    }

    PIOS_USART_Setup(usart_dev);
}
#endif /* BOOTLOADER */

static void PIOS_USART_RegisterRxCallback(uint32_t usart_id, pios_com_callback rx_in_cb, uint32_t context)
{
    struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    usart_dev->rx_in_context    = context;
    usart_dev->rx_in_cb = rx_in_cb;

    usart_dev->init.USART_Mode |= USART_Mode_Rx;

    PIOS_USART_Setup(usart_dev);
}

static void PIOS_USART_RegisterTxCallback(uint32_t usart_id, pios_com_callback tx_out_cb, uint32_t context)
{
    struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    usart_dev->tx_out_context   = context;
    usart_dev->tx_out_cb = tx_out_cb;

    usart_dev->init.USART_Mode |= USART_Mode_Tx;

    PIOS_USART_Setup(usart_dev);
}

static void PIOS_USART_generic_irq_handler(uint32_t usart_id)
{
    struct pios_usart_dev *usart_dev = PIOS_USART_validate(usart_id);

    /* Force read of dr after sr to make sure to clear error flags */


    /* Check if RXNE flag is set */
    bool rx_need_yield = false;

    if (usart_dev->cfg->regs->ISR & USART_ISR_RXNE) {
        uint8_t byte = usart_dev->cfg->regs->RDR & 0xFF;
        if (usart_dev->rx_in_cb) {
            uint16_t rc;
            rc = (usart_dev->rx_in_cb)(usart_dev->rx_in_context, &byte, 1, NULL, &rx_need_yield);
            if (rc < 1) {
                /* Lost bytes on rx */
                usart_dev->rx_dropped += 1;
            }
        }
    }

    /* Check if TXE flag is set */
    bool tx_need_yield = false;
    if (usart_dev->cfg->regs->ISR & USART_ISR_TXE) {
        if (usart_dev->tx_out_cb) {
            if (!usart_dev->tx_len) {
                usart_dev->tx_len = (usart_dev->tx_out_cb)(usart_dev->tx_out_context, usart_dev->tx_buffer, PIOS_UART_TX_BUFFER, NULL, &tx_need_yield);
                usart_dev->tx_pos = 0;
            }
            if (usart_dev->tx_len > 0) {
                /* Send the byte we've been given */
                usart_dev->cfg->regs->TDR = usart_dev->tx_buffer[usart_dev->tx_pos++];
                usart_dev->tx_len--;
            } else {
                /* No bytes to send, disable TXE interrupt */
                usart_dev->cfg->regs->CR1 &= ~(USART_CR1_TXEIE);
            }
        } else {
            /* No bytes to send, disable TXE interrupt */
            usart_dev->cfg->regs->CR1 &= ~(USART_CR1_TXEIE);
        }
    }
    usart_dev->cfg->regs->ICR = USART_ICR_ORECF | USART_ICR_TCCF;
#if defined(PIOS_INCLUDE_FREERTOS)
    if (rx_need_yield || tx_need_yield) {
        vPortYield();
    }
#endif /* PIOS_INCLUDE_FREERTOS */
}

#endif /* PIOS_INCLUDE_USART */

/**
 * @}
 * @}
 */
