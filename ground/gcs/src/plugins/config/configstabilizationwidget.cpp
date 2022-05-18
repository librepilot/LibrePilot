/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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
#include "configstabilizationwidget.h"

#include "ui_stabilization.h"

#include <uavobjectmanager.h>
#include "objectpersistence.h"

#include "altitudeholdsettings.h"
#include "stabilizationsettings.h"

#include "qwt/src/qwt.h"
#include "qwt/src/qwt_plot.h"
#include "qwt/src/qwt_plot_canvas.h"
#include "qwt/src/qwt_scale_widget.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QList>
#include <QTabBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>

ConfigStabilizationWidget::ConfigStabilizationWidget(QWidget *parent) : ConfigTaskWidget(parent),
    m_stabSettingsBankCount(0), m_currentStabSettingsBank(0)
{
    ui = new Ui_StabilizationWidget();
    ui->setupUi(this);

    // must be done before auto binding !
    setWikiURL("Stabilization+Configuration");

    setupStabBanksGUI();

    addAutoBindings();

    disableMouseWheelEvents();

    connect(this, SIGNAL(enableControlsChanged(bool)), this, SLOT(enableControlsChanged(bool)));

    setupExpoPlot();

    realtimeUpdates = new QTimer(this);
    connect(realtimeUpdates, SIGNAL(timeout()), this, SLOT(apply()));

    connect(ui->realTimeUpdates_6, SIGNAL(toggled(bool)), this, SLOT(realtimeUpdatesSlot(bool)));
    addWidget(ui->realTimeUpdates_6);
    connect(ui->realTimeUpdates_8, SIGNAL(toggled(bool)), this, SLOT(realtimeUpdatesSlot(bool)));
    addWidget(ui->realTimeUpdates_8);
    connect(ui->realTimeUpdates_12, SIGNAL(toggled(bool)), this, SLOT(realtimeUpdatesSlot(bool)));
    addWidget(ui->realTimeUpdates_12);
    connect(ui->realTimeUpdates_7, SIGNAL(toggled(bool)), this, SLOT(realtimeUpdatesSlot(bool)));
    addWidget(ui->realTimeUpdates_7);

    connect(ui->checkBox_7, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));
    addWidget(ui->checkBox_7);
    connect(ui->checkBox_2, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));
    addWidget(ui->checkBox_2);
    connect(ui->checkBox_8, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));
    addWidget(ui->checkBox_8);
    connect(ui->checkBox_3, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));
    addWidget(ui->checkBox_3);

    connect(ui->checkBoxLinkAcroFactors, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));
    addWidget(ui->checkBoxLinkAcroFactors);

    addWidget(ui->pushButton_2);
    addWidget(ui->pushButton_3);
    addWidget(ui->pushButton_4);
    addWidget(ui->pushButton_5);
    addWidget(ui->pushButton_6);
    addWidget(ui->pushButton_7);
    addWidget(ui->pushButton_8);
    addWidget(ui->pushButton_9);
    addWidget(ui->pushButton_10);
    addWidget(ui->pushButton_11);
    addWidget(ui->pushButton_12);
    addWidget(ui->pushButton_13);
    addWidget(ui->pushButton_14);
    addWidget(ui->pushButton_20);
    addWidget(ui->pushButton_21);
    addWidget(ui->pushButton_22);

    addWidget(ui->basicResponsivenessGroupBox);
    addWidget(ui->basicResponsivenessCheckBox);
    connect(ui->basicResponsivenessCheckBox, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));

    addWidget(ui->advancedResponsivenessGroupBox);
    addWidget(ui->advancedResponsivenessCheckBox);
    connect(ui->advancedResponsivenessCheckBox, SIGNAL(toggled(bool)), this, SLOT(linkCheckBoxes(bool)));

    connect(ui->defaultThrottleCurveButton, SIGNAL(clicked()), this, SLOT(resetThrottleCurveToDefault()));
    connect(ui->enableThrustPIDScalingCheckBox, SIGNAL(toggled(bool)), ui->ThrustPIDSource, SLOT(setEnabled(bool)));
    connect(ui->enableThrustPIDScalingCheckBox, SIGNAL(toggled(bool)), ui->ThrustPIDTarget, SLOT(setEnabled(bool)));
    connect(ui->enableThrustPIDScalingCheckBox, SIGNAL(toggled(bool)), ui->ThrustPIDAxis, SLOT(setEnabled(bool)));
    connect(ui->enableThrustPIDScalingCheckBox, SIGNAL(toggled(bool)), ui->thrustPIDScalingCurve, SLOT(setEnabled(bool)));
    ui->thrustPIDScalingCurve->setXAxisLabel(tr("Thrust"));
    ui->thrustPIDScalingCurve->setYAxisLabel(tr("Scaling factor"));
    ui->thrustPIDScalingCurve->setMin(-0.5);
    ui->thrustPIDScalingCurve->setMax(0.5);
    ui->thrustPIDScalingCurve->initLinearCurve(5, -0.25, 0.25);
    connect(ui->thrustPIDScalingCurve, SIGNAL(curveUpdated()), this, SLOT(throttleCurveUpdated()));

    addWidget(ui->defaultThrottleCurveButton);
    addWidget(ui->enableThrustPIDScalingCheckBox);
    addWidget(ui->thrustPIDScalingCurve);
    addWidget(ui->thrustPIDScalingCurve);
    connect(this, SIGNAL(widgetContentsChanged(QWidget *)), this, SLOT(processLinkedWidgets(QWidget *)));

    addWidget(ui->expoPlot);
    connect(ui->expoSpinnerRoll, SIGNAL(valueChanged(int)), this, SLOT(replotExpoRoll(int)));
    connect(ui->expoSpinnerPitch, SIGNAL(valueChanged(int)), this, SLOT(replotExpoPitch(int)));
    connect(ui->expoSpinnerYaw, SIGNAL(valueChanged(int)), this, SLOT(replotExpoYaw(int)));

    ui->AltitudeHold->setEnabled(false);
}

void ConfigStabilizationWidget::setupStabBanksGUI()
{
    StabilizationSettings *stabSettings = qobject_cast<StabilizationSettings *>(getObject("StabilizationSettings"));

    Q_ASSERT(stabSettings);

    m_stabSettingsBankCount = stabSettings->getField("FlightModeMap")->getOptions().count();

    // Set up fake tab widget stuff for pid banks support
    m_stabTabBars.append(ui->basicPIDBankTabBar);
    m_stabTabBars.append(ui->advancedPIDBankTabBar);

    QAction *defaultStabMenuAction = new QAction(QIcon(":configgadget/images/gear.png"), QString(), this);

    connect(&m_bankActionSignalMapper, SIGNAL(mapped(QString)), this, SLOT(bankAction(QString)));

    foreach(QTabBar * tabBar, m_stabTabBars) {
        for (int i = 0; i < m_stabSettingsBankCount; i++) {
            tabBar->addTab(tr("Settings Bank %1").arg(i + 1));
            tabBar->setTabData(i, QString("StabilizationSettingsBank%1").arg(i + 1));
            QToolButton *tabButton = new QToolButton();
            connect(this, SIGNAL(enableControlsChanged(bool)), tabButton, SLOT(setEnabled(bool)));
            tabButton->setDefaultAction(defaultStabMenuAction);
            tabButton->setAutoRaise(true);
            tabButton->setPopupMode(QToolButton::InstantPopup);
            tabButton->setToolTip(tr("The functions in this menu affect all fields in the settings banks,\n"
                                     "not only the ones visible on screen."));
            QMenu *tabMenu     = new QMenu();
            QMenu *restoreMenu = new QMenu(tr("Restore"));
            QMenu *resetMenu   = new QMenu(tr("Reset"));
            QMenu *copyMenu    = new QMenu(tr("Copy"));
            QMenu *swapMenu    = new QMenu(tr("Swap"));
            QAction *menuAction;
            for (int j = 0; j < m_stabSettingsBankCount; j++) {
                if (j != i) {
                    menuAction = new QAction(tr("from %1").arg(j + 1), this);
                    connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
                    m_bankActionSignalMapper.setMapping(menuAction, QString("copy:%1:%2").arg(j).arg(i));
                    copyMenu->addAction(menuAction);

                    menuAction = new QAction(tr("to %1").arg(j + 1), this);
                    connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
                    m_bankActionSignalMapper.setMapping(menuAction, QString("copy:%1:%2").arg(i).arg(j));
                    copyMenu->addAction(menuAction);

                    menuAction = new QAction(tr("with %1").arg(j + 1), this);
                    connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
                    m_bankActionSignalMapper.setMapping(menuAction, QString("swap:%1:%2").arg(i).arg(j));
                    swapMenu->addAction(menuAction);
                }
            }
            // copy bank to all others
            menuAction = new QAction(tr("to others"), this);
            connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
            m_bankActionSignalMapper.setMapping(menuAction, QString("copyAll:%1").arg(i));
            copyMenu->addAction(menuAction);
            // restore
            menuAction = new QAction(tr("to saved"), this);
            connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
            m_bankActionSignalMapper.setMapping(menuAction, QString("restore:%1").arg(i));
            restoreMenu->addAction(menuAction);
            // restore all
            menuAction = new QAction(tr("all to saved"), this);
            connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
            m_bankActionSignalMapper.setMapping(menuAction, "restoreAll");
            restoreMenu->addAction(menuAction);
            // reset
            menuAction = new QAction(tr("to default"), this);
            connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
            m_bankActionSignalMapper.setMapping(menuAction, QString("reset:%1").arg(i));
            resetMenu->addAction(menuAction);
            // reset all
            menuAction = new QAction(tr("all to default"), this);
            connect(menuAction, SIGNAL(triggered()), &m_bankActionSignalMapper, SLOT(map()));
            m_bankActionSignalMapper.setMapping(menuAction, "resetAll");
            resetMenu->addAction(menuAction);
            // menu
            tabMenu->addMenu(copyMenu);
            tabMenu->addMenu(swapMenu);
            tabMenu->addMenu(resetMenu);
            tabMenu->addMenu(restoreMenu);
            tabButton->setMenu(tabMenu);
            tabBar->setTabButton(i, QTabBar::RightSide, tabButton);
        }
        tabBar->setExpanding(false);
        connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(stabBankChanged(int)));
    }

    for (int i = 0; i < m_stabSettingsBankCount; i++) {
        if (i > 0) {
            m_stabilizationObjectsString.append(",");
        }
        m_stabilizationObjectsString.append(m_stabTabBars.at(0)->tabData(i).toString());
    }
}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
    // Do nothing
}

void ConfigStabilizationWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    updateThrottleCurveFromObject();

    // Check and update basic/advanced checkboxes only if something connected
    // if something not "basic": Rate value out of slider limits or different Pitch/Roll values
    if (ui->lowThrottleZeroIntegral_8->isEnabled() && !realtimeUpdates->isActive()) {
        if ((ui->attitudeRollResponse->value() == ui->attitudePitchResponse->value()) &&
            (ui->rateRollResponse->value() == ui->ratePitchResponse->value()) &&
            (ui->rateRollResponse->value() <= ui->RateResponsivenessSlider->maximum()) &&
            (ui->ratePitchResponse->value() <= ui->RateResponsivenessSlider->maximum())) {
            ui->basicResponsivenessCheckBox->setChecked(true);
            ui->advancedResponsivenessCheckBox->setChecked(false);
        } else {
            ui->basicResponsivenessCheckBox->setChecked(false);
            ui->advancedResponsivenessCheckBox->setChecked(true);
        }
    }
}

void ConfigStabilizationWidget::updateObjectsFromWidgetsImpl()
{
    updateObjectFromThrottleCurve();
}

void ConfigStabilizationWidget::updateThrottleCurveFromObject()
{
    bool dirty = isDirty();
    UAVObject *stabBank = getObjectManager()->getObject(QString(m_stabTabBars.at(0)->tabData(m_currentStabSettingsBank).toString()));

    Q_ASSERT(stabBank);

    UAVObjectField *field = stabBank->getField("ThrustPIDScaleCurve");
    Q_ASSERT(field);

    QList<double> curve;
    for (quint32 i = 0; i < field->getNumElements(); i++) {
        curve.append(field->getValue(i).toDouble() / 100);
    }

    ui->thrustPIDScalingCurve->setCurve(&curve);

    field = stabBank->getField("EnableThrustPIDScaling");
    Q_ASSERT(field);

    bool enabled = field->getValue() == "True";
    ui->enableThrustPIDScalingCheckBox->setChecked(enabled);
    ui->thrustPIDScalingCurve->setEnabled(enabled);
    setDirty(dirty);
}

void ConfigStabilizationWidget::updateObjectFromThrottleCurve()
{
    UAVObject *stabBank = getObjectManager()->getObject(QString(m_stabTabBars.at(0)->tabData(m_currentStabSettingsBank).toString()));

    Q_ASSERT(stabBank);

    UAVObjectField *field = stabBank->getField("ThrustPIDScaleCurve");
    Q_ASSERT(field);

    QList<double> curve   = ui->thrustPIDScalingCurve->getCurve();
    for (quint32 i = 0; i < field->getNumElements(); i++) {
        field->setValue(curve.at(i) * 100, i);
    }

    field = stabBank->getField("EnableThrustPIDScaling");
    Q_ASSERT(field);
    field->setValue(ui->enableThrustPIDScalingCheckBox->isChecked() ? "True" : "False");
}

void ConfigStabilizationWidget::setupExpoPlot()
{
    ui->expoPlot->setMouseTracking(false);
    ui->expoPlot->setAxisScale(QwtPlot::xBottom, 0, 100, 25);

    QwtText title;
    title.setText(tr("Input %"));
    title.setFont(ui->expoPlot->axisFont(QwtPlot::xBottom));
    ui->expoPlot->setAxisTitle(QwtPlot::xBottom, title);
    ui->expoPlot->setAxisScale(QwtPlot::yLeft, 0, 100, 25);

    title.setText(tr("Output %"));
    title.setFont(ui->expoPlot->axisFont(QwtPlot::yLeft));
    ui->expoPlot->setAxisTitle(QwtPlot::yLeft, title);
    QwtPlotCanvas *plotCanvas = dynamic_cast<QwtPlotCanvas *>(ui->expoPlot->canvas());
    if (plotCanvas) {
        plotCanvas->setFrameStyle(QFrame::NoFrame);
    }
    ui->expoPlot->canvas()->setCursor(QCursor());

    m_plotGrid.setMajorPen(QColor(Qt::gray));
    m_plotGrid.setMinorPen(QColor(Qt::lightGray));
    m_plotGrid.enableXMin(false);
    m_plotGrid.enableYMin(false);
    m_plotGrid.attach(ui->expoPlot);

    m_expoPlotCurveRoll.setRenderHint(QwtPlotCurve::RenderAntialiased);
    QColor rollColor(Qt::red);
    rollColor.setAlpha(180);
    m_expoPlotCurveRoll.setPen(QPen(rollColor, 2));
    m_expoPlotCurveRoll.attach(ui->expoPlot);
    replotExpoRoll(ui->expoSpinnerRoll->value());
    m_expoPlotCurveRoll.show();

    QColor pitchColor(Qt::green);
    pitchColor.setAlpha(180);
    m_expoPlotCurvePitch.setRenderHint(QwtPlotCurve::RenderAntialiased);
    m_expoPlotCurvePitch.setPen(QPen(pitchColor, 2));
    m_expoPlotCurvePitch.attach(ui->expoPlot);
    replotExpoPitch(ui->expoSpinnerPitch->value());
    m_expoPlotCurvePitch.show();

    QColor yawColor(Qt::blue);
    yawColor.setAlpha(180);
    m_expoPlotCurveYaw.setRenderHint(QwtPlotCurve::RenderAntialiased);
    m_expoPlotCurveYaw.setPen(QPen(yawColor, 2));
    m_expoPlotCurveYaw.attach(ui->expoPlot);
    replotExpoYaw(ui->expoSpinnerYaw->value());
    m_expoPlotCurveYaw.show();
}

void ConfigStabilizationWidget::resetThrottleCurveToDefault()
{
    UAVDataObject *defaultStabBank = (UAVDataObject *)getObjectManager()->getObject(QString(m_stabTabBars.at(0)->tabData(m_currentStabSettingsBank).toString()));

    Q_ASSERT(defaultStabBank);
    defaultStabBank = defaultStabBank->dirtyClone();

    UAVObjectField *field = defaultStabBank->getField("ThrustPIDScaleCurve");
    Q_ASSERT(field);

    QList<double> curve;
    for (quint32 i = 0; i < field->getNumElements(); i++) {
        curve.append(field->getValue(i).toDouble() / 100);
    }

    ui->thrustPIDScalingCurve->setCurve(&curve);

    field = defaultStabBank->getField("EnableThrustPIDScaling");
    Q_ASSERT(field);

    bool enabled = field->getValue() == "True";
    ui->enableThrustPIDScalingCheckBox->setChecked(enabled);
    ui->thrustPIDScalingCurve->setEnabled(enabled);

    delete defaultStabBank;
}

void ConfigStabilizationWidget::throttleCurveUpdated()
{
    setDirty(true);
}

void ConfigStabilizationWidget::replotExpo(int value, QwtPlotCurve &curve)
{
    double x[EXPO_CURVE_POINTS_COUNT] = { 0 };
    double y[EXPO_CURVE_POINTS_COUNT] = { 0 };
    double factor = pow(EXPO_CURVE_CONSTANT, value);
    double step   = 1.0 / (EXPO_CURVE_POINTS_COUNT - 1);

    for (int i = 0; i < EXPO_CURVE_POINTS_COUNT; i++) {
        double val = i * step;
        x[i] = val * 100.0;
        y[i] = pow(val, factor) * 100.0;
    }
    curve.setSamples(x, y, EXPO_CURVE_POINTS_COUNT);
    ui->expoPlot->replot();
}

void ConfigStabilizationWidget::replotExpoRoll(int value)
{
    replotExpo(value, m_expoPlotCurveRoll);
}

void ConfigStabilizationWidget::replotExpoPitch(int value)
{
    replotExpo(value, m_expoPlotCurvePitch);
}

void ConfigStabilizationWidget::replotExpoYaw(int value)
{
    replotExpo(value, m_expoPlotCurveYaw);
}

UAVObject *ConfigStabilizationWidget::getStabBankObject(int bank)
{
    return getObject(QString("StabilizationSettingsBank%1").arg(bank + 1));
}

void ConfigStabilizationWidget::resetBank(int bank)
{
    UAVDataObject *stabBankObject = dynamic_cast<UAVDataObject *>(getStabBankObject(bank));

    if (stabBankObject) {
        UAVDataObject *defaultStabBankObject = stabBankObject->dirtyClone();
        quint8 data[stabBankObject->getNumBytes()];
        defaultStabBankObject->pack(data);
        stabBankObject->unpack(data);
    }
}

void ConfigStabilizationWidget::restoreBank(int bank)
{
    UAVObject *stabBankObject = getStabBankObject(bank);

    if (stabBankObject) {
        ObjectPersistence *objectPersistenceObject = ObjectPersistence::GetInstance(getObjectManager());
        QTimer updateTimer(this);
        QEventLoop eventLoop(this);
        connect(&updateTimer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
        connect(objectPersistenceObject, SIGNAL(objectUpdated(UAVObject *)), &eventLoop, SLOT(quit()));

        ObjectPersistence::DataFields data;
        data.Operation  = ObjectPersistence::OPERATION_LOAD;
        data.Selection  = ObjectPersistence::SELECTION_SINGLEOBJECT;
        data.ObjectID   = stabBankObject->getObjID();
        data.InstanceID = stabBankObject->getInstID();
        objectPersistenceObject->setData(data);
        objectPersistenceObject->updated();
        updateTimer.start(500);
        eventLoop.exec();
        if (updateTimer.isActive()) {
            stabBankObject->requestUpdate();
        }
        updateTimer.stop();
    }
}

void ConfigStabilizationWidget::copyBank(int fromBank, int toBank)
{
    UAVObject *fromStabBankObject = getStabBankObject(fromBank);
    UAVObject *toStabBankObject   = getStabBankObject(toBank);

    if (fromStabBankObject && toStabBankObject) {
        quint8 data[fromStabBankObject->getNumBytes()];
        fromStabBankObject->pack(data);
        toStabBankObject->unpack(data);
    }
}

void ConfigStabilizationWidget::swapBank(int fromBank, int toBank)
{
    UAVObject *fromStabBankObject = getStabBankObject(fromBank);
    UAVObject *toStabBankObject   = getStabBankObject(toBank);

    if (fromStabBankObject && toStabBankObject) {
        quint8 fromStabBankObjectData[fromStabBankObject->getNumBytes()];
        quint8 toStabBankObjectData[toStabBankObject->getNumBytes()];
        fromStabBankObject->pack(fromStabBankObjectData);
        toStabBankObject->pack(toStabBankObjectData);
        toStabBankObject->unpack(fromStabBankObjectData);
        fromStabBankObject->unpack(toStabBankObjectData);
    }
}

void ConfigStabilizationWidget::bankAction(const QString &mapping)
{
    QStringList list = mapping.split(":");
    QString action   = list[0];

    if (action == "copy") {
        int fromBank = list[1].toInt();
        Q_ASSERT((fromBank >= 0) && (fromBank < m_stabSettingsBankCount));
        int toBank   = list[2].toInt();
        Q_ASSERT((toBank >= 0) && (toBank < m_stabSettingsBankCount));
        copyBank(fromBank, toBank);
    } else if (action == "copyAll") {
        int fromBank = list[1].toInt();
        Q_ASSERT((fromBank >= 0) && (fromBank < m_stabSettingsBankCount));
        for (int toBank = 0; toBank < m_stabSettingsBankCount; toBank++) {
            if (fromBank != toBank) {
                copyBank(fromBank, toBank);
            }
        }
    } else if (action == "swap") {
        int fromBank = list[1].toInt();
        Q_ASSERT((fromBank >= 0) && (fromBank < m_stabSettingsBankCount));
        int toBank   = list[2].toInt();
        Q_ASSERT((toBank >= 0) && (toBank < m_stabSettingsBankCount));
        swapBank(fromBank, toBank);
    } else if (action == "restore") {
        int bank = list[1].toInt();
        Q_ASSERT((bank >= 0) && (bank < m_stabSettingsBankCount));
        restoreBank(bank);
    } else if (action == "restoreAll") {
        for (int bank = 0; bank < m_stabSettingsBankCount; bank++) {
            restoreBank(bank);
        }
    } else if (action == "reset") {
        int bank = list[1].toInt();
        Q_ASSERT((bank >= 0) && (bank < m_stabSettingsBankCount));
        resetBank(bank);
    } else if (action == "resetAll") {
        for (int bank = 0; bank < m_stabSettingsBankCount; bank++) {
            resetBank(bank);
        }
    }
}

void ConfigStabilizationWidget::realtimeUpdatesSlot(bool value)
{
    ui->realTimeUpdates_6->setChecked(value);
    ui->realTimeUpdates_8->setChecked(value);
    ui->realTimeUpdates_12->setChecked(value);
    ui->realTimeUpdates_7->setChecked(value);

    if (value && !realtimeUpdates->isActive()) {
        realtimeUpdates->start(AUTOMATIC_UPDATE_RATE);
    } else if (!value && realtimeUpdates->isActive()) {
        realtimeUpdates->stop();
    }
}

void ConfigStabilizationWidget::linkCheckBoxes(bool value)
{
    if (sender() == ui->checkBox_7) {
        ui->checkBox_3->setChecked(value);
    } else if (sender() == ui->checkBox_3) {
        ui->checkBox_7->setChecked(value);
    } else if (sender() == ui->checkBox_8) {
        ui->checkBox_2->setChecked(value);
    } else if (sender() == ui->checkBox_2) {
        ui->checkBox_8->setChecked(value);
    } else if (sender() == ui->basicResponsivenessCheckBox) {
        ui->advancedResponsivenessCheckBox->setChecked(!value);
        ui->basicResponsivenessControls->setEnabled(value);
        ui->advancedResponsivenessControls->setEnabled(!value);
        if (value) {
            processLinkedWidgets(ui->AttitudeResponsivenessSlider);
            processLinkedWidgets(ui->RateResponsivenessSlider);
        }
    } else if (sender() == ui->advancedResponsivenessCheckBox) {
        ui->basicResponsivenessCheckBox->setChecked(!value);
        ui->basicResponsivenessControls->setEnabled(!value);
        ui->advancedResponsivenessControls->setEnabled(value);
    } else if (sender() == ui->checkBoxLinkAcroFactors) {
        processLinkedWidgets(ui->AcroFactorRollSlider);
    }
}

void ConfigStabilizationWidget::processLinkedWidgets(QWidget *widget)
{
    if (ui->checkBox_7->isChecked()) {
        if (widget == ui->RateRollKp_2) {
            ui->RatePitchKp->setValue(ui->RateRollKp_2->value());
        } else if (widget == ui->RateRollKi_2) {
            ui->RatePitchKi->setValue(ui->RateRollKi_2->value());
        } else if (widget == ui->RatePitchKp) {
            ui->RateRollKp_2->setValue(ui->RatePitchKp->value());
        } else if (widget == ui->RatePitchKi) {
            ui->RateRollKi_2->setValue(ui->RatePitchKi->value());
        } else if (widget == ui->RollRateKd) {
            ui->PitchRateKd->setValue(ui->RollRateKd->value());
        } else if (widget == ui->PitchRateKd) {
            ui->RollRateKd->setValue(ui->PitchRateKd->value());
        }
    }

    if (ui->checkBox_8->isChecked()) {
        if (widget == ui->AttitudeRollKp) {
            ui->AttitudePitchKp_2->setValue(ui->AttitudeRollKp->value());
        } else if (widget == ui->AttitudeRollKi) {
            ui->AttitudePitchKi_2->setValue(ui->AttitudeRollKi->value());
        } else if (widget == ui->AttitudePitchKp_2) {
            ui->AttitudeRollKp->setValue(ui->AttitudePitchKp_2->value());
        } else if (widget == ui->AttitudePitchKi_2) {
            ui->AttitudeRollKi->setValue(ui->AttitudePitchKi_2->value());
        }
    }

    if (ui->basicResponsivenessCheckBox->isChecked()) {
        if (widget == ui->AttitudeResponsivenessSlider) {
            ui->attitudePitchResponse->setValue(ui->AttitudeResponsivenessSlider->value());
            ui->attitudeRollResponse->setValue(ui->AttitudeResponsivenessSlider->value());
        } else if (widget == ui->RateResponsivenessSlider) {
            ui->ratePitchResponse->setValue(ui->RateResponsivenessSlider->value());
            ui->rateRollResponse->setValue(ui->RateResponsivenessSlider->value());
        }
    }
    if (ui->checkBoxLinkAcroFactors->isChecked()) {
        if (widget == ui->AcroFactorRollSlider) {
            ui->AcroFactorPitchSlider->setValue(ui->AcroFactorRollSlider->value());
        } else if (widget == ui->AcroFactorPitchSlider) {
            ui->AcroFactorRollSlider->setValue(ui->AcroFactorPitchSlider->value());
        }
    }
}

void ConfigStabilizationWidget::enableControlsChanged(bool enable)
{
    // If Revolution/Sparky2 board enable Althold tab, otherwise disable it
    bool enableAltitudeHold = (((boardModel() & 0xff00) == 0x0900) || ((boardModel() & 0xff00) == 0x9200));

    ui->AltitudeHold->setEnabled(enable && enableAltitudeHold);
}

void ConfigStabilizationWidget::stabBankChanged(int index)
{
    bool dirty = isDirty();

    disconnect(this, SIGNAL(widgetContentsChanged(QWidget *)), this, SLOT(processLinkedWidgets(QWidget *)));

    updateObjectFromThrottleCurve();
    foreach(QTabBar * tabBar, m_stabTabBars) {
        disconnect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(stabBankChanged(int)));
        tabBar->setCurrentIndex(index);
        connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(stabBankChanged(int)));
    }

    for (int i = 0; i < m_stabTabBars.at(0)->count(); i++) {
        setWidgetBindingObjectEnabled(m_stabTabBars.at(0)->tabData(i).toString(), false);
    }

    setWidgetBindingObjectEnabled(m_stabTabBars.at(0)->tabData(index).toString(), true);

    m_currentStabSettingsBank = index;
    updateThrottleCurveFromObject();

    connect(this, SIGNAL(widgetContentsChanged(QWidget *)), this, SLOT(processLinkedWidgets(QWidget *)));
    setDirty(dirty);
}

bool ConfigStabilizationWidget::shouldObjectBeSaved(UAVObject *object)
{
    // AltitudeHoldSettings should only be saved for Revolution/Sparky2 board to avoid error.
    if (((boardModel() & 0xff00) != 0x0900) && ((boardModel() & 0xff00) != 0x9200)) {
        return dynamic_cast<AltitudeHoldSettings *>(object) == 0;
    } else {
        return true;
    }
}

QString ConfigStabilizationWidget::mapObjectName(const QString objectName)
{
    if (objectName == "StabilizationSettingsBankX") {
        return m_stabilizationObjectsString;
    }
    return ConfigTaskWidget::mapObjectName(objectName);
}
