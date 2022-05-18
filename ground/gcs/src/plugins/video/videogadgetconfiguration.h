/**
 ******************************************************************************
 *
 * @file       videogadgetconfiguration.h
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

#ifndef VIDEOGADGETCONFIGURATION_H
#define VIDEOGADGETCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>

using namespace Core;

class VideoGadgetConfiguration : public IUAVGadgetConfiguration {
    Q_OBJECT
public:
    explicit VideoGadgetConfiguration(QString classId, QSettings &Settings, QObject *parent = 0);
    explicit VideoGadgetConfiguration(const VideoGadgetConfiguration &obj);

    IUAVGadgetConfiguration *clone() const;
    void saveConfig(QSettings &settings) const;

    bool displayVideo() const
    {
        return m_displayVideo;
    }
    void setDisplayVideo(bool displayVideo)
    {
        m_displayVideo = displayVideo;
    }
    bool displayControls() const
    {
        return m_displayControls;
    }
    void setDisplayControls(bool displayControls)
    {
        m_displayControls = displayControls;
    }
    bool autoStart() const
    {
        return m_autoStart;
    }
    void setAutoStart(bool autoStart)
    {
        m_autoStart = autoStart;
    }
    bool respectAspectRatio() const
    {
        return m_respectAspectRatio;
    }
    void setRespectAspectRatio(bool respectAspectRatio)
    {
        m_respectAspectRatio = respectAspectRatio;
    }
    QString pipelineDesc() const
    {
        return m_pipelineDesc;
    }
    void setPipelineDesc(QString pipelineDesc)
    {
        m_pipelineDesc = pipelineDesc;
    }
    QString pipelineInfo() const
    {
        return m_pipelineInfo;
    }
    void setPipelineInfo(QString pipelineInfo)
    {
        m_pipelineInfo = pipelineInfo;
    }

private:
    // video
    bool m_displayVideo;
    bool m_respectAspectRatio;
    // controls
    bool m_displayControls;
    bool m_autoStart;
    QString m_pipelineDesc;
    QString m_pipelineInfo;
};

#endif // VIDEOGADGETCONFIGURATION_H
