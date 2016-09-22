/**
 ******************************************************************************
 *
 * @file       devicemonitor.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
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
#ifndef DEVICEMONITOR_H_
#define DEVICEMONITOR_H_

#include "gst_global.h"

#include <QObject>

typedef struct _GstDeviceMonitor GstDeviceMonitor;

class Device {
public:
    Device(QString displayName, QString deviceClass) : m_displayName(displayName), m_deviceClass(deviceClass)
    {}
    QString displayName() const
    {
        return m_displayName;
    }
    QString deviceClass() const
    {
        return m_deviceClass;
    }
private:
    QString m_displayName;
    QString m_deviceClass;
};

class GST_LIB_EXPORT DeviceMonitor : public QObject {
    Q_OBJECT
public:
    DeviceMonitor(QObject *parent = NULL);
    virtual ~DeviceMonitor();

    QList<Device> devices() const;

signals:
    void deviceAdded(QString name);
    void deviceRemoved(QString name);

private:
    GstDeviceMonitor *monitor;

private slots:
    void device_added(QString name);
    void device_removed(QString name);
};

#endif /* DEVICEMONITOR_H_ */
