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
#include "streamserviceplugin.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QDateTime>
#include <QTcpServer>
#include <QTcpSocket>

#include "extensionsystem/pluginmanager.h"
#include "../uavobjects/uavobjectmanager.h"
#include "../uavobjects/uavdataobject.h"

StreamServicePlugin::StreamServicePlugin() :
    port(7891),
    isSubscribed(false) {}

StreamServicePlugin::~StreamServicePlugin()
{
    if (pServer == Q_NULLPTR) {
        return;
    }
    if (pServer->isListening()) {
        foreach(QTcpSocket * client, activeClients) {
            /* Disconnect the client discarding pending
             * bytes */
            if (client->isOpen()) {
                client->close();
            }
        }
        pServer->close();
    }
}

bool StreamServicePlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    pServer = new QTcpServer();

    if (!pServer->listen(QHostAddress::Any, port)) {
        *errorString = tr("Couldn't start StreamService: ") + pServer->errorString();
        return false;
    }

    connect(pServer, &QTcpServer::newConnection, this, &StreamServicePlugin::clientConnected);

    return true;
}

void StreamServicePlugin::extensionsInitialized()
{
    if (pServer == Q_NULLPTR) {
        return;
    }

    pServer->resumeAccepting();
}

void StreamServicePlugin::shutdown()
{
    if (pServer != Q_NULLPTR) {
        pServer->pauseAccepting();
    }

    foreach(QTcpSocket * pClient, activeClients) {
        pClient->disconnectFromHost();
    }
}

void StreamServicePlugin::objectUpdated(UAVObject *pObj)
{
    QJsonObject qtjson;

    pObj->toJson(qtjson);

    // Adds timestamp: Milliseconds from epoch
    qtjson.insert("gcs_timestamp_ms", QJsonValue(QDateTime::currentMSecsSinceEpoch()));

    QJsonDocument jsonDoc(qtjson);
    QString strJson = QString(jsonDoc.toJson(QJsonDocument::Compact)) + "\n";

    foreach(QTcpSocket * pClient, activeClients) {
        if (pClient->isOpen()) {
            if (pClient->write(strJson.toUtf8().constData(), strJson.length())) {
                pClient->flush();
            }
        }
    }
}

void StreamServicePlugin::clientConnected()
{
    QTcpSocket *pending = pServer->nextPendingConnection();

    if (pending == Q_NULLPTR) {
        return;
    }
    makeSureIsSubscribed();

    connect(pending, &QTcpSocket::disconnected, this, &StreamServicePlugin::clientDisconnected);
    activeClients.append(pending);
}

void StreamServicePlugin::clientDisconnected()
{
    disconnect(sender());
    activeClients.removeAll((QTcpSocket *)sender());
}

inline void StreamServicePlugin::makeSureIsSubscribed()
{
    if (isSubscribed) {
        return;
    }

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objManager);

    QList< QList<UAVDataObject *> > objList = objManager->getDataObjects();
    foreach(QList<UAVDataObject *> list, objList) {
        foreach(UAVDataObject * obj, list) {
            connect(obj, &UAVDataObject::objectUpdated, this, &StreamServicePlugin::objectUpdated);
        }
    }
    isSubscribed = true;
}
