/**
 ******************************************************************************
 *
 * @file       oplinkmanager.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief The UAVTalk protocol plugin
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

#ifndef OPLINK_MANAGER_H
#define OPLINK_MANAGER_H

#include "uavtalk_global.h"

#include <QObject>

class OPLinkStatus;

class UAVTALK_EXPORT OPLinkManager : public QObject {
    Q_OBJECT

public:
    enum OPLinkType {
        OPLINK_UNKNOWN,
        OPLINK_MINI,
        OPLINK_REVOLUTION,
        OPLINK_SPARKY2
    };

    OPLinkManager();
    ~OPLinkManager();

    OPLinkManager::OPLinkType opLinkType() const
    {
        return m_opLinkType;
    }

    bool isConnected() const;

signals:
    void connected();
    void disconnected();

private slots:
    void onDeviceConnect();
    void onDeviceDisconnect();
    void onOPLinkStatusUpdate();
    void onOPLinkConnect();
    void onOPLinkDisconnect();

private:
    bool m_isConnected;
    OPLinkType m_opLinkType;
    OPLinkStatus *m_opLinkStatus;
};

#endif // OPLINK_MANAGER_H
