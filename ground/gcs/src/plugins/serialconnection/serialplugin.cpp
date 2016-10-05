/**
 ******************************************************************************
 *
 * @file       serialplugin.cpp
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

#include "serialplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <QDebug>

SerialEnumerationThread::SerialEnumerationThread(SerialConnection *serial)
    : m_serial(serial), m_running(false)
{}

void SerialEnumerationThread::run()
{
    m_running = true;
    QList <Core::IConnection::device> devices = m_serial->availableDevices();
    while (m_running) {
        if (!m_serial->deviceOpened()) {
            QList <Core::IConnection::device> newDev = m_serial->availableDevices();
            if (devices != newDev) {
                devices = newDev;
                emit enumerationChanged();
            }
        }
        // update available devices every two seconds (doesn't need more)
        msleep(2000);
    }
}

void SerialEnumerationThread::stop()
{
    if (!m_running) {
        return;
    }
    m_running = false;
    // wait for the thread to terminate
    if (wait(2100) == false) {
        qDebug() << "Cannot terminate SerialEnumerationThread";
    }
}

SerialConnection::SerialConnection(SerialPluginConfiguration *config) :
    serialHandle(NULL),
    enablePolling(true),
    m_enumerateThread(this),
    m_deviceOpened(false),
    m_config(config)
{
    m_optionsPage = new SerialPluginOptionsPage(m_config, this);


    // Experimental: enable polling on all OS'es since there
    // were reports that autodetect does not work on XP amongst
    // others.

    // #ifdef Q_OS_WIN
////I'm cheating a little bit here:
////Knowing if the device enumeration really changed is a bit complicated
////so I just signal it whenever we have a device event...
// QMainWindow *mw = Core::ICore::instance()->mainWindow();
// QObject::connect(mw, SIGNAL(deviceChange()),
// this, SLOT(onEnumerationChanged()));
// #else
    // Other OSes do not send such signals:
    QObject::connect(&m_enumerateThread, SIGNAL(enumerationChanged()), this, SLOT(onEnumerationChanged()));
    QObject::connect(m_optionsPage, SIGNAL(availableDevChanged()), this, SLOT(onEnumerationChanged()));
    m_enumerateThread.start();
// #endif
}

SerialConnection::~SerialConnection()
{
    m_enumerateThread.stop();
}

void SerialConnection::onEnumerationChanged()
{
    if (enablePolling) {
        emit availableDevChanged(this);
    }
}

bool sortPorts(const QSerialPortInfo &s1, const QSerialPortInfo &s2)
{
    return s1.portName() < s2.portName();
}

QList<QSerialPortInfo> SerialConnection::availablePorts()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
#if QT_VERSION == 0x050301 && defined(Q_OS_WIN)
    // workaround to QTBUG-39748 (https://bugreports.qt-project.org/browse/QTBUG-39748)
    // Qt 5.3.1 reports spurious ports with an empty description...
    QMutableListIterator<QSerialPortInfo> i(ports);
    while (i.hasNext()) {
        if (i.next().description().isEmpty()) {
            i.remove();
        }
    }
#endif
    return ports;
}


QList <Core::IConnection::device> SerialConnection::availableDevices()
{
    QList <Core::IConnection::device> list;

    if (enablePolling) {
        QList<QSerialPortInfo> ports = availablePorts();

        // sort the list by port number (nice idea from PT_Dreamer :))
        qSort(ports.begin(), ports.end(), sortPorts);

        foreach(QSerialPortInfo port, ports) {
            device d;

            d.name = port.portName();
            d.displayName = port.portName();
            d.description = port.description();

            list.append(d);
        }
    }

    return list;
}

QIODevice *SerialConnection::openDevice(const QString &deviceName)
{
    if (serialHandle) {
        closeDevice(deviceName);
    }
    QList<QSerialPortInfo> ports = availablePorts();
    foreach(QSerialPortInfo port, ports) {
        if (port.portName() == deviceName) {
            // don't specify a parent when constructing the QSerialPort as this object will be moved
            // to a different thread later on (see telemetrymanager.cpp)
            serialHandle = new QSerialPort(port);
            connect(serialHandle, static_cast<void(QSerialPort::*) (QSerialPort::SerialPortError)>(&QSerialPort::error),
                    [ = ](QSerialPort::SerialPortError error) { qWarning() << "serial port error:" << error; }
                    );
            // we need to handle port settings here...
            if (serialHandle->open(QIODevice::ReadWrite)) {
                if (serialHandle->setBaudRate(m_config->speed().toInt())
                    && serialHandle->setDataBits(QSerialPort::Data8)
                    && serialHandle->setParity(QSerialPort::NoParity)
                    && serialHandle->setStopBits(QSerialPort::OneStop)
                    && serialHandle->setFlowControl(QSerialPort::NoFlowControl)) {
                    qDebug() << "Serial telemetry running at " << m_config->speed();
                    m_deviceOpened = true;
                }
                // see https://librepilot.atlassian.net/browse/LP-341
                serialHandle->setDataTerminalReady(true);
            }
            return serialHandle;
        }
    }
    return NULL;
}

void SerialConnection::closeDevice(const QString &deviceName)
{
    Q_UNUSED(deviceName);
    // we have to delete the serial connection we created
    if (serialHandle) {
        serialHandle->deleteLater();
        serialHandle = NULL;
    }
    m_deviceOpened = false;
}

QString SerialConnection::connectionName()
{
    return QString("Serial port");
}

QString SerialConnection::shortName()
{
    return QString("Serial");
}

/**
   Tells the Serial plugin to stop polling for serial devices
 */
void SerialConnection::suspendPolling()
{
    enablePolling = false;
}

/**
   Tells the Serial plugin to resume polling for serial devices
 */
void SerialConnection::resumePolling()
{
    enablePolling = true;
}

SerialPlugin::SerialPlugin() : m_connection(0), m_config(0)
{}

SerialPlugin::~SerialPlugin()
{
    removeObject(m_connection->optionsPage());
}

bool SerialPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    Core::ICore::instance()->readSettings(this);

    m_connection = new SerialConnection(m_config);

    // must manage this registration of child object ourselves
    // if we use an autorelease here it causes the GCS to crash
    // as it is deleting objects as the app closes...
    addObject(m_connection->optionsPage());

    // FIXME this is really a contrived way to save the settings...
    // needs to be done centrally from
    QObject::connect(m_connection, &SerialConnection::availableDevChanged,
                     [this]() { Core::ICore::instance()->saveSettings(this); }
                     );

    return true;
}

void SerialPlugin::extensionsInitialized()
{
    addAutoReleasedObject(m_connection);
}

void SerialPlugin::readConfig(QSettings &settings, Core::UAVConfigInfo *configInfo)
{
    Q_UNUSED(configInfo);

    m_config = new SerialPluginConfiguration("SerialConnection", settings, this);
}

void SerialPlugin::saveConfig(QSettings &settings, Core::UAVConfigInfo *configInfo) const
{
    Q_UNUSED(configInfo);

    if (m_config) {
        m_config->saveConfig(settings);
    }
}
