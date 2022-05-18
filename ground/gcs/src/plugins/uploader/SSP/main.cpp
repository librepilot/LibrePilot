/**
 ******************************************************************************
 *
 * @file       main.cpp
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
#include "qssp.h"
#include "port.h"
#include "qsspt.h"

#include "../../../libs/qextserialport/src/qextserialport.h"

#include <QtCore/QCoreApplication>
#include <QTime>
#include <QDebug>

#define MAX_PACKET_DATA_LEN 255
#define MAX_PACKET_BUF_SIZE (1 + 1 + 255 + 2)

int main(int argc, char *argv[])
{
    uint8_t sspTxBuf[MAX_PACKET_BUF_SIZE];
    uint8_t sspRxBuf[MAX_PACKET_BUF_SIZE];

    port *info;
    PortSettings settings;

    settings.BaudRate    = BAUD57600;
    settings.DataBits    = DATA_8;
    settings.FlowControl = FLOW_OFF;
    settings.Parity = PAR_NONE;
    settings.StopBits    = STOP_1;
    settings.Timeout_Millisec = 5000;

    info = new port(settings, "COM3");
    info->rxBuf      = sspRxBuf;
    info->rxBufSize  = MAX_PACKET_DATA_LEN;
    info->txBuf      = sspTxBuf;
    info->txBufSize  = 255;
    info->max_retry  = 3;
    info->timeoutLen = 5000;
    // qssp b(info);
    qsspt bb(info);
    uint8_t buf[1000];
    QCoreApplication a(argc, argv);
    while (!bb.ssp_Synchronise()) {
        qDebug() << "trying sync";
    }
    bb.start();
    qDebug() << "sync complete";
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    while (true) {
        if (bb.sendData(buf, 63)) {
            qDebug() << "send OK";
        }
// else
// qDebug()<<"send NOK";
////bb.ssp_SendData(buf,63);
    }
    while (true) {
        if (bb.packets_Available() > 0) {
            bb.read_Packet(buf);
            qDebug() << "receive=" << (int)buf[0] << (int)buf[1] << (int)buf[2];
            ++buf[0];
            ++buf[1];
            ++buf[2];
            // bb.ssp_SendData(buf,63);
            bb.sendData(buf, 63);
        }
        // bb.ssp_ReceiveProcess();
        // bb.ssp_SendProcess();
    }
    return a.exec();
}
