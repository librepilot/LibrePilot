/**
 ******************************************************************************
 *
 * @file       GCSControlgadgetwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GCSControlGadgetPlugin GCSControl Gadget Plugin
 * @{
 * @brief A gadget to control the UAV, either from the keyboard or a joystick
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

    disableButtons();

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
    connect(m_logging->playPauseButton, &QPushButton::clicked, this, &LoggingGadgetWidget::playPauseButtonAction);
    connect(m_logging->stopButton, &QPushButton::clicked, this, &LoggingGadgetWidget::stopButtonAction);
    connect(m_logging->playBackPosition, &QSlider::valueChanged, this, &LoggingGadgetWidget::sliderMoved);

    connect(m_logging->playbackSpeed, static_cast<void(QDoubleSpinBox::*) (double)>(&QDoubleSpinBox::valueChanged), logFile, &LogFile::setReplaySpeed);

	// gadgetwidget functions to logfile actions
    connect(this, &LoggingGadgetWidget::resumeReplay, logFile, &LogFile::resumeReplay);
    connect(this, &LoggingGadgetWidget::pauseReplay, logFile, &LogFile::pauseReplay);
    connect(this, &LoggingGadgetWidget::pauseAndResetPosition, logFile, &LogFile::pauseAndResetPosition);

	// gadgetwidget functions to scope actions
    connect(this, &LoggingGadgetWidget::resumeReplay, scpPlugin, &ScopeGadgetFactory::startPlotting);
    connect(this, &LoggingGadgetWidget::pauseReplay, scpPlugin, &ScopeGadgetFactory::stopPlotting);
    connect(this, &LoggingGadgetWidget::pauseAndResetPosition, scpPlugin, &ScopeGadgetFactory::stopPlotting);

	// Feedback from logfile to GUI
    connect(loggingPlugin, &LoggingPlugin::stateChanged, this, &LoggingGadgetWidget::stateChanged);
    connect(logFile, &LogFile::updateBeginAndEndtimes, this, &LoggingGadgetWidget::updateBeginAndEndtimes);
    connect(logFile, &LogFile::playbackPosition, this, &LoggingGadgetWidget::playbackPosition);
    connect(logFile, &LogFile::replayStarted, this, &LoggingGadgetWidget::enableButtons);
    connect(logFile, &LogFile::replayFinished, this, &LoggingGadgetWidget::disableButtons);

	// Feedback from logfile to scope
    connect(logFile, &LogFile::replayFinished, scpPlugin, &ScopeGadgetFactory::stopPlotting);


    stateChanged(loggingPlugin->getState());
}


/*
    setPlayPauseButtonToPlay()

    Changes the appearance of the playPause button to the PLAY appearance.
*/
void LoggingGadgetWidget::setPlayPauseButtonToPlay()
{
    m_logging->playPauseButton->setIcon(QIcon(":/logging/images/play.png"));
    m_logging->playPauseButton->setText(tr("  Play"));
}

/*
    setPlayPauseButtonToPlay()

    Changes the appearance of the playPause button to the PAUSE appearance.
*/
void LoggingGadgetWidget::setPlayPauseButtonToPause()
{
    m_logging->playPauseButton->setIcon(QIcon(":/logging/images/pause.png"));
    m_logging->playPauseButton->setText(tr("  Pause"));
}

void LoggingGadgetWidget::playPauseButtonAction()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    if (replayState == PLAYING){
        emit pauseReplay();
        setPlayPauseButtonToPlay();
    } else {
        emit resumeReplay(m_logging->playBackPosition->value());
        setPlayPauseButtonToPause();
    }
    m_logging->stopButton->setEnabled(true);
}

void LoggingGadgetWidget::stopButtonAction()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    if (replayState != STOPPED) {
        emit pauseAndResetPosition();
    }
    m_logging->stopButton->setEnabled(false);
    setPlayPauseButtonToPlay();

    // Block signals while setting the slider to the start position
    m_logging->playBackPosition->blockSignals(true);
    m_logging->playBackPosition->setValue(m_logging->playBackPosition->minimum());
    m_logging->playBackPosition->blockSignals(false);
}

void LoggingGadgetWidget::stateChanged(LoggingPlugin::State state)
{
    QString status;
    bool enabled = false;

    switch (state) {
    case LoggingPlugin::IDLE:
        status  = tr("Idle");
        playbackPosition(0);
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
    m_logging->stopButton->setEnabled(enabled && playing);
    m_logging->playPauseButton->setEnabled(enabled);
    if (playing){
        setPlayPauseButtonToPause();
    } else {
        setPlayPauseButtonToPlay();
    }
}

void LoggingGadgetWidget::updateBeginAndEndtimes(quint32 startTimeStamp, quint32 endTimeStamp)
{
    int startSec, startMin, endSec, endMin;

    startSec = (startTimeStamp / 1000) % 60;
    startMin = startTimeStamp / (60 * 1000);

    endSec = (endTimeStamp / 1000) % 60;
    endMin = endTimeStamp / (60 * 1000);

    // update start and end labels
    m_logging->startTimeLabel->setText(QString("%1:%2").arg(startMin, 2, 10, QChar('0')).arg(startSec, 2, 10, QChar('0')));
    m_logging->endTimeLabel->setText(QString("%1:%2").arg(endMin, 2, 10, QChar('0')).arg(endSec, 2, 10, QChar('0')));

    // Update position bar
    m_logging->playBackPosition->setMinimum(startTimeStamp);
    m_logging->playBackPosition->setMaximum(endTimeStamp);

    m_logging->playBackPosition->setSingleStep((endTimeStamp - startTimeStamp) / 100);
    m_logging->playBackPosition->setPageStep((endTimeStamp - startTimeStamp) / 10);
    m_logging->playBackPosition->setTickInterval((endTimeStamp - startTimeStamp) / 10);
    m_logging->playBackPosition->setTickPosition(QSlider::TicksBothSides);
}

void LoggingGadgetWidget::playbackPosition(quint32 positionTimeStamp)
{
    // Update position bar, but only if the user is not updating the slider position
    if (!m_logging->playBackPosition->isSliderDown() && !sliderActionDelay.isActive()) {

        // Block signals during slider position update:
        m_logging->playBackPosition->blockSignals(true);
        m_logging->playBackPosition->setValue(positionTimeStamp);
        m_logging->playBackPosition->blockSignals(false);

        // update position label
        updatePositionLabel(positionTimeStamp);
    }
}

void LoggingGadgetWidget::enableButtons()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    switch (replayState)
    {
    case STOPPED:
        m_logging->stopButton->setEnabled(false);
        setPlayPauseButtonToPlay();
        break;

    case PLAYING:
        m_logging->stopButton->setEnabled(true);
        setPlayPauseButtonToPause();
        break;

    case PAUSED:
        m_logging->stopButton->setEnabled(true);
        setPlayPauseButtonToPlay();
        break;
    }
    m_logging->playPauseButton->setEnabled(true);
    m_logging->playBackPosition->setEnabled(true);
}

void LoggingGadgetWidget::disableButtons()
{

    m_logging->playPauseButton->setEnabled(false);
    setPlayPauseButtonToPlay();
    m_logging->stopButton->setEnabled(false);

    m_logging->playBackPosition->setEnabled(false);
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
    emit resumeReplay(m_logging->playBackPosition->value());
}



/**
 * @}
 * @}
 */
