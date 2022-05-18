/**
 ******************************************************************************
 *
 * @file       configspracingf3evohwwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief SPRacingF3EVO hardware configuration panel
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
#ifndef CONFIGSPRACINGF3EVOHWWIDGET_H
#define CONFIGSPRACINGF3EVOHWWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

#include "hwspracingf3evosettings.h"

class Ui_SPRacingF3EVOHWWidget;

class UAVObject;

class QWidget;

class ConfigSPRacingF3EVOHWWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigSPRacingF3EVOHWWidget(QWidget *parent = 0);
    ~ConfigSPRacingF3EVOHWWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_SPRacingF3EVOHWWidget *m_ui;

    QComboBox *m_cbUART[HwSPRacingF3EVOSettings::UARTPORT_NUMELEM];

    void updateFeatures();

    bool optionConflict(int uartOption, int vcpOption);

private slots:
    void UARTxChanged(int index);
    void USBVCPFunctionChanged(int index);
};

#endif // CONFIGSPRACINGF3EVOHWWIDGET_H
