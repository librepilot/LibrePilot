/**
 ******************************************************************************
 *
 * @file       configsparky2hwwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Sparky2 hardware configuration panel
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
#ifndef CONFIGSPARKY2HWWIDGET_H
#define CONFIGSPARKY2HWWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

class Ui_Sparky2HWWidget;

class UAVObject;

class QWidget;

class ConfigSparky2HWWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigSparky2HWWidget(QWidget *parent = 0);
    ~ConfigSparky2HWWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_Sparky2HWWidget *m_ui;

    void setupCustomCombos();

private slots:
    void usbVCPPortChanged(int index);
    void usbHIDPortChanged(int index);
    void flexiPortChanged(int index);
    void mainPortChanged(int index);
};

#endif // CONFIGSPARKY2HWWIDGET_H
