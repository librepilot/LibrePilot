/**
 ******************************************************************************
 *
 * @file       config_cc_hw_widget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update hardware settings in the firmware
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
#ifndef CONFIGCCHWWIDGET_H
#define CONFIGCCHWWIDGET_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

class Ui_CC_HW_Widget;
class QWidget;
class QSvgRenderer;

class ConfigCCHWWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigCCHWWidget(QWidget *parent = 0);
    ~ConfigCCHWWidget();
private slots:
    void refreshValues();
    void widgetsContentsChanged();
    void enableSaveButtons(bool enable);

private:
    Ui_CC_HW_Widget *m_telemetry;
    QSvgRenderer *m_renderer;
};

#endif // CONFIGCCHWWIDGET_H
