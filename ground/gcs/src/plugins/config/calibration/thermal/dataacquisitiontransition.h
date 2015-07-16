/**
 ******************************************************************************
 *
 * @file       thermalcalibrationtransitions.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 *
 * @brief      State transitions used to implement thermal calibration
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
 *
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

#ifndef DATAACQUISITIONTRANSITION_H
#define DATAACQUISITIONTRANSITION_H

#include "thermalcalibrationhelper.h"

#include <QSignalTransition>
#include <QEventTransition>

namespace OpenPilot {
class DataAcquisitionTransition : public QSignalTransition {
    Q_OBJECT

public:
    DataAcquisitionTransition(ThermalCalibrationHelper *helper, QState *currentState, QState *targetState)
        : QSignalTransition(helper, SIGNAL(collectionCompleted())),
        m_helper(helper)
    {
        QObject::connect(currentState, SIGNAL(entered()), this, SLOT(enterState()));

        setTargetState(targetState);
    }

    virtual void onTransition(QEvent *e)
    {
        Q_UNUSED(e);
        m_helper->endAcquisition();
    }

public slots:
    void enterState()
    {
        m_helper->setProgressMax(0);
        m_helper->setProgress(0);

        m_helper->addInstructions(tr("Please wait during samples acquisition. This can take several minutes..."), WizardModel::Prompt);
        m_helper->addInstructions(tr("Acquisition will run until the rate of temperature change is less than %1°C/min.").arg(ThermalCalibrationHelper::TargetGradient, 4, 'f', 2));
        m_helper->addInstructions(tr("For the calibration to be valid, the temperature span during acquisition must be greater than %1°C.").arg(ThermalCalibrationHelper::TargetTempDelta, 4, 'f', 2));
        m_helper->addInstructions(tr("Estimating acquisition duration..."));

        m_helper->initAcquisition();
    }

private:
    ThermalCalibrationHelper *m_helper;
};
}

#endif // DATAACQUISITIONTRANSITION_H
