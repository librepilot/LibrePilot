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

    sliderActionDelay.setSingleShot(true);
    sliderActionDelay.setInterval(200); // Delay for 200 ms

    connect(&sliderActionDelay, SIGNAL(timeout()), this, SLOT(sendResumeReplayFrom()));

    m_storedPosition = 0;
}

LoggingGadgetWidget::~LoggingGadgetWidget()
{
    // Do nothing
}

void LoggingGadgetWidget::setPlugin(LoggingPlugin *p)
{
    loggingPlugin = p;

    connect(p->getLogfile(), SIGNAL(updateBeginAndEndtimes(quint32, quint32)), this, SLOT(updateBeginAndEndtimes(quint32, quint32)));
    connect(p->getLogfile(), SIGNAL(replayPosition(quint32)), this, SLOT(replayPosition(quint32)));
    connect(m_logging->playBackPosition, SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
    connect(this, SIGNAL(resumeReplayFrom(quint32)), p->getLogfile(), SLOT(resumeReplayFrom(quint32)));

    connect(this, SIGNAL(startReplay()), p->getLogfile(), SLOT(restartReplay()));
    connect(this, SIGNAL(stopReplay()), p->getLogfile(), SLOT(haltReplay()));
    connect(this, SIGNAL(pauseReplay()), p->getLogfile(), SLOT(pauseReplay()));
    connect(this, SIGNAL(resumeReplay()), p->getLogfile(), SLOT(resumeReplay()));

    connect(this, SIGNAL(startReplay()), scpPlugin, SLOT(startPlotting()));
    connect(this, SIGNAL(stopReplay()), scpPlugin, SLOT(stopPlotting()));
    connect(this, SIGNAL(pauseReplay()), scpPlugin, SLOT(stopPlotting()));
    connect(this, SIGNAL(resumeReplay()), scpPlugin, SLOT(startPlotting()));

    connect(m_logging->playButton, SIGNAL(clicked()), this, SLOT(playButton()));
    connect(m_logging->pauseButton, SIGNAL(clicked()), this, SLOT(pauseButton()));
    connect(m_logging->stopButton, SIGNAL(clicked()), this, SLOT(stopButton()));

    connect(p->getLogfile(), SIGNAL(replayStarted()), this, SLOT(enableButtons()));
    connect(p->getLogfile(), SIGNAL(replayFinished()), this, SLOT(disableButtons()));
    connect(p->getLogfile(), SIGNAL(replayFinished()), scpPlugin, SLOT(stopPlotting()));

    connect(m_logging->playbackSpeed, SIGNAL(valueChanged(double)), p->getLogfile(), SLOT(setReplaySpeed(double)));

    connect(m_logging->playButton, &QPushButton::clicked, scpPlugin, &ScopeGadgetFactory::startPlotting);
    connect(m_logging->pauseButton, &QPushButton::clicked, scpPlugin, &ScopeGadgetFactory::stopPlotting);

    LogFile *logFile = loggingPlugin->getLogfile();
    connect(m_logging->playButton, &QPushButton::clicked, logFile, &LogFile::resumeReplay);
    connect(m_logging->pauseButton, &QPushButton::clicked, logFile, &LogFile::pauseReplay);
    connect(m_logging->playbackSpeed, static_cast<void(QDoubleSpinBox::*) (double)>(&QDoubleSpinBox::valueChanged), logFile, &LogFile::setReplaySpeed);

    connect(loggingPlugin, &LoggingPlugin::stateChanged, this, &LoggingGadgetWidget::stateChanged);

    stateChanged(loggingPlugin->getState());
}

void LoggingGadgetWidget::playButton()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    if (replayState == STOPPED) {
        emit startReplay();
    } else if (replayState == PAUSED) {
        emit resumeReplay();
    }
    m_logging->playButton->setEnabled(false);
    m_logging->pauseButton->setEnabled(true);
    m_logging->stopButton->setEnabled(true);
}

void LoggingGadgetWidget::pauseButton()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    if (replayState == PLAYING) {
        emit pauseReplay();
    }
    m_logging->playButton->setEnabled(true);
    m_logging->pauseButton->setEnabled(false);
    m_logging->stopButton->setEnabled(true);
}

void LoggingGadgetWidget::stopButton()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    if (replayState != STOPPED) {
        emit stopReplay();
    }
    m_logging->playButton->setEnabled(true);
    m_logging->pauseButton->setEnabled(false);
    m_logging->stopButton->setEnabled(false);

    replayPosition(0);
}

void LoggingGadgetWidget::stateChanged(LoggingPlugin::State state)
{
    QString status;
    bool enabled = false;

    switch (state) {
    case LoggingPlugin::IDLE:
        status  = tr("Idle");
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
    m_logging->playButton->setEnabled(enabled && !playing);
    m_logging->pauseButton->setEnabled(enabled && playing);
    m_logging->stopButton->setEnabled(enabled && playing);
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

void LoggingGadgetWidget::replayPosition(quint32 positionTimeStamp)
{
    // Update position bar, but only if the user is not updating the slider position
    if (!m_logging->playBackPosition->isSliderDown() && !sliderActionDelay.isActive()) {
        // Block signals during slider position update:
        m_logging->playBackPosition->blockSignals(true);
        m_logging->playBackPosition->setValue(positionTimeStamp);
        m_logging->playBackPosition->blockSignals(false);

        // update current position label
        updatePositionLabel(positionTimeStamp);
    }
}

void LoggingGadgetWidget::enableButtons()
{
    ReplayState replayState = (loggingPlugin->getLogfile())->getReplayStatus();

    switch (replayState)
    {
    case STOPPED:
        m_logging->playButton->setEnabled(true);
        m_logging->pauseButton->setEnabled(false);
        m_logging->stopButton->setEnabled(false);
        break;

    case PLAYING:
        m_logging->playButton->setEnabled(false);
        m_logging->pauseButton->setEnabled(true);
        m_logging->stopButton->setEnabled(true);
        break;

    case PAUSED:
        m_logging->playButton->setEnabled(true);
        m_logging->pauseButton->setEnabled(false);
        m_logging->stopButton->setEnabled(true);
        break;
    }
    m_logging->playBackPosition->setEnabled(true);
}

void LoggingGadgetWidget::disableButtons()
{
//    m_logging->startTimeLabel->setText(QString(""));
//    m_logging->endTimeLabel->setText(QString(""));

    m_logging->playButton->setEnabled(false);
    m_logging->pauseButton->setEnabled(false);
    m_logging->stopButton->setEnabled(false);

    m_logging->playBackPosition->setEnabled(false);
}

void LoggingGadgetWidget::sliderMoved(int position)
{
    qDebug() << "sliderMoved(): start of function, stored position was: " << m_storedPosition;

    m_storedPosition = position;
    // pause
    emit pauseButton();

    updatePositionLabel(position);

    // Start or restarts a time-out after which replay is resumed from the new position.
    sliderActionDelay.start();

    qDebug() << "sliderMoved(): end of function, stored position is now: " << m_storedPosition;
}

void LoggingGadgetWidget::updatePositionLabel(quint32 positionTimeStamp)
{
    // update position timestamp label
    int sec = (positionTimeStamp / 1000) % 60;
    int min = positionTimeStamp / (60 * 1000);
    m_logging->positionTimestampLabel->setText(QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
}

void LoggingGadgetWidget::sendResumeReplayFrom()
{
    qDebug() << "sendResumeReplayFrom(): start of function, stored position is: " << m_storedPosition;

    emit resumeReplayFrom(m_storedPosition);

    emit resumeReplay();

    qDebug() << "sendResumeReplayFrom(): end of function, stored position is: " << m_storedPosition;
}


/**
 * @}
 * @}
 */
