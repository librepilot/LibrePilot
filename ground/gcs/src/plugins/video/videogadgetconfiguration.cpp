/**
 ******************************************************************************
 *
 * @file       videogadgetconfiguration.cpp
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

VideoGadgetConfiguration::VideoGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    m_displayVideo       = settings.value("displayVideo").toBool();
    m_autoStart          = settings.value("autoStart").toBool();
    m_displayControls    = settings.value("displayControls").toBool();
    m_respectAspectRatio = settings.value("respectAspectRatio").toBool();
    m_pipelineDesc       = settings.value("pipelineDesc").toString();
    m_pipelineInfo       = settings.value("pipelineInfo").toString();
}

VideoGadgetConfiguration::VideoGadgetConfiguration(const VideoGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_displayVideo       = obj.m_displayVideo;
    m_autoStart          = obj.m_autoStart;
    m_displayControls    = obj.m_displayControls;
    m_respectAspectRatio = obj.m_respectAspectRatio;
    m_pipelineDesc       = obj.m_pipelineDesc;
    m_pipelineInfo       = obj.m_pipelineInfo;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *VideoGadgetConfiguration::clone() const
{
    return new VideoGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void VideoGadgetConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("displayVideo", m_displayVideo);
    settings.setValue("autoStart", m_autoStart);
    settings.setValue("displayControls", m_displayControls);
    settings.setValue("respectAspectRatio", m_respectAspectRatio);
    settings.setValue("pipelineDesc", m_pipelineDesc);
    settings.setValue("pipelineInfo", m_pipelineInfo);
}
