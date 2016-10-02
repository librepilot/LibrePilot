/**
 ******************************************************************************
 *
 * @file       uavobjectbrowserconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectBrowserPlugin UAVObject Browser Plugin
 * @{
 * @brief The UAVObject Browser gadget plugin
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

#include "uavobjectbrowserconfiguration.h"

UAVObjectBrowserConfiguration::UAVObjectBrowserConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    m_unknownObjectColor       = settings.value("unknownObjectColor", QColor(Qt::gray)).value<QColor>();
    m_useCategorizedView       = settings.value("CategorizedView", false).toBool();
    m_useScientificView        = settings.value("ScientificView", false).toBool();
    m_showMetaData = settings.value("showMetaData", false).toBool();
    m_showDescription          = settings.value("showDescription", false).toBool();
    m_splitterState = settings.value("splitterState").toByteArray();
    m_recentlyUpdatedColor     = settings.value("recentlyUpdatedColor", QColor(255, 230, 230)).value<QColor>();
    m_manuallyChangedColor     = settings.value("manuallyChangedColor", QColor(230, 230, 255)).value<QColor>();
    m_recentlyUpdatedTimeout   = settings.value("recentlyUpdatedTimeout", 500).toInt();
    m_onlyHilightChangedValues = settings.value("onlyHilightChangedValues", false).toBool();
}

UAVObjectBrowserConfiguration::UAVObjectBrowserConfiguration(const UAVObjectBrowserConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_recentlyUpdatedColor     = obj.m_recentlyUpdatedColor;
    m_manuallyChangedColor     = obj.m_manuallyChangedColor;
    m_recentlyUpdatedTimeout   = obj.m_recentlyUpdatedTimeout;
    m_onlyHilightChangedValues = obj.m_onlyHilightChangedValues;
    m_useCategorizedView = obj.m_useCategorizedView;
    m_useScientificView  = obj.m_useScientificView;
    m_splitterState = obj.m_splitterState;
    m_showMetaData = obj.m_showMetaData;
    m_unknownObjectColor = obj.m_unknownObjectColor;
    m_showDescription    = obj.m_showDescription;
}

IUAVGadgetConfiguration *UAVObjectBrowserConfiguration::clone() const
{
    return new UAVObjectBrowserConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void UAVObjectBrowserConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("unknownObjectColor", m_unknownObjectColor);
    settings.setValue("recentlyUpdatedColor", m_recentlyUpdatedColor);
    settings.setValue("manuallyChangedColor", m_manuallyChangedColor);
    settings.setValue("recentlyUpdatedTimeout", m_recentlyUpdatedTimeout);
    settings.setValue("onlyHilightChangedValues", m_onlyHilightChangedValues);
    settings.setValue("CategorizedView", m_useCategorizedView);
    settings.setValue("ScientificView", m_useScientificView);
    settings.setValue("showMetaData", m_showMetaData);
    settings.setValue("showDescription", m_showDescription);
    settings.setValue("splitterState", m_splitterState);
}
