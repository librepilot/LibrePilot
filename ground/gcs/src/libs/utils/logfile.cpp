/**
 ******************************************************************************
 *
 * @file       logfile.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017-2018.
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

#define TIMESTAMP_SIZE_BYTES 4

LogFile::LogFile(QObject *parent) : QIODevice(parent),
    m_timer(this),
    m_previousTimeStamp(0),
    m_nextTimeStamp(0),
    m_lastPlayed(0),
    m_timeOffset(0),
    m_playbackSpeed(1.0),
    m_replayState(STOPPED),
    m_useProvidedTimeStamp(false),
    m_providedTimeStamp(0),
    m_beginTimeStamp(0),
    m_endTimeStamp(0),
    m_timerTick(0)
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
    if (m_replayState != PLAYING) {
        return;
    }
    m_timerTick++;

    if (m_file.bytesAvailable() > TIMESTAMP_SIZE_BYTES) {
        int time;
        time = m_myTime.elapsed();

        /*
            This code generates an advancing playback window. All samples that fit the window
            are replayed. The window is about the size of the timer interval: 10 ms.

            Description of used variables:

            time              : real-time interval since start of playback (in ms) - now()
            m_timeOffset      : real-time interval since start of playback (in ms) - when timerFired() was previously run
            m_nextTimeStamp   : read log until this log timestamp has been reached (in ms)
            m_lastPlayed      : log referenced timestamp advanced to during previous cycle (in ms)
            m_playbackSpeed   : 0.1 .. 1.0 .. 10 replay speedup factor

         */

        while (m_nextTimeStamp < (m_lastPlayed + (double)(time - m_timeOffset) * m_playbackSpeed)) {
            // advance the replay window for the next time period
            m_lastPlayed += ((double)(time - m_timeOffset) * m_playbackSpeed);

            // read data size
            qint64 dataSize;
            if (m_file.bytesAvailable() < (qint64)sizeof(dataSize)) {
                qDebug() << "LogFile replay - end of log file reached";
                resetReplay();
                return;
            }
            m_file.read((char *)&dataSize, sizeof(dataSize));

            // check size consistency
            if (dataSize < 1 || dataSize > (1024 * 1024)) {
                qWarning() << "LogFile replay - corrupted log file! Unlikely packet size:" << dataSize;
                stopReplay();
                return;
            }

            // read data
            if (m_file.bytesAvailable() < dataSize) {
                qDebug() << "LogFile replay - end of log file reached";
                resetReplay();
                return;
            }
            QByteArray data = m_file.read(dataSize);

            // make data available
            m_mutex.lock();
            m_dataBuffer.append(data);
            m_mutex.unlock();

            emit readyRead();

            // rate-limit slider bar position updates to 10 updates per second
            if (m_timerTick % 10 == 0) {
                emit playbackPositionChanged(m_nextTimeStamp);
            }
            // read next timestamp
            if (m_file.bytesAvailable() < (qint64)sizeof(m_nextTimeStamp)) {
                qDebug() << "LogFile replay - end of log file reached";
                resetReplay();
                return;
            }
            m_previousTimeStamp = m_nextTimeStamp;
            m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

            // some validity checks
            if ((m_nextTimeStamp < m_previousTimeStamp) // logfile goes back in time
                || ((m_nextTimeStamp - m_previousTimeStamp) > 60 * 60 * 1000)) { // gap of more than 60 minutes
                qWarning() << "LogFile replay - corrupted log file! Unlikely timestamp:" << m_nextTimeStamp << "after" << m_previousTimeStamp;
                stopReplay();
                return;
            }

            m_timeOffset = time;
            time = m_myTime.elapsed(); // number of milliseconds since start of playback
        }
    } else {
        qDebug() << "LogFile replay - end of log file reached";
        resetReplay();
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
 * This is required for correct control of the timer.
 *
 */
bool LogFile::startReplay()
{
    // Walk through logfile and create timestamp index
    // Don't start replay if there was a problem indexing the logfile.
    if (!buildIndex()) {
        return false;
    }

    m_timerTick = 0;

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
    m_replayState = PLAYING;

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
    if (!m_file.isOpen()) {
        return false;
    }
    if (m_timer.isActive()) {
        m_timer.stop();
    }

    qDebug() << "LogFile - stopReplay";
    m_replayState = STOPPED;

    emit replayFinished();
    return true;
}

/**
 * FUNCTION: resetReplay()
 *
 * Stops replaying the logfile.
 * Stops the timer: m_timer
 * Resets playback position to the start of the logfile
 * through the emission of a replayCompleted signal.
 *
 */
bool LogFile::resetReplay()
{
    if (!m_file.isOpen()) {
        return false;
    }
    if (m_timer.isActive()) {
        m_timer.stop();
    }

    qDebug() << "LogFile - resetReplay";
    m_replayState = STOPPED;

    emit replayCompleted();
    return true;
}

/**
 * SLOT: resumeReplay()
 *
 * Resumes replay from the given position.
 * If no position is given, resumes from the last position
 *
 */
bool LogFile::resumeReplay(quint32 desiredPosition)
{
    if (m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - resumeReplay";

    // Clear the playout buffer:
    m_mutex.lock();
    m_dataBuffer.clear();
    m_mutex.unlock();

    m_file.seek(0);

    /* Skip through the logfile until we reach the desired position.
       Looking for the next log timestamp after the desired position
       has the advantage that it skips over parts of the log
       where data might be missing.
     */
    for (int i = 0; i < m_timeStamps.size(); i++) {
        if (m_timeStamps.at(i) >= desiredPosition) {
            int bytesToSkip = m_timeStampPositions.at(i);
            bool seek_ok    = m_file.seek(bytesToSkip);
            if (!seek_ok) {
                qWarning() << "LogFile resumeReplay - an error occurred while seeking through the logfile.";
            }
            m_lastPlayed = m_timeStamps.at(i);
            break;
        }
    }
    m_file.read((char *)&m_nextTimeStamp, sizeof(m_nextTimeStamp));

    // Real-time timestamps don't not need to match the log timestamps.
    // However the delta between real-time variables "m_timeOffset" and "m_myTime" is important.
    // This delta determines the number of log entries replayed per cycle.

    // Set the real-time interval to 0 to start with:
    m_myTime.restart();
    m_timeOffset  = 0;

    m_replayState = PLAYING;

    m_timer.start();

    // Notify UI that playback has resumed
    emit replayStarted();
    return true;
}

/**
 * SLOT: pauseReplay()
 *
 * Pauses replay while storing the current playback position
 *
 */
bool LogFile::pauseReplay()
{
    if (!m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - pauseReplay";
    m_timer.stop();
    m_replayState = PAUSED;
    return true;
}

/**
 * SLOT: pauseReplayAndResetPosition()
 *
 * Pauses replay and resets the playback position to the start of the logfile
 *
 */
bool LogFile::pauseReplayAndResetPosition()
{
    if (!m_file.isOpen() || !m_timer.isActive()) {
        return false;
    }
    qDebug() << "LogFile - pauseReplayAndResetPosition";
    m_timer.stop();
    m_replayState       = STOPPED;

    m_timeOffset        = 0;
    m_lastPlayed        = m_timeStamps.at(0);
    m_previousTimeStamp = 0;
    m_nextTimeStamp     = 0;

    return true;
}

/**
 * FUNCTION: getReplayState()
 *
 * Returns the current replay status.
 *
 */
ReplayState LogFile::getReplayState()
{
    return m_replayState;
}

/**
 * FUNCTION: buildIndex()
 *
 * Walk through the opened logfile and stores the first and last position timestamps.
 * Also builds an index for quickly skipping to a specific position in the logfile.
 *
 * Returns true when indexing has completed successfully.
 * Returns false when a problem was encountered.
 *
 */
bool LogFile::buildIndex()
{
    quint32 timeStamp;
    qint64 totalSize;
    qint64 readPointer = 0;
    quint64 index = 0;
    int bytesRead = 0;

    qDebug() << "LogFile - buildIndex";

    // Ensure empty vectors:
    m_timeStampPositions.clear();
    m_timeStamps.clear();

    QByteArray arr = m_file.readAll();
    totalSize = arr.size();
    QDataStream dataStream(&arr, QIODevice::ReadOnly);

    // set the first timestamp
    if (totalSize - readPointer >= TIMESTAMP_SIZE_BYTES) {
        bytesRead = dataStream.readRawData((char *)&timeStamp, TIMESTAMP_SIZE_BYTES);
        if (bytesRead != TIMESTAMP_SIZE_BYTES) {
            qWarning() << "LogFile buildIndex - read first timeStamp: readRawData returned unexpected number of bytes:" << bytesRead << "at position" << readPointer << "\n";
            return false;
        }
        m_timeStamps.append(timeStamp);
        m_timeStampPositions.append(readPointer);
        readPointer     += TIMESTAMP_SIZE_BYTES;
        index++;
        m_beginTimeStamp = timeStamp;
        m_endTimeStamp   = timeStamp;
    }

    while (true) {
        qint64 dataSize;

        // Check if there are enough bytes remaining for a correct "dataSize" field
        if (totalSize - readPointer < (qint64)sizeof(dataSize)) {
            qWarning() << "LogFile buildIndex - logfile corrupted! Unexpected end of file";
            return false;
        }

        // Read the dataSize field and check for I/O errors
        bytesRead = dataStream.readRawData((char *)&dataSize, sizeof(dataSize));
        if (bytesRead != sizeof(dataSize)) {
            qWarning() << "LogFile buildIndex - read dataSize: readRawData returned unexpected number of bytes:" << bytesRead << "at position" << readPointer << "\n";
            return false;
        }

        readPointer += sizeof(dataSize);

        if (dataSize < 1 || dataSize > (1024 * 1024)) {
            qWarning() << "LogFile buildIndex - logfile corrupted! Unlikely packet size: " << dataSize << "\n";
            return false;
        }

        // Check if there are enough bytes remaining
        if (totalSize - readPointer < dataSize) {
            qWarning() << "LogFile buildIndex - logfile corrupted! Unexpected end of file";
            return false;
        }

        // skip reading the data (we don't need it at this point)
        readPointer += dataStream.skipRawData(dataSize);

        // read the next timestamp
        if (totalSize - readPointer >= TIMESTAMP_SIZE_BYTES) {
            bytesRead = dataStream.readRawData((char *)&timeStamp, TIMESTAMP_SIZE_BYTES);
            if (bytesRead != TIMESTAMP_SIZE_BYTES) {
                qWarning() << "LogFile buildIndex - read timeStamp, readRawData returned unexpected number of bytes:" << bytesRead << "at position" << readPointer << "\n";
                return false;
            }

            // some validity checks
            if (timeStamp < m_endTimeStamp // logfile goes back in time
                || (timeStamp - m_endTimeStamp) > (60 * 60 * 1000)) { // gap of more than 60 minutes)
                qWarning() << "LogFile buildIndex - logfile corrupted! Unlikely timestamp " << timeStamp << " after " << m_endTimeStamp;
                return false;
            }

            m_timeStamps.append(timeStamp);
            m_timeStampPositions.append(readPointer);
            readPointer   += TIMESTAMP_SIZE_BYTES;
            index++;
            m_endTimeStamp = timeStamp;
        } else {
            // Break without error (we expect to end at this location when we are at the end of the logfile)
            break;
        }
    }

    emit timesChanged(m_beginTimeStamp, m_endTimeStamp);

    // reset the read pointer to the start of the file
    m_file.seek(0);

    return true;
}
