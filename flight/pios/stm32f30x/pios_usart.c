/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USART USART Functions
 * @brief PIOS interface for USART port
 * @{
 *
 * @file       pios_usart.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

/* Provide a COM driver */
static void PIOS_USART_ChangeBaud(uint32_t usart_id, uint32_t baud);
static void PIOS_USART_ChangeConfig(uint32_t usart_id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate);
static void PIOS_USART_RegisterRxCallback(uint32_t usart_id, pios_com_callback rx_in_cb, uint32_t context);
static void PIOS_USART_RegisterTxCallback(uint32_t usart_id, pios_com_callback tx_out_cb, uint32_t context);
static void PIOS_USART_TxStart(uint32_t usart_id, uint16_t tx_bytes_avail);
static void PIOS_USART_RxStart(uint32_t usart_id, uint16_t rx_bytes_avail);
static int32_t PIOS_USART_Ioctl(uint32_t usart_id, uint32_t ctl, void *param);

const struct pios_com_driver pios_usart_com_driver = {
    .set_baud   = PIOS_USART_ChangeBaud,
    .set_config = PIOS_USART_ChangeConfig,
    .tx_start   = PIOS_USART_TxStart,
    .rx_start   = PIOS_USART_RxStart,
    .bind_tx_cb = PIOS_USART_RegisterTxCallback,
    .bind_rx_cb = PIOS_USART_RegisterRxCallback,
    .ioctl      = PIOS_USART_Ioctl,
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
    bool     config_locked;
    uint32_t rx_dropped;
    uint8_t  irq_channel;
};

static bool PIOS_USART_validate(struct pios_usart_dev *usart_dev)
{
    return usart_dev->magic == PIOS_USART_DEV_MAGIC;
}

static int32_t PIOS_USART_SetIrqPrio(struct pios_usart_dev *usart_dev, uint8_t irq_prio)
{
    NVIC_InitTypeDef init = {
        .NVIC_IRQChannel    = usart_dev->irq_channel,
        .NVIC_IRQChannelPreemptionPriority = irq_prio,
        .NVIC_IRQChannelCmd = ENABLE,
    };

    NVIC_Init(&init);

    return 0;
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_usart_dev *PIOS_USART_alloc(void)
{
    struct pios_usart_dev *usart_dev;

    usart_dev = (struct pios_usart_dev *)pios_malloc(sizeof(struct pios_usart_dev));
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
void USART1_EXTI25_IRQHandler(void) __attribute__((alias("PIOS_USART_1_irq_handler")));
static void PIOS_USART_1_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_1_id);
}

static uint32_t PIOS_USART_2_id;
void USART2_EXTI26_IRQHandler(void) __attribute__((alias("PIOS_USART_2_irq_handler")));
static void PIOS_USART_2_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_2_id);
}

static uint32_t PIOS_USART_3_id;
void USART3_EXTI28_IRQHandler(void) __attribute__((alias("PIOS_USART_3_irq_handler")));
static void PIOS_USART_3_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_USART_3_id);
}


static uint32_t PIOS_UART_4_id;
void UART4_EXTI34_IRQHandler(void) __attribute__((alias("PIOS_UART_4_irq_handler")));
static void PIOS_UART_4_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_UART_4_id);
}

static uint32_t PIOS_UART_5_id;
void UART5_EXTI35_IRQHandler(void) __attribute__((alias("PIOS_UART_5_irq_handler")));
static void PIOS_UART_5_irq_handler(void)
{
    PIOS_USART_generic_irq_handler(PIOS_UART_5_id);
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

    switch ((uint32_t)cfg->regs) {
    case (uint32_t)USART1:
        local_id    = &PIOS_USART_1_id;
        irq_channel = USART1_IRQn;
        break;
    case (uint32_t)USART2:
        local_id    = &PIOS_USART_2_id;
        irq_channel = USART2_IRQn;
        break;
    case (uint32_t)USART3:
        local_id    = &PIOS_USART_3_id;
        irq_channel = USART3_IRQn;
        break;
    case (uint32_t)UART4:
        local_id    = &PIOS_UART_4_id;
        irq_channel = UART4_IRQn;
        break;
    case (uint32_t)UART5:
        local_id    = &PIOS_UART_5_id;
        irq_channel = UART5_IRQn;
        break;
    default:
        goto out_fail;
    }

    if (*local_id) {
        /* this port is already open */
        *usart_id = *local_id;
        return 0;
    }

    struct pios_usart_dev *usart_dev;

    usart_dev = (struct pios_usart_dev *)PIOS_USART_alloc();
    if (!usart_dev) {
        goto out_fail;
    }

    /* Bind the configuration to the device instance */
    usart_dev->cfg = cfg;
    usart_dev->irq_channel = irq_channel;

    /* Set some sane defaults */
    USART_StructInit(&usart_dev->init);

    /* We will set modes later, depending on installed callbacks */
    usart_dev->init.USART_Mode = 0;

    /* DTR handling? */

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
                             usart_dev->cfg->rx.pin_source,
                             usart_dev->cfg->remap);
        }

        GPIO_Init(usart_dev->cfg->rx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->rx.init);

        USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
    }

    /* Configure TX GPIO */
    if ((usart_dev->init.USART_Mode & USART_Mode_Tx) && usart_dev->cfg->tx.gpio) {
        if (usart_dev->cfg->remap) {
            GPIO_PinAFConfig(usart_dev->cfg->tx.gpio,
                             usart_dev->cfg->tx.pin_source,
                             usart_dev->cfg->remap);
        }

        GPIO_Init(usart_dev->cfg->tx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->tx.init);
    }

    /* Write new configuration */
#if 0
    const char *dbg_parity = "?";
    switch (usart_dev->init.USART_Parity) {
    case USART_Parity_No: dbg_parity   = "N"; break;
    case USART_Parity_Even: dbg_parity = "E"; break;
    case USART_Parity_Odd: dbg_parity  = "O"; break;
    }
    const char *dbg_mode = "???";
    switch (usart_dev->init.USART_Mode) {
    case USART_Mode_Rx: dbg_mode = "rx"; break;
    case USART_Mode_Tx: dbg_mode = "tx"; break;
    case USART_Mode_Rx | USART_Mode_Tx: dbg_mode = "rx_tx"; break;
    }
    const char *dbg_flow_control = "???";
    switch (usart_dev->init.USART_HardwareFlowControl) {
    case USART_HardwareFlowControl_None: dbg_flow_control    = "none"; break;
    case USART_HardwareFlowControl_RTS: dbg_flow_control     = "rts"; break;
    case USART_HardwareFlowControl_CTS: dbg_flow_control     = "cts"; break;
    case USART_HardwareFlowControl_RTS_CTS: dbg_flow_control = "rts_cts"; break;
    }
    const char *dbg_stop_bits = "???";
    switch (usart_dev->init.USART_StopBits) {
    case USART_StopBits_1: dbg_stop_bits   = "1"; break;
    case USART_StopBits_2: dbg_stop_bits   = "2"; break;
    case USART_StopBits_1_5: dbg_stop_bits = "1.5"; break;
    }

    DEBUG_PRINTF(0, "PIOS_USART_Setup: 0x%08x %u %u%s%s mode=%s flow_control=%s\r\n",
                 (uint32_t)usart_dev,
                 usart_dev->init.USART_BaudRate,
                 usart_dev->init.USART_WordLength == USART_WordLength_8b ? 8 : 9,
                 dbg_parity,
                 dbg_stop_bits,
                 dbg_mode,
                 dbg_flow_control);
#endif /* if 0 */
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
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
}
static void PIOS_USART_TxStart(uint32_t usart_id, __attribute__((unused)) uint16_t tx_bytes_avail)
{
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, ENABLE);
}

/**
 * Changes the baud rate of the USART peripheral without re-initialising.
 * \param[in] usart_id USART name (GPS, TELEM, AUX)
 * \param[in] baud Requested baud rate
 */
static void PIOS_USART_ChangeBaud(uint32_t usart_id, uint32_t baud)
{
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    if (usart_dev->config_locked) {
        return;
    }

    usart_dev->init.USART_BaudRate = baud;

    PIOS_USART_Setup(usart_dev);
}

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
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    if (usart_dev->config_locked) {
        return;
    }

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


static void PIOS_USART_RegisterRxCallback(uint32_t usart_id, pios_com_callback rx_in_cb, uint32_t context)
{
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

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
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

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
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    /* Force read of dr after sr to make sure to clear error flags */
    volatile uint16_t sr = usart_dev->cfg->regs->ISR;
    volatile uint8_t dr  = usart_dev->cfg->regs->RDR;

    /* Check if RXNE flag is set */
    bool rx_need_yield   = false;
    if (sr & USART_ISR_RXNE) {
        uint8_t byte = dr;
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
    if (sr & USART_ISR_TXE) {
        if (usart_dev->tx_out_cb) {
            uint8_t b;
            uint16_t bytes_to_send;

            bytes_to_send = (usart_dev->tx_out_cb)(usart_dev->tx_out_context, &b, 1, NULL, &tx_need_yield);

            if (bytes_to_send > 0) {
                /* Send the byte we've been given */
                usart_dev->cfg->regs->TDR = b;
            } else {
                /* No bytes to send, disable TXE interrupt */
                USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
            }
        } else {
            /* No bytes to send, disable TXE interrupt */
            USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
        }
    }

#if defined(PIOS_INCLUDE_FREERTOS)
    if (rx_need_yield || tx_need_yield) {
        vPortYield();
    }
#endif /* PIOS_INCLUDE_FREERTOS */
}

static int32_t PIOS_USART_Ioctl(uint32_t usart_id, uint32_t ctl, void *param)
{
    struct pios_usart_dev *usart_dev = (struct pios_usart_dev *)usart_id;

    bool valid = PIOS_USART_validate(usart_dev);

    PIOS_Assert(valid);

    /* some sort of locking would be great to have */

    uint32_t cr1_ue = usart_dev->cfg->regs->CR1 & USART_CR1_UE;

    switch (ctl) {
    case PIOS_IOCTL_USART_SET_IRQ_PRIO:
        return PIOS_USART_SetIrqPrio(usart_dev, *(uint8_t *)param);

    case PIOS_IOCTL_USART_SET_INVERTED:
    {
        enum PIOS_USART_Inverted inverted = *(enum PIOS_USART_Inverted *)param;

        usart_dev->cfg->regs->CR1 &= ~((uint32_t)USART_CR1_UE);

        if (usart_dev->cfg->rx.gpio != 0) {
            GPIO_InitTypeDef gpioInit = usart_dev->cfg->rx.init;

            gpioInit.GPIO_PuPd = (inverted & PIOS_USART_Inverted_Rx) ? GPIO_PuPd_DOWN : GPIO_PuPd_UP;

            GPIO_Init(usart_dev->cfg->rx.gpio, &gpioInit);

            USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Rx, (inverted & PIOS_USART_Inverted_Rx) ? ENABLE : DISABLE);
        }

        if (usart_dev->cfg->tx.gpio != 0) {
            USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Tx, (inverted & PIOS_USART_Inverted_Tx) ? ENABLE : DISABLE);
        }
    }
    break;
    case PIOS_IOCTL_USART_SET_SWAPPIN:
        usart_dev->cfg->regs->CR1 &= ~((uint32_t)USART_CR1_UE);
        USART_SWAPPinCmd(usart_dev->cfg->regs, *(bool *)param ? ENABLE : DISABLE);
        break;
    case PIOS_IOCTL_USART_SET_HALFDUPLEX:
        usart_dev->cfg->regs->CR1 &= ~((uint32_t)USART_CR1_UE);
        USART_HalfDuplexCmd(usart_dev->cfg->regs, *(bool *)param ? ENABLE : DISABLE);
        break;
    case PIOS_IOCTL_USART_GET_DSMBIND:
    case PIOS_IOCTL_USART_GET_RXGPIO:
        *(struct stm32_gpio *)param = usart_dev->cfg->rx;
        break;
    case PIOS_IOCTL_USART_GET_TXGPIO:
        *(struct stm32_gpio *)param = usart_dev->cfg->tx;
        break;
    case PIOS_IOCTL_USART_LOCK_CONFIG:
        usart_dev->config_locked    = *(bool *)param;
        break;
    default:
        return -1;
    }

    usart_dev->cfg->regs->CR1 |= cr1_ue;

    return 0;
}

#endif /* PIOS_INCLUDE_USART */

/**
 * @}
 * @}
 */
