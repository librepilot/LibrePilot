/**
 ******************************************************************************
 *
 * @file       oplinkmanager.cpp
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

#include "oplinkmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/connectionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <uavobjects/uavobjectmanager.h>

#include <oplinkstatus.h>

#include <QDebug>

OPLinkManager::OPLinkManager() : QObject(), m_isConnected(false), m_opLinkType(OPLinkManager::OPLINK_UNKNOWN)
{
    // connect to the connection manager
    Core::ConnectionManager *cm = Core::ICore::instance()->connectionManager();

    connect(cm, SIGNAL(deviceConnected(QIODevice *)), this, SLOT(onDeviceConnect()));
    connect(cm, SIGNAL(deviceDisconnected()), this, SLOT(onDeviceDisconnect()));

    if (cm->isConnected()) {
        onDeviceConnect();
    }
}

OPLinkManager::~OPLinkManager()
{}

bool OPLinkManager::isConnected() const
{
    return m_isConnected;
}

void OPLinkManager::onDeviceConnect()
{
    if (m_isConnected) {
        return;
    }

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);

    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objManager);

    m_opLinkStatus = OPLinkStatus::GetInstance(objManager);
    Q_ASSERT(m_opLinkStatus);

    // start monitoring OPLinkStatus
    connect(m_opLinkStatus, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(onOPLinkStatusUpdate()), Qt::UniqueConnection);
}

void OPLinkManager::onDeviceDisconnect()
{
    onOPLinkDisconnect();
}

void OPLinkManager::onOPLinkStatusUpdate()
{
    int type = m_opLinkStatus->boardType();

    switch (type) {
    case 0x03:
        m_opLinkType = OPLINK_MINI;
        onOPLinkConnect();
        break;
    case 0x09:
        m_opLinkType = OPLINK_REVOLUTION;
        onOPLinkConnect();
        break;
    case 0x92:
        m_opLinkType = OPLINK_SPARKY2;
        onOPLinkConnect();
        break;
    default:
        m_opLinkType = OPLINK_UNKNOWN;
        // stop monitoring status updates...
        m_opLinkStatus->disconnect(this);
        break;
    }
}

void OPLinkManager::onOPLinkConnect()
{
    if (m_isConnected) {
        return;
    }
    // stop monitoring status updates...
    m_opLinkStatus->disconnect(this);

    m_isConnected = true;
    emit connected();
}

void OPLinkManager::onOPLinkDisconnect()
{
    if (!m_isConnected) {
        return;
    }
    // stop monitoring status updates...
    m_opLinkStatus->disconnect(this);

    m_isConnected = false;
    emit disconnected();
}
