/**
 ******************************************************************************
 *
 * @file       qmlviewgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin QML Viewer Plugin
 * @{
 * @brief The QML Viewer Gadget
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

#include "qmlviewgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
QmlViewGadgetConfiguration::QmlViewGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    m_defaultDial = settings.value("dialFile", "Unknown").toString();
    m_defaultDial = Utils::InsertDataPath(m_defaultDial);
    useOpenGLFlag = settings.value("useOpenGLFlag").toBool();
}

QmlViewGadgetConfiguration::QmlViewGadgetConfiguration(const QmlViewGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_defaultDial = obj.m_defaultDial;
    useOpenGLFlag = obj.useOpenGLFlag;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *QmlViewGadgetConfiguration::clone() const
{
    return new QmlViewGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void QmlViewGadgetConfiguration::saveConfig(QSettings &settings) const
{
    QString dialFile = Utils::RemoveDataPath(m_defaultDial);

    settings.setValue("dialFile", dialFile);
    settings.setValue("useOpenGLFlag", useOpenGLFlag);
}
