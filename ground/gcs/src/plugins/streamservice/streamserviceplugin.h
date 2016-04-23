/**
 ******************************************************************************
 *
 * @file       streamserviceplugin.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup StreamServicePlugin Plugin
 * @{
 * @brief Data UAV Data objects TCP/IP stream service
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
#ifndef STREAMSERVICEPLUGIN_H
#define STREAMSERVICEPLUGIN_H

#include <extensionsystem/iplugin.h>
#include "../uavobjects/uavobject.h"

#include <QtPlugin>

class QTcpServer;
class QTcpSocket;

class StreamServicePlugin : public ExtensionSystem::IPlugin {
    Q_OBJECT
                                                    Q_PLUGIN_METADATA(IID "Openpilot.StreamService")

public:
    StreamServicePlugin();
    ~StreamServicePlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    void shutdown();

public slots:
    void objectUpdated(UAVObject *pObj);

private slots:
    void clientConnected();
    void clientDisconnected();

private:
    quint16 port;

    QTcpServer *pServer;
    QList<QTcpSocket *> activeClients;
    bool isSubscribed;

    inline void makeSureIsSubscribed();
};

#endif // STREAMSERVICEPLUGIN_H
