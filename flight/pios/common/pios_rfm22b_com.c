/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RFM22B Radio Functions
 * @brief PIOS COM interface for for the RFM22B radio
 * @{
 *
 * @file       pios_rfm22b_com.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Implements a driver the the RFM22B driver
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

#include <pios.h>

#ifdef PIOS_INCLUDE_RFM22B_COM

#include <pios_rfm22b_priv.h>

/* Provide a COM driver */
static void PIOS_RFM22B_COM_ChangeBaud(uint32_t rfm22b_id, uint32_t baud);
static void PIOS_RFM22B_COM_RegisterRxCallback(uint32_t rfm22b_id, pios_com_callback rx_in_cb, uint32_t context);
static void PIOS_RFM22B_COM_RegisterTxCallback(uint32_t rfm22b_id, pios_com_callback tx_out_cb, uint32_t context);
static void PIOS_RFM22B_COM_TxStart(uint32_t rfm22b_id, uint16_t tx_bytes_avail);
static void PIOS_RFM22B_COM_RxStart(uint32_t rfm22b_id, uint16_t rx_bytes_avail);

static void PIOS_RFM22B_COM_RegisterAuxRxCallback(uint32_t rfm22b_id, pios_com_callback rx_in_cb, uint32_t context);
static void PIOS_RFM22B_COM_RegisterAuxTxCallback(uint32_t rfm22b_id, pios_com_callback tx_out_cb, uint32_t context);

static uint32_t PIOS_RFM22B_COM_Available(uint32_t rfm22b_com_id);

/* Local variables */
const struct pios_com_driver pios_rfm22b_com_driver = {
    .set_baud   = PIOS_RFM22B_COM_ChangeBaud,
    .tx_start   = PIOS_RFM22B_COM_TxStart,
    .rx_start   = PIOS_RFM22B_COM_RxStart,
    .bind_tx_cb = PIOS_RFM22B_COM_RegisterTxCallback,
    .bind_rx_cb = PIOS_RFM22B_COM_RegisterRxCallback,
    .available  = PIOS_RFM22B_COM_Available
};

/* Local variables */
const struct pios_com_driver pios_rfm22b_aux_com_driver = {
    .set_baud   = PIOS_RFM22B_COM_ChangeBaud,
    .tx_start   = PIOS_RFM22B_COM_TxStart,
    .rx_start   = PIOS_RFM22B_COM_RxStart,
    .bind_tx_cb = PIOS_RFM22B_COM_RegisterAuxTxCallback,
    .bind_rx_cb = PIOS_RFM22B_COM_RegisterAuxRxCallback,
    .available  = PIOS_RFM22B_COM_Available
};
/**
 * Changes the baud rate of the RFM22B peripheral without re-initialising.
 *
 * @param[in] rfm22b_id  The defice ID
 * @param[in] baud Requested baud rate
 */
static void PIOS_RFM22B_COM_ChangeBaud(__attribute__((unused)) uint32_t rfm22b_id,
                                       __attribute__((unused)) uint32_t baud)
{}

/**
 * Start a receive from the COM device
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] rx_bytes_available  The number of bytes available to receive
 */
static void PIOS_RFM22B_COM_RxStart(__attribute__((unused)) uint32_t rfm22b_id,
                                    __attribute__((unused)) uint16_t rx_bytes_avail)
{}

/**
 * Start a transmit from the COM device
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] tx_bytes_available  The number of bytes available to transmit
 */
static void PIOS_RFM22B_COM_TxStart(__attribute__((unused)) uint32_t rfm22b_id,
                                    __attribute__((unused)) uint16_t tx_bytes_avail)
{}


/**
 * Register the callback to pass received data to
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] rx_in_cb  The Rx callback function.
 * @param[in] context  The callback context.
 */
static void PIOS_RFM22B_COM_RegisterRxCallback(uint32_t rfm22b_id, pios_com_callback rx_in_cb, uint32_t context)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    rfm22b_dev->rx_in_context = context;
    rfm22b_dev->rx_in_cb = rx_in_cb;
}

/**
 * Register the callback to get data from.
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] rx_in_cb  The Tx callback function.
 * @param[in] context  The callback context.
 */
static void PIOS_RFM22B_COM_RegisterTxCallback(uint32_t rfm22b_id, pios_com_callback tx_out_cb, uint32_t context)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    rfm22b_dev->tx_out_context = context;
    rfm22b_dev->tx_out_cb = tx_out_cb;
}

/**
 * Register the callback to pass received data to
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] rx_in_cb  The Rx callback function.
 * @param[in] context  The callback context.
 */
static void PIOS_RFM22B_COM_RegisterAuxRxCallback(uint32_t rfm22b_id, pios_com_callback rx_in_cb, uint32_t context)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    rfm22b_dev->aux_rx_in_context = context;
    rfm22b_dev->aux_rx_in_cb = rx_in_cb;
}

/**
 * Register the callback to get data from.
 *
 * @param[in] rfm22b_dev  The device ID.
 * @param[in] rx_in_cb  The Tx callback function.
 * @param[in] context  The callback context.
 */
static void PIOS_RFM22B_COM_RegisterAuxTxCallback(uint32_t rfm22b_id, pios_com_callback tx_out_cb, uint32_t context)
{
    struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

    if (!PIOS_RFM22B_Validate(rfm22b_dev)) {
        return;
    }

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    rfm22b_dev->aux_tx_out_context = context;
    rfm22b_dev->aux_tx_out_cb = tx_out_cb;
}
/**
 * See if the COM port is alive
 *
 * @param[in] rfm22b_dev  The device ID.
 * @return True of the device is available.
 */
static uint32_t PIOS_RFM22B_COM_Available(uint32_t rfm22b_id)
{
    return PIOS_RFM22B_LinkStatus(rfm22b_id) ? COM_AVAILABLE_RXTX : COM_AVAILABLE_NONE;
}

#endif /* PIOS_INCLUDE_RFM22B_COM */
