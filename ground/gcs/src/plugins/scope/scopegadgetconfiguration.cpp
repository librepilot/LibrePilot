/**
 ******************************************************************************
 *
 * @file       scopegadgetconfiguration.cpp
 * @author     The LibrePilot Team http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ScopePlugin Scope Gadget Plugin
 * @{
 * @brief The scope Gadget, graphically plots the states of UAVObjects
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

#include "scopegadgetconfiguration.h"

ScopeGadgetConfiguration::ScopeGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    uint currentStreamVersion = settings.value("configurationStreamVersion").toUInt();

    if (currentStreamVersion != m_configurationStreamVersion) {
        return;
    }

    m_plotType = settings.value("plotType", ChronoPlot).toInt();
    m_dataSize = settings.value("dataSize", 60).toInt();
    m_refreshInterval  = settings.value("refreshInterval", 1000).toInt();
    m_mathFunctionType = 0;

    int plotCurveCount = settings.value("plotCurveCount").toInt();
    for (int i = 0; i < plotCurveCount; i++) {
        settings.beginGroup(QString("plotCurve") + QString().number(i));

        PlotCurveConfiguration *plotCurveConf = new PlotCurveConfiguration();
        plotCurveConf->uavObject       = settings.value("uavObject").toString();
        plotCurveConf->uavField        = settings.value("uavField").toString();
        plotCurveConf->color           = settings.value("color").value<QRgb>();
        plotCurveConf->yScalePower     = settings.value("yScalePower").toInt();
        plotCurveConf->mathFunction    = settings.value("mathFunction").toString();
        plotCurveConf->yMeanSamples    = settings.value("yMeanSamples", 1).toInt();
        plotCurveConf->yMeanSamples    = settings.value("yMeanSamples", 1).toInt();
        plotCurveConf->drawAntialiased = settings.value("drawAntialiased", true).toBool();
        plotCurveConf->yMinimum        = settings.value("yMinimum").toDouble();
        plotCurveConf->yMaximum        = settings.value("yMaximum").toDouble();
        m_plotCurveConfigs.append(plotCurveConf);

        settings.endGroup();
    }

    m_loggingEnabled = settings.value("LoggingEnabled").toBool();
    m_loggingNewFileOnConnect = settings.value("LoggingNewFileOnConnect").toBool();
    m_loggingPath    = settings.value("LoggingPath").toString();
}

ScopeGadgetConfiguration::ScopeGadgetConfiguration(const ScopeGadgetConfiguration & obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_plotType = obj.m_plotType;
    m_dataSize = obj.m_dataSize;
    m_mathFunctionType = obj.m_mathFunctionType;
    m_refreshInterval  = obj.m_refreshInterval;

    int plotCurveCount = obj.m_plotCurveConfigs.size();
    for (int i = 0; i < plotCurveCount; i++) {
        PlotCurveConfiguration *currentPlotCurveConf = obj.m_plotCurveConfigs.at(i);

        PlotCurveConfiguration *newPlotCurveConf     = new PlotCurveConfiguration();
        newPlotCurveConf->uavObject       = currentPlotCurveConf->uavObject;
        newPlotCurveConf->uavField        = currentPlotCurveConf->uavField;
        newPlotCurveConf->color           = currentPlotCurveConf->color;
        newPlotCurveConf->yScalePower     = currentPlotCurveConf->yScalePower;
        newPlotCurveConf->yMeanSamples    = currentPlotCurveConf->yMeanSamples;
        newPlotCurveConf->mathFunction    = currentPlotCurveConf->mathFunction;
        newPlotCurveConf->drawAntialiased = currentPlotCurveConf->drawAntialiased;
        newPlotCurveConf->yMinimum        = currentPlotCurveConf->yMinimum;
        newPlotCurveConf->yMaximum        = currentPlotCurveConf->yMaximum;

        addPlotCurveConfig(newPlotCurveConf);
    }

    m_loggingEnabled = obj.m_loggingEnabled;
    m_loggingNewFileOnConnect = obj.m_loggingNewFileOnConnect;
    m_loggingPath    = obj.m_loggingPath;
}

ScopeGadgetConfiguration::~ScopeGadgetConfiguration()
{
    clearPlotData();
}

void ScopeGadgetConfiguration::clearPlotData()
{
    PlotCurveConfiguration *plotCurveConfig;

    while (m_plotCurveConfigs.size() > 0) {
        plotCurveConfig = m_plotCurveConfigs.first();
        m_plotCurveConfigs.pop_front();
        delete plotCurveConfig;
    }
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *ScopeGadgetConfiguration::clone() const
{
    return new ScopeGadgetConfiguration(*this);
}


/**
 * Saves a configuration. //REDEFINES saveConfig CHILD BEHAVIOR?
 *
 */
void ScopeGadgetConfiguration::saveConfig(QSettings &settings) const
{
    int plotCurveCount = m_plotCurveConfigs.size();

    settings.setValue("configurationStreamVersion", m_configurationStreamVersion);
    settings.setValue("plotType", m_plotType);
    settings.setValue("dataSize", m_dataSize);
    settings.setValue("refreshInterval", m_refreshInterval);
    settings.setValue("plotCurveCount", plotCurveCount);

    for (int i = 0; i < plotCurveCount; i++) {
        settings.beginGroup(QString("plotCurve") + QString().number(i));

        PlotCurveConfiguration *plotCurveConf = m_plotCurveConfigs.at(i);
        settings.setValue("uavObject", plotCurveConf->uavObject);
        settings.setValue("uavField", plotCurveConf->uavField);
        settings.setValue("color", plotCurveConf->color);
        settings.setValue("mathFunction", plotCurveConf->mathFunction);
        settings.setValue("yScalePower", plotCurveConf->yScalePower);
        settings.setValue("yMeanSamples", plotCurveConf->yMeanSamples);
        settings.setValue("yMinimum", plotCurveConf->yMinimum);
        settings.setValue("yMaximum", plotCurveConf->yMaximum);
        settings.setValue("drawAntialiased", plotCurveConf->drawAntialiased);

        settings.endGroup();
    }

    settings.setValue("LoggingEnabled", m_loggingEnabled);
    settings.setValue("LoggingNewFileOnConnect", m_loggingNewFileOnConnect);
    settings.setValue("LoggingPath", m_loggingPath);
}

void ScopeGadgetConfiguration::replacePlotCurveConfig(QList<PlotCurveConfiguration *> newPlotCurveConfigs)
{
    clearPlotData();

    m_plotCurveConfigs.append(newPlotCurveConfigs);
}
