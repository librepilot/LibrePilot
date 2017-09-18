/**
 ******************************************************************************
 *
 * @file       devicemonitor.cpp
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
#include "devicemonitor.h"

#include "gst_util.h"

#include <gst/gst.h>

#include <QDebug>

static GstBusSyncReply my_bus_sync_func(GstBus *bus, GstMessage *message, gpointer user_data)
{
    Q_UNUSED(bus)

    DeviceMonitor * dm;
    GstDevice *device;
    gchar *name;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_DEVICE_ADDED:
        gst_message_parse_device_added(message, &device);
        name = gst_device_get_display_name(device);

        dm   = (DeviceMonitor *)user_data;
        QMetaObject::invokeMethod(dm, "device_added", Qt::QueuedConnection,
                                  Q_ARG(QString, QString(name)));

        g_free(name);
        break;
    case GST_MESSAGE_DEVICE_REMOVED:
        gst_message_parse_device_removed(message, &device);
        name = gst_device_get_display_name(device);

        dm   = (DeviceMonitor *)user_data;
        QMetaObject::invokeMethod(dm, "device_removed", Qt::QueuedConnection,
                                  Q_ARG(QString, QString(name)));

        g_free(name);
        break;
    default:
        break;
    }

    // no need to pass it to the async queue, there is none...
    return GST_BUS_DROP;
}

DeviceMonitor::DeviceMonitor(QObject *parent) : QObject(parent)
{
    // initialize gstreamer
    gst::init(NULL, NULL);

    monitor = gst_device_monitor_new();

    GstBus *bus = gst_device_monitor_get_bus(monitor);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)my_bus_sync_func, this, NULL);
    gst_object_unref(bus);

    GstCaps *caps = NULL; // gst_caps_new_empty_simple("video/x-raw");
    const gchar *classes = "Video/Source";
    gst_device_monitor_add_filter(monitor, classes, caps);
    if (caps) {
        gst_caps_unref(caps);
    }

    if (!gst_device_monitor_start(monitor)) {
        qWarning() << "Failed to start device monitor";
    }
}

DeviceMonitor::~DeviceMonitor()
{
    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);
}

QList<Device> DeviceMonitor::devices() const
{
    QList<Device> devices;

    GList *list = gst_device_monitor_get_devices(monitor);
    while (list != NULL) {
        gchar *name;
        gchar *device_class;

        GstDevice *device = (GstDevice *)list->data;
        name = gst_device_get_display_name(device);
        device_class = gst_device_get_device_class(device);

        devices << Device(name, device_class);

        g_free(name);
        g_free(device_class);

        gst_object_unref(device);
        list = g_list_remove_link(list, list);
    }

    return devices;
}

void DeviceMonitor::device_added(QString name)
{
    // qDebug() << "**** ADDED:" << name;
    emit deviceAdded(name);
}

void DeviceMonitor::device_removed(QString name)
{
    // qDebug() << "**** REMOVED:" << name;
    emit deviceRemoved(name);
}
