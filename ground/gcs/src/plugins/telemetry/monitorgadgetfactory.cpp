/**
 ******************************************************************************
 *
 * @file       monitorgadgetfactory.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   telemetryplugin
 * @{
 *
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
#include "monitorgadgetfactory.h"

#include "monitorgadgetconfiguration.h"
#include "monitorgadget.h"
#include "monitorgadgetoptionspage.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/connectionmanager.h>
#include <coreplugin/icore.h>
#include <uavtalk/telemetrymanager.h>

MonitorGadgetFactory::MonitorGadgetFactory(QObject *parent) :
    IUAVGadgetFactory(QString("TelemetryMonitorGadget"), tr("Telemetry Monitor"), parent)
{}

MonitorGadgetFactory::~MonitorGadgetFactory()
{}

Core::IUAVGadget *MonitorGadgetFactory::createGadget(QWidget *parent)
{
    MonitorWidget *widget = createMonitorWidget(parent);

    return new MonitorGadget(QString("TelemetryMonitorGadget"), widget, parent);
}

MonitorWidget *MonitorGadgetFactory::createMonitorWidget(QWidget *parent)
{
    MonitorWidget *widget = new MonitorWidget(parent);

    // connect widget to telemetry manager
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    TelemetryManager *tm = pm->getObject<TelemetryManager>();

    // connect(tm, SIGNAL(connected()), widget, SLOT(telemetryConnected()));
    // connect(tm, SIGNAL(disconnected()), widget, SLOT(telemetryDisconnected()));
    connect(tm, SIGNAL(telemetryUpdated(double, double)), widget, SLOT(telemetryUpdated(double, double)));

    // connect widget to connection manager
    Core::ConnectionManager *cm = Core::ICore::instance()->connectionManager();

    connect(cm, SIGNAL(deviceConnected(QIODevice *)), widget, SLOT(telemetryConnected()));
    connect(cm, SIGNAL(deviceDisconnected()), widget, SLOT(telemetryDisconnected()));

    if (tm->isConnected()) {
        widget->telemetryConnected();
    }

    return widget;
}

IUAVGadgetConfiguration *MonitorGadgetFactory::createConfiguration(QSettings &settings)
{
    return new MonitorGadgetConfiguration(QString("TelemetryMonitorGadget"), settings);
}

IOptionsPage *MonitorGadgetFactory::createOptionsPage(IUAVGadgetConfiguration *config)
{
    Q_UNUSED(config);
    return 0; // new MonitorGadgetOptionsPage(qobject_cast<MonitorGadgetConfiguration *>(config));
}
