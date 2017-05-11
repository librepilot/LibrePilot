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
}

LoggingGadgetWidget::~LoggingGadgetWidget()
{
    // Do nothing
}

void LoggingGadgetWidget::setPlugin(LoggingPlugin *p)
{
    loggingPlugin = p;

    connect(m_logging->playButton, &QPushButton::clicked, scpPlugin, &ScopeGadgetFactory::startPlotting);
    connect(m_logging->pauseButton, &QPushButton::clicked, scpPlugin, &ScopeGadgetFactory::stopPlotting);

    LogFile *logFile = loggingPlugin->getLogfile();
    connect(m_logging->playButton, &QPushButton::clicked, logFile, &LogFile::resumeReplay);
    connect(m_logging->pauseButton, &QPushButton::clicked, logFile, &LogFile::pauseReplay);
    connect(m_logging->playbackSpeed, static_cast<void(QDoubleSpinBox::*) (double)>(&QDoubleSpinBox::valueChanged), logFile, &LogFile::setReplaySpeed);

    connect(loggingPlugin, &LoggingPlugin::stateChanged, this, &LoggingGadgetWidget::stateChanged);

    stateChanged(loggingPlugin->getState());
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
}

/**
 * @}
 * @}
 */
