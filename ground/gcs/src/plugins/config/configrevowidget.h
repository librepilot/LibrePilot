/**
 ******************************************************************************
 *
 * @file       configrevowidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Telemetry configuration panel
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
#ifndef CONFIGREVOWIDGET_H
#define CONFIGREVOWIDGET_H

#include "configtaskwidget.h"

#include "uavobject.h"

#include "calibration/thermal/thermalcalibrationmodel.h"
#include "calibration/sixpointcalibrationmodel.h"
#include "calibration/levelcalibrationmodel.h"
#include "calibration/gyrobiascalibrationmodel.h"

class Ui_RevoSensorsWidget;

class QWidget;

class ConfigRevoWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigRevoWidget(QWidget *parent = 0);
    ~ConfigRevoWidget();

protected:
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    OpenPilot::SixPointCalibrationModel *m_accelCalibrationModel;
    OpenPilot::SixPointCalibrationModel *m_magCalibrationModel;
    OpenPilot::LevelCalibrationModel *m_levelCalibrationModel;
    OpenPilot::GyroBiasCalibrationModel *m_gyroBiasCalibrationModel;
    OpenPilot::ThermalCalibrationModel *m_thermalCalibrationModel;

    Ui_RevoSensorsWidget *m_ui;

    // Board rotation store/recall for FC and for aux mag
    float storedBoardRotation[3];
    float storedBoardLevelTrim[2];
    qint16 auxMagStoredBoardRotation[3];
    bool isBoardRotationStored;

    bool displayMagError;

    float onboardMagFiltered[3];
    float auxMagFiltered[3];
    float magBe[3];

    int magWarningCount;
    int magErrorCount;
    int auxMagWarningCount;
    int auxMagErrorCount;

private slots:
    void storeAndClearBoardRotation();
    void recallBoardRotation();
    void displayVisualHelp(QString elementID);
    void clearInstructions();
    void addInstructions(QString text, WizardModel::MessageType type = WizardModel::Info);
    void displayTemperature(float tempareture);
    void displayTemperatureGradient(float temparetureGradient);
    void displayTemperatureRange(float temparetureRange);

    // Slot for clearing home location
    void clearHomeLocation();

    void disableAllCalibrations();
    void enableAllCalibrations();

    void onBoardAuxMagError();
    void updateMagStatus();
    void updateMagBeVector();
    void updateMagAlarm(float errorMag, float errorAuxMag);

    float getMagError(float mag[3]);

    void updateVisualHelp();

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // ConfigRevoWidget_H
