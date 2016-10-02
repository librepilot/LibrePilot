/**
 ******************************************************************************
 *
 * @file       gcscontrolgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GCSControlGadgetPlugin GCSControl Gadget Plugin
 * @{
 * @brief A gadget to control the UAV, either from the keyboard or a joystick
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

#include "gcscontrolgadgetconfiguration.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
GCSControlGadgetConfiguration::GCSControlGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    controlsMode    = settings.value("controlsMode").toInt();
    rollChannel     = settings.value("rollChannel", -1).toInt();
    pitchChannel    = settings.value("pitchChannel", -1).toInt();
    yawChannel      = settings.value("yawChannel", -1).toInt();
    throttleChannel = settings.value("throttleChannel", -1).toInt();

    udp_port = settings.value("controlPortUDP").toUInt();
    udp_host = QHostAddress(settings.value("controlHostUDP").toString());

    for (int i = 0; i < 8; i++) {
        buttonSettings[i].ActionID   = settings.value(QString().sprintf("button%dAction", i)).toInt();
        buttonSettings[i].FunctionID = settings.value(QString().sprintf("button%dFunction", i)).toInt();
        buttonSettings[i].Amount     = settings.value(QString().sprintf("button%dAmount", i)).toDouble();
        channelReverse[i] = settings.value(QString().sprintf("channel%dReverse", i)).toBool();
    }
}

GCSControlGadgetConfiguration::GCSControlGadgetConfiguration(const GCSControlGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    controlsMode    = obj.controlsMode;
    rollChannel     = obj.rollChannel;
    pitchChannel    = obj.pitchChannel;
    yawChannel      = obj.yawChannel;
    throttleChannel = obj.throttleChannel;

    udp_host = obj.udp_host;
    udp_port = obj.udp_port;

    for (int i = 0; i < 8; i++) {
        buttonSettings[i].ActionID   = obj.buttonSettings[i].ActionID;
        buttonSettings[i].FunctionID = obj.buttonSettings[i].FunctionID;
        buttonSettings[i].Amount     = obj.buttonSettings[i].Amount;
        channelReverse[i] = obj.channelReverse[i];
    }
}

void GCSControlGadgetConfiguration::setUDPControlSettings(int port, QString host)
{
    udp_port = port;
    udp_host = QHostAddress(host);
}

int GCSControlGadgetConfiguration::getUDPControlPort()
{
    return udp_port;
}

QHostAddress GCSControlGadgetConfiguration::getUDPControlHost()
{
    return udp_host;
}

void GCSControlGadgetConfiguration::setRPYTchannels(int roll, int pitch, int yaw, int throttle)
{
    rollChannel     = roll;
    pitchChannel    = pitch;
    yawChannel      = yaw;
    throttleChannel = throttle;
}

QList<int> GCSControlGadgetConfiguration::getChannelsMapping()
{
    QList<int> ql;
    ql << rollChannel << pitchChannel << yawChannel << throttleChannel;
    return ql;
}

QList<bool> GCSControlGadgetConfiguration::getChannelsReverse()
{
    QList<bool> ql;
    for (int i = 0; i < 8; i++) {
        ql << channelReverse[i];
    }
    return ql;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *GCSControlGadgetConfiguration::clone() const
{
    return new GCSControlGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void GCSControlGadgetConfiguration::saveConfig(QSettings &settings) const
{
    settings.setValue("controlsMode", controlsMode);
    settings.setValue("rollChannel", rollChannel);
    settings.setValue("pitchChannel", pitchChannel);
    settings.setValue("yawChannel", yawChannel);
    settings.setValue("throttleChannel", throttleChannel);

    settings.setValue("controlPortUDP", QString::number(udp_port));
    settings.setValue("controlHostUDP", udp_host.toString());

    for (int i = 0; i < 8; i++) {
        settings.setValue(QString().sprintf("button%dAction", i), buttonSettings[i].ActionID);
        settings.setValue(QString().sprintf("button%dFunction", i), buttonSettings[i].FunctionID);
        settings.setValue(QString().sprintf("button%dAmount", i), buttonSettings[i].Amount);
        settings.setValue(QString().sprintf("channel%dReverse", i), channelReverse[i]);
    }
}
