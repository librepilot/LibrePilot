/**
 ******************************************************************************
 *
 * @file       ConfigRevoWidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#include <attitudestate.h>
#include <attitudesettings.h>
#include <revocalibration.h>
#include <accelgyrosettings.h>
#include <homelocation.h>
#include <accelstate.h>
#include <magstate.h>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>

#include "assertions.h"
#include "calibration.h"
#include "calibration/calibrationutils.h"

#include "math.h"
#include <QDebug>
#include <QTimer>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QThread>
#include <QErrorMessage>
#include <QDesktopServices>
#include <QUrl>
#include <iostream>

#include <math.h>

// #define DEBUG

// Uncomment this to enable 6 point calibration on the accels
#define NOISE_SAMPLES 50

class Thread : public QThread {
public:
    static void usleep(unsigned long usecs)
    {
        QThread::usleep(usecs);
    }
};

ConfigRevoWidget::ConfigRevoWidget(QWidget *parent) :
    ConfigTaskWidget(parent),
    m_ui(new Ui_RevoSensorsWidget()),
    isBoardRotationStored(false)
{
    m_ui->setupUi(this);
    m_ui->tabWidget->setCurrentIndex(0);

    addApplySaveButtons(m_ui->revoCalSettingsSaveRAM, m_ui->revoCalSettingsSaveSD);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    if (!settings->useExpertMode()) {
        m_ui->revoCalSettingsSaveRAM->setVisible(false);
    }

    // Initialization of the visual help
    m_ui->calibrationVisualHelp->setScene(new QGraphicsScene(this));
    m_ui->calibrationVisualHelp->setRenderHint(QPainter::HighQualityAntialiasing, true);
    m_ui->calibrationVisualHelp->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_ui->calibrationVisualHelp->setBackgroundBrush(QBrush(QColor(51, 51, 51)));
    displayVisualHelp("empty");

    // Must set up the UI (above) before setting up the UAVO mappings or refreshWidgetValues
    // will be dealing with some null pointers
    addUAVObject("HomeLocation");
    addUAVObject("RevoCalibration");
    addUAVObject("AttitudeSettings");
    addUAVObject("RevoSettings");
    addUAVObject("AccelGyroSettings");
    addUAVObject("AuxMagSettings");
    autoLoadWidgets();

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

    populateWidgets();
    enableAllCalibrations();

    updateEnableControls();

    forceConnectedState();
    refreshWidgetsValues();
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
        // Store current board rotation
        isBoardRotationStored = true;
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        AttitudeSettings::DataFields data  = attitudeSettings->getData();
        storedBoardRotation[AttitudeSettings::BOARDROTATION_YAW]   = data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW];
        storedBoardRotation[AttitudeSettings::BOARDROTATION_ROLL]  = data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL];
        storedBoardRotation[AttitudeSettings::BOARDROTATION_PITCH] = data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH];

        // Set board rotation to no rotation
        data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW]    = 0;
        data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL]   = 0;
        data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH]  = 0;
        attitudeSettings->setData(data);
    }
}

void ConfigRevoWidget::recallBoardRotation()
{
    if (isBoardRotationStored) {
        // Recall current board rotation
        isBoardRotationStored = false;

        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        AttitudeSettings::DataFields data  = attitudeSettings->getData();
        data.BoardRotation[AttitudeSettings::BOARDROTATION_YAW]   = storedBoardRotation[AttitudeSettings::BOARDROTATION_YAW];
        data.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL]  = storedBoardRotation[AttitudeSettings::BOARDROTATION_ROLL];
        data.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] = storedBoardRotation[AttitudeSettings::BOARDROTATION_PITCH];
        attitudeSettings->setData(data);
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
void ConfigRevoWidget::refreshWidgetsValues(UAVObject *object)
{
    ConfigTaskWidget::refreshWidgetsValues(object);

    m_ui->isSetCheckBox->setEnabled(false);

    HomeLocation *homeLocation = HomeLocation::GetInstance(getObjectManager());
    Q_ASSERT(homeLocation);
    HomeLocation::DataFields homeLocationData = homeLocation->getData();

    QString beStr = QString("%1:%2:%3").arg(QString::number(homeLocationData.Be[0]), QString::number(homeLocationData.Be[1]), QString::number(homeLocationData.Be[2]));
    m_ui->beBox->setText(beStr);
}

void ConfigRevoWidget::updateObjectsFromWidgets()
{
    ConfigTaskWidget::updateObjectsFromWidgets();

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
