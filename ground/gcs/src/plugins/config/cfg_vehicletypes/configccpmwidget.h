/**
 ******************************************************************************
 *
 * @file       configccpmtwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief ccpm configuration panel
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
#ifndef CONFIGCCPMWIDGET_H
#define CONFIGCCPMWIDGET_H

#include "cfg_vehicletypes/vehicleconfig.h"

class Ui_CcpmConfigWidget;

class QWidget;
class QSpinBox;
class QGraphicsSvgItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsTextItem;

#define CCPM_MAX_SWASH_SERVOS 4

typedef struct {
    int ServoChannels[CCPM_MAX_SWASH_SERVOS];
    int Used[CCPM_MAX_SWASH_SERVOS];
    int Max[CCPM_MAX_SWASH_SERVOS];
    int Neutral[CCPM_MAX_SWASH_SERVOS];
    int Min[CCPM_MAX_SWASH_SERVOS];
} SwashplateServoSettingsStruct;

class ConfigCcpmWidget : public VehicleConfig {
    Q_OBJECT

public:
    static QStringList getChannelDescriptions();

    ConfigCcpmWidget(QWidget *parent = 0);
    ~ConfigCcpmWidget();

    virtual QString getFrameType();

public slots:
    void getMixer();
    void setMixer();

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

    virtual void enableControls(bool enable);
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

    virtual void registerWidgets(ConfigTaskWidget &parent);
    virtual void setupUI(QString frameType);

private:
    Ui_CcpmConfigWidget *m_aircraft;

    QGraphicsSvgItem *SwashplateImg;
    QGraphicsSvgItem *CurveImg;
    QGraphicsSvgItem *Servos[CCPM_MAX_SWASH_SERVOS];
    QGraphicsTextItem *ServosText[CCPM_MAX_SWASH_SERVOS];
    QGraphicsLineItem *ServoLines[CCPM_MAX_SWASH_SERVOS];
    QGraphicsEllipseItem *ServosTextCircles[CCPM_MAX_SWASH_SERVOS];
    QSpinBox *SwashLvlSpinBoxes[CCPM_MAX_SWASH_SERVOS];

    QString typeText;

    bool SwashLvlConfigurationInProgress;
    UAVObject::Metadata SwashLvlaccInitialData;
    int SwashLvlState;
    int SwashLvlServoInterlock;

    SwashplateServoSettingsStruct oldSwashLvlConfiguration;
    SwashplateServoSettingsStruct newSwashLvlConfiguration;

    int MixerChannelData[6];

    void resetActuators(GUIConfigDataUnion *configData);

    int ShowDisclaimer(int messageID);

    bool updatingFromHardware;
    bool updatingToHardware;

    void updateConfigObjects();

    void saveObjectToSD(UAVObject *obj);

    bool throwConfigError(int typeInt);

private slots:
    void ccpmSwashplateUpdate();
    void ccpmSwashplateRedraw();
    void UpdateMixer();
    void UpdateType();

    void SwashLvlStartButtonPressed();
    void SwashLvlPrevButtonPressed();
    void SwashLvlNextButtonPressed();
    void SwashLvlPrevNextButtonPressed();
    void SwashLvlCancelButtonPressed();
    void SwashLvlFinishButtonPressed();

    void SetUIComponentVisibilities();

    void enableSwashplateLevellingControl(bool state);
    void setSwashplateLevel(int percent);
    void SwashLvlSpinBoxChanged(int value);
};

#endif // CONFIGCCPMWIDGET_H
