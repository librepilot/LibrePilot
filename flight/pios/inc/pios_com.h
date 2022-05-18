/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_COM COM layer functions
 * @brief Hardware communication layer
 * @{
 *
 * @file       pios_com.h
 * @author     The LibrePilot Project, http://www.librepilot.org, Copyright (c) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Thorsten Klose (tk@midibox.org)
 * @brief      COM layer functions header
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

#ifndef PIOS_COM_H
#define PIOS_COM_H

#include <stdint.h> /* uint*_t */
#include <stdbool.h> /* bool */

typedef uint16_t (*pios_com_callback)(uint32_t context, uint8_t *buf, uint16_t buf_len, uint16_t *headroom, bool *task_woken);
typedef void (*pios_com_callback_ctrl_line)(uint32_t context, uint32_t mask, uint32_t state);
typedef void (*pios_com_callback_baud_rate)(uint32_t context, uint32_t baud);
typedef void (*pios_com_callback_available)(uint32_t context, uint32_t available);

enum PIOS_COM_Word_Length {
    PIOS_COM_Word_length_Unchanged = 0,
    PIOS_COM_Word_length_8b,
    PIOS_COM_Word_length_9b,
};

enum PIOS_COM_StopBits {
    PIOS_COM_StopBits_Unchanged = 0,
    PIOS_COM_StopBits_0_5,
    PIOS_COM_StopBits_1,
    PIOS_COM_StopBits_1_5,
    PIOS_COM_StopBits_2,
};

enum PIOS_COM_Parity {
    PIOS_COM_Parity_Unchanged = 0,
    PIOS_COM_Parity_No,
    PIOS_COM_Parity_Even,
    PIOS_COM_Parity_Odd,
};

struct pios_com_driver {
    void     (*set_baud)(uint32_t id, uint32_t baud);
    void     (*set_config)(uint32_t usart_id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate);
    void     (*set_ctrl_line)(uint32_t id, uint32_t mask, uint32_t state);
    void     (*tx_start)(uint32_t id, uint16_t tx_bytes_avail);
    void     (*rx_start)(uint32_t id, uint16_t rx_bytes_avail);
    void     (*bind_rx_cb)(uint32_t id, pios_com_callback rx_in_cb, uint32_t context);
    void     (*bind_tx_cb)(uint32_t id, pios_com_callback tx_out_cb, uint32_t context);
    void     (*bind_ctrl_line_cb)(uint32_t id, pios_com_callback_ctrl_line ctrl_line_cb, uint32_t context);
    void     (*bind_baud_rate_cb)(uint32_t id, pios_com_callback_baud_rate baud_rate_cb, uint32_t context);
    uint32_t (*available)(uint32_t id);
    void     (*bind_available_cb)(uint32_t id, pios_com_callback_available available_cb, uint32_t context);
    int32_t  (*ioctl)(uint32_t id, uint32_t ctl, void *param);
};

/* Control line definitions */
#define COM_CTRL_LINE_DTR_MASK 0x01
#define COM_CTRL_LINE_RTS_MASK 0x02

/* Public Functions */
extern int32_t PIOS_COM_Init(uint32_t *com_id, const struct pios_com_driver *driver, uint32_t lower_id, uint8_t *rx_buffer, uint16_t rx_buffer_len, uint8_t *tx_buffer, uint16_t tx_buffer_len);
extern int32_t PIOS_COM_ChangeBaud(uint32_t com_id, uint32_t baud);
extern int32_t PIOS_COM_ChangeConfig(uint32_t com_id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate);
extern int32_t PIOS_COM_SetCtrlLine(uint32_t com_id, uint32_t mask, uint32_t state);
extern int32_t PIOS_COM_RegisterCtrlLineCallback(uint32_t usart_id, pios_com_callback_ctrl_line ctrl_line_cb, uint32_t context);
extern int32_t PIOS_COM_RegisterBaudRateCallback(uint32_t usart_id, pios_com_callback_baud_rate baud_rate_cb, uint32_t context);
extern int32_t PIOS_COM_SendCharNonBlocking(uint32_t com_id, char c);
extern int32_t PIOS_COM_SendChar(uint32_t com_id, char c);
extern int32_t PIOS_COM_SendBufferNonBlocking(uint32_t com_id, const uint8_t *buffer, uint16_t len);
extern int32_t PIOS_COM_SendBuffer(uint32_t com_id, const uint8_t *buffer, uint16_t len);
extern int32_t PIOS_COM_SendStringNonBlocking(uint32_t com_id, const char *str);
extern int32_t PIOS_COM_SendString(uint32_t com_id, const char *str);
extern int32_t PIOS_COM_SendFormattedStringNonBlocking(uint32_t com_id, const char *format, ...);
extern int32_t PIOS_COM_SendFormattedString(uint32_t com_id, const char *format, ...);
extern uint16_t PIOS_COM_ReceiveBuffer(uint32_t com_id, uint8_t *buf, uint16_t buf_len, uint32_t timeout_ms);
extern uint32_t PIOS_COM_Available(uint32_t com_id);
extern int32_t PIOS_COM_RegisterAvailableCallback(uint32_t com_id, pios_com_callback_available, uint32_t context);

/* Event driven asynchronous API */
extern int32_t PIOS_COM_ASYNC_TxStart(uint32_t id, uint16_t tx_bytes_avail);
extern int32_t PIOS_COM_ASYNC_RxStart(uint32_t id, uint16_t rx_bytes_avail);
extern int32_t PIOS_COM_ASYNC_RegisterRxCallback(uint32_t id, pios_com_callback rx_in_cb, uint32_t context);
extern int32_t PIOS_COM_ASYNC_RegisterTxCallback(uint32_t id, pios_com_callback tx_out_cb, uint32_t context);

extern void PIOS_COM_LinkComPair(uint32_t com1_id, uint32_t com2_id, bool link_ctrl_line, bool link_baud_rate);

#define COM_AVAILABLE_NONE  (0)
#define COM_AVAILABLE_RX    (1 << 0)
#define COM_AVAILABLE_TX    (1 << 1)
#define COM_AVAILABLE_RXTX  (COM_AVAILABLE_RX | COM_AVAILABLE_TX)

/* ioctl */
extern int32_t PIOS_COM_Ioctl(uint32_t com_id, uint32_t ctl, void *param);

#define COM_IOCTL_NRBITS    16
#define COM_IOCTL_TYPEBITS  16

#define COM_IOCTL_NRMASK    ((1 << COM_IOCTL_NRBITS) - 1)
#define COM_IOCTL_TYPEMASK  ((1 << COM_IOCTL_TYPEBITS) - 1)

#define COM_IOCTL_NRSHIFT   0
#define COM_IOCTL_TYPESHIFT (COM_IOCTL_NRSHIFT + COM_IOCTL_NRBITS)

#define COM_IOCTL(type, nr, pt) \
    (((type) << COM_IOCTL_TYPESHIFT) | \
     ((nr) << COM_IOCTL_NRSHIFT))

/* used to decode ioctl numbers.. */
#define COM_IOCTL_TYPE(nr) (((nr) >> COM_IOCTL_TYPESHIFT) & COM_IOCTL_TYPEMASK)
#define COM_IOCTL_NR(nr)   (((nr) >> COM_IOCTL_NRSHIFT) & COM_IOCTL_NRMASK)

enum pios_com_ioctl_type {
    COM_IOCTL_TYPE_USART,
    COM_IOCTL_TYPE_USB_CDC,
    COM_IOCTL_TYPE_SOFT_UART,
};

#define COM_IOCTL_ENOSYS (-1) /* Function not implemented */

#endif /* PIOS_COM_H */

/**
 * @}
 * @}
 */
