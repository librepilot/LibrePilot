/**
 ******************************************************************************
 * @file       loggingplugin.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup loggingplugin
 * @{
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

#ifndef LOGGINGPLUGIN_H_
#define LOGGINGPLUGIN_H_

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/iconnection.h>
#include <extensionsystem/iplugin.h>
#include <utils/logfile.h>

#include <QThread>
#include <QQueue>
#include <QReadWriteLock>

class UAVObject;
class UAVDataObject;
class UAVTalk;
class LoggingPlugin;
class LoggingGadgetFactory;

namespace Core {
class Command;
}

/**
 *   Define a connection via the IConnection interface
 *   Plugin will add a instance of this class to the pool,
 *   so the connection manager can use it.
 */
class LoggingConnection : public Core::IConnection {
    Q_OBJECT
public:
    LoggingConnection();
    virtual ~LoggingConnection();

    virtual QList <Core::IConnection::device> availableDevices();

    virtual QIODevice *openDevice(const QString &deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName();
    virtual QString shortName();

    bool deviceOpened()
    {
        return m_deviceOpened;
    }
    LogFile *getLogfile()
    {
        return &logFile;
    }

private:
    bool m_deviceOpened;
    LogFile logFile;
};

class LoggingThread : public QThread {
    Q_OBJECT
public:
    LoggingThread();
    virtual ~LoggingThread();

    bool openFile(QString file);

public slots:
    void startLogging();
    void stopLogging();

protected:
    void run();

private slots:
    void objectUpdated(UAVObject *obj);
    void transactionCompleted(UAVObject *obj, bool success);

private:
    QReadWriteLock lock;
    QQueue<UAVDataObject *> queue;
    LogFile logFile;
    UAVTalk *uavTalk;

    void retrieveSettings();
    void retrieveNextObject();
};

class LoggingPlugin : public ExtensionSystem::IPlugin {
    Q_OBJECT
                                              Q_PLUGIN_METADATA(IID "OpenPilot.Logging")

    friend class LoggingConnection;

public:
    enum State { IDLE, LOGGING, REPLAY };

    LoggingPlugin();
    ~LoggingPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString *errorString);
    void shutdown();

    LogFile *getLogfile()
    {
        return logConnection->getLogfile();
    }

    State getState()
    {
        return state;
    }

signals:
    void stateChanged(State);

private slots:
    void toggleLogging();
    void startLogging(QString file);
    void stopLogging();
    void loggingStarted();
    void loggingStopped();
    void replayStarted();
    void replayStopped();

private:
    Core::Command *loggingCommand;
    State state;
    // These are used for replay, logging in its own thread
    LoggingThread *loggingThread;
    LoggingConnection *logConnection;
};
#endif /* LoggingPLUGIN_H_ */
/**
 * @}
 * @}
 */
