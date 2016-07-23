/**
 ******************************************************************************
 *
 * @file       filelogger.h
 * @author     The LibrePilot Project, http://www.openpilot.org Copyright (C) 2016.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
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

#pragma once

#include "utils_global.h"

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QDebug>

class QTCREATOR_UTILS_EXPORT FileLogger : public QObject {
    Q_OBJECT

private:
    QTextStream * logStream;
    QThread *loggerThread;
    bool started;

public:
    FileLogger() : QObject(), logStream(NULL), loggerThread(NULL), started(false)
    {}

    virtual ~FileLogger()
    {
        stop();
        if (logStream) {
            delete logStream;
        }
    }

public:
    bool start(const QString &fileName)
    {
        if (started) {
            return false;
        }

        QFile *file = new QFile(fileName);
        if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        logStream    = new QTextStream(file);

        loggerThread = new QThread(this);
        moveToThread(loggerThread);
        loggerThread->start();

        started = true;

        return true;
    }

    bool stop()
    {
        if (!started) {
            return false;
        }
        // stop accepting messages
        started = false;

        // make sure all messages are flushed by sending a blocking message
        QtMsgType type    = QtDebugMsg;
        const QString msg = "stopping file logger";
        QMetaObject::invokeMethod(this, "doLog", Qt::BlockingQueuedConnection,
                                  Q_ARG(QtMsgType, type), Q_ARG(const QString &, msg));

        return true;
    }

    void log(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        Q_UNUSED(context);

        if (!started) {
            return;
        }

        QMetaObject::invokeMethod(this, "doLog", Qt::QueuedConnection,
                                  Q_ARG(QtMsgType, type), Q_ARG(const QString &, msg));
    }

private slots:
    void doLog(QtMsgType type, const QString &msg)
    {
        QTextStream &out = *logStream;

        // logStream << QTime::currentTime().toString("hh:mm:ss.zzz ");

        switch (type) {
        case QtDebugMsg:
            out << "DBG: ";
            break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        case QtInfoMsg:
            out << "INF: ";
            break;
#endif
        case QtWarningMsg:
            out << "WRN: ";
            break;
        case QtCriticalMsg:
            out << "CRT: ";
            break;
        case QtFatalMsg:
            out << "FTL: ";
            break;
        }

        out << msg << '\n';
        out.flush();
    }
};
