/**
 ******************************************************************************
 *
 * @file       commonhwsettingswidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Common hardware configuration panel
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
#ifndef COMMONHWSETTINGSWIDGET_H
#define COMMONHWSETTINGSWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

class Ui_CommonHWSettingsWidget;

class CommonHWSettingsWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    static const quint32 F_USB = (1 << 0);
    static const quint32 F_SBUS = (1 << 1);
    static const quint32 F_DSM  = (1 << 2);
    static const quint32 F_TELEMETRY = (1 << 3);
    static const quint32 F_DEBUGCONSOLE = (1 << 4);
    static const quint32 F_GPS  = (1 << 5);

    CommonHWSettingsWidget(QWidget *parent = 0);
    virtual ~CommonHWSettingsWidget();

    void registerWidgets(ConfigTaskWidget &ct);
    void refreshWidgetsValues(UAVObject *obj);

    void setFeatures(quint32 features);

    QComboBox *USBVCPComboBox();

signals:
    void USBHIDFunctionChanged(int index);
    void USBVCPFunctionChanged(int index);

private slots:
    void USBHIDComboChanged(int index);
    void USBVCPComboChanged(int index);

private:
    Ui_CommonHWSettingsWidget *m_ui;

    bool USBFunctionConflict();
};

#endif // COMMONHWSETTINGSWIDGET_H
