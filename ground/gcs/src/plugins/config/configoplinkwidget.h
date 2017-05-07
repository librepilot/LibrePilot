/**
 ******************************************************************************
 *
 * @file       configoplinkwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to configure the OPLink, Revo and Sparky2 modems
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
#ifndef CONFIGOPLINKWIDGET_H
#define CONFIGOPLINKWIDGET_H

#include "configtaskwidget.h"

class OPLinkStatus;
class OPLinkSettings;

class Ui_OPLinkWidget;

class ConfigOPLinkWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigOPLinkWidget(QWidget *parent = 0);
    ~ConfigOPLinkWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);

private:
    Ui_OPLinkWidget *m_oplink;

    OPLinkStatus *oplinkStatusObj;
    OPLinkSettings *oplinkSettingsObj;

    // Frequency display settings
    float frequency_base;
    float frequency_step;
    QString channel_tooltip;

    // Is the status current?
    bool statusUpdated;

    void updateStatus();
    void updateInfo();
    void updateSettings();

    void setOPLMOptionsVisible(bool visible);

private slots:
    void connected();

    void protocolChanged();
    void linkTypeChanged();
    void customIDChanged();
    void coordIDChanged();

    void minChannelChanged();
    void maxChannelChanged();
    void rfBandChanged();
    void channelChanged(bool isMax);
    void updateFrequencyDisplay();

    void mainPortChanged();
    void flexiPortChanged();
    void radioPriStreamChanged();
    void radioAuxStreamChanged();
    void vcpBridgeChanged();

    void unbind();
    void clearDeviceID();
};

#endif // CONFIGOPLINKWIDGET_H
