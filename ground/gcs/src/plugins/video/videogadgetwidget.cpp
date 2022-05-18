/**
 ******************************************************************************
 *
 * @file       videogadgetwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup VideoGadgetPlugin Video Gadget Plugin
 * @{
 * @brief A video gadget plugin
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
#include "videogadgetconfiguration.h"
#include "videogadgetwidget.h"
#include "pipeline.h"

#include <QtCore>
#include <QDebug>
#include <QStringList>
#include <QTextEdit>
#include <QPushButton>
#include <QWidget>

VideoGadgetWidget::VideoGadgetWidget(QWidget *parent) :
    QFrame(parent)
{
    m_ui = new Ui_Form();
    m_ui->setupUi(this);

    m_ui->consoleTextBrowser->setVisible(false);

    connect(videoWidget(), &VideoWidget::stateChanged, this, &VideoGadgetWidget::onStateChanged);
    connect(videoWidget(), &VideoWidget::message, this, &VideoGadgetWidget::msg);

    connect(m_ui->startButton, &QPushButton::clicked, this, &VideoGadgetWidget::start);
    connect(m_ui->pauseButton, &QPushButton::clicked, this, &VideoGadgetWidget::pause);
    connect(m_ui->stopButton, &QPushButton::clicked, this, &VideoGadgetWidget::stop);

    connect(m_ui->consoleButton, &QPushButton::clicked, this, &VideoGadgetWidget::console);

    onStateChanged(Pipeline::Null, Pipeline::Null, Pipeline::Null);
}

VideoGadgetWidget::~VideoGadgetWidget()
{
    m_ui = 0;
}

void VideoGadgetWidget::setConfiguration(VideoGadgetConfiguration *config)
{
    videoWidget()->setVisible(config->displayVideo());
    // m_ui->control->setEnabled(config->displayControls());
    bool restart = false;
    if (videoWidget()->pipelineDesc() != config->pipelineDesc()) {
        if (videoWidget()->isPlaying()) {
            restart = true;
            stop();
        }
        msg(QString("setting pipeline %0").arg(config->pipelineDesc()));
        videoWidget()->setPipelineDesc(config->pipelineDesc());
    }
    if (restart || (!videoWidget()->isPlaying() && config->autoStart())) {
        start();
    }
}

void VideoGadgetWidget::start()
{
    msg(QString("starting..."));
    m_ui->startButton->setEnabled(false);
    videoWidget()->start();
}

void VideoGadgetWidget::pause()
{
    msg(QString("pausing..."));
    m_ui->pauseButton->setEnabled(false);
    videoWidget()->pause();
}

void VideoGadgetWidget::stop()
{
    msg(QString("stopping..."));
    videoWidget()->stop();
}

void VideoGadgetWidget::console()
{
    m_ui->consoleTextBrowser->setVisible(!m_ui->consoleTextBrowser->isVisible());
}

void VideoGadgetWidget::onStateChanged(Pipeline::State oldState, Pipeline::State newState, Pipeline::State pendingState)
{
    Q_UNUSED(oldState);

    // msg(QString("state changed: ") + VideoWidget::name(oldState) + " -> " + VideoWidget::name(newState) + " / " + VideoWidget::name(pendingState));

    bool startEnabled = true;
    bool pauseEnabled = true;
    bool stopEnabled  = true;

    bool startVisible = false;
    bool pauseVisible = false;
    bool stopVisible  = true;

    switch (newState) {
    case Pipeline::Ready:
        // start & !stop
        startVisible = true;
        stopEnabled  = false;
        break;
    case Pipeline::Paused:
        if (pendingState == Pipeline::Playing) {
            // !pause & stop
            pauseVisible = true;
            pauseEnabled = false;
        } else if (pendingState == Pipeline::Ready) {
            // start & !stop
            startVisible = true;
            stopEnabled  = false;
        } else {
            // start & stop
            startVisible = true;
        }
        break;
    case Pipeline::Playing:
        // pause & stop
        pauseVisible = true;
        break;
    default:
        // start & !stop
        startVisible = true;
        stopEnabled  = false;
        break;
    }
    m_ui->startButton->setVisible(startVisible);
    m_ui->startButton->setEnabled(startEnabled);

    m_ui->pauseButton->setVisible(pauseVisible);
    m_ui->pauseButton->setEnabled(pauseEnabled);

    m_ui->stopButton->setVisible(stopVisible);
    m_ui->stopButton->setEnabled(stopEnabled);
}

void VideoGadgetWidget::msg(const QString &str)
{
    if (m_ui) {
        m_ui->consoleTextBrowser->append(str);
    }
}

VideoWidget *VideoGadgetWidget::videoWidget()
{
    return m_ui->video;
}
