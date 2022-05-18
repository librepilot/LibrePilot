/**
 ******************************************************************************
 *
 * @file       configairframetwidget.h
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
#ifndef CONFIGMULTIROTORWIDGET_H
#define CONFIGMULTIROTORWIDGET_H

#include "cfg_vehicletypes/vehicleconfig.h"

#include <QList>

class Ui_MultiRotorConfigWidget;

class QWidget;
class QGraphicsSvgItem;

class ConfigMultiRotorWidget : public VehicleConfig {
    Q_OBJECT

public:
    static const QString CHANNELBOXNAME;
    static QStringList getChannelDescriptions();

    ConfigMultiRotorWidget(QWidget *parent = 0);
    ~ConfigMultiRotorWidget();

    virtual QString getFrameType();

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

    virtual void enableControls(bool enable);
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

    virtual void registerWidgets(ConfigTaskWidget &parent);
    virtual void setupUI(QString frameType);

private:
    Ui_MultiRotorConfigWidget *m_aircraft;
    QGraphicsSvgItem *quad;
    bool invertMotors;

    void resetActuators(GUIConfigDataUnion *configData);
    void resetRcOutputs(GUIConfigDataUnion *configData);

    bool setupQuad(bool pLayout);
    bool setupHexa(bool pLayout);
    bool setupOcto();
    bool setupMultiRotorMixer(double mixerFactors[8][3]);
    void setupMotors(QList<QString> motorList);
    void setupRcOutputs(QList<QString> rcOutputList);
    void setupQuadMotor(int channel, double roll, double pitch, double yaw);

    void setYawMixLevel(int);
    void updateRcCurvesUsed();
    void updateAirframe(QString multiRotorType);
    void updateMotorsPositionLabels(QStringList motorLabels);
    void setupEnabledControls(QString multiRotorType);

    bool throwConfigError(int numMotors);

private slots:
    void reverseMultirotorMotor();
};

#endif // CONFIGMULTIROTORWIDGET_H
