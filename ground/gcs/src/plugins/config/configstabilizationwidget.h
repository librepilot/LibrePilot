/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#include "../uavobjectwidgetutils/configtaskwidget.h"

#include "uavobject.h"

#include "qwt/src/qwt_plot_curve.h"
#include "qwt/src/qwt_plot_grid.h"

#include <QTimer>
#include <QSignalMapper>

class Ui_StabilizationWidget;

class QWidget;
class QTabBar;

class ConfigStabilizationWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigStabilizationWidget(QWidget *parent = 0);
    ~ConfigStabilizationWidget();

    bool shouldObjectBeSaved(UAVObject *object);

protected:
    QString mapObjectName(const QString objectName);

    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_StabilizationWidget *ui;
    QTimer *realtimeUpdates;
    QList<QTabBar *> m_stabTabBars;
    QString m_stabilizationObjectsString;

    // Milliseconds between automatic 'Instant Updates'
    static const int AUTOMATIC_UPDATE_RATE   = 500;

    static const int EXPO_CURVE_POINTS_COUNT = 100;
    constexpr static const double EXPO_CURVE_CONSTANT = 1.01395948;

    int m_stabSettingsBankCount;
    int m_currentStabSettingsBank;

    QwtPlotCurve m_expoPlotCurveRoll;
    QwtPlotCurve m_expoPlotCurvePitch;
    QwtPlotCurve m_expoPlotCurveYaw;
    QwtPlotGrid m_plotGrid;

    QSignalMapper m_bankActionSignalMapper;

    UAVObject *getStabBankObject(int bank);

    void updateThrottleCurveFromObject();
    void updateObjectFromThrottleCurve();
    void setupExpoPlot();
    void setupStabBanksGUI();
    void resetBank(int bank);
    void restoreBank(int bank);
    void copyBank(int fromBank, int toBank);
    void swapBank(int fromBank, int toBank);

private slots:
    void enableControlsChanged(bool enable);

    void realtimeUpdatesSlot(bool value);
    void linkCheckBoxes(bool value);
    void processLinkedWidgets(QWidget *);
    void stabBankChanged(int index);
    void resetThrottleCurveToDefault();
    void throttleCurveUpdated();
    void replotExpo(int value, QwtPlotCurve &curve);
    void replotExpoRoll(int value);
    void replotExpoPitch(int value);
    void replotExpoYaw(int value);

    void bankAction(const QString &mapping);
};
#endif // ConfigStabilizationWidget_H
