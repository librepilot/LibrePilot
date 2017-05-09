/**
 ******************************************************************************
 *
 * @file       logging.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Import/Export Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup   Logging
 * @{
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

#include "loggingplugin.h"
#include "logginggadgetfactory.h"

#include <QApplication>
#include <QDebug>
#include <QtPlugin>
#include <QThread>
#include <QStringList>
#include <QDir>
#include <QFileDialog>
#include <QList>
#include <QErrorMessage>
#include <QWriteLocker>

#include <extensionsystem/pluginmanager.h>
#include <QKeySequence>
#include "uavobjectmanager.h"


LoggingConnection::LoggingConnection() :
    m_deviceOpened(false), logFile()
{}

LoggingConnection::~LoggingConnection()
{
    // make sure to close device to kill timers appropriately
    closeDevice("");
}

QList <Core::IConnection::device> LoggingConnection::availableDevices()
{
    QList <device> list;
    device d;
    d.displayName = "Logfile replay...";
    d.name = "Logfile replay...";
    list << d;

    return list;
}

QIODevice *LoggingConnection::openDevice(const QString &deviceName)
{
    closeDevice(deviceName);

    QString fileName = QFileDialog::getOpenFileName(NULL, tr("Open file"), QString(""), tr("OpenPilot Log (*.opl)"));
    if (!fileName.isNull()) {
        logFile.setFileName(fileName);
        if (logFile.open(QIODevice::ReadOnly)) {
            // call startReplay on correct thread to avoid error from LogFile's replay QTimer
            // you can't start or stop the timer from a thread other than the QTimer owner thread.
            // note that the LogFile IO device (and thus its owned QTimer) is moved to a dedicated thread by the TelemetryManager
            Qt::ConnectionType ct = (QApplication::instance()->thread() == logFile.thread()) ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
            QMetaObject::invokeMethod(&logFile, "startReplay", ct);
            m_deviceOpened = true;
        }
        return &logFile;
    }

    return NULL;
}

void LoggingConnection::closeDevice(const QString &deviceName)
{
    Q_UNUSED(deviceName);

    if (logFile.isOpen()) {
        m_deviceOpened = false;
        // call stoptReplay on correct thread to avoid error from LogFile's replay QTimer
        // you can't start or stop the timer from a thread other than the QTimer owner thread.
        // note that the LogFile IO device (and thus its owned QTimer) is moved to a dedicated thread by the TelemetryManager
        Qt::ConnectionType ct = (QApplication::instance()->thread() == logFile.thread()) ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(&logFile, "stopReplay", ct);
        logFile.close();
    }
}

QString LoggingConnection::connectionName()
{
    return QString("Logfile replay");
}

QString LoggingConnection::shortName()
{
    return QString("Logfile");
}

LoggingThread::LoggingThread() : QThread(), uavTalk(0)
{}

LoggingThread::~LoggingThread()
{
    delete uavTalk;
}

/**
 * Sets the file to use for logging and takes the parent plugin
 * to connect to stop logging signal
 * @param[in] file File name to write to
 * @param[in] parent plugin
 */
bool LoggingThread::openFile(QString file)
{
    logFile.setFileName(file);
    logFile.open(QIODevice::WriteOnly);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    uavTalk = new UAVTalk(&logFile, objManager);

    return true;
};

/**
 * Logs an object update to the file.  Data format is the
 * timestamp as a 32 bit uint counting ms from start of
 * file writing (flight time will be embedded in stream),
 * then object packet size, then the packed UAVObject.
 */
void LoggingThread::objectUpdated(UAVObject *obj)
{
    QWriteLocker locker(&lock);

    if (!uavTalk->sendObject(obj, false, false)) {
        qDebug() << "LoggingThread - error logging" << obj->getName();
    }
};


void LoggingThread::run()
{
    qDebug() << "LoggingThread - run";
    startLogging();
}

/**
 * Connect signals from all the objects updates to the write routine then
 * run event loop
 */
void LoggingThread::startLogging()
{
    qDebug() << "LoggingThread - start logging";
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    QList< QList<UAVObject *> > list;
    list = objManager->getObjects();
    QList< QList<UAVObject *> >::const_iterator i;
    QList<UAVObject *>::const_iterator j;
    int objects = 0;

    for (i = list.constBegin(); i != list.constEnd(); ++i) {
        for (j = (*i).constBegin(); j != (*i).constEnd(); ++j) {
            connect(*j, SIGNAL(objectUpdated(UAVObject *)), (LoggingThread *)this, SLOT(objectUpdated(UAVObject *)));
            objects++;
        }
    }

    GCSTelemetryStats *gcsStatsObj = GCSTelemetryStats::GetInstance(objManager);
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED) {
        qDebug() << "LoggingThread - connected, ask for all settings";
        retrieveSettings();
    } else {
        qDebug() << "LoggingThread - not connected, do not ask for settings";
    }

    exec();
}

/**
 * Pass this command to the correct thread then close the file
 */
void LoggingThread::stopLogging()
{
    qDebug() << "LoggingThread - stop logging";
    QWriteLocker locker(&lock);

    // Disconnect all objects we registered with:
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    QList< QList<UAVObject *> > list;
    list = objManager->getObjects();
    QList< QList<UAVObject *> >::const_iterator i;
    QList<UAVObject *>::const_iterator j;

    for (i = list.constBegin(); i != list.constEnd(); ++i) {
        for (j = (*i).constBegin(); j != (*i).constEnd(); ++j) {
            disconnect(*j, SIGNAL(objectUpdated(UAVObject *)), (LoggingThread *)this, SLOT(objectUpdated(UAVObject *)));
        }
    }

    logFile.close();

    quit();

    // wait for thread to finish
    wait();
}

/**
 * Initialize queue with settings objects to be retrieved.
 */
void LoggingThread::retrieveSettings()
{
    // Clear object queue
    queue.clear();
    // Get all objects, add metaobjects, settings and data objects with OnChange update mode to the queue
    // Get UAVObjectManager instance
    ExtensionSystem::PluginManager *pm   = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objMngr = pm->getObject<UAVObjectManager>();
    QList< QList<UAVDataObject *> > objs = objMngr->getDataObjects();
    for (int n = 0; n < objs.length(); ++n) {
        UAVDataObject *obj = objs[n][0];
        if (obj->isSettingsObject()) {
            queue.enqueue(obj);
        }
    }
    // Start retrieving
    qDebug() << QString("LoggingThread - retrieve settings objects from the autopilot (%1 objects)")
        .arg(queue.length());
    retrieveNextObject();
}

/**
 * Retrieve the next object in the queue
 */
void LoggingThread::retrieveNextObject()
{
    // If queue is empty return
    if (queue.isEmpty()) {
        qDebug() << "LoggingThread - Object retrieval completed";
        return;
    }
    // Get next object from the queue
    UAVObject *obj = queue.dequeue();
    // Connect to object
    connect(obj, SIGNAL(transactionCompleted(UAVObject *, bool)), this, SLOT(transactionCompleted(UAVObject *, bool)));
    // Request update
    obj->requestUpdate();
}

/**
 * Called by the retrieved object when a transaction is completed.
 */
void LoggingThread::transactionCompleted(UAVObject *obj, bool success)
{
    Q_UNUSED(success);
    // Disconnect from sending object
    obj->disconnect(this);
    // Process next object if telemetry is still available
    // Get stats objects
    ExtensionSystem::PluginManager *pm     = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    GCSTelemetryStats *gcsStatsObj         = GCSTelemetryStats::GetInstance(objManager);
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED) {
        retrieveNextObject();
    } else {
        qDebug() << "LoggingThread - object retrieval has been cancelled";
        queue.clear();
    }
}

LoggingPlugin::LoggingPlugin() :
    state(IDLE),
    loggingThread(NULL),
    logConnection(new LoggingConnection()),
    mf(NULL),
    cmd(NULL)
{}

LoggingPlugin::~LoggingPlugin()
{
    stopLogging();
    // logConnection will be auto released
}

/**
 * Add Logging plugin to File menu
 */
bool LoggingPlugin::initialize(const QStringList & args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    // Add Menu entry
    Core::ActionManager *am   = Core::ICore::instance()->actionManager();
    Core::ActionContainer *ac = am->actionContainer(Core::Constants::M_TOOLS);

    // Command to start logging
    cmd = am->registerAction(new QAction(this),
                             "LoggingPlugin.Logging",
                             QList<int>() <<
                             Core::Constants::C_GLOBAL_ID);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+L"));
    cmd->action()->setText(tr("Start logging..."));

    ac->menu()->addSeparator();
    ac->appendGroup("Logging");
    ac->addAction(cmd, "Logging");

    connect(cmd->action(), SIGNAL(triggered(bool)), this, SLOT(toggleLogging()));

    mf = new LoggingGadgetFactory(this);
    addAutoReleasedObject(mf);

    // Map signal from end of replay to replay stopped
    connect(getLogfile(), SIGNAL(replayFinished()), this, SLOT(replayStopped()));
    connect(getLogfile(), SIGNAL(replayStarted()), this, SLOT(replayStarted()));

    return true;
}

/**
 * The action that is triggered by the menu item which opens the
 * file and begins logging if successful
 */
void LoggingPlugin::toggleLogging()
{
    if (state == IDLE) {
        QString fileName = QFileDialog::getSaveFileName(NULL, tr("Start Log"),
                                                        tr("OP-%0.opl").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")),
                                                        tr("OpenPilot Log (*.opl)"));
        if (fileName.isEmpty()) {
            return;
        }

        startLogging(fileName);
    } else if (state == LOGGING) {
        stopLogging();
    }
}


/**
 * Starts the logging thread to a certain file
 */
void LoggingPlugin::startLogging(QString file)
{
    qDebug() << "LoggingPlugin - start logging to " << file;
    // needed ?
    stopLogging();
    // Start logging thread
    loggingThread = new LoggingThread();
    if (loggingThread->openFile(file)) {
        connect(loggingThread, &LoggingThread::finished, this, &LoggingPlugin::loggingStopped);
        loggingThread->start();
        loggingStarted();
    } else {
        delete loggingThread;
        loggingThread = NULL;
        QErrorMessage err;
        err.showMessage(tr("Unable to open file for logging"));
        err.exec();
    }
}

/**
 * Send the stop logging signal to the LoggingThread
 */
void LoggingPlugin::stopLogging()
{
    if (!loggingThread) {
        return;
    }

    qDebug() << "LoggingPlugin - stop logging";
    loggingThread->stopLogging();

    delete loggingThread;
    loggingThread = NULL;
}

void LoggingPlugin::loggingStarted()
{
    state = LOGGING;
    cmd->action()->setText(tr("Stop logging"));
    emit stateChanged("LOGGING");
}

/**
 * Receive the logging stopped signal from the LoggingThread
 * and change status to not logging
 */
void LoggingPlugin::loggingStopped()
{
    if (state == LOGGING) {
        state = IDLE;
        cmd->action()->setText(tr("Start logging..."));
        emit stateChanged("IDLE");
    }
}

/**
 * Received the replay stopped signal from the LogFile
 */
void LoggingPlugin::replayStopped()
{
    state = IDLE;
    emit stateChanged("IDLE");
}

/**
 * Received the replay started signal from the LogFile
 */
void LoggingPlugin::replayStarted()
{
    state = REPLAY;
    emit stateChanged("REPLAY");
}


void LoggingPlugin::extensionsInitialized()
{
    addAutoReleasedObject(logConnection);
}

void LoggingPlugin::shutdown()
{
    // Do nothing
}

/**
 * @}
 * @}
 */
