/**
 ******************************************************************************
 *
 * @file       ipconnectionconfiguration.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin implements telemetry over TCP/IP and UDP/IP
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

#include "ipconnectionconfiguration.h"

#include <coreplugin/icore.h>

IPConnectionConfiguration::IPConnectionConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    m_hostName = settings.value("HostName", "").toString();
    m_port     = settings.value("Port", 9000).toInt();
    m_useTCP   = settings.value("UseTCP", true).toInt();
}

IPConnectionConfiguration::IPConnectionConfiguration(const IPConnectionConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_hostName = obj.m_hostName;
    m_port     = obj.m_port;
    m_useTCP   = obj.m_useTCP;
}

IPConnectionConfiguration::~IPConnectionConfiguration()
{}

IUAVGadgetConfiguration *IPConnectionConfiguration::clone() const
{
    return new IPConnectionConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void IPConnectionConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("HostName", m_hostName);
    settings.setValue("Port", m_port);
    settings.setValue("UseTCP", m_useTCP);
}
