/**
 ******************************************************************************
 *
 * @file       ipconnectionconfiguration.h
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

#ifndef IPCONFIGURATIONCONFIGURATION_H
#define IPCONFIGURATIONCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>

#include <QString>

using namespace Core;

class IPConnectionConfiguration : public IUAVGadgetConfiguration {
    Q_OBJECT Q_PROPERTY(QString HostName READ hostName WRITE setHostName)
    Q_PROPERTY(int Port READ port WRITE setPort)
    Q_PROPERTY(int UseTCP READ useTCP WRITE setUseTCP)

public:
    explicit IPConnectionConfiguration(QString classId, QSettings &settings, QObject *parent = 0);
    explicit IPConnectionConfiguration(const IPConnectionConfiguration &obj);

    virtual ~IPConnectionConfiguration();

    IUAVGadgetConfiguration *clone() const;
    void saveConfig(QSettings &settings) const;

    QString hostName() const
    {
        return m_hostName;
    }
    int port() const
    {
        return m_port;
    }
    int useTCP() const
    {
        return m_useTCP;
    }

public slots:
    void setHostName(QString hostName)
    {
        m_hostName = hostName;
    }
    void setPort(int port)
    {
        m_port = port;
    }
    void setUseTCP(int useTCP)
    {
        m_useTCP = useTCP;
    }

private:
    QString m_hostName;
    int m_port;
    int m_useTCP;
};

#endif // IPCONFIGURATIONCONFIGURATION_H
