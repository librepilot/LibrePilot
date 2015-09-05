/**
 ******************************************************************************
 *
 * @file       outputcalibrationpage.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup OutputCalibrationPage
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

#ifndef OUTPUTCALIBRATIONPAGE_H
#define OUTPUTCALIBRATIONPAGE_H

#include <QtSvg>
#include "abstractwizardpage.h"
#include "setupwizard.h"
#include "outputcalibrationutil.h"
#include "vehicleconfigurationhelper.h"

namespace Ui {
class OutputCalibrationPage;
}

class OutputCalibrationPage : public AbstractWizardPage {
    Q_OBJECT

public:
    explicit OutputCalibrationPage(SetupWizard *wizard, QWidget *parent = 0);
    ~OutputCalibrationPage();
    void initializePage();
    bool validatePage();

    bool isFinished()
    {
        return m_currentWizardIndex >= m_wizardIndexes.size();
    }

    void loadSVGFile(QString file);
    void setupActuatorMinMaxAndNeutral(int motorChannelStart, int motorChannelEnd, int totalUsedChannels);
protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

public slots:
    void customBackClicked();

private slots:
    void on_motorNeutralButton_toggled(bool checked);
    void on_motorNeutralSlider_valueChanged(int value);

    void on_servoButton_toggled(bool checked);
    void on_servoCenterAngleSlider_valueChanged(int position);
    void on_servoMinAngleSlider_valueChanged(int position);
    void on_servoMaxAngleSlider_valueChanged(int position);
    void on_reverseCheckbox_toggled(bool checked);

    void on_dualservoButton_toggled(bool checked);
    void on_servoCenterAngleSlider1_valueChanged(int position);
    void on_servoMinAngleSlider1_valueChanged(int position);
    void on_servoMaxAngleSlider1_valueChanged(int position);
    void on_reverseCheckbox1_toggled(bool checked);

    void on_servoCenterAngleSlider2_valueChanged(int position);
    void on_servoMinAngleSlider2_valueChanged(int position);
    void on_servoMaxAngleSlider2_valueChanged(int position);
    void on_reverseCheckbox2_toggled(bool checked);

    void on_calibrateAllMotors_toggled(bool checked);

private:
    enum ElementType { FULL, FRAME, MOTOR, SERVO };
    static const int LOW_OUTPUT_RATE_MILLISECONDS      = 1000;
    static const int NEUTRAL_OUTPUT_RATE_MILLISECONDS  = 1500;
    static const int HIGH_OUTPUT_RATE_MILLISECONDS_PWM = 1900;
    static const int HIGH_OUTPUT_RATE_MILLISECONDS_ONESHOT125 = 2000;

    void setupVehicle();
    void startWizard();
    void setupVehicleItems();
    void setupVehicleHighlightedPart();
    void showElementMovement(bool isUp, bool FirstServo, qreal value);
    void setWizardPage();
    void enableButtons(bool enable);
    void enableServoSliders(bool enabled);
    void onStartButtonToggle(QAbstractButton *button, QList<quint16> &channels,
                             quint16 value, quint16 safeValue, QSlider *slider);
    void onStartButtonToggleDual(QAbstractButton *button, QList<quint16> &channels,
                                 quint16 value1, quint16 value2, quint16 safeValue, QSlider *slider1, QSlider *slider2);
    void setSliderLimitsAndArrows(quint16 currentChannel, bool showFirst, quint16 value, QCheckBox *revCheckbox, QSlider *minSlider, QSlider *maxSlider);
    void reverseCheckBoxIsToggled(quint16 currentChannel, QCheckBox *checkBox, QSlider *centerSlider, QSlider *minSlider, QSlider *maxSlider);

    bool checkAlarms();
    void debugLogChannelValues(bool showFirst);

    void getCurrentChannels(QList<quint16> &channels);
    void enableAllMotorsCheckBox(bool enable);
    int getHighOutputRate();
    quint16 getCurrentChannel();

    Ui::OutputCalibrationPage *ui;
    QSvgRenderer *m_vehicleRenderer;
    QGraphicsScene *m_vehicleScene;
    QGraphicsSvgItem *m_vehicleBoundsItem;

    qint16 m_currentWizardIndex;

    QList<QString> m_vehicleElementIds;
    QList<ElementType> m_vehicleElementTypes;
    QList<QGraphicsSvgItem *> m_vehicleItems;
    QList<QGraphicsSvgItem *> m_arrowsItems;
    QList<quint16> m_vehicleHighlightElementIndexes;
    QList<quint16> m_channelIndex;
    QList<qint16> m_wizardIndexes;

    QList<actuatorChannelSettings> m_actuatorSettings;

    OutputCalibrationUtil *m_calibrationUtil;
    void resetOutputCalibrationUtil();

    static const QString MULTI_SVG_FILE;
    static const QString FIXEDWING_SVG_FILE;
    static const QString GROUND_SVG_FILE;
};

#endif // OUTPUTCALIBRATIONPAGE_H
