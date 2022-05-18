/**
 ******************************************************************************
 *
 * @file       qsspt.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Uploader Serial and USB Uploader Plugin
 * @{
 * @brief The USB and Serial protocol uploader plugin
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
#include "qsspt.h"

#include <QDebug>

qsspt::qsspt(port *info, bool debug) : qssp(info, debug), endthread(false), datapending(false), debug(debug)
{}

qsspt::~qsspt()
{
    endthread = true;
    // TODO bad...
    wait(1000);
}

void qsspt::run()
{
    // TODO the loop is non blocking and will spin as fast as the CPU allows
    while (!endthread) {
        receivestatus = ssp_ReceiveProcess();
        sendstatus    = ssp_SendProcess();
        sendbufmutex.lock();
        if (datapending && receivestatus == SSP_TX_IDLE) {
            ssp_SendData(mbuf, msize);
            datapending = false;
        }
        sendbufmutex.unlock();
        if (sendstatus == SSP_TX_ACKED) {
            sendwait.wakeAll();
        }
    }
}

bool qsspt::sendData(uint8_t *buf, uint16_t size)
{
    if (debug) {
        QByteArray data((const char *)buf, size);
        qDebug() << "SSP TX " << data.toHex();
    }
    if (datapending) {
        return false;
    }
    sendbufmutex.lock();
    datapending = true;
    mbuf  = buf;
    msize = size;
    sendbufmutex.unlock();
    // TODO why do we wait 10 seconds ? why do we then ignore the timeout ?
    // There is a ssp_SendDataBlock method...
    msendwait.lock();
    sendwait.wait(&msendwait, 10000);
    msendwait.unlock();
    return true;
}

void qsspt::pfCallBack(uint8_t *buf, uint16_t size)
{
    if (debug) {
        qDebug() << "receive callback" << buf[0] << buf[1] << buf[2] << buf[3] << buf[4] << "queue size=" << queue.count();
    }
    QByteArray data((const char *)buf, size);
    mutex.lock();
    queue.enqueue(data);
    mutex.unlock();
}

int qsspt::packets_Available()
{
    return queue.count();
}

int qsspt::read_Packet(void *data)
{
    mutex.lock();
    if (queue.size() == 0) {
        mutex.unlock();
        return -1;
    }
    QByteArray arr = queue.dequeue();
    memcpy(data, (uint8_t *)arr.data(), arr.length());
    if (debug) {
        qDebug() << "SSP RX " << arr.toHex();
    }
    mutex.unlock();
    return arr.length();
}
