/**
 ******************************************************************************
 *
 * @file       biascalibrationpage.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup BiasCalibrationPage
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


#ifndef ESCCALIBRATIONPAGE_H
#define ESCCALIBRATIONPAGE_H

#include "abstractwizardpage.h"
#include "outputcalibrationutil.h"

namespace Ui {
class EscCalibrationPage;
}

class EscCalibrationPage : public AbstractWizardPage {
    Q_OBJECT

public:
    explicit EscCalibrationPage(SetupWizard *wizard, QWidget *parent = 0);
    ~EscCalibrationPage();
    void initializePage();
    bool validatePage();

private slots:
    void startButtonClicked();
    void stopButtonClicked();
    void securityCheckBoxesToggled();
    void enableButtons(bool enable);
    void resetAllSecurityCheckboxes();

private:
    static const int OFF_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS  = 900;

    // Min value should match min value defined in vehicleconfigurationsource.h
    static const int LOW_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS  = 1000;
    static const int HIGH_PWM_OUTPUT_PULSE_LENGTH_MICROSECONDS = 1900;
    static const int HIGH_ONESHOT_MULTISHOT_OUTPUT_PULSE_LENGTH_MICROSECONDS = 2000;
    Ui::EscCalibrationPage *ui;
    bool m_isCalibrating;
    OutputCalibrationUtil m_outputUtil;
    QList<quint16> m_outputChannels;
    int getHighOutputRate();
};

#endif // ESCCALIBRATIONPAGE_H
