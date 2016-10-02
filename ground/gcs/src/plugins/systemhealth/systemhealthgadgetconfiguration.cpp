/**
 ******************************************************************************
 *
 * @file       systemhealthgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SystemHealthPlugin System Health Plugin
 * @{
 * @brief The System Health gadget plugin
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

#include "systemhealthgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
SystemHealthGadgetConfiguration::SystemHealthGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    systemFile = settings.value("diagram", "Unknown").toString();
    systemFile = Utils::InsertDataPath(systemFile);
}

SystemHealthGadgetConfiguration::SystemHealthGadgetConfiguration(const SystemHealthGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    systemFile = obj.systemFile;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *SystemHealthGadgetConfiguration::clone() const
{
    return new SystemHealthGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void SystemHealthGadgetConfiguration::saveConfig(QSettings &settings) const
{
    QString diagram = Utils::RemoveDataPath(systemFile);

    settings.setValue("diagram", diagram);
}
