/**
 ******************************************************************************
 *
 * @file       configrevohwwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Revolution hardware configuration panel
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
#ifndef CONFIGREVOHWWIDGET_H
#define CONFIGREVOHWWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

class Ui_RevoHWWidget;

class UAVObject;

class QWidget;

class ConfigRevoHWWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigRevoHWWidget(QWidget *parent = 0);
    ~ConfigRevoHWWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_RevoHWWidget *m_ui;

    void setupCustomCombos();

private slots:
    void usbVCPPortChanged(int index);
    void usbHIDPortChanged(int index);
    void flexiPortChanged(int index);
    void mainPortChanged(int index);
    void rcvrPortChanged(int index);
};

#endif // CONFIGREVOHWWIDGET_H
