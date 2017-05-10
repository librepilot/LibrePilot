/**
 ******************************************************************************
 *
 * @file       logfile.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
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
#include "logfile.h"

#include <QDebug>
#include <QtGlobal>

LogFile::LogFile(QObject *parent) : QIODevice(parent),
    m_timer(this),
    m_previousTimeStamp(0),
    m_nextTimeStamp(0),
    m_lastPlayed(0),
    m_timeOffset(0),
    m_playbackSpeed(1.0),
    m_useProvidedTimeStamp(false),
    m_providedTimeStamp(0)
{
    connect(&m_timer, &QTimer::timeout, this, &LogFile::timerFired);
}

bool LogFile::isSequential() const
{
    // returning true fixes "UAVTalk - error : bad type" errors when replaying a log file
    return true;
}

/**
 * Opens the logfile QIODevice and the underlying logfile. In case
 * we want to save the logfile, we open in WriteOnly. In case we
 * want to read the logfile, we open in ReadOnly.
 */
bool LogFile::open(OpenMode mode)
{
    // start a timer for playback
    m_myTime.restart();
    if (m_file.isOpen()) {
        // We end up here when doing a replay, because the connection
        // manager will also try to open the QIODevice, even though we just
        // opened it after selecting the file, which happens before the
        // connection manager call...
        return true;
    }
    qDebug() << "LogFile - open" << fileName();

    if (m_file.open(mode) == false) {
        qWarning() << "Unable to open " << m_file.fileName() << " for logging";
        return false;
    }

    // TODO: Write a header at the beginng describing objects so that in future
    // they can be read back if ID's change

    // Must call parent function for QIODevice to pass calls to writeData
    // We always open ReadWrite, because otherwise we will get tons of warnings
    // during a logfile replay. Read nature is checked upon write ops below.
    QIODevice::open(QIODevice::ReadWrite);

    return true;
}

void LogFile::close()
{
    qDebug() << "LogFile - close" << fileName();
    emit aboutToClose();
    m_file.close();
    QIODevice::close();
}

qint64 LogFile::writeData(const char *data, qint64 dataSize)
{
    if (!m_file.isWritable()) {
        return dataSize;
    }

    // If needed, use provided timestamp instead of the GCS timer
    // This is used when saving logs from on-board logging
    quint32 timeStamp = m_useProvidedTimeStamp ? m_providedTimeStamp : m_myTime.elapsed();

    m_file.write((char *)&timeStamp, sizeof(timeStamp));
    m_file.write((char *)&dataSize, sizeof(dataSize));

    qint64 written = m_file.write(data, dataSize);

    // flush (needed to avoid UAVTalk device full errors)
    m_file.flush();

    if (written != -1) {
        emit bytesWritten(written);
    }

    return dataSize;
}

qint64 LogFile::readData(char *data, qint64 maxlen)
{
    QMutexLocker locker(&m_mutex);

    qint64 len = qMin(maxlen, (qint64)m_dataBuffer.size());

    if (len) {
        memcpy(data, m_dataBuffer.data(), len);
        m_dataBuffer.remove(0, len);
    }

    return len;
}

qint64 LogFile::bytesAvailable() const
{
    QMutexLocker locker(&m_mutex);

    qint64 len = m_dataBuffer.size();

    return len;
}

void LogFile::timerFired()
{
    if (m_file.bytesAvailable() > 4) {
        int time;
        time = m_myTime.elapsed();

        // TODO: going back in time will be a problem
        while ((m_lastPlayed + ((double)(time - m_timeOffset) * m_playbackSpeed) > m_nextTimeStamp)) {
            m_lastPlayed += ((double)(time - m_timeOffset) * m_playbackSpeed);

            // read data size
            qint64 dataSize;
            if (m_file.bytesAvailable() < (qint64)sizeof(dataSize)) {
                qDebug() << "LogFile - end of log file reached";
                stopReplay();
                return;
            }
            m_file.read((char *)&dataSize, sizeof(dataSize));

            // check size consistency
            if (dataSize < 1 || dataSize > (1024 * 1024)) {
                qWarning() << "LogFile - corrupted log file! Unlikely packet size:" << dataSize;
                stopReplay();
                return;
            }

            // read data
            if (m_file.bytesAvailable() < dataSize) {
                qDebug() << "LogFile - end of log file reached";
                stopReplay();
                return;
            }
            QByteArray data = m_file.read(dataSize);

            // make data available
            m_mutex.lock();
            m_dataBuffer.append(data);
            m_mutex.unlock();

            emit readyRead();

            // read next timestamp
            if (m_file.bytesAvailable() < (qint64)sizeof(m_nextTimeStamp)) {
                qDebug() << "LogFile - end of log file reached";
                stopReplay();
                return;
            }
            m_previousTimeStamp = m_nextTimeStamp;
            m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

            // some validity checks
            if ((m_nextTimeStamp < m_previousTimeStamp) // logfile goes back in time
                || ((m_nextTimeStamp - m_previousTimeStamp) > 60 * 60 * 1000)) { // gap of more than 60 minutes
                qWarning() << "LogFile - corrupted log file! Unlikely timestamp:" << m_nextTimeStamp << "after" << m_previousTimeStamp;
                stopReplay();
                return;
            }

            m_timeOffset = time;
            time = m_myTime.elapsed();
        }
    } else {
        qDebug() << "LogFile - end of log file reached";
        stopReplay();
    }
}

bool LogFile::startReplay()
{
    if (!m_file.isOpen() || m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - startReplay";

    m_myTime.restart();
    m_timeOffset        = 0;
    m_lastPlayed        = 0;
    m_previousTimeStamp = 0;
    m_nextTimeStamp     = 0;
    m_dataBuffer.clear();

    // read next timestamp
    if (m_file.bytesAvailable() < (qint64)sizeof(m_nextTimeStamp)) {
        qWarning() << "LogFile - invalid log file!";
        return false;
    }
    m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

    m_timer.setInterval(10);
    m_timer.start();

    emit replayStarted();
    return true;
}

bool LogFile::stopReplay()
{
    if (!m_file.isOpen() || !m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - stopReplay";
    m_timer.stop();

    emit replayFinished();
    return true;
}

void LogFile::pauseReplay()
{
    m_timer.stop();
}

void LogFile::resumeReplay()
{
    m_timeOffset = m_myTime.elapsed();
    m_timer.start();
}
