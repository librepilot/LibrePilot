/********************************************************************************
 * @file       osgearthviewgadgetconfiguration.cpp
 * @author     The OpenPilot Team Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OsgEarthview Plugin
 * @{
 * @brief Osg Earth view of UAV
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

#include "osgearthviewgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
OsgEarthviewGadgetConfiguration::OsgEarthviewGadgetConfiguration(QString classId, QSettings *settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    Q_UNUSED(settings);
    // if a saved configuration exists load it
    if (settings != 0) {
        QString dialFile = settings->value("sceneFile").toString();
        m_sceneFile = Utils::PathUtils().InsertDataPath(dialFile);
    }
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *OsgEarthviewGadgetConfiguration::clone()
{
    OsgEarthviewGadgetConfiguration *m = new OsgEarthviewGadgetConfiguration(this->classId());
    m->m_sceneFile = m_sceneFile;
    return m;
}

/**
 * Saves a configuration.
 *
 */
void OsgEarthviewGadgetConfiguration::saveConfig(QSettings *settings) const {
    QString sceneFile = Utils::PathUtils().RemoveDataPath(m_sceneFile);
    settings->setValue("sceneFile", sceneFile);
}
