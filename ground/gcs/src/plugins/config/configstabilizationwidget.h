/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Stabilization configuration panel
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
#ifndef CONFIGSTABILIZATIONWIDGET_H
#define CONFIGSTABILIZATIONWIDGET_H

#include "ui_stabilization.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "stabilizationsettings.h"
#include <QWidget>
#include <QTimer>
#include <QSignalMapper>
#include "qwt/src/qwt_plot_curve.h"
#include "qwt/src/qwt_plot_grid.h"

class ConfigStabilizationWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigStabilizationWidget(QWidget *parent = 0);
    ~ConfigStabilizationWidget();
    bool shouldObjectBeSaved(UAVObject *object);

private:
    Ui_StabilizationWidget *ui;
    QTimer *realtimeUpdates;
    QList<QTabBar *> m_stabTabBars;
    QString m_stabilizationObjectsString;

    // Milliseconds between automatic 'Instant Updates'
    static const int AUTOMATIC_UPDATE_RATE   = 500;

    static const int EXPO_CURVE_POINTS_COUNT = 100;
    static const double EXPO_CURVE_CONSTANT  = 1.00695;

    int boardModel;
    int m_stabSettingsBankCount;
    int m_currentStabSettingsBank;

    QwtPlotCurve m_expoPlotCurveRoll;
    QwtPlotCurve m_expoPlotCurvePitch;
    QwtPlotCurve m_expoPlotCurveYaw;
    QwtPlotGrid m_plotGrid;
    QSignalMapper m_stabSettingsCopyFromSignalMapper;
    QSignalMapper m_stabSettingsCopyToSignalMapper;
    QSignalMapper m_stabSettingsSwapSignalMapper;

    UAVObject *getStabBankObject(int bank);

    void updateThrottleCurveFromObject();
    void updateObjectFromThrottleCurve();
    void setupExpoPlot();
    void setupStabBanksGUI();
    void resetStabBank(int bank);
    void restoreStabBank(int bank);

protected:
    QString mapObjectName(const QString objectName);

protected slots:
    void refreshWidgetsValues(UAVObject *o = NULL);
    void updateObjectsFromWidgets();

private slots:
    void realtimeUpdatesSlot(bool value);
    void linkCheckBoxes(bool value);
    void processLinkedWidgets(QWidget *);
    void onBoardConnected();
    void stabBankChanged(int index);
    void resetThrottleCurveToDefault();
    void throttleCurveUpdated();
    void replotExpo(int value, QwtPlotCurve &curve);
    void replotExpoRoll(int value);
    void replotExpoPitch(int value);
    void replotExpoYaw(int value);

    void restoreAllStabBanks();
    void resetAllStabBanks();
    void restoreCurrentAction();
    void resetCurrentStabBank();
    void copyCurrentStabBank();

    void copyFromBankToBank(int fromBank, int toBank);
    void copyFromBankToCurrent(int bank);
    void copyToBankFromCurrent(int bank);
    void swapBankAndCurrent(int bank);
};
#endif // ConfigStabilizationWidget_H
