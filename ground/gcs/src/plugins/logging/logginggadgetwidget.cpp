/**
 ******************************************************************************
 *
 * @file       logginggadgetwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2018.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup LoggingGadgetPlugin Logging Gadget Plugin
 * @{
 * @brief      A gadget to control playback of a GCS log.
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
#include "logginggadgetwidget.h"

#include "ui_logging.h"

#include <loggingplugin.h>

#include <QWidget>
#include <QPushButton>

LoggingGadgetWidget::LoggingGadgetWidget(QWidget *parent) : QWidget(parent), loggingPlugin(NULL)
{
    m_logging = new Ui_Logging();
    m_logging->setupUi(this);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    scpPlugin = pm->getObject<ScopeGadgetFactory>();

    disableWidgets();

    // Configure timer to delay application of slider position action for 200ms
    sliderActionDelay.setSingleShot(true);
    sliderActionDelay.setInterval(200);

    connect(&sliderActionDelay, SIGNAL(timeout()), this, SLOT(sliderAction()));
}

LoggingGadgetWidget::~LoggingGadgetWidget()
{
    // Do nothing
}

void LoggingGadgetWidget::setPlugin(LoggingPlugin *p)
{
    loggingPlugin = p;
    LogFile *logFile = loggingPlugin->getLogfile();

    // GUI elements to gadgetwidget functions
    connect(m_logging->playButton, &QPushButton::clicked, this, &LoggingGadgetWidget::playButtonAction);
    connect(m_logging->pauseButton, &QPushButton::clicked, this, &LoggingGadgetWidget::pauseButtonAction);
    connect(m_logging->stopButton, &QPushButton::clicked, this, &LoggingGadgetWidget::stopButtonAction);
    connect(m_logging->playbackPosition, &QSlider::valueChanged, this, &LoggingGadgetWidget::sliderMoved);

    connect(m_logging->playbackSpeed, static_cast<void(QDoubleSpinBox::*) (double)>(&QDoubleSpinBox::valueChanged), logFile, &LogFile::setReplaySpeed);

    // gadgetwidget functions to logfile actions
    connect(this, &LoggingGadgetWidget::resumeReplay, logFile, &LogFile::resumeReplay);
    connect(this, &LoggingGadgetWidget::pauseReplay, logFile, &LogFile::pauseReplay);
    connect(this, &LoggingGadgetWidget::pauseReplayAndResetPosition, logFile, &LogFile::pauseReplayAndResetPosition);

    // gadgetwidget functions to scope actions
    connect(this, &LoggingGadgetWidget::resumeReplay, scpPlugin, &ScopeGadgetFactory::startPlotting);
    connect(this, &LoggingGadgetWidget::pauseReplay, scpPlugin, &ScopeGadgetFactory::stopPlotting);
    connect(this, &LoggingGadgetWidget::pauseReplayAndResetPosition, scpPlugin, &ScopeGadgetFactory::stopPlotting);

    // Feedback from logfile to GUI
    connect(loggingPlugin, &LoggingPlugin::stateChanged, this, &LoggingGadgetWidget::stateChanged);
    connect(logFile, &LogFile::timesChanged, this, &LoggingGadgetWidget::setBeginAndEndTimes);
    connect(logFile, &LogFile::playbackPositionChanged, this, &LoggingGadgetWidget::setPlaybackPosition);
    connect(logFile, &LogFile::replayStarted, this, &LoggingGadgetWidget::enableWidgets);
    connect(logFile, &LogFile::replayFinished, this, &LoggingGadgetWidget::disableWidgets);
    connect(logFile, &LogFile::replayCompleted, this, &LoggingGadgetWidget::stopButtonAction);

    // Feedback from logfile to scope
    connect(logFile, &LogFile::replayFinished, scpPlugin, &ScopeGadgetFactory::stopPlotting);

    // Perform actions as if the plugin state has been changed
    stateChanged(loggingPlugin->getState());
}

void LoggingGadgetWidget::playButtonAction()
{
    ReplayState replayState = loggingPlugin->getLogfile()->getReplayState();

    if (replayState != PLAYING) {
        emit resumeReplay(m_logging->playbackPosition->value());
    }

    m_logging->playButton->setVisible(false);
    m_logging->pauseButton->setVisible(true);
    m_logging->stopButton->setEnabled(true);
}

void LoggingGadgetWidget::pauseButtonAction()
{
    ReplayState replayState = loggingPlugin->getLogfile()->getReplayState();

    if (replayState == PLAYING) {
        emit pauseReplay();
    }

    m_logging->playButton->setVisible(true);
    m_logging->pauseButton->setVisible(false);
    m_logging->stopButton->setEnabled(true);

    m_logging->statusLabel->setText(tr("Paused"));
}

void LoggingGadgetWidget::stopButtonAction()
{
    emit pauseReplayAndResetPosition();

    m_logging->playButton->setVisible(true);
    m_logging->pauseButton->setVisible(false);
    m_logging->stopButton->setEnabled(false);

    setPlaybackPosition(0);

    m_logging->statusLabel->setText(tr("Stopped"));
}

void LoggingGadgetWidget::stateChanged(LoggingPlugin::State state)
{
    QString status;
    bool enabled = false;

    switch (state) {
    case LoggingPlugin::IDLE:
        status = tr("Idle");
        setPlaybackPosition(0);
        break;
    case LoggingPlugin::LOGGING:
        status  = tr("Logging");
        break;
    case LoggingPlugin::REPLAY:
        status  = tr("Replaying");
        enabled = true;
        break;
    }
    m_logging->statusLabel->setText(status);

    bool playing = loggingPlugin->getLogfile()->isPlaying();
    m_logging->playButton->setEnabled(enabled);
    m_logging->pauseButton->setEnabled(enabled);
    m_logging->stopButton->setEnabled(enabled && playing);

    if (playing) {
        m_logging->playButton->setVisible(false);
        m_logging->pauseButton->setVisible(true);
    } else {
        m_logging->playButton->setVisible(true);
        m_logging->pauseButton->setVisible(false);
    }
}

void LoggingGadgetWidget::setBeginAndEndTimes(quint32 startTimeStamp, quint32 endTimeStamp)
{
    int startSec, startMin, endSec, endMin;

    startSec = (startTimeStamp / 1000) % 60;
    startMin = startTimeStamp / (60 * 1000);

    endSec   = (endTimeStamp / 1000) % 60;
    endMin   = endTimeStamp / (60 * 1000);

    // update start and end labels
    m_logging->startTimeLabel->setText(QString("%1:%2").arg(startMin, 2, 10, QChar('0')).arg(startSec, 2, 10, QChar('0')));
    m_logging->endTimeLabel->setText(QString("%1:%2").arg(endMin, 2, 10, QChar('0')).arg(endSec, 2, 10, QChar('0')));

    // Update position bar
    m_logging->playbackPosition->setMinimum(startTimeStamp);
    m_logging->playbackPosition->setMaximum(endTimeStamp);

    m_logging->playbackPosition->setSingleStep((endTimeStamp - startTimeStamp) / 100);
    m_logging->playbackPosition->setPageStep((endTimeStamp - startTimeStamp) / 10);
}

void LoggingGadgetWidget::setPlaybackPosition(quint32 positionTimeStamp)
{
    // Update position bar, but only if the user is not updating the slider position
    if (!m_logging->playbackPosition->isSliderDown() && !sliderActionDelay.isActive()) {
        // Block signals during slider position update:
        m_logging->playbackPosition->blockSignals(true);
        m_logging->playbackPosition->setValue(positionTimeStamp);
        m_logging->playbackPosition->blockSignals(false);

        // update position label
        updatePositionLabel(positionTimeStamp);
    }
}

void LoggingGadgetWidget::enableWidgets()
{
    ReplayState replayState = loggingPlugin->getLogfile()->getReplayState();

    m_logging->playButton->setEnabled(true);
    m_logging->pauseButton->setEnabled(true);

    switch (replayState) {
    case STOPPED:
        m_logging->stopButton->setEnabled(false);
        m_logging->playButton->setVisible(true);
        m_logging->pauseButton->setVisible(false);
        break;

    case PLAYING:
        m_logging->stopButton->setEnabled(true);
        m_logging->playButton->setVisible(false);
        m_logging->pauseButton->setVisible(true);
        break;

    case PAUSED:
        m_logging->stopButton->setEnabled(true);
        m_logging->playButton->setVisible(true);
        m_logging->pauseButton->setVisible(false);
        break;
    }
    m_logging->playbackPosition->setEnabled(true);
}

void LoggingGadgetWidget::disableWidgets()
{
    m_logging->playButton->setVisible(true);
    m_logging->pauseButton->setVisible(false);

    m_logging->playButton->setEnabled(false);
    m_logging->pauseButton->setEnabled(false);
    m_logging->stopButton->setEnabled(false);

    m_logging->playbackPosition->setEnabled(false);

    // reset start and end labels
    m_logging->startTimeLabel->setText("");
    m_logging->endTimeLabel->setText("");
}

void LoggingGadgetWidget::updatePositionLabel(quint32 positionTimeStamp)
{
    // update position label -> MM:SS
    int sec = (positionTimeStamp / 1000) % 60;
    int min = positionTimeStamp / (60 * 1000);

    m_logging->positionTimestampLabel->setText(QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
}

void LoggingGadgetWidget::sliderMoved(int position)
{
    // pause playback while the user is dragging the slider to change position
    emit pauseReplay();

    updatePositionLabel(position);

    // Start or restarts a time-out after which replay will resume from the new position.
    sliderActionDelay.start();
}

void LoggingGadgetWidget::sliderAction()
{
    emit resumeReplay(m_logging->playbackPosition->value());
}


/**
 * @}
 * @}
 */
