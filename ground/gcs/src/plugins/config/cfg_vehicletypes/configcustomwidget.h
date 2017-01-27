/**
 ******************************************************************************
 *
 * @file       configcustomwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief custom configuration panel
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
#ifndef CONFIGCUSTOMWIDGET_H
#define CONFIGCUSTOMWIDGET_H

#include "cfg_vehicletypes/vehicleconfig.h"

#include <QList>
#include <QItemDelegate>

class Ui_CustomConfigWidget;

class QWidget;

class ConfigCustomWidget : public VehicleConfig {
    Q_OBJECT

public:
    static QStringList getChannelDescriptions();

    ConfigCustomWidget(QWidget *parent = 0);
    ~ConfigCustomWidget();

    virtual QString getFrameType();

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

    virtual void registerWidgets(ConfigTaskWidget &parent);
    virtual void setupUI(QString frameType);

private:
    Ui_CustomConfigWidget *m_aircraft;

    void resetActuators(GUIConfigDataUnion *configData);

    bool throwConfigError(int numMotors);
};

class SpinBoxDelegate : public QItemDelegate {
    Q_OBJECT

public:
    SpinBoxDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // CONFIGCUSTOMWIDGET_H
