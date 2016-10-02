/**
 ******************************************************************************
 *
 * @file       IPconnectionconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin impliment telemetry over TCP/IP and UDP/IP
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

IPconnectionConfiguration::IPconnectionConfiguration(QString classId, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent),
    m_HostName("127.0.0.1"),
    m_Port(1000),
    m_UseTCP(1)
{
}

IPconnectionConfiguration::IPconnectionConfiguration(const IPconnectionConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_HostName = obj.m_HostName;
    m_Port = obj.m_Port;
    m_UseTCP = obj.m_UseTCP;
}

IPconnectionConfiguration::~IPconnectionConfiguration()
{}

IUAVGadgetConfiguration *IPconnectionConfiguration::clone() const
{
    return new IPconnectionConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void IPconnectionConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("port", m_Port);
    settings.setValue("hostName", m_HostName);
    settings.setValue("useTCP", m_UseTCP);
}

void IPconnectionConfiguration::saveSettings() const
{
    QSettings settings;

    settings.beginGroup("IPconnection");

    settings.beginWriteArray("Current");
    settings.setArrayIndex(0);
    settings.setValue("HostName", m_HostName);
    settings.setValue("Port", m_Port);
    settings.setValue("UseTCP", m_UseTCP);
    settings.endArray();

    settings.endGroup();
}


void IPconnectionConfiguration::restoreSettings()
{
    QSettings settings;

    settings.beginGroup("IPconnection");

    settings.beginReadArray("Current");
    settings.setArrayIndex(0);
    m_HostName = settings.value("HostName", "").toString();
    m_Port     = settings.value("Port", 0).toInt();
    m_UseTCP   = settings.value("UseTCP", 0).toInt();
    settings.endArray();

    settings.endGroup();
}
