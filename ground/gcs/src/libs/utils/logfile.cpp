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
#include <QDataStream>
#include <QThread> // DEBUG: to display the thread ID

LogFile::LogFile(QObject *parent) : QIODevice(parent),
    m_timer(this),
    m_previousTimeStamp(0),
    m_nextTimeStamp(0),
    m_lastPlayed(0),
    m_timeOffset(0),
    m_playbackSpeed(1.0),
    m_replayStatus(STOPPED),
    m_useProvidedTimeStamp(false),
    m_providedTimeStamp(0),
    m_beginTimeStamp(0),
    m_endTimeStamp(0),
    m_timer_tick(0)
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

/**
    timerFired()

    This function is called at a 10 ms interval to fill the replay buffers.

 */

void LogFile::timerFired()
{
    if (m_replayStatus != PLAYING) {
        return;
    }

    m_timer_tick++;
    if ( m_timer_tick % 100 == 0 ) {
      qDebug() << "----------------------------------------------------------";
      qDebug() << "LogFile::timerFired() -> Tick = " << m_timer_tick;
    }

    if (m_file.bytesAvailable() > 4) {
        int time;
        time = m_myTime.elapsed();

        /*
            This code generates an advancing window. All samples that fit in the window
            are replayed. The window is about the size of the timer interval: 10 ms.
  
            Description of used variables:

            time              : time passed since start of playback (in ms) - current
            m_timeOffset      : time passed since start of playback (in ms) - when timerFired() was previously run
            m_lastPlayed      : next log timestamp to advance to (in ms)
            m_nextTimeStamp   : timestamp of most recently read log entry (in ms)
            m_playbackSpeed   : 1 .. 10 replay speedup factor

         */

        while ( m_nextTimeStamp < (m_lastPlayed + (double)(time - m_timeOffset) * m_playbackSpeed) ) {
//            if ( m_timer_tick % 100 == 0 ) {
//            if ( true ) {
//              qDebug() << "LogFile::timerFired() -> m_lastPlayed    = " << m_lastPlayed;
//              qDebug() << "LogFile::timerFired() -> m_nextTimeStamp = " << m_nextTimeStamp;
//              qDebug() << "LogFile::timerFired() -> time            = " << time;
//              qDebug() << "LogFile::timerFired() -> m_timeOffset    = " << m_timeOffset;
//              qDebug() << "---";
//              qDebug() << "LogFile::timerFired() -> m_nextTimeStamp = " << m_nextTimeStamp;
//              qDebug() << "LogFile::timerFired() -> (m_lastPlayed + (double)(time - m_timeOffset) * m_playbackSpeed) = " << (m_lastPlayed + (double)(time - m_timeOffset) * m_playbackSpeed);
//              qDebug() << "---";
//            }

            // advance the replay window for the next time period

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

            // rate-limit slider bar position updates to 10 updates per second
            if (m_timer_tick % 10 == 0) {
                emit replayPosition(m_nextTimeStamp);
            }
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
            time = m_myTime.elapsed(); // number of milliseconds since start of playback
        }
    } else {
        qDebug() << "LogFile - end of log file reached";
        stopReplay();
    }
}

bool LogFile::isPlaying() const
{
    return m_file.isOpen() && m_timer.isActive();
}

/**
 * FUNCTION: startReplay()
 *
 * Starts replaying a newly opened logfile.
 * Starts a timer: m_timer
 *
 * This function and the stopReplay() function should only ever be called from the same thread.
 * This is required for correctly controlling the timer.
 *
 */
bool LogFile::startReplay()
{
    qDebug() << "startReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    // Walk through logfile and create timestamp index
    // Don't start replay if there was a problem indexing the logfile.
    if (!buildIndex()) {
        return false;
    }

    m_timer_tick = 0;

    if (!m_file.isOpen() || m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - startReplay";

    m_myTime.restart();
    m_timeOffset        = 0;
    m_lastPlayed        = 0;
    m_previousTimeStamp = 0;
    m_nextTimeStamp     = 0;
    m_mutex.lock();
    m_dataBuffer.clear();
    m_mutex.unlock();

    // read next timestamp
    if (m_file.bytesAvailable() < (qint64)sizeof(m_nextTimeStamp)) {
        qWarning() << "LogFile - invalid log file!";
        return false;
    }
    m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

    m_timer.setInterval(10);
    m_timer.start();
    m_replayStatus = PLAYING;

    emit replayStarted();
    return true;
}

/**
 * FUNCTION: stopReplay()
 *
 * Stops replaying the logfile.
 * Stops the timer: m_timer
 *
 * This function and the startReplay() function should only ever be called from the same thread.
 * This is a requirement to be able to control the timer.
 *
 */
bool LogFile::stopReplay()
{
    qDebug() << "stopReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    if (!m_file.isOpen() || !m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - stopReplay";
    m_timer.stop();
    m_replayStatus = STOPPED;

    emit replayFinished();
    return true;
}


/**
 * SLOT: restartReplay()
 *
 * This function starts replay from the begining of the currently opened logfile.
 *
 */
void LogFile::restartReplay()
{
    qDebug() << "restartReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    resumeReplayFrom(0);

    qDebug() << "restartReplay(): end of function, current Thread ID is: " << QThread::currentThreadId();
}

/**
 * SLOT: haltReplay()
 *
 * Stops replay without storing the current playback position
 *
 */
void LogFile::haltReplay()
{
    qDebug() << "haltReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    qDebug() << "haltReplay() time = m_myTime.elapsed() = " << m_myTime.elapsed();
    qDebug() << "haltReplay() m_timeOffset = " << m_timeOffset;
    qDebug() << "haltReplay() m_nextTimeStamp = " << m_nextTimeStamp;
    qDebug() << "haltReplay() m_lastPlayed = " << m_lastPlayed;

    m_replayStatus = STOPPED;

    qDebug() << "haltReplay(): end of function, current Thread ID is: " << QThread::currentThreadId();
}
/**
 * SLOT: pauseReplay()
 *
 * Pauses replay while storing the current playback position
 *
 */
bool LogFile::pauseReplay()
{
    qDebug() << "pauseReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    qDebug() << "pauseReplay() time = m_myTime.elapsed() = " << m_myTime.elapsed();
    qDebug() << "pauseReplay() m_timeOffset = " << m_timeOffset;
    qDebug() << "pauseReplay() m_nextTimeStamp = " << m_nextTimeStamp;
    qDebug() << "pauseReplay() m_lastPlayed = " << m_lastPlayed;

    if (!m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - pauseReplay";
    m_timer.stop();
    m_replayStatus = PAUSED;

    // hack to notify UI that replay paused
    emit replayStarted();
    return true;
}

/**
 * SLOT: resumeReplay()
 *
 * Resumes replay from the stored playback position
 *
 */
bool LogFile::resumeReplay()
{
    qDebug() << "resumeReplay(): start of function, current Thread ID is: " << QThread::currentThreadId();

    m_mutex.lock();
    m_dataBuffer.clear();
    m_mutex.unlock();

    m_file.seek(0);

    for (int i = 0; i < m_timeStamps.size(); ++i) {
        if (m_timeStamps.at(i) >= m_lastPlayed) {
            m_file.seek(m_timeStampPositions.at(i));
            break;
        }
    }
    m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

    m_myTime.restart();
    m_myTime = m_myTime.addMSecs(-m_timeOffset); // Set startpoint this far back in time.

    qDebug() << "resumeReplay() time = m_myTime.elapsed() = " << m_myTime.elapsed();
    qDebug() << "resumeReplay() m_timeOffset = " << m_timeOffset;
    qDebug() << "resumeReplay() m_nextTimeStamp = " << m_nextTimeStamp;
    qDebug() << "resumeReplay() m_lastPlayed = " << m_lastPlayed;

    qDebug() << "resumeReplay(): end of function, current Thread ID is: " << QThread::currentThreadId();
    if (m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - resumeReplay";
    m_timeOffset = m_myTime.elapsed();
    m_timer.start();
    m_replayStatus = PLAYING;

    // Notify UI that replay has been resumed
    emit replayStarted();
    return true;
}

/**
 * SLOT: resumeReplayFrom()
 *
 * Resumes replay from the given position
 *
 */
void LogFile::resumeReplayFrom(quint32 desiredPosition)
{
    qDebug() << "resumeReplayFrom(): start of function, current Thread ID is: " << QThread::currentThreadId();

    m_mutex.lock();
    m_dataBuffer.clear();
    m_mutex.unlock();

    m_file.seek(0);

    qint32 i;
    for (i = 0; i < m_timeStamps.size(); ++i) {
        if (m_timeStamps.at(i) >= desiredPosition) {
            m_file.seek(m_timeStampPositions.at(i));
            m_lastPlayed = m_timeStamps.at(i);
            break;
        }
    }
    m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

    if (m_nextTimeStamp != m_timeStamps.at(i)) {
        qDebug() << "resumeReplayFrom() m_nextTimeStamp != m_timeStamps.at(i) -> " << m_nextTimeStamp << " != " << m_timeStamps.at(i);
    }

//    m_timeOffset = (m_lastPlayed - m_nextTimeStamp) / m_playbackSpeed;
    m_timeOffset = 0;

    m_myTime.restart();
//    m_myTime = m_myTime.addMSecs(-m_timeOffset); // Set startpoint this far back in time.
    // TODO: The above line is a possible memory leak. I'm not sure how to handle this correctly.

    qDebug() << "resumeReplayFrom() time = m_myTime.elapsed() = " << m_myTime.elapsed();
    qDebug() << "resumeReplayFrom() m_timeOffset = " << m_timeOffset;
    qDebug() << "resumeReplayFrom() m_nextTimeStamp = " << m_nextTimeStamp;
    qDebug() << "resumeReplayFrom() m_lastPlayed = " << m_lastPlayed;

    m_replayStatus = PLAYING;
    emit replayStarted();

    qDebug() << "resumeReplayFrom(): end of function, current Thread ID is: " << QThread::currentThreadId();
}

/**
 * FUNCTION: getReplayStatus()
 *
 * Returns the current replay status.
 *
 */
ReplayState LogFile::getReplayStatus()
{
    return m_replayStatus;
}

/**
 * FUNCTION: buildIndex()
 *
 * Walk through the opened logfile and sets the start and end position timestamps
 * Also builds an index for quickly skipping to a specific position in the logfile.
 *
 * returns true when indexing has completed successfully
 * returns false when a problem was encountered
 *
 */
bool LogFile::buildIndex()
{
    quint32 timeStamp;
    qint64 totalSize;
    qint64 readPointer = 0;
    quint64 index = 0;

    QByteArray arr     = m_file.readAll();

    totalSize = arr.size();
    QDataStream dataStream(&arr, QIODevice::ReadOnly);

    // set the start timestamp
    if (totalSize - readPointer >= 4) {
        dataStream.readRawData((char *)&timeStamp, 4);
        m_timeStamps.append(timeStamp);
        m_timeStampPositions.append(readPointer);
        qDebug() << "LogFile::buildIndex() element index = " << index << " \t-> timestamp = " << timeStamp << " \t-> bytes in file = " << readPointer;
        readPointer += 4;
        index++;
        m_beginTimeStamp = timeStamp;
        m_endTimeStamp = timeStamp;
    }

    while (true) {
        qint64 dataSize;

        // Check if there are enough bytes remaining for a correct "dataSize" field
        if (totalSize - readPointer < (qint64)sizeof(dataSize)) {
            qDebug() << "Error: Logfile corrupted! Unexpected end of file";
            return false;
        }

        // Read the dataSize field
        dataStream.readRawData((char *)&dataSize, sizeof(dataSize));
        readPointer += sizeof(dataSize);

        if (dataSize < 1 || dataSize > (1024 * 1024)) {
            qDebug() << "Error: Logfile corrupted! Unlikely packet size: " << dataSize << "\n";
            return false;
        }

        // Check if there are enough bytes remaining
        if (totalSize - readPointer < dataSize) {
            qDebug() << "Error: Logfile corrupted! Unexpected end of file";
            return false;
        }

        // skip reading the data (we don't need it at this point)
        readPointer += dataStream.skipRawData(dataSize);

        // read the next timestamp
        if (totalSize - readPointer >= 4) {
            dataStream.readRawData((char *)&timeStamp, 4);
            qDebug() << "LogFile::buildIndex() element index = " << index << " \t-> timestamp = " << timeStamp << " \t-> bytes in file = " << readPointer;

            // some validity checks
            if (timeStamp < m_endTimeStamp // logfile goes back in time
                || (timeStamp - m_endTimeStamp) > (60 * 60 * 1000)) { // gap of more than 60 minutes)
                qDebug() << "Error: Logfile corrupted! Unlikely timestamp " << timeStamp << " after " << m_endTimeStamp;
//                return false;
            }

            m_timeStamps.append(timeStamp);
            m_timeStampPositions.append(readPointer);
            readPointer += 4;
            index++;
            m_endTimeStamp = timeStamp;
        } else {
            // Break without error (we expect to end at this location when we are at the end of the logfile)
            break;
        }
    }

    qDebug() << "buildIndex() -> first timestamp in log = " << m_beginTimeStamp;
    qDebug() << "buildIndex() -> last timestamp in log = " << m_endTimeStamp;

    emit updateBeginAndEndtimes(m_beginTimeStamp, m_endTimeStamp);

    // reset the read pointer to the start of the file
    m_file.seek(0);

    return true;
}

/**
 * FUNCTION: setReplaySpeed()
 *
 * Update the replay speed.
 *
 * FIXME: currently, changing the replay speed, while skipping through the logfile
 * with the position bar causes position alignment to be lost.
 * 
 */
void LogFile::setReplaySpeed(double val)
{
    m_playbackSpeed = val;
    qDebug() << "Playback speed is now " << QString("%1").arg(m_playbackSpeed, 4, 'f', 2, QChar('0'));
}


