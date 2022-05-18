/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_COM COM layer functions
 * @brief Hardware communication layer
 * @{
 *
 * @file       pios_com.c
 * @author     The LibrePilot Project, http://www.librepilot.org, Copyright (c) 2015-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      COM layer functions
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

#ifdef PIOS_INCLUDE_COM

#include "fifo_buffer.h"
#include <pios_com_priv.h>

#ifndef PIOS_INCLUDE_FREERTOS
#include "pios_delay.h" /* PIOS_DELAY_WaitmS */
#endif

enum pios_com_dev_magic {
    PIOS_COM_DEV_MAGIC = 0xaa55aa55,
};

struct pios_com_dev {
    enum pios_com_dev_magic magic;
    uint32_t lower_id;
    const struct pios_com_driver *driver;

#if defined(PIOS_INCLUDE_FREERTOS)
    xSemaphoreHandle tx_sem;
    xSemaphoreHandle rx_sem;
    xSemaphoreHandle sendbuffer_sem;
#endif

    bool has_rx;
    bool has_tx;

    t_fifo_buffer rx;
    t_fifo_buffer tx;
};

static bool PIOS_COM_validate(struct pios_com_dev *com_dev)
{
    return com_dev && (com_dev->magic == PIOS_COM_DEV_MAGIC);
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_com_dev *PIOS_COM_alloc(void)
{
    struct pios_com_dev *com_dev;

    com_dev = (struct pios_com_dev *)pios_fastheapmalloc(sizeof(struct pios_com_dev));
    if (!com_dev) {
        return NULL;
    }

    memset(com_dev, 0, sizeof(struct pios_com_dev));
    com_dev->magic = PIOS_COM_DEV_MAGIC;
    return com_dev;
}
#else
static struct pios_com_dev pios_com_devs[PIOS_COM_MAX_DEVS];
static uint8_t pios_com_num_devs;
static struct pios_com_dev *PIOS_COM_alloc(void)
{
    struct pios_com_dev *com_dev;

    if (pios_com_num_devs >= PIOS_COM_MAX_DEVS) {
        return NULL;
    }

    com_dev = &pios_com_devs[pios_com_num_devs++];

    memset(com_dev, 0, sizeof(struct pios_com_dev));
    com_dev->magic = PIOS_COM_DEV_MAGIC;

    return com_dev;
}
#endif /* if defined(PIOS_INCLUDE_FREERTOS) */

static uint16_t PIOS_COM_TxOutCallback(uint32_t context, uint8_t *buf, uint16_t buf_len, uint16_t *headroom, bool *need_yield);
static uint16_t PIOS_COM_RxInCallback(uint32_t context, uint8_t *buf, uint16_t buf_len, uint16_t *headroom, bool *need_yield);
static void PIOS_COM_UnblockRx(struct pios_com_dev *com_dev, bool *need_yield);
static void PIOS_COM_UnblockTx(struct pios_com_dev *com_dev, bool *need_yield);

/**
 * Initialises COM layer
 * \param[out] handle
 * \param[in] driver
 * \param[in] id
 * \return < 0 if initialisation failed
 */
int32_t PIOS_COM_Init(uint32_t *com_id, const struct pios_com_driver *driver, uint32_t lower_id, uint8_t *rx_buffer, uint16_t rx_buffer_len, uint8_t *tx_buffer, uint16_t tx_buffer_len)
{
    PIOS_Assert(com_id);
    PIOS_Assert(driver);

    if ((rx_buffer_len > 0) && !rx_buffer) {
#if defined(PIOS_INCLUDE_FREERTOS)
        rx_buffer = (uint8_t *)pios_fastheapmalloc(rx_buffer_len);
#endif
        PIOS_Assert(rx_buffer);
    }

    if ((tx_buffer_len > 0) && !tx_buffer) {
#if defined(PIOS_INCLUDE_FREERTOS)
        tx_buffer = (uint8_t *)pios_fastheapmalloc(tx_buffer_len);
#endif
        PIOS_Assert(tx_buffer);
    }

    bool has_rx = (rx_buffer_len > 0);
    bool has_tx = (tx_buffer_len > 0);

    PIOS_Assert(driver->bind_tx_cb || !has_tx);
    PIOS_Assert(driver->bind_rx_cb || !has_rx);

    struct pios_com_dev *com_dev;

    com_dev = (struct pios_com_dev *)PIOS_COM_alloc();
    if (!com_dev) {
        goto out_fail;
    }

    com_dev->driver   = driver;
    com_dev->lower_id = lower_id;

    com_dev->has_rx   = has_rx;
    com_dev->has_tx   = has_tx;

    if (has_rx) {
        fifoBuf_init(&com_dev->rx, rx_buffer, rx_buffer_len);
#if defined(PIOS_INCLUDE_FREERTOS)
        vSemaphoreCreateBinary(com_dev->rx_sem);
#endif /* PIOS_INCLUDE_FREERTOS */
        (com_dev->driver->bind_rx_cb)(lower_id, PIOS_COM_RxInCallback, (uint32_t)com_dev);
        if (com_dev->driver->rx_start) {
            /* Start the receiver */
            (com_dev->driver->rx_start)(com_dev->lower_id,
                                        fifoBuf_getFree(&com_dev->rx));
        }
    }

    if (has_tx) {
        fifoBuf_init(&com_dev->tx, tx_buffer, tx_buffer_len);
#if defined(PIOS_INCLUDE_FREERTOS)
        vSemaphoreCreateBinary(com_dev->tx_sem);
        com_dev->sendbuffer_sem = xSemaphoreCreateMutex();
#endif /* PIOS_INCLUDE_FREERTOS */
        (com_dev->driver->bind_tx_cb)(lower_id, PIOS_COM_TxOutCallback, (uint32_t)com_dev);
    }

    *com_id = (uint32_t)com_dev;
    return 0;

out_fail:
    return -1;
}

#if defined(PIOS_INCLUDE_FREERTOS)
static void PIOS_COM_UnblockRx(struct pios_com_dev *com_dev, bool *need_yield)
{
    static signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(com_dev->rx_sem, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken != pdFALSE) {
        *need_yield = true;
    } else {
        *need_yield = false;
    }
}
#else
static void PIOS_COM_UnblockRx(__attribute__((unused)) struct pios_com_dev *com_dev, bool *need_yield)
{
    *need_yield = false;
}
#endif

#if defined(PIOS_INCLUDE_FREERTOS)
static void PIOS_COM_UnblockTx(struct pios_com_dev *com_dev, bool *need_yield)
{
    static signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(com_dev->tx_sem, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken != pdFALSE) {
        *need_yield = true;
    } else {
        *need_yield = false;
    }
}
#else
static void PIOS_COM_UnblockTx(__attribute__((unused)) struct pios_com_dev *com_dev, bool *need_yield)
{
    *need_yield = false;
}
#endif


static uint16_t PIOS_COM_RxInCallback(uint32_t context, uint8_t *buf, uint16_t buf_len, uint16_t *headroom, bool *need_yield)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)context;

    bool valid = PIOS_COM_validate(com_dev);

    PIOS_Assert(valid);
    PIOS_Assert(com_dev->has_rx);
    uint16_t bytes_into_fifo;
    if (buf_len == 1) {
        bytes_into_fifo = fifoBuf_putByte(&com_dev->rx, buf[0]);
    } else {
        bytes_into_fifo = fifoBuf_putData(&com_dev->rx, buf, buf_len);
    }
    if (bytes_into_fifo > 0) {
        /* Data has been added to the buffer */
        PIOS_COM_UnblockRx(com_dev, need_yield);
    }

    if (headroom) {
        *headroom = fifoBuf_getFree(&com_dev->rx);
    }

    return bytes_into_fifo;
}

static uint16_t PIOS_COM_TxOutCallback(uint32_t context, uint8_t *buf, uint16_t buf_len, uint16_t *headroom, bool *need_yield)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)context;

    bool valid = PIOS_COM_validate(com_dev);

    PIOS_Assert(valid);
    PIOS_Assert(buf);
    PIOS_Assert(buf_len);
    PIOS_Assert(com_dev->has_tx);

    uint16_t bytes_from_fifo = fifoBuf_getData(&com_dev->tx, buf, buf_len);

    if (bytes_from_fifo > 0) {
        /* More space has been made in the buffer */
        PIOS_COM_UnblockTx(com_dev, need_yield);
    }

    if (headroom) {
        *headroom = fifoBuf_getUsed(&com_dev->tx);
    }

    return bytes_from_fifo;
}

/**
 * Change the port speed without re-initializing
 * \param[in] port COM port
 * \param[in] baud Requested baud rate
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_ChangeBaud(uint32_t com_id, uint32_t baud)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->set_baud) {
        com_dev->driver->set_baud(com_dev->lower_id, baud);
    }

    return 0;
}

int32_t PIOS_COM_ChangeConfig(uint32_t com_id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->set_config) {
        com_dev->driver->set_config(com_dev->lower_id, word_len, parity, stop_bits, baud_rate);
    }

    return 0;
}

/**
 * Set control lines associated with the port
 * \param[in] port COM port
 * \param[in] mask Lines to change
 * \param[in] state New state for lines
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_SetCtrlLine(uint32_t com_id, uint32_t mask, uint32_t state)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->set_ctrl_line) {
        com_dev->driver->set_ctrl_line(com_dev->lower_id, mask, state);
    }

    return 0;
}

/**
 * Set control lines associated with the port
 * \param[in] port COM port
 * \param[in] ctrl_line_cb Callback function
 * \param[in] context context to pass to the callback function
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_RegisterCtrlLineCallback(uint32_t com_id, pios_com_callback_ctrl_line ctrl_line_cb, uint32_t context)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->bind_ctrl_line_cb) {
        com_dev->driver->bind_ctrl_line_cb(com_dev->lower_id, ctrl_line_cb, context);
    }

    return 0;
}

/**
 * Set baud rate callback associated with the port
 * \param[in] port COM port
 * \param[in] baud_rate_cb Callback function
 * \param[in] context context to pass to the callback function
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_RegisterBaudRateCallback(uint32_t com_id, pios_com_callback_baud_rate baud_rate_cb, uint32_t context)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->bind_baud_rate_cb) {
        com_dev->driver->bind_baud_rate_cb(com_dev->lower_id, baud_rate_cb, context);
    }

    return 0;
}

int32_t PIOS_COM_ASYNC_TxStart(uint32_t com_id, uint16_t tx_bytes_avail)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->tx_start) {
        com_dev->driver->tx_start(com_dev->lower_id, tx_bytes_avail);
    }

    return 0;
}

int32_t PIOS_COM_ASYNC_RxStart(uint32_t com_id, uint16_t rx_bytes_avail)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->rx_start) {
        com_dev->driver->rx_start(com_dev->lower_id, rx_bytes_avail);
    }

    return 0;
}

int32_t PIOS_COM_ASYNC_RegisterRxCallback(uint32_t com_id, pios_com_callback rx_in_cb, uint32_t context)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->bind_rx_cb) {
        com_dev->driver->bind_rx_cb(com_dev->lower_id, rx_in_cb, context);
    }

    return 0;
}

int32_t PIOS_COM_ASYNC_RegisterTxCallback(uint32_t com_id, pios_com_callback tx_out_cb, uint32_t context)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->bind_tx_cb) {
        com_dev->driver->bind_tx_cb(com_dev->lower_id, tx_out_cb, context);
    }

    return 0;
}

static int32_t PIOS_COM_SendBufferNonBlockingInternal(struct pios_com_dev *com_dev, const uint8_t *buffer, uint16_t len)
{
    PIOS_Assert(com_dev);
    PIOS_Assert(com_dev->has_tx);
    if (com_dev->driver->available && !(com_dev->driver->available(com_dev->lower_id) & COM_AVAILABLE_TX)) {
        /*
         * Underlying device is down/unconnected.
         * Dump our fifo contents and act like an infinite data sink.
         * Failure to do this results in stale data in the fifo as well as
         * possibly having the caller block trying to send to a device that's
         * no longer accepting data.
         */
        fifoBuf_clearData(&com_dev->tx);
        return len;
    }

    if (len > fifoBuf_getFree(&com_dev->tx)) {
        /* Buffer cannot accept all requested bytes (retry) */
        return -2;
    }

    uint16_t bytes_into_fifo = fifoBuf_putData(&com_dev->tx, buffer, len);

    if (bytes_into_fifo > 0) {
        /* More data has been put in the tx buffer, make sure the tx is started */
        if (com_dev->driver->tx_start) {
            com_dev->driver->tx_start(com_dev->lower_id,
                                      fifoBuf_getUsed(&com_dev->tx));
        }
    }
    return bytes_into_fifo;
}

/**
 * Sends a package over given port
 * \param[in] port COM port
 * \param[in] buffer character buffer
 * \param[in] len buffer length
 * \return -1 if port not available
 * \return -2 if non-blocking mode activated: buffer is full
 *            caller should retry until buffer is free again
 * \return -3 another thread is already sending, caller should
 *            retry until com is available again
 * \return number of bytes transmitted on success
 */
int32_t PIOS_COM_SendBufferNonBlocking(uint32_t com_id, const uint8_t *buffer, uint16_t len)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }
    PIOS_Assert(com_dev->has_tx);
#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreTake(com_dev->sendbuffer_sem, 0) != pdTRUE) {
        return -3;
    }
#endif /* PIOS_INCLUDE_FREERTOS */
    int32_t ret = PIOS_COM_SendBufferNonBlockingInternal(com_dev, buffer, len);
#if defined(PIOS_INCLUDE_FREERTOS)
    xSemaphoreGive(com_dev->sendbuffer_sem);
#endif /* PIOS_INCLUDE_FREERTOS */
    return ret;
}


/**
 * Sends a package over given port
 * (blocking function)
 * \param[in] port COM port
 * \param[in] buffer character buffer
 * \param[in] len buffer length
 * \return -1 if port not available
 * \return -2 if mutex can't be taken;
 * \return -3 if data cannot be sent in the max allotted time of 5000msec
 * \return number of bytes transmitted on success
 */
int32_t PIOS_COM_SendBuffer(uint32_t com_id, const uint8_t *buffer, uint16_t len)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }
    PIOS_Assert(com_dev->has_tx);
#if defined(PIOS_INCLUDE_FREERTOS)
    if (xSemaphoreTake(com_dev->sendbuffer_sem, 5) != pdTRUE) {
        return -2;
    }
#endif /* PIOS_INCLUDE_FREERTOS */
    uint32_t max_frag_len  = fifoBuf_getSize(&com_dev->tx);
    uint32_t bytes_to_send = len;
    while (bytes_to_send) {
        uint32_t frag_size;

        if (bytes_to_send > max_frag_len) {
            frag_size = max_frag_len;
        } else {
            frag_size = bytes_to_send;
        }
        int32_t rc = PIOS_COM_SendBufferNonBlockingInternal(com_dev, buffer, frag_size);
        if (rc >= 0) {
            bytes_to_send -= rc;
            buffer += rc;
        } else {
            switch (rc) {
            case -1:
#if defined(PIOS_INCLUDE_FREERTOS)
                xSemaphoreGive(com_dev->sendbuffer_sem);
#endif /* PIOS_INCLUDE_FREERTOS */
                /* Device is invalid, this will never work */
                return -1;

            case -2:
                /* Device is busy, wait for the underlying device to free some space and retry */
                /* Make sure the transmitter is running while we wait */
                if (com_dev->driver->tx_start) {
                    (com_dev->driver->tx_start)(com_dev->lower_id,
                                                fifoBuf_getUsed(&com_dev->tx));
                }
#if defined(PIOS_INCLUDE_FREERTOS)
                if (xSemaphoreTake(com_dev->tx_sem, 5000) != pdTRUE) {
                    xSemaphoreGive(com_dev->sendbuffer_sem);
                    return -3;
                }
#endif
                continue;
            default:
                /* Unhandled return code */
#if defined(PIOS_INCLUDE_FREERTOS)
                xSemaphoreGive(com_dev->sendbuffer_sem);
#endif /* PIOS_INCLUDE_FREERTOS */
                return rc;
            }
        }
    }
#if defined(PIOS_INCLUDE_FREERTOS)
    xSemaphoreGive(com_dev->sendbuffer_sem);
#endif /* PIOS_INCLUDE_FREERTOS */
    return len;
}

/**
 * Sends a single character over given port
 * \param[in] port COM port
 * \param[in] c character
 * \return -1 if port not available
 * \return -2 buffer is full
 *            caller should retry until buffer is free again
 * \return 0 on success
 */
int32_t PIOS_COM_SendCharNonBlocking(uint32_t com_id, char c)
{
    return PIOS_COM_SendBufferNonBlocking(com_id, (uint8_t *)&c, 1);
}

/**
 * Sends a single character over given port
 * (blocking function)
 * \param[in] port COM port
 * \param[in] c character
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_SendChar(uint32_t com_id, char c)
{
    return PIOS_COM_SendBuffer(com_id, (uint8_t *)&c, 1);
}

/**
 * Sends a string over given port
 * \param[in] port COM port
 * \param[in] str zero-terminated string
 * \return -1 if port not available
 * \return -2 buffer is full
 *         caller should retry until buffer is free again
 * \return 0 on success
 */
int32_t PIOS_COM_SendStringNonBlocking(uint32_t com_id, const char *str)
{
    return PIOS_COM_SendBufferNonBlocking(com_id, (uint8_t *)str, (uint16_t)strlen(str));
}

/**
 * Sends a string over given port
 * (blocking function)
 * \param[in] port COM port
 * \param[in] str zero-terminated string
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_SendString(uint32_t com_id, const char *str)
{
    return PIOS_COM_SendBuffer(com_id, (uint8_t *)str, strlen(str));
}

/**
 * Sends a formatted string (-> printf) over given port
 * \param[in] port COM port
 * \param[in] *format zero-terminated format string - 128 characters supported maximum!
 * \param[in] ... optional arguments,
 *        128 characters supported maximum!
 * \return -2 if non-blocking mode activated: buffer is full
 *         caller should retry until buffer is free again
 * \return 0 on success
 */
int32_t PIOS_COM_SendFormattedStringNonBlocking(uint32_t com_id, const char *format, ...)
{
    uint8_t buffer[128]; // TODO: tmp!!! Provide a streamed COM method later!

    va_list args;

    va_start(args, format);
    vsprintf((char *)buffer, format, args);
    return PIOS_COM_SendBufferNonBlocking(com_id, buffer, (uint16_t)strlen((char *)buffer));
}

/**
 * Sends a formatted string (-> printf) over given port
 * (blocking function)
 * \param[in] port COM port
 * \param[in] *format zero-terminated format string - 128 characters supported maximum!
 * \param[in] ... optional arguments,
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_SendFormattedString(uint32_t com_id, const char *format, ...)
{
    uint8_t buffer[128]; // TODO: tmp!!! Provide a streamed COM method later!
    va_list args;

    va_start(args, format);
    vsprintf((char *)buffer, format, args);
    return PIOS_COM_SendBuffer(com_id, buffer, (uint16_t)strlen((char *)buffer));
}

/**
 * Transfer bytes from port buffers into another buffer
 * \param[in] port COM port
 * \returns Byte from buffer
 */
uint16_t PIOS_COM_ReceiveBuffer(uint32_t com_id, uint8_t *buf, uint16_t buf_len, uint32_t timeout_ms)
{
    PIOS_Assert(buf);
    PIOS_Assert(buf_len);
    uint16_t bytes_from_fifo;

    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        PIOS_Assert(0);
    }
    PIOS_Assert(com_dev->has_rx);

check_again:
    bytes_from_fifo = fifoBuf_getData(&com_dev->rx, buf, buf_len);

    if (bytes_from_fifo == 0) {
        /* No more bytes in receive buffer */
        /* Make sure the receiver is running while we wait */
        if (com_dev->driver->rx_start) {
            /* Notify the lower layer that there is now room in the rx buffer */
            (com_dev->driver->rx_start)(com_dev->lower_id,
                                        fifoBuf_getFree(&com_dev->rx));
        }
        if (timeout_ms > 0) {
#if defined(PIOS_INCLUDE_FREERTOS)
            if (xSemaphoreTake(com_dev->rx_sem, timeout_ms / portTICK_RATE_MS) == pdTRUE) {
                /* Make sure we don't come back here again */
                timeout_ms = 0;
                goto check_again;
            }
#else
            PIOS_DELAY_WaitmS(1);
            timeout_ms--;
            goto check_again;
#endif
        }
    }

    /* Return received byte */
    return bytes_from_fifo;
}

/**
 * Query if a com port is available for use.  That can be
 * used to check a link is established even if the device
 * is valid.
 */
uint32_t PIOS_COM_Available(uint32_t com_id)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        return COM_AVAILABLE_NONE;
    }

    // If a driver does not provide a query method assume always
    // available if valid
    if (com_dev->driver->available == NULL) {
        if (com_dev->has_rx && com_dev->has_tx) {
            return COM_AVAILABLE_RXTX;
        } else if (com_dev->has_rx) {
            return COM_AVAILABLE_RX;
        } else if (com_dev->has_tx) {
            return COM_AVAILABLE_TX;
        }

        return COM_AVAILABLE_RXTX; /* This is the case for com_dev without buffers - for async use only */
    }

    return (com_dev->driver->available)(com_dev->lower_id);
}

/*
 * Set available callback
 * \param[in] port COM port
 * \param[in] available_cb Callback function
 * \param[in] context context to pass to the callback function
 * \return -1 if port not available
 * \return 0 on success
 */
int32_t PIOS_COM_RegisterAvailableCallback(uint32_t com_id, pios_com_callback_available available_cb, uint32_t context)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    /* Invoke the driver function if it exists */
    if (com_dev->driver->bind_available_cb) {
        com_dev->driver->bind_available_cb(com_dev->lower_id, available_cb, context);
    }

    return 0;
}

/*
 * Callback used to pass data from one ocm port to another in PIOS_COM_LinkComPare.
 * \param[in] context     The "to" com port
 * \param[in] buf         The data buffer
 * \param[in] buf_len     The number of bytes received
 * \param[in] headroom    Not used
 * \param[in] task_woken  Not used
 */
static uint16_t PIOS_COM_LinkComPairRxCallback(uint32_t context, uint8_t *buf, uint16_t buf_len, __attribute__((unused)) uint16_t *headroom, __attribute__((unused)) bool *task_woken)
{
    int32_t sent = PIOS_COM_SendBufferNonBlocking(context, buf, buf_len);

    if (sent > 0) {
        return sent;
    }
    return 0;
}

/*
 * Link a pair of com ports so that any data arriving on one is sent out the other.
 * \param[in] com1_id  The first com port
 * \param[in] com2_id  The second com port
 */
void PIOS_COM_LinkComPair(uint32_t com1_id, uint32_t com2_id, bool link_ctrl_line, bool link_baud_rate)
{
    PIOS_COM_ASYNC_RegisterRxCallback(com1_id, PIOS_COM_LinkComPairRxCallback, com2_id);
    PIOS_COM_ASYNC_RegisterRxCallback(com2_id, PIOS_COM_LinkComPairRxCallback, com1_id);
    // Optionally link the control like and baudrate changes between the two.
    if (link_ctrl_line) {
        PIOS_COM_RegisterCtrlLineCallback(com1_id, (pios_com_callback_ctrl_line)PIOS_COM_SetCtrlLine, com2_id);
        PIOS_COM_RegisterCtrlLineCallback(com2_id, (pios_com_callback_ctrl_line)PIOS_COM_SetCtrlLine, com1_id);
    }
    if (link_baud_rate) {
        PIOS_COM_RegisterBaudRateCallback(com1_id, (pios_com_callback_baud_rate)PIOS_COM_ChangeBaud, com2_id);
        PIOS_COM_RegisterBaudRateCallback(com2_id, (pios_com_callback_baud_rate)PIOS_COM_ChangeBaud, com1_id);
    }
}

/*
 * Invoke driver specific control functions
 * \param[in] port COM port
 * \param[in] ctl control function number
 * \param[inout] control function parameter
 * \return 0 on success
 */
int32_t PIOS_COM_Ioctl(uint32_t com_id, uint32_t ctl, void *param)
{
    struct pios_com_dev *com_dev = (struct pios_com_dev *)com_id;

    if (!PIOS_COM_validate(com_dev)) {
        /* Undefined COM port for this board (see pios_board.c) */
        return -1;
    }

    if (!com_dev->driver->ioctl) {
        return -1;
    }

    return com_dev->driver->ioctl(com_dev->lower_id, ctl, param);
}

#endif /* PIOS_INCLUDE_COM */

/**
 * @}
 * @}
 */
