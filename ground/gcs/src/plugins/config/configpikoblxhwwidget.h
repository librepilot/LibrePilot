/**
 ******************************************************************************
 *
 * @file       configpikoblxhwwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief PikoBLX hardware configuration panel
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
#ifndef CONFIGPIKOBLXHWWIDGET_H
#define CONFIGPIKOBLXHWWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

#include "hwpikoblxsettings.h"

class Ui_PikoBLXHWWidget;

class UAVObject;

class QWidget;

class ConfigPikoBLXHWWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigPikoBLXHWWidget(QWidget *parent = 0);
    ~ConfigPikoBLXHWWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_PikoBLXHWWidget *m_ui;

    QComboBox *m_cbUART[HwPikoBLXSettings::UARTPORT_NUMELEM];

    void updateFeatures();

    bool optionConflict(int uartOption, int vcpOption);

private slots:
    void UARTxChanged(int index);
    void USBVCPFunctionChanged(int index);
};

#endif // CONFIGPIKOBLXHWWIDGET_H
