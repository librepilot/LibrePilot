/**
 ******************************************************************************
 *
 * @file       configrevowidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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
#include "configrevowidget.h"

#include "ui_revosensors.h"

#include <uavobjectmanager.h>
#include <uavobjecthelper.h>

#include <attitudestate.h>
#include <attitudesettings.h>
#include <revocalibration.h>
#include <accelgyrosettings.h>
#include <homelocation.h>
#include <accelstate.h>
#include <magstate.h>

#include "assertions.h"
#include "calibration.h"
#include "calibration/calibrationutils.h"

#include <math.h>
#include <iostream>

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QThread>

// #define DEBUG

#define MAG_ALARM_THRESHOLD 5

// Uncomment this to enable 6 point calibration on the accels
#define NOISE_SAMPLES       50

class Thread : public QThread {
public:
    static void usleep(unsigned long usecs)
    {
        QThread::usleep(usecs);
    }
};

ConfigRevoWidget::ConfigRevoWidget(QWidget *parent) :
    ConfigTaskWidget(parent), isBoardRotationStored(false)
{
    m_ui = new Ui_RevoSensorsWidget();
    m_ui->setupUi(this);
    m_ui->tabWidget->setCurrentIndex(0);

    // must be done before auto binding !
    setWikiURL("Revo+Attitude+Configuration");

    addAutoBindings();

    // Initialization of the visual help
    m_ui->calibrationVisualHelp->setScene(new QGraphicsScene(this));
    m_ui->calibrationVisualHelp->setRenderHint(QPainter::HighQualityAntialiasing, true);
    m_ui->calibrationVisualHelp->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_ui->calibrationVisualHelp->setBackgroundBrush(QBrush(QColor(200, 200, 200)));
    displayVisualHelp("empty");

    // Must set up the UI (above) before setting up the UAVO mappings or refreshWidgetValues
    // will be dealing with some null pointers
    addUAVObject("HomeLocation");
    addUAVObject("RevoCalibration");
    addUAVObject("AttitudeSettings");
    addUAVObject("RevoSettings");
    addUAVObject("AccelGyroSettings");
    addUAVObject("AuxMagSettings");

    // accel calibration
    m_accelCalibrationModel = new OpenPilot::SixPointCalibrationModel(this);
    connect(m_ui->accelStart, SIGNAL(clicked()), m_accelCalibrationModel, SLOT(accelStart()));
    connect(m_ui->accelSavePos, SIGNAL(clicked()), m_accelCalibrationModel, SLOT(savePositionData()));

    connect(m_accelCalibrationModel, SIGNAL(started()), this, SLOT(disableAllCalibrations()));
    connect(m_accelCalibrationModel, SIGNAL(stopped()), this, SLOT(enableAllCalibrations()));
    connect(m_accelCalibrationModel, SIGNAL(storeAndClearBoardRotation()), this, SLOT(storeAndClearBoardRotation()));
    connect(m_accelCalibrationModel, SIGNAL(recallBoardRotation()), this, SLOT(recallBoardRotation()));
    connect(m_accelCalibrationModel, SIGNAL(displayInstructions(QString, WizardModel::MessageType)),
            this, SLOT(addInstructions(QString, WizardModel::MessageType)));
    connect(m_accelCalibrationModel, SIGNAL(displayVisualHelp(QString)), this, SLOT(displayVisualHelp(QString)));
    connect(m_accelCalibrationModel, SIGNAL(savePositionEnabledChanged(bool)), m_ui->accelSavePos, SLOT(setEnabled(bool)));
    connect(m_accelCalibrationModel, SIGNAL(progressChanged(int)), m_ui->accelProgress, SLOT(setValue(int)));
    m_ui->accelSavePos->setEnabled(false);

    // mag calibration
    m_magCalibrationModel = new OpenPilot::SixPointCalibrationModel(this);
    connect(m_ui->magStart, SIGNAL(clicked()), m_magCalibrationModel, SLOT(magStart()));
    connect(m_ui->magSavePos, SIGNAL(clicked()), m_magCalibrationModel, SLOT(savePositionData()));

    connect(m_magCalibrationModel, SIGNAL(started()), this, SLOT(disableAllCalibrations()));
    connect(m_magCalibrationModel, SIGNAL(stopped()), this, SLOT(enableAllCalibrations()));
    connect(m_magCalibrationModel, SIGNAL(storeAndClearBoardRotation()), this, SLOT(storeAndClearBoardRotation()));
    connect(m_magCalibrationModel, SIGNAL(recallBoardRotation()), this, SLOT(recallBoardRotation()));
    connect(m_magCalibrationModel, SIGNAL(displayInstructions(QString, WizardModel::MessageType)),
            this, SLOT(addInstructions(QString, WizardModel::MessageType)));
    connect(m_magCalibrationModel, SIGNAL(displayVisualHelp(QString)), this, SLOT(displayVisualHelp(QString)));
    connect(m_magCalibrationModel, SIGNAL(savePositionEnabledChanged(bool)), m_ui->magSavePos, SLOT(setEnabled(bool)));
    connect(m_magCalibrationModel, SIGNAL(progressChanged(int)), m_ui->magProgress, SLOT(setValue(int)));
    m_ui->magSavePos->setEnabled(false);

    // board level calibration
    m_levelCalibrationModel = new OpenPilot::LevelCalibrationModel(this);
    connect(m_ui->boardLevelStart, SIGNAL(clicked()), m_levelCalibrationModel, SLOT(start()));
    connect(m_ui->boardLevelSavePos, SIGNAL(clicked()), m_levelCalibrationModel, SLOT(savePosition()));

    connect(m_levelCalibrationModel, SIGNAL(started()), this, SLOT(disableAllCalibrations()));
    connect(m_levelCalibrationModel, SIGNAL(stopped()), this, SLOT(enableAllCalibrations()));
    connect(m_levelCalibrationModel, SIGNAL(displayInstructions(QString, WizardModel::MessageType)),
            this, SLOT(addInstructions(QString, WizardModel::MessageType)));
    connect(m_levelCalibrationModel, SIGNAL(displayVisualHelp(QString)), this, SLOT(displayVisualHelp(QString)));
    connect(m_levelCalibrationModel, SIGNAL(savePositionEnabledChanged(bool)), m_ui->boardLevelSavePos, SLOT(setEnabled(bool)));
    connect(m_levelCalibrationModel, SIGNAL(progressChanged(int)), m_ui->boardLevelProgress, SLOT(setValue(int)));
    m_ui->boardLevelSavePos->setEnabled(false);

    // gyro zero calibration
    m_gyroBiasCalibrationModel = new OpenPilot::GyroBiasCalibrationModel(this);
    connect(m_ui->gyroBiasStart, SIGNAL(clicked()), m_gyroBiasCalibrationModel, SLOT(start()));

    connect(m_gyroBiasCalibrationModel, SIGNAL(progressChanged(int)), m_ui->gyroBiasProgress, SLOT(setValue(int)));

    connect(m_gyroBiasCalibrationModel, SIGNAL(started()), this, SLOT(disableAllCalibrations()));
    connect(m_gyroBiasCalibrationModel, SIGNAL(stopped()), this, SLOT(enableAllCalibrations()));
    connect(m_gyroBiasCalibrationModel, SIGNAL(displayInstructions(QString, WizardModel::MessageType)),
            this, SLOT(addInstructions(QString, WizardModel::MessageType)));
    connect(m_gyroBiasCalibrationModel, SIGNAL(displayVisualHelp(QString)), this, SLOT(displayVisualHelp(QString)));

    // thermal calibration
    m_thermalCalibrationModel = new OpenPilot::ThermalCalibrationModel(this);
    connect(m_ui->thermalBiasStart, SIGNAL(clicked()), m_thermalCalibrationModel, SLOT(btnStart()));
    connect(m_ui->thermalBiasEnd, SIGNAL(clicked()), m_thermalCalibrationModel, SLOT(btnEnd()));
    connect(m_ui->thermalBiasCancel, SIGNAL(clicked()), m_thermalCalibrationModel, SLOT(btnAbort()));

    connect(m_thermalCalibrationModel, SIGNAL(startEnabledChanged(bool)), m_ui->thermalBiasStart, SLOT(setEnabled(bool)));
    connect(m_thermalCalibrationModel, SIGNAL(endEnabledChanged(bool)), m_ui->thermalBiasEnd, SLOT(setEnabled(bool)));
    connect(m_thermalCalibrationModel, SIGNAL(cancelEnabledChanged(bool)), m_ui->thermalBiasCancel, SLOT(setEnabled(bool)));
    connect(m_thermalCalibrationModel, SIGNAL(wizardStarted()), this, SLOT(disableAllCalibrations()));
    connect(m_thermalCalibrationModel, SIGNAL(wizardStopped()), this, SLOT(enableAllCalibrations()));

    connect(m_thermalCalibrationModel, SIGNAL(instructionsAdded(QString, WizardModel::MessageType)),
            this, SLOT(addInstructions(QString, WizardModel::MessageType)));
    connect(m_thermalCalibrationModel, SIGNAL(temperatureChanged(float)), this, SLOT(displayTemperature(float)));
    connect(m_thermalCalibrationModel, SIGNAL(temperatureGradientChanged(float)), this, SLOT(displayTemperatureGradient(float)));
    connect(m_thermalCalibrationModel, SIGNAL(temperatureRangeChanged(float)), this, SLOT(displayTemperatureRange(float)));
    connect(m_thermalCalibrationModel, SIGNAL(progressChanged(int)), m_ui->thermalBiasProgress, SLOT(setValue(int)));
    connect(m_thermalCalibrationModel, SIGNAL(progressMaxChanged(int)), m_ui->thermalBiasProgress, SLOT(setMaximum(int)));
    m_thermalCalibrationModel->init();

    // home location
    connect(m_ui->hlClearButton, SIGNAL(clicked()), this, SLOT(clearHomeLocation()));

    addWidgetBinding("RevoSettings", "FusionAlgorithm", m_ui->FusionAlgorithm, 0, 1, true);

    addWidgetBinding("AttitudeSettings", "BoardRotation", m_ui->rollRotation, AttitudeSettings::BOARDROTATION_ROLL);
    addWidgetBinding("AttitudeSettings", "BoardRotation", m_ui->pitchRotation, AttitudeSettings::BOARDROTATION_PITCH);
    addWidgetBinding("AttitudeSettings", "BoardRotation", m_ui->yawRotation, AttitudeSettings::BOARDROTATION_YAW);
    addWidgetBinding("AttitudeSettings", "AccelTau", m_ui->accelTau);

    addWidgetBinding("AttitudeSettings", "ZeroDuringArming", m_ui->zeroGyroBiasOnArming);
    addWidgetBinding("AttitudeSettings", "InitialZeroWhenBoardSteady", m_ui->initGyroWhenBoardSteady);

    addWidgetBinding("AuxMagSettings", "Usage", m_ui->auxMagUsage, 0, 1, true);
    addWidgetBinding("AuxMagSettings", "Type", m_ui->auxMagType, 0, 1, true);

    addWidgetBinding("RevoSettings", "MagnetometerMaxDeviation", m_ui->maxDeviationWarning, RevoSettings::MAGNETOMETERMAXDEVIATION_WARNING);
    addWidgetBinding("RevoSettings", "MagnetometerMaxDeviation", m_ui->maxDeviationError, RevoSettings::MAGNETOMETERMAXDEVIATION_ERROR);

    addWidgetBinding("AuxMagSettings", "BoardRotation", m_ui->auxMagRollRotation, AuxMagSettings::BOARDROTATION_ROLL);
    addWidgetBinding("AuxMagSettings", "BoardRotation", m_ui->auxMagPitchRotation, AuxMagSettings::BOARDROTATION_PITCH);
    addWidgetBinding("AuxMagSettings", "BoardRotation", m_ui->auxMagYawRotation, AuxMagSettings::BOARDROTATION_YAW);

    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onBoardAuxMagError()));
    connect(MagSensor::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(onBoardAuxMagError()));
    connect(MagState::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateMagStatus()));
    connect(HomeLocation::GetInstance(getObjectManager()), SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateMagBeVector()));

    addWidget(m_ui->internalAuxErrorX);
    addWidget(m_ui->internalAuxErrorY);
    addWidget(m_ui->internalAuxErrorZ);

    displayMagError = false;

    enableAllCalibrations();
}

ConfigRevoWidget::~ConfigRevoWidget()
{
    // Do nothing
}

void ConfigRevoWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    updateVisualHelp();
}

void ConfigRevoWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateVisualHelp();
}

void ConfigRevoWidget::updateVisualHelp()
{
    m_ui->calibrationVisualHelp->fitInView(m_ui->calibrationVisualHelp->scene()->sceneRect(), Qt::KeepAspectRatio);
}

void ConfigRevoWidget::storeAndClearBoardRotation()
{
    if (!isBoardRotationStored) {
        UAVObjectUpdaterHelper updateHelper;

        // Store current board rotation and board level trim
        isBoardRotationStored = true;
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        AttitudeSettings::DataFields data  = attitudeSettings->getData();
        storedBoardRotation[AttitudeSettings::BOARDROTATION_YAW]     = data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW];
        storedBoardRotation[AttitudeSettings::BOARDROTATION_ROLL]    = data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL];
        storedBoardRotation[AttitudeSettings::BOARDROTATION_PITCH]   = data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH];
        storedBoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_ROLL]  = data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_ROLL];
        storedBoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_PITCH] = data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_PITCH];

        // Set board rotation to zero
        data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW]     = 0;
        data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL]    = 0;
        data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH]   = 0;

        // Set board level trim to zero
        data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_ROLL]  = 0;
        data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_PITCH] = 0;

        attitudeSettings->setData(data, false);
        updateHelper.doObjectAndWait(attitudeSettings);

        // Store current aux mag board rotation
        AuxMagSettings *auxMagSettings = AuxMagSettings::GetInstance(getObjectManager());
        Q_ASSERT(auxMagSettings);
        AuxMagSettings::DataFields auxMagData = auxMagSettings->getData();
        auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_YAW]   = auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_YAW];
        auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_ROLL]  = auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_ROLL];
        auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_PITCH] = auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_PITCH];

        // Set aux mag board rotation to no rotation
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_YAW]    = 0;
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_ROLL]   = 0;
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_PITCH]  = 0;

        auxMagSettings->setData(auxMagData, false);
        updateHelper.doObjectAndWait(auxMagSettings);
    }
}

void ConfigRevoWidget::recallBoardRotation()
{
    if (isBoardRotationStored) {
        UAVObjectUpdaterHelper updateHelper;

        // Recall current board rotation
        isBoardRotationStored = false;

        // Restore the flight controller board rotation and board level trim
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        AttitudeSettings::DataFields data  = attitudeSettings->getData();
        data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW]     = storedBoardRotation[AttitudeSettings::BOARDROTATION_YAW];
        data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL]    = storedBoardRotation[AttitudeSettings::BOARDROTATION_ROLL];
        data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH]   = storedBoardRotation[AttitudeSettings::BOARDROTATION_PITCH];
        data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_ROLL]  = storedBoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_ROLL];
        data.BoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_PITCH] = storedBoardLevelTrim[AttitudeSettings::BOARDLEVELTRIM_PITCH];

        attitudeSettings->setData(data, false);
        updateHelper.doObjectAndWait(attitudeSettings);

        // Restore the aux mag board rotation
        AuxMagSettings *auxMagSettings = AuxMagSettings::GetInstance(getObjectManager());
        Q_ASSERT(auxMagSettings);
        AuxMagSettings::DataFields auxMagData = auxMagSettings->getData();
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_YAW]   = auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_YAW];
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_ROLL]  = auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_ROLL];
        auxMagData.BoardRotation[AuxMagSettings::BOARDROTATION_PITCH] = auxMagStoredBoardRotation[AuxMagSettings::BOARDROTATION_PITCH];

        auxMagSettings->setData(auxMagData, false);
        updateHelper.doObjectAndWait(auxMagSettings);
    }
}

/**
   Show the selected visual aid
 */
void ConfigRevoWidget::displayVisualHelp(QString elementID)
{
    m_ui->calibrationVisualHelp->scene()->clear();
    QPixmap pixmap = QPixmap(":/configgadget/images/calibration/" + elementID + ".png");
    m_ui->calibrationVisualHelp->scene()->addPixmap(pixmap);
    m_ui->calibrationVisualHelp->setSceneRect(pixmap.rect());
    updateVisualHelp();
}

void ConfigRevoWidget::clearInstructions()
{
    m_ui->calibrationInstructions->clear();
}

void ConfigRevoWidget::addInstructions(QString text, WizardModel::MessageType type)
{
    QString msg;

    switch (type) {
    case WizardModel::Debug:
#ifdef DEBUG
        msg = QString("<i>%1</i>").arg(text);
#endif
        break;
    case WizardModel::Info:
        msg = QString("%1").arg(text);
        break;
    case WizardModel::Prompt:
        msg = QString("<b><font color='blue'>%1</font>").arg(text);
        break;
    case WizardModel::Warn:
        msg = QString("<b>%1</b>").arg(text);
        break;
    case WizardModel::Success:
        msg = QString("<b><font color='green'>%1</font>").arg(text);
        break;
    case WizardModel::Failure:
        msg = QString("<b><font color='red'>%1</font>").arg(text);
        break;
    default:
        break;
    }
    if (!msg.isEmpty()) {
        m_ui->calibrationInstructions->append(msg);
    }
}

static QString format(float v)
{
    QString str;

    if (!std::isnan(v)) {
        // format as ##.##
        str = QString("%1").arg(v, 5, 'f', 2, ' ');
        str = str.replace(" ", "&nbsp;");
    } else {
        str = "--.--";
    }
    // use a fixed width font
    QString style("font-family:courier new,monospace;");
    return QString("<span style=\"%1\">%2</span>").arg(style).arg(str);
}

void ConfigRevoWidget::displayTemperature(float temperature)
{
    m_ui->temperatureLabel->setText(tr("Temperature: %1°C").arg(format(temperature)));
}

void ConfigRevoWidget::displayTemperatureGradient(float temperatureGradient)
{
    m_ui->temperatureGradientLabel->setText(tr("Gradient: %1°C/min").arg(format(temperatureGradient)));
}

void ConfigRevoWidget::displayTemperatureRange(float temperatureRange)
{
    m_ui->temperatureRangeLabel->setText(tr("Sampled range: %1°C").arg(format(temperatureRange)));
}

/**
 * Called by the ConfigTaskWidget parent when RevoCalibration is updated
 * to update the UI
 */
void ConfigRevoWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    Q_UNUSED(obj);

    m_ui->isSetCheckBox->setEnabled(true);
    m_ui->isSetCheckBox->setToolTip(tr("When checked, the current Home Location is saved to the board.\n"
                                       "When unchecked, the Home Location will be updated and set using\n"
                                       "the first GPS position received after power up."));

    HomeLocation *homeLocation = HomeLocation::GetInstance(getObjectManager());
    Q_ASSERT(homeLocation);
    HomeLocation::DataFields homeLocationData = homeLocation->getData();

    QString beStr = QString("%1:%2:%3").arg(QString::number(homeLocationData.Be[0]), QString::number(homeLocationData.Be[1]), QString::number(homeLocationData.Be[2]));
    m_ui->beBox->setText(beStr);

    updateMagBeVector();
    onBoardAuxMagError();
}

void ConfigRevoWidget::updateObjectsFromWidgetsImpl()
{
    if (m_accelCalibrationModel->dirty()) {
        m_accelCalibrationModel->save();
    }
    if (m_magCalibrationModel->dirty()) {
        m_magCalibrationModel->save();
    }
    if (m_levelCalibrationModel->dirty()) {
        m_levelCalibrationModel->save();
    }
    if (m_gyroBiasCalibrationModel->dirty()) {
        m_gyroBiasCalibrationModel->save();
    }
    if (m_thermalCalibrationModel->dirty()) {
        m_thermalCalibrationModel->save();
    }
}

void ConfigRevoWidget::clearHomeLocation()
{
    HomeLocation *homeLocation = HomeLocation::GetInstance(getObjectManager());

    Q_ASSERT(homeLocation);
    HomeLocation::DataFields homeLocationData;
    homeLocationData.Latitude  = 0;
    homeLocationData.Longitude = 0;
    homeLocationData.Altitude  = 0;
    homeLocationData.Be[0]     = 0;
    homeLocationData.Be[1]     = 0;
    homeLocationData.Be[2]     = 0;
    homeLocationData.g_e = 9.81f;
    homeLocationData.Set = HomeLocation::SET_FALSE;
    homeLocation->setData(homeLocationData);
}

void ConfigRevoWidget::disableAllCalibrations()
{
    clearInstructions();

    m_ui->accelStart->setEnabled(false);
    m_ui->magStart->setEnabled(false);
    m_ui->boardLevelStart->setEnabled(false);
    m_ui->gyroBiasStart->setEnabled(false);
    m_ui->thermalBiasStart->setEnabled(false);
}

void ConfigRevoWidget::enableAllCalibrations()
{
    // TODO this logic should not be here and should use a signal instead
    // need to check if ConfigTaskWidget has support for this kind of use cases
    if (m_accelCalibrationModel->dirty() || m_magCalibrationModel->dirty() || m_levelCalibrationModel->dirty()
        || m_gyroBiasCalibrationModel->dirty() || m_thermalCalibrationModel->dirty()) {
        widgetsContentsChanged();
    }

    m_ui->accelStart->setEnabled(true);
    m_ui->magStart->setEnabled(true);
    m_ui->boardLevelStart->setEnabled(true);
    m_ui->gyroBiasStart->setEnabled(true);
    m_ui->thermalBiasStart->setEnabled(true);
}

void ConfigRevoWidget::onBoardAuxMagError()
{
    MagSensor *magSensor = MagSensor::GetInstance(getObjectManager());

    Q_ASSERT(magSensor);
    AuxMagSensor *auxMagSensor = AuxMagSensor::GetInstance(getObjectManager());
    Q_ASSERT(auxMagSensor);

    if (m_ui->tabWidget->currentIndex() != 2) {
        // Apply default metadata
        if (displayMagError) {
            magSensor->setMetadata(magSensor->getDefaultMetadata());
            auxMagSensor->setMetadata(auxMagSensor->getDefaultMetadata());
            displayMagError = false;
        }
        return;
    }

    if (!displayMagError) {
        // Apply new rates
        UAVObject::Metadata mdata = magSensor->getMetadata();
        UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_PERIODIC);
        mdata.flightTelemetryUpdatePeriod = 300;
        magSensor->setMetadata(mdata);

        mdata = auxMagSensor->getMetadata();
        UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_PERIODIC);
        mdata.flightTelemetryUpdatePeriod = 300;
        auxMagSensor->setMetadata(mdata);

        displayMagError = true;
        return;
    }

    float onboardMag[3];
    float auxMag[3];

    onboardMag[0] = magSensor->x();
    onboardMag[1] = magSensor->y();
    onboardMag[2] = magSensor->z();

    float normalizedMag[3];
    float normalizedAuxMag[3];
    float xDiff     = 0.0f;
    float yDiff     = 0.0f;
    float zDiff     = 0.0f;

    // Smooth Mag readings
    float alpha     = 0.7f;
    float inv_alpha = (1.0f - alpha);
    // Onboard mag
    onboardMagFiltered[0] = (onboardMagFiltered[0] * alpha) + (onboardMag[0] * inv_alpha);
    onboardMagFiltered[1] = (onboardMagFiltered[1] * alpha) + (onboardMag[1] * inv_alpha);
    onboardMagFiltered[2] = (onboardMagFiltered[2] * alpha) + (onboardMag[2] * inv_alpha);

    // Normalize vector
    float magLength = sqrt((onboardMagFiltered[0] * onboardMagFiltered[0]) +
                           (onboardMagFiltered[1] * onboardMagFiltered[1]) +
                           (onboardMagFiltered[2] * onboardMagFiltered[2]));

    normalizedMag[0] = onboardMagFiltered[0] / magLength;
    normalizedMag[1] = onboardMagFiltered[1] / magLength;
    normalizedMag[2] = onboardMagFiltered[2] / magLength;

    if (auxMagSensor->status() > (int)AuxMagSensor::STATUS_NONE) {
        auxMag[0] = auxMagSensor->x();
        auxMag[1] = auxMagSensor->y();
        auxMag[2] = auxMagSensor->z();

        auxMagFiltered[0] = (auxMagFiltered[0] * alpha) + (auxMag[0] * inv_alpha);
        auxMagFiltered[1] = (auxMagFiltered[1] * alpha) + (auxMag[1] * inv_alpha);
        auxMagFiltered[2] = (auxMagFiltered[2] * alpha) + (auxMag[2] * inv_alpha);

        // Normalize vector
        float auxMagLength = sqrt((auxMagFiltered[0] * auxMagFiltered[0]) +
                                  (auxMagFiltered[1] * auxMagFiltered[1]) +
                                  (auxMagFiltered[2] * auxMagFiltered[2]));

        normalizedAuxMag[0] = auxMagFiltered[0] / auxMagLength;
        normalizedAuxMag[1] = auxMagFiltered[1] / auxMagLength;
        normalizedAuxMag[2] = auxMagFiltered[2] / auxMagLength;

        // Calc diff and scale
        xDiff = (normalizedMag[0] - normalizedAuxMag[0]) * 25.0f;
        yDiff = (normalizedMag[1] - normalizedAuxMag[1]) * 25.0f;
        zDiff = (normalizedMag[2] - normalizedAuxMag[2]) * 25.0f;
    } else {
        auxMag[0] = auxMag[1] = auxMag[2] = 0.0f;
        auxMagFiltered[0] = auxMagFiltered[1] = auxMagFiltered[2] = 0.0f;
    }

    // Display Mag/AuxMag diff for every axis
    m_ui->internalAuxErrorX->setValue(xDiff > 50.0f ? 50.0f : xDiff < -50.0f ? -50.0f : xDiff);
    m_ui->internalAuxErrorY->setValue(yDiff > 50.0f ? 50.0f : yDiff < -50.0f ? -50.0f : yDiff);
    m_ui->internalAuxErrorZ->setValue(zDiff > 50.0f ? 50.0f : zDiff < -50.0f ? -50.0f : zDiff);

    updateMagAlarm(getMagError(onboardMag), (auxMagSensor->status() == (int)AuxMagSensor::STATUS_NONE) ? -1.0f : getMagError(auxMag));
}

void ConfigRevoWidget::updateMagAlarm(float errorMag, float errorAuxMag)
{
    RevoSettings *revoSettings = RevoSettings::GetInstance(getObjectManager());

    Q_ASSERT(revoSettings);
    RevoSettings::DataFields revoSettingsData = revoSettings->getData();

    QStringList AlarmColor;
    AlarmColor << "grey" << "green" << "orange" << "red";
    enum magAlarmState { MAG_NOT_FOUND = 0, MAG_OK = 1, MAG_WARNING = 2, MAG_ERROR = 3 };

    QString bgColorMag    = AlarmColor[MAG_OK];
    QString bgColorAuxMag = AlarmColor[MAG_OK];

    // Onboard Mag
    if (errorMag < revoSettingsData.MagnetometerMaxDeviation[RevoSettings::MAGNETOMETERMAXDEVIATION_WARNING]) {
        magWarningCount = 0;
        magErrorCount   = 0;
    }

    if (errorMag < revoSettingsData.MagnetometerMaxDeviation[RevoSettings::MAGNETOMETERMAXDEVIATION_ERROR]) {
        magErrorCount = 0;
        if (magWarningCount > MAG_ALARM_THRESHOLD) {
            bgColorMag = AlarmColor[MAG_WARNING];
        } else {
            magWarningCount++;
        }
    }

    if (magErrorCount > MAG_ALARM_THRESHOLD) {
        bgColorMag = AlarmColor[MAG_ERROR];
    } else {
        magErrorCount++;
    }

    // Auxiliary Mag
    if (errorAuxMag > -1.0f) {
        if (errorAuxMag < revoSettingsData.MagnetometerMaxDeviation[RevoSettings::MAGNETOMETERMAXDEVIATION_WARNING]) {
            auxMagWarningCount = 0;
            auxMagErrorCount   = 0;
        }

        if (errorAuxMag < revoSettingsData.MagnetometerMaxDeviation[RevoSettings::MAGNETOMETERMAXDEVIATION_ERROR]) {
            auxMagErrorCount = 0;
            if (auxMagWarningCount > MAG_ALARM_THRESHOLD) {
                bgColorAuxMag = AlarmColor[MAG_WARNING];
            } else {
                auxMagWarningCount++;
            }
        }

        if (auxMagErrorCount > MAG_ALARM_THRESHOLD) {
            bgColorAuxMag = AlarmColor[MAG_ERROR];
        } else {
            auxMagErrorCount++;
        }
        errorAuxMag = ((errorAuxMag * 100.0f) <= 100.0f) ? errorAuxMag * 100.0f : 100.0f;
        m_ui->auxMagStatus->setText("AuxMag\n" + QString::number(errorAuxMag, 'f', 1) + "%");
    } else {
        // Disable aux mag alarm
        bgColorAuxMag = AlarmColor[MAG_NOT_FOUND];
        m_ui->auxMagStatus->setText("AuxMag\nnot found");
    }

    errorMag = ((errorMag * 100.0f) <= 100.0f) ? errorMag * 100.0f : 100.0f;
    m_ui->onBoardMagStatus->setText("Onboard\n" + QString::number(errorMag, 'f', 1) + "%");
    m_ui->onBoardMagStatus->setStyleSheet(
        "QLabel { background-color: " + bgColorMag + ";"
        "color: rgb(255, 255, 255); border-radius: 5; margin:1px; font:bold; }");
    m_ui->auxMagStatus->setStyleSheet(
        "QLabel { background-color: " + bgColorAuxMag + ";"
        "color: rgb(255, 255, 255); border-radius: 5; margin:1px; font:bold; }");
}

float ConfigRevoWidget::getMagError(float mag[3])
{
    float magnitude      = sqrt((mag[0] * mag[0]) + (mag[1] * mag[1]) + (mag[2] * mag[2]));
    float magnitudeBe    = sqrt((magBe[0] * magBe[0]) + (magBe[1] * magBe[1]) + (magBe[2] * magBe[2]));
    float invMagnitudeBe = 1.0f / magnitudeBe;
    // Absolute value of relative error against Be
    float error = fabsf(magnitude - magnitudeBe) * invMagnitudeBe;

    return error;
}

void ConfigRevoWidget::updateMagBeVector()
{
    HomeLocation *homeLocation = HomeLocation::GetInstance(getObjectManager());

    Q_ASSERT(homeLocation);
    HomeLocation::DataFields homeLocationData = homeLocation->getData();

    magBe[0] = homeLocationData.Be[0];
    magBe[1] = homeLocationData.Be[1];
    magBe[2] = homeLocationData.Be[2];
}

void ConfigRevoWidget::updateMagStatus()
{
    MagState *magState = MagState::GetInstance(getObjectManager());

    Q_ASSERT(magState);

    MagState::DataFields magStateData = magState->getData();

    if (magStateData.Source == MagState::SOURCE_INVALID) {
        m_ui->magStatusSource->setText(tr("Source invalid"));
        m_ui->magStatusSource->setToolTip(tr("Currently no attitude estimation algorithm uses magnetometer or there is something wrong"));
    } else if (magStateData.Source == MagState::SOURCE_ONBOARD) {
        m_ui->magStatusSource->setText(tr("Onboard magnetometer"));
        m_ui->magStatusSource->setToolTip("");
    } else if (magStateData.Source == MagState::SOURCE_AUX) {
        m_ui->magStatusSource->setText(tr("Auxiliary magnetometer"));
        m_ui->magStatusSource->setToolTip("");
    } else {
        m_ui->magStatusSource->setText(tr("Unknown"));
        m_ui->magStatusSource->setToolTip("");
    }
}
