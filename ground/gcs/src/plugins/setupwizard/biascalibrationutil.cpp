/**
 ******************************************************************************
 *
 * @file       biascalibrationutil.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup BiasCalibrationUtil
 * @{
 * @brief
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

#include "biascalibrationutil.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "attitudesettings.h"
#include "accelstate.h"
#include "gyrostate.h"
#include "qdebug.h"
#include "revocalibration.h"
#include "accelgyrosettings.h"


BiasCalibrationUtil::BiasCalibrationUtil(long measurementCount, long measurementRate) : QObject(),
    m_isMeasuring(false), m_accelMeasurementCount(measurementCount), m_gyroMeasurementCount(measurementCount),
    m_accelMeasurementRate(measurementRate), m_gyroMeasurementRate(measurementRate)
{}

BiasCalibrationUtil::BiasCalibrationUtil(long accelMeasurementCount, long accelMeasurementRate,
                                         long gyroMeasurementCount, long gyroMeasurementRate) : QObject(),
    m_isMeasuring(false), m_accelMeasurementCount(accelMeasurementCount), m_gyroMeasurementCount(gyroMeasurementCount),
    m_accelMeasurementRate(accelMeasurementRate), m_gyroMeasurementRate(gyroMeasurementRate)
{}

void BiasCalibrationUtil::start()
{
    if (!m_isMeasuring) {
        startMeasurement();

        // Set up timeout timer
        connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(timeout()));
        // Set timeout to max time of measurement + 10 secs
        m_timeoutTimer.start(std::max(m_accelMeasurementCount * m_accelMeasurementRate, m_gyroMeasurementCount * m_gyroMeasurementRate) + 10000);
    }
}

void BiasCalibrationUtil::abort()
{
    if (m_isMeasuring) {
        stopMeasurement();
    }
}

void BiasCalibrationUtil::gyroMeasurementsUpdated(UAVObject *obj)
{
    Q_UNUSED(obj);

    if (m_receivedGyroUpdates < m_gyroMeasurementCount) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        UAVObjectManager *uavObjectManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(uavObjectManager);

        GyroState *gyroState = GyroState::GetInstance(uavObjectManager);
        GyroState::DataFields gyroStateData = gyroState->getData();

        m_gyroX += gyroStateData.x;
        m_gyroY += gyroStateData.y;
        m_gyroZ += gyroStateData.z;

        m_receivedGyroUpdates++;
        emit progress(m_receivedGyroUpdates + m_receivedAccelUpdates, m_gyroMeasurementCount + m_accelMeasurementCount);
    } else if (m_receivedAccelUpdates >= m_accelMeasurementCount &&
               m_receivedGyroUpdates >= m_gyroMeasurementCount && m_isMeasuring) {
        stopMeasurement();
    }
}

void BiasCalibrationUtil::accelMeasurementsUpdated(UAVObject *obj)
{
    Q_UNUSED(obj);

    if (m_receivedAccelUpdates < m_accelMeasurementCount) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        UAVObjectManager *uavObjectManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(uavObjectManager);

        AccelState *accelState = AccelState::GetInstance(uavObjectManager);
        AccelState::DataFields AccelStateData = accelState->getData();

        m_accelerometerX += AccelStateData.x;
        m_accelerometerY += AccelStateData.y;
        m_accelerometerZ += AccelStateData.z;

        m_receivedAccelUpdates++;
        emit progress(m_receivedGyroUpdates + m_receivedAccelUpdates, m_gyroMeasurementCount + m_accelMeasurementCount);
    } else if (m_receivedAccelUpdates >= m_accelMeasurementCount &&
               m_receivedGyroUpdates >= m_gyroMeasurementCount && m_isMeasuring) {
        stopMeasurement();
    }
}

void BiasCalibrationUtil::timeout()
{
    stopMeasurement();
    emit timeout(tr("Calibration timed out before receiving required updates."));
}

void BiasCalibrationUtil::startMeasurement()
{
    m_isMeasuring = true;

    // Reset variables
    m_receivedAccelUpdates = 0;
    m_accelerometerX = 0;
    m_accelerometerY = 0;
    m_accelerometerZ = 0;

    m_receivedGyroUpdates = 0;
    m_gyroX = 0;
    m_gyroY = 0;
    m_gyroZ = 0;

    ExtensionSystem::PluginManager *pm     = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavObjectManager     = pm->getObject<UAVObjectManager>();
    Q_ASSERT(uavObjectManager);

    RevoCalibration *revolutionCalibration = RevoCalibration::GetInstance(uavObjectManager);
    Q_ASSERT(revolutionCalibration);
    AccelGyroSettings *accelGyroSettings   = AccelGyroSettings::GetInstance(uavObjectManager);
    Q_ASSERT(accelGyroSettings);

    RevoCalibration::DataFields revoCalibrationData     = revolutionCalibration->getData();
    AccelGyroSettings::DataFields accelGyroSettingsData = accelGyroSettings->getData();
    // Disable gyro bias correction to see raw data
    AttitudeSettings::DataFields attitudeSettingsData   = AttitudeSettings::GetInstance(uavObjectManager)->getData();

    attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_FALSE;
    revoCalibrationData.BiasCorrectedRaw = RevoCalibration::BIASCORRECTEDRAW_FALSE;

    accelGyroSettingsData.accel_bias[AccelGyroSettings::ACCEL_BIAS_X] = 0;
    accelGyroSettingsData.accel_bias[AccelGyroSettings::ACCEL_BIAS_Y] = 0;
    accelGyroSettingsData.accel_bias[AccelGyroSettings::ACCEL_BIAS_Z] = 0;
    accelGyroSettingsData.gyro_bias[AccelGyroSettings::GYRO_BIAS_X]   = 0;
    accelGyroSettingsData.gyro_bias[AccelGyroSettings::GYRO_BIAS_Y]   = 0;
    accelGyroSettingsData.gyro_bias[AccelGyroSettings::GYRO_BIAS_Z]   = 0;
    int i;
    for (i = 0; i < 5; i++) {
        RevoCalibration::GetInstance(uavObjectManager)->setData(revoCalibrationData);
        AccelGyroSettings::GetInstance(uavObjectManager)->setData(accelGyroSettingsData);
        AttitudeSettings::GetInstance(uavObjectManager)->setData(attitudeSettingsData);
    }
    // Set up to receive updates for accels
    UAVDataObject *uavObject = AccelState::GetInstance(uavObjectManager);
    connect(uavObject, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(accelMeasurementsUpdated(UAVObject *)));

    // Set update period for accels
    m_previousAccelMetaData = uavObject->getMetadata();
    UAVObject::Metadata newMetaData = m_previousAccelMetaData;
    UAVObject::SetFlightTelemetryUpdateMode(newMetaData, UAVObject::UPDATEMODE_PERIODIC);
    newMetaData.flightTelemetryUpdatePeriod = m_accelMeasurementRate;

    for (i = 0; i < 5; i++) {
        uavObject->setMetadata(newMetaData);
    }
    // Set up to receive updates from gyros
    uavObject = GyroState::GetInstance(uavObjectManager);
    connect(uavObject, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(gyroMeasurementsUpdated(UAVObject *)));

    // Set update period for gyros
    m_previousGyroMetaData = uavObject->getMetadata();
    newMetaData = m_previousGyroMetaData;
    UAVObject::SetFlightTelemetryUpdateMode(newMetaData, UAVObject::UPDATEMODE_PERIODIC);
    newMetaData.flightTelemetryUpdatePeriod = m_gyroMeasurementRate;
    for (i = 0; i < 5; i++) {
        uavObject->setMetadata(newMetaData);
    }
}

void BiasCalibrationUtil::stopMeasurement()
{
    qDebug() << "Sampling done, G =" << m_receivedGyroUpdates << "A =" << m_receivedAccelUpdates;

    m_isMeasuring = false;

    // Stop timeout timer
    m_timeoutTimer.stop();
    disconnect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavObjectManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(uavObjectManager);

    // Stop listening for updates from accels
    UAVDataObject *uavObject = AccelState::GetInstance(uavObjectManager);
    disconnect(uavObject, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(accelMeasurementsUpdated(UAVObject *)));
    uavObject->setMetadata(m_previousAccelMetaData);

    // Stop listening for updates from gyros
    uavObject = GyroState::GetInstance(uavObjectManager);
    disconnect(uavObject, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(gyroMeasurementsUpdated(UAVObject *)));
    uavObject->setMetadata(m_previousGyroMetaData);

    // Enable gyro bias correction again
    AttitudeSettings::DataFields attitudeSettingsData = AttitudeSettings::GetInstance(uavObjectManager)->getData();
    attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_TRUE;
    AttitudeSettings::GetInstance(uavObjectManager)->setData(attitudeSettingsData);

    accelGyroBias bias;
    bias.m_accelerometerXBias = m_accelerometerX / (double)m_receivedAccelUpdates;
    bias.m_accelerometerYBias = m_accelerometerY / (double)m_receivedAccelUpdates;
    bias.m_accelerometerZBias = m_accelerometerZ / (double)m_receivedAccelUpdates;

    bias.m_gyroXBias = m_gyroX / (double)m_receivedGyroUpdates;
    bias.m_gyroYBias = m_gyroY / (double)m_receivedGyroUpdates;
    bias.m_gyroZBias = m_gyroZ / (double)m_receivedGyroUpdates;

    qDebug() << "Bias calculations finished";
    emit done(bias);
}
