/**
 ******************************************************************************
 *
 * @file       ipconnectionplugin.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin implements telemetry over TCP/IP and UDP/IP
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

#ifndef IPCONNECTIONPLUGIN_H
#define IPCONNECTIONPLUGIN_H

#include "ipconnection_global.h"
#include "ipconnectionoptionspage.h"
#include "ipconnectionconfiguration.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/iconfigurableplugin.h>
#include <coreplugin/iconnection.h>

class QAbstractSocket;
class QTcpSocket;
class QUdpSocket;

class IConnection;

/**
 *   Define a connection via the IConnection interface
 *   Plugin will add a instance of this class to the pool,
 *   so the connection manager can use it.
 */
class IPCONNECTION_EXPORT IPConnectionConnection : public Core::IConnection {
    Q_OBJECT

public:
    IPConnectionConnection(IPConnectionConfiguration *config);
    virtual ~IPConnectionConnection();

    virtual QList <Core::IConnection::device> availableDevices();
    virtual QIODevice *openDevice(const QString &deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName();
    virtual QString shortName();

    IPConnectionOptionsPage *optionsPage() const
    {
        return m_optionsPage;
    }

protected slots:
    void onEnumerationChanged();

signals:
    // For the benefit of IPConnection
    // FIXME change to camel case
    void CreateSocket(QString HostName, int Port, bool UseTCP);
    void CloseSocket(QAbstractSocket *socket);

private:
    QAbstractSocket *m_ipSocket;
    // FIXME m_config and m_optionsPage belong in IPConnectionPlugin
    IPConnectionConfiguration *m_config;
    IPConnectionOptionsPage *m_optionsPage;
};

class IPCONNECTION_EXPORT IPConnectionPlugin : public Core::IConfigurablePlugin {
    Q_OBJECT
                          Q_PLUGIN_METADATA(IID "OpenPilot.IPConnection")

public:
    IPConnectionPlugin();
    ~IPConnectionPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();

    void readConfig(QSettings &settings, Core::UAVConfigInfo *configInfo);
    void saveConfig(QSettings &settings, Core::UAVConfigInfo *configInfo) const;

private:
    IPConnectionConnection *m_connection;
    IPConnectionConfiguration *m_config;
};

#endif // IPCONNECTIONPLUGIN_H
