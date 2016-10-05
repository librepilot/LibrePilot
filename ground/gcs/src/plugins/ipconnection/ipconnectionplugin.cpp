/**
 ******************************************************************************
 *
 * @file       ipconnectionplugin.cpp
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

// The core of this plugin has been directly copied from the serial plugin and converted to work over a TCP link instead of a direct serial link

#include "ipconnectionplugin.h"

#include "ipconnection_internal.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/threadmanager.h>

#include <QMainWindow>
#include <QMessageBox>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QWaitCondition>
#include <QMutex>
#include <QDebug>

// Communication between IPConnectionConnection::OpenDevice() and IPConnection::onOpenDevice()
QString errorMsg;
QWaitCondition openDeviceWait;
QWaitCondition closeDeviceWait;
// QReadWriteLock dummyLock;
QMutex ipConMutex;
QAbstractSocket *ret;

IPConnection::IPConnection(IPConnectionConnection *connection) : QObject()
{
    moveToThread(Core::ICore::instance()->threadManager()->getRealTimeThread());

    QObject::connect(connection, SIGNAL(CreateSocket(QString, int, bool)),
                     this, SLOT(onOpenDevice(QString, int, bool)));
    QObject::connect(connection, SIGNAL(CloseSocket(QAbstractSocket *)),
                     this, SLOT(onCloseDevice(QAbstractSocket *)));
}

/*IPConnection::~IPConnection()
   {

   }*/

void IPConnection::onOpenDevice(QString HostName, int Port, bool UseTCP)
{
    QAbstractSocket *ipSocket;
    const int Timeout = 5 * 1000;

    ipConMutex.lock();
    if (UseTCP) {
        ipSocket = new QTcpSocket(this);
    } else {
        ipSocket = new QUdpSocket(this);
    }

    // do sanity check on hostname and port...
    if ((HostName.length() == 0) || (Port < 1)) {
        errorMsg = "Please configure Host and Port options before opening the connection";
    } else {
        // try to connect...
        ipSocket->connectToHost(HostName, Port);

        // in blocking mode so we wait for the connection to succeed
        if (ipSocket->waitForConnected(Timeout)) {
            ret = ipSocket;
            openDeviceWait.wakeAll();
            ipConMutex.unlock();
            return;
        }
        // tell user something went wrong
        errorMsg = ipSocket->errorString();
    }
    /* BUGBUG TODO - returning null here leads to segfault because some caller still calls disconnect without checking our return value properly
     * someone needs to debug this, I got lost in the calling chain.*/
    ret = NULL;
    openDeviceWait.wakeAll();
    ipConMutex.unlock();
}

void IPConnection::onCloseDevice(QAbstractSocket *ipSocket)
{
    ipConMutex.lock();
    ipSocket->close();
    delete (ipSocket);
    closeDeviceWait.wakeAll();
    ipConMutex.unlock();
}


IPConnection *connection = 0;

IPConnectionConnection::IPConnectionConnection(IPConnectionConfiguration *config) : m_config(config)
{
    m_ipSocket    = NULL;

    m_optionsPage = new IPConnectionOptionsPage(m_config, this);

    if (!connection) {
        connection = new IPConnection(this);
    }

    // just signal whenever we have a device event...
    QMainWindow *mw = Core::ICore::instance()->mainWindow();
    QObject::connect(mw, SIGNAL(deviceChange()), this, SLOT(onEnumerationChanged()));
    QObject::connect(m_optionsPage, SIGNAL(availableDevChanged()), this, SLOT(onEnumerationChanged()));
}

IPConnectionConnection::~IPConnectionConnection()
{
    // clean up out resources...
    if (m_ipSocket) {
        m_ipSocket->close();
        delete m_ipSocket;
        m_ipSocket = NULL;
    }
    if (connection) {
        delete connection;
        connection = NULL;
    }
}

void IPConnectionConnection::onEnumerationChanged()
{
    emit availableDevChanged(this);
}

QList <Core::IConnection::device> IPConnectionConnection::availableDevices()
{
    QList <Core::IConnection::device> list;
    device d;
    if (m_config->hostName().length() > 1) {
        d.displayName = (const QString)m_config->hostName();
    } else {
        d.displayName = tr("Unconfigured");
    }
    d.name = (const QString)m_config->hostName();
    // we only have one "device" as defined by the configuration m_config
    list.append(d);

    return list;
}

QIODevice *IPConnectionConnection::openDevice(const QString &)
{
    QString hostName;
    int port;
    bool useTCP;
    QMessageBox msgBox;

    // get the configuration info
    hostName = m_config->hostName();
    port     = m_config->port();
    useTCP   = m_config->useTCP();

    if (m_ipSocket) {
        // Andrew: close any existing socket... this should never occur
        ipConMutex.lock();
        emit CloseSocket(m_ipSocket);
        closeDeviceWait.wait(&ipConMutex);
        ipConMutex.unlock();
        m_ipSocket = NULL;
    }

    ipConMutex.lock();
    emit CreateSocket(hostName, port, useTCP);
    openDeviceWait.wait(&ipConMutex);
    ipConMutex.unlock();
    m_ipSocket = ret;
    if (m_ipSocket == NULL) {
        msgBox.setText((const QString)errorMsg);
        msgBox.exec();
    }
    return m_ipSocket;
}

void IPConnectionConnection::closeDevice(const QString &)
{
    if (m_ipSocket) {
        ipConMutex.lock();
        emit CloseSocket(m_ipSocket);
        closeDeviceWait.wait(&ipConMutex);
        ipConMutex.unlock();
        m_ipSocket = NULL;
    }
}


QString IPConnectionConnection::connectionName()
{
    return QString("Network telemetry port");
}

QString IPConnectionConnection::shortName()
{
    if (m_config->useTCP()) {
        return QString("TCP");
    } else {
        return QString("UDP");
    }
}


IPConnectionPlugin::IPConnectionPlugin() : m_connection(0), m_config(0)
{}

IPConnectionPlugin::~IPConnectionPlugin()
{
    // manually remove the options page object
    removeObject(m_connection->optionsPage());
}

bool IPConnectionPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    Core::ICore::instance()->readSettings(this);

    m_connection = new IPConnectionConnection(m_config);

    // must manage this registration of child object ourselves
    // if we use an autorelease here it causes the GCS to crash
    // as it is deleting objects as the app closes...
    addObject(m_connection->optionsPage());

    // FIXME this is really a contrived way to save the settings...
    // needs to be done centrally from
    QObject::connect(m_connection, &IPConnectionConnection::availableDevChanged,
                     [this]() { Core::ICore::instance()->saveSettings(this); }
                     );

    return true;
}

void IPConnectionPlugin::extensionsInitialized()
{
    addAutoReleasedObject(m_connection);
}

void IPConnectionPlugin::readConfig(QSettings &settings, Core::UAVConfigInfo *configInfo)
{
    Q_UNUSED(configInfo);

    m_config = new IPConnectionConfiguration("IPConnection", settings, this);
}

void IPConnectionPlugin::saveConfig(QSettings &settings, Core::UAVConfigInfo *configInfo) const
{
    Q_UNUSED(configInfo);

    if (m_config) {
        m_config->saveConfig(settings);
    }
}
