/**
 ******************************************************************************
 *
 * @file       serialplugin.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SerialPlugin Serial Connection Plugin
 * @{
 * @brief Impliments serial connection to the flight hardware for Telemetry
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

#ifndef SERIALPLUGIN_H
#define SERIALPLUGIN_H

// #include "serial_global.h"
#include <extensionsystem/iplugin.h>
#include <coreplugin/iconfigurableplugin.h>
#include "coreplugin/iconnection.h"

#include "serialpluginconfiguration.h"
#include "serialpluginoptionspage.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QThread>

class IConnection;
class QSerialPortInfo;
class SerialConnection;

/**
 *   Helper thread to check on new serial port connection/disconnection
 *   Some operating systems do not send device insertion events so
 *   for those we have to poll
 */
// class SERIAL_EXPORT SerialEnumerationThread : public QThread
class SerialEnumerationThread : public QThread {
    Q_OBJECT
public:
    SerialEnumerationThread(SerialConnection *serial);

    virtual void run();

    void stop();

signals:
    void enumerationChanged();

protected:
    SerialConnection *m_serial;
    bool m_running;
};


/**
 *   Define a connection via the IConnection interface
 *   Plugin will add a instance of this class to the pool,
 *   so the connection manager can use it.
 */
class SerialConnection : public Core::IConnection {
    Q_OBJECT
public:
    SerialConnection(SerialPluginConfiguration *config);
    virtual ~SerialConnection();

    virtual QList <Core::IConnection::device> availableDevices();
    virtual QIODevice *openDevice(const QString &deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName();
    virtual QString shortName();
    virtual void suspendPolling();
    virtual void resumePolling();

    bool deviceOpened()
    {
        return m_deviceOpened;
    }

    SerialPluginOptionsPage *optionsPage() const
    {
        return m_optionsPage;
    }

protected slots:
    void onEnumerationChanged();


private:
    QSerialPort *serialHandle;
    bool enablePolling;

    SerialEnumerationThread m_enumerateThread;
    bool m_deviceOpened;

    // FIXME m_config and m_optionsPage belong in IPConnectionPlugin
    SerialPluginConfiguration *m_config;
    SerialPluginOptionsPage *m_optionsPage;

    QList<QSerialPortInfo> availablePorts();
};


class SerialPlugin : public Core::IConfigurablePlugin {
    Q_OBJECT
                                  Q_PLUGIN_METADATA(IID "OpenPilot.Serial")

public:
    SerialPlugin();
    ~SerialPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();

    void readConfig(QSettings &settings, Core::UAVConfigInfo *configInfo);
    void saveConfig(QSettings &settings, Core::UAVConfigInfo *configInfo) const;

private:
    SerialConnection *m_connection;
    SerialPluginConfiguration *m_config;
};

#endif // SERIALPLUGIN_H
