/**
 ******************************************************************************
 *
 * @file       configairframetwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Airframe configuration panel
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
#ifndef CONFIGVEHICLETYPEWIDGET_H
#define CONFIGVEHICLETYPEWIDGET_H

#include "cfg_vehicletypes/vehicleconfig.h"
#include "uavobject.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"

#include <QMap>
#include <QString>
#include <QStringList>

class Ui_AircraftWidget;

class QWidget;

/*
 * This class derives from ConfigTaskWidget but almost bypasses the need its default "binding" mechanism.
 *
 * It does use the "dirty" state management and registers its relevant widgets with ConfigTaskWidget to do so.
 *
 * This class also manages child ConfigTaskWidget : see VehicleConfig class and its derived classes.
 * Note: for "dirty" state management it is important to register the fields of child widgets with the parent
 * ConfigVehicleTypeWidget class.
 *
 * TODO improve handling of relationship with VehicleConfig derived classes (i.e. ConfigTaskWidget within ConfigTaskWidget)
 */
class ConfigVehicleTypeWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    static QStringList getChannelDescriptions();

    ConfigVehicleTypeWidget(QWidget *parent = 0);
    ~ConfigVehicleTypeWidget();

protected:
    virtual void enableControls(bool enable);
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_AircraftWidget *m_aircraft;

    static enum { MULTIROTOR = 0, FIXED_WING, HELICOPTER, GROUND, CUSTOM } AirframeCategory;

    // Maps a frame category to its index in the m_aircraft->airframesWidget QStackedWidget
    QMap<int, int> m_vehicleIndexMap;

    QString frameType();
    void setFrameType(QString frameType);

    QString vehicleName();
    void setVehicleName(QString name);

    static int frameCategory(QString frameType);

    VehicleConfig *getVehicleConfigWidget(int frameCategory);
    VehicleConfig *createVehicleConfigWidget(int frameCategory);

private slots:
    void frameTypeChanged(int index);
};

#endif // CONFIGVEHICLETYPEWIDGET_H
