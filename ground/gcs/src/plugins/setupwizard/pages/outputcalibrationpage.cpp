/**
 ******************************************************************************
 *
 * @file       outputcalibrationpage.cpp
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

#include "outputcalibrationpage.h"

#include "ui_outputcalibrationpage.h"

#include "systemalarms.h"

#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"

const QString OutputCalibrationPage::MULTI_SVG_FILE     = QString(":/setupwizard/resources/multirotor-shapes.svg");
const QString OutputCalibrationPage::FIXEDWING_SVG_FILE = QString(":/setupwizard/resources/fixedwing-shapes-wizard.svg");
const QString OutputCalibrationPage::GROUND_SVG_FILE    = QString(":/setupwizard/resources/ground-shapes-wizard.svg");

OutputCalibrationPage::OutputCalibrationPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent), ui(new Ui::OutputCalibrationPage), m_vehicleBoundsItem(0),
    m_currentWizardIndex(-1), m_calibrationUtil(0)
{
    ui->setupUi(this);

    qDebug() << "calling output calibration page";
    m_vehicleRenderer = new QSvgRenderer();

    // move the code that was here to setupVehicle() so we can determine which image to use.
    m_vehicleScene    = new QGraphicsScene(this);
    ui->vehicleView->setScene(m_vehicleScene);
}

OutputCalibrationPage::~OutputCalibrationPage()
{
    if (m_calibrationUtil) {
        delete m_calibrationUtil;
        m_calibrationUtil = 0;
    }

    OutputCalibrationUtil::stopOutputCalibration();
    delete ui;
}

void OutputCalibrationPage::loadSVGFile(QString file)
{
    if (QFile::exists(file) && m_vehicleRenderer->load(file) && m_vehicleRenderer->isValid()) {
        ui->vehicleView->setScene(m_vehicleScene);
    }
}

void OutputCalibrationPage::setupActuatorMinMaxAndNeutral(int motorChannelStart, int motorChannelEnd, int totalUsedChannels)
{
    // Default values for the actuator settings, extra important for
    // servos since a value out of range can actually destroy the
    // vehicle if unlucky.
    // Motors are not that important. REMOVE propellers always!!
    OutputCalibrationUtil::startOutputCalibration();

    for (int servoid = 0; servoid < 12; servoid++) {
        if (servoid >= motorChannelStart && servoid <= motorChannelEnd) {
            // Set to motor safe values
            m_actuatorSettings[servoid].channelMin        = getLowOutputRate();
            m_actuatorSettings[servoid].channelNeutral    = getLowOutputRate();
            m_actuatorSettings[servoid].channelMax        = getHighOutputRate();
            m_actuatorSettings[servoid].isReversableMotor = false;
            // Car, Tank, Boat and Boat differential should use reversable Esc/motors
            if ((getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_CAR)
                || (getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_DIFFERENTIAL)
                || (getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_BOAT)
                || (getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_DIFFERENTIAL_BOAT)) {
                m_actuatorSettings[servoid].channelNeutral    = NEUTRAL_OUTPUT_RATE_MS;
                m_actuatorSettings[servoid].isReversableMotor = true;
            }
            // Set initial output value
            m_calibrationUtil->startChannelOutput(servoid, m_actuatorSettings[servoid].channelNeutral);
            m_calibrationUtil->stopChannelOutput();
        } else if (servoid < totalUsedChannels) {
            // Set to servo safe values
            m_actuatorSettings[servoid].channelMin     = NEUTRAL_OUTPUT_RATE_MS;
            m_actuatorSettings[servoid].channelNeutral = NEUTRAL_OUTPUT_RATE_MS;
            m_actuatorSettings[servoid].channelMax     = NEUTRAL_OUTPUT_RATE_MS;
            // Set initial servo output value
            m_calibrationUtil->startChannelOutput(servoid, NEUTRAL_OUTPUT_RATE_MS);
            m_calibrationUtil->stopChannelOutput();
        } else {
            // "Disable" these channels
            m_actuatorSettings[servoid].channelMin     = LOW_OUTPUT_RATE_MS;
            m_actuatorSettings[servoid].channelNeutral = LOW_OUTPUT_RATE_MS;
            m_actuatorSettings[servoid].channelMax     = LOW_OUTPUT_RATE_MS;
        }
    }
}

void OutputCalibrationPage::setupVehicle()
{
    m_actuatorSettings = getWizard()->getActuatorSettings();
    m_wizardIndexes.clear();
    m_vehicleElementIds.clear();
    m_vehicleElementTypes.clear();
    m_vehicleHighlightElementIndexes.clear();
    m_channelIndex.clear();
    m_currentWizardIndex = 0;
    m_vehicleScene->clear();

    resetOutputCalibrationUtil();

    switch (getWizard()->getVehicleSubType()) {
    case SetupWizard::MULTI_ROTOR_TRI_Y:
        // Loads the SVG file resourse and sets the scene
        loadSVGFile(MULTI_SVG_FILE);

        // The m_wizardIndexes array contains the index of the QStackedWidget
        // in the page to use for each step.
        // 0 : Start
        // 1 : Motor page
        // 2 : single Servo page
        // 3 : Dual servo page, followed with -1 : Blank page.
        m_wizardIndexes << 0 << 1 << 1 << 1 << 2;

        // All element ids to load from the svg file and manage.
        m_vehicleElementIds << "tri" << "tri-frame" << "tri-m1" << "tri-m2" << "tri-m3" << "tri-s1";

        // The type of each element.
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << SERVO;

        // The index of the elementId to highlight ( not dim ) for each step
        // this is the index in the m_vehicleElementIds - 1.
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;

        // The channel number to configure for each step.
        m_channelIndex << 0 << 0 << 1 << 2 << 3;

        setupActuatorMinMaxAndNeutral(0, 2, 4);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::MULTI_ROTOR_QUAD_X:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "quad-x" << "quad-x-frame" << "quad-x-m1" << "quad-x-m2" << "quad-x-m3" << "quad-x-m4";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 0 << 1 << 2 << 3;
        setupActuatorMinMaxAndNeutral(0, 3, 4);
        break;
    case SetupWizard::MULTI_ROTOR_QUAD_PLUS:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "quad-p" << "quad-p-frame" << "quad-p-m1" << "quad-p-m2" << "quad-p-m3" << "quad-p-m4";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 0 << 1 << 2 << 3;
        setupActuatorMinMaxAndNeutral(0, 3, 4);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa" << "hexa-frame" << "hexa-m1" << "hexa-m2" << "hexa-m3" << "hexa-m4" << "hexa-m5" << "hexa-m6";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_COAX_Y:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-y6" << "hexa-y6-frame" << "hexa-y6-m2" << "hexa-y6-m1" << "hexa-y6-m4" << "hexa-y6-m3" << "hexa-y6-m6" << "hexa-y6-m5";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 2 << 1 << 4 << 3 << 6 << 5;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_H:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-h" << "hexa-h-frame" << "hexa-h-m1" << "hexa-h-m2" << "hexa-h-m3" << "hexa-h-m4" << "hexa-h-m5" << "hexa-h-m6";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    case SetupWizard::MULTI_ROTOR_HEXA_X:
        loadSVGFile(MULTI_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1 << 1 << 1 << 1 << 1;
        m_vehicleElementIds << "hexa-x" << "hexa-x-frame" << "hexa-x-m1" << "hexa-x-m2" << "hexa-x-m3" << "hexa-x-m4" << "hexa-x-m5" << "hexa-x-m6";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        m_channelIndex << 0 << 0 << 1 << 2 << 3 << 4 << 5;
        setupActuatorMinMaxAndNeutral(0, 5, 6);
        break;
    // Fixed Wing
    case SetupWizard::FIXED_WING_DUAL_AILERON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 3 << -1 << 2 << 2;
        m_vehicleElementIds << "aileron" << "aileron-frame" << "aileron-motor" << "aileron-ail-left" << "aileron-ail-right" << "aileron-elevator" << "aileron-rudder";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 3 << 0 << 5 << 1 << 2;

        setupActuatorMinMaxAndNeutral(3, 3, 6); // should be 5 instead 6 but output 5 is not used

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_AILERON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2;
        m_vehicleElementIds << "singleaileron" << "singleaileron-frame" << "singleaileron-motor" << "singleaileron-aileron" << "singleaileron-elevator" << "singleaileron-rudder";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 3 << 0 << 1 << 2;

        setupActuatorMinMaxAndNeutral(3, 3, 4);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_ELEVON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 3 << -1;
        m_vehicleElementIds << "elevon" << "elevon-frame" << "elevon-motor" << "elevon-left" << "elevon-right";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3;
        m_channelIndex << 0 << 3 << 0 << 1;

        setupActuatorMinMaxAndNeutral(3, 3, 3);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_VTAIL:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 3 << -1 << 3 << -1;
        m_vehicleElementIds << "vtail" << "vtail-frame" << "vtail-motor" << "vtail-ail-left" << "vtail-ail-right" << "vtail-rudder-left" << "vtail-rudder-right";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 3 << 0 << 5 << 2 << 1;

        setupActuatorMinMaxAndNeutral(3, 3, 6); // should be 5 instead 6 but output 5 is not used

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;

    // Ground vehicles
    case SetupWizard::GROUNDVEHICLE_CAR:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2;
        m_vehicleElementIds << "car" << "car-frame" << "car-motor" << "car-steering";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 3 << 0;

        setupActuatorMinMaxAndNeutral(3, 3, 2);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::GROUNDVEHICLE_DIFFERENTIAL:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1;
        m_vehicleElementIds << "tank" << "tank-frame" << "tank-left-motor" << "tank-right-motor";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 0 << 1;

        setupActuatorMinMaxAndNeutral(0, 1, 2);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::GROUNDVEHICLE_MOTORCYCLE:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2;
        m_vehicleElementIds << "motorbike" << "motorbike-frame" << "motorbike-motor" << "motorbike-steering";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 3 << 0;

        setupActuatorMinMaxAndNeutral(3, 3, 2);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::GROUNDVEHICLE_BOAT:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2;
        m_vehicleElementIds << "boat" << "boat-frame" << "boat-motor" << "boat-rudder";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 3 << 0;

        setupActuatorMinMaxAndNeutral(3, 3, 2);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::GROUNDVEHICLE_DIFFERENTIAL_BOAT:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 1;
        m_vehicleElementIds << "boat_diff" << "boat_diff-frame" << "boat_diff-left-motor" << "boat_diff-right-motor";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << MOTOR;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 0 << 1;

        setupActuatorMinMaxAndNeutral(0, 1, 2);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;

    default:
        break;
    }

    setupVehicleItems();
}

void OutputCalibrationPage::setupVehicleItems()
{
    m_vehicleItems.clear();
    m_arrowsItems.clear();
    m_vehicleBoundsItem = new QGraphicsSvgItem();
    m_vehicleBoundsItem->setSharedRenderer(m_vehicleRenderer);
    m_vehicleBoundsItem->setElementId(m_vehicleElementIds[0]);
    m_vehicleBoundsItem->setZValue(-1);
    m_vehicleBoundsItem->setOpacity(0);
    m_vehicleScene->addItem(m_vehicleBoundsItem);

    QRectF parentBounds = m_vehicleRenderer->boundsOnElement(m_vehicleElementIds[0]);

    for (int i = 1; i < m_vehicleElementIds.size(); i++) {
        QGraphicsSvgItem *item = new QGraphicsSvgItem();
        item->setSharedRenderer(m_vehicleRenderer);
        item->setElementId(m_vehicleElementIds[i]);
        item->setZValue(i);
        item->setOpacity(1.0);

        QRectF itemBounds = m_vehicleRenderer->boundsOnElement(m_vehicleElementIds[i]);
        item->setPos(itemBounds.x() - parentBounds.x(), itemBounds.y() - parentBounds.y());

        m_vehicleScene->addItem(item);
        m_vehicleItems << item;

        bool addArrows = false;

        if ((m_vehicleElementIds[i].contains("left")) || (m_vehicleElementIds[i].contains("right"))
            || (m_vehicleElementIds[i].contains("elevator")) || (m_vehicleElementIds[i].contains("rudder"))
            || (m_vehicleElementIds[i].contains("steering")) || (m_vehicleElementIds[i] == "singleaileron-aileron")) {
            addArrows = true;
        }

        if (addArrows) {
            QString arrowUp   = "-up"; // right if rudder / steering
            QString arrowDown = "-down"; // left

            QGraphicsSvgItem *itemUp = new QGraphicsSvgItem();

            itemUp->setSharedRenderer(m_vehicleRenderer);
            QString elementUp = m_vehicleElementIds[i] + arrowUp;
            itemUp->setElementId(elementUp);
            itemUp->setZValue(i + 10);
            itemUp->setOpacity(0);

            QRectF itemBounds = m_vehicleRenderer->boundsOnElement(elementUp);
            itemUp->setPos(itemBounds.x() - parentBounds.x(), itemBounds.y() - parentBounds.y());
            m_vehicleScene->addItem(itemUp);

            m_arrowsItems << itemUp;

            QGraphicsSvgItem *itemDown = new QGraphicsSvgItem();
            itemDown->setSharedRenderer(m_vehicleRenderer);
            QString elementDown = m_vehicleElementIds[i] + arrowDown;
            itemDown->setElementId(elementDown);
            itemDown->setZValue(i + 10);
            itemDown->setOpacity(0);

            itemBounds = m_vehicleRenderer->boundsOnElement(elementDown);
            itemDown->setPos(itemBounds.x() - parentBounds.x(), itemBounds.y() - parentBounds.y());
            m_vehicleScene->addItem(itemDown);

            m_arrowsItems << itemDown;
        }
    }
}

void OutputCalibrationPage::startWizard()
{
    ui->calibrationStack->setCurrentIndex(m_wizardIndexes[0]);
    enableAllMotorsCheckBox(true);
    setupVehicleHighlightedPart();
}

void OutputCalibrationPage::setupVehicleHighlightedPart()
{
    qreal dimOpaque = m_currentWizardIndex == 0 ? 1.0 : 0.3;
    qreal highlightOpaque = 1.0;
    int highlightedIndex  = m_vehicleHighlightElementIndexes[m_currentWizardIndex];

    bool isDualServoSetup = (m_wizardIndexes[m_currentWizardIndex] == 3);

    for (int i = 0; i < m_vehicleItems.size(); i++) {
        QGraphicsSvgItem *item = m_vehicleItems[i];
        if (highlightedIndex == i || (isDualServoSetup && ((highlightedIndex + 1) == i)) ||
            (ui->calibrateAllMotors->isChecked() && m_vehicleElementTypes[i + 1] == MOTOR)) {
            item->setOpacity(highlightOpaque);
        } else {
            item->setOpacity(dimOpaque);
        }
    }
}

void OutputCalibrationPage::showElementMovement(bool isUp, bool firstServo, qreal value)
{
    QString highlightedItemName;

    if (firstServo) {
        highlightedItemName = m_vehicleItems[m_currentWizardIndex]->elementId();
    } else {
        if ((m_currentWizardIndex + 1) < m_wizardIndexes.size()) {
            highlightedItemName = m_vehicleItems[m_currentWizardIndex + 1]->elementId();
        }
    }

    for (int i = 0; i < m_arrowsItems.size(); i++) {
        QString upItemName   = highlightedItemName + "-up";
        QString downItemName = highlightedItemName + "-down";
        if (m_arrowsItems[i]->elementId() == upItemName) {
            QGraphicsSvgItem *itemUp = m_arrowsItems[i];
            itemUp->setOpacity(isUp ? value : 0);
        }
        if (m_arrowsItems[i]->elementId() == downItemName) {
            QGraphicsSvgItem *itemDown = m_arrowsItems[i];
            itemDown->setOpacity(isUp ? 0 : value);
        }
    }
}

void OutputCalibrationPage::setWizardPage()
{
    qDebug() << "Wizard index: " << m_currentWizardIndex;

    QApplication::processEvents();

    int currentPageIndex = m_wizardIndexes[m_currentWizardIndex];
    qDebug() << "Current page: " << currentPageIndex;
    ui->calibrationStack->setCurrentIndex(currentPageIndex);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    int currentChannel = currentChannels[0];
    qDebug() << "Current channel: " << currentChannel + 1;
    if (currentChannel >= 0) {
        if (currentPageIndex == 1) {
            // Set Min, Neutral and Max for slider in all cases, needed for DShot.
            ui->motorNeutralSlider->setMinimum(m_actuatorSettings[currentChannel].channelMin);
            ui->motorNeutralSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
            ui->motorNeutralSlider->setMaximum(m_actuatorSettings[currentChannel].channelMin + NEUTRAL_OUTPUT_RATE_RANGE);
            if (ui->motorNeutralSlider->minimum() == LOW_OUTPUT_RATE_DSHOT) {
                // DShot output
                ui->motorPWMValue->setText(tr("Digital output value : <b>%1</b>").arg(m_actuatorSettings[currentChannel].channelNeutral));
            } else {
                ui->motorPWMValue->setText(tr("Output value : <b>%1</b> µs").arg(m_actuatorSettings[currentChannel].channelNeutral));
            }
            // Reversable motor found
            if (m_actuatorSettings[currentChannel].isReversableMotor) {
                ui->motorNeutralSlider->setMaximum(m_actuatorSettings[currentChannel].channelMax);
                ui->motorInfo->setText(tr("<html><head/><body><p><span style=\" font-size:10pt;\">To find </span><span style=\" font-size:10pt; font-weight:600;\">the neutral rate for this reversable motor</span><span style=\" font-size:10pt;\">, press the Start button below and slide the slider to the right or left until you find the value where the motor doesn't start. <br/><br/>When done press button again to stop.</span></p></body></html>"));
            }
        } else if (currentPageIndex == 2) {
            ui->servoPWMValue->setText(tr("Output value : <b>%1</b> µs").arg(m_actuatorSettings[currentChannel].channelNeutral));
            if (m_actuatorSettings[currentChannel].channelMax < m_actuatorSettings[currentChannel].channelMin &&
                !ui->reverseCheckbox->isChecked()) {
                ui->reverseCheckbox->setChecked(true);
            } else {
                ui->reverseCheckbox->setChecked(false);
            }
            enableServoSliders(false);
            if (ui->reverseCheckbox->isChecked()) {
                ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
                ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
            } else {
                ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
                ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
            }
        } else if (currentPageIndex == 3) {
            // Dual channel setup : two ailerons or Vtail
            // First channel
            ui->servoPWMValue1->setText(tr("Output %1 value : <b>%2</b> µs").arg(currentChannel + 1).arg(m_actuatorSettings[currentChannel].channelNeutral));


            if (m_actuatorSettings[currentChannel].channelMax < m_actuatorSettings[currentChannel].channelMin &&
                !ui->reverseCheckbox1->isChecked()) {
                ui->reverseCheckbox1->setChecked(true);
            } else {
                ui->reverseCheckbox1->setChecked(false);
            }
            enableServoSliders(false);
            if (ui->reverseCheckbox1->isChecked()) {
                ui->servoMaxAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelMax);
                ui->servoCenterAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMinAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelMin);
            } else {
                ui->servoMinAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelMin);
                ui->servoCenterAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelNeutral);
                ui->servoMaxAngleSlider1->setValue(m_actuatorSettings[currentChannel].channelMax);
            }
            // Second channel
            int nextChannel = currentChannels[1];
            qDebug() << "Current channel: " << currentChannel + 1 << " and " << nextChannel + 1
            ;
            ui->servoPWMValue2->setText(tr("Output %1 value : <b>%2</b> µs").arg(nextChannel + 1).arg(m_actuatorSettings[nextChannel].channelNeutral));

            if (m_actuatorSettings[nextChannel].channelMax < m_actuatorSettings[nextChannel].channelMin &&
                !ui->reverseCheckbox2->isChecked()) {
                ui->reverseCheckbox2->setChecked(true);
            } else {
                ui->reverseCheckbox2->setChecked(false);
            }
            enableServoSliders(false);
            if (ui->reverseCheckbox2->isChecked()) {
                ui->servoMaxAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelMax);
                ui->servoCenterAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelNeutral);
                ui->servoMinAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelMin);
            } else {
                ui->servoMinAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelMin);
                ui->servoCenterAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelNeutral);
                ui->servoMaxAngleSlider2->setValue(m_actuatorSettings[nextChannel].channelMax);
            }
        }
    }
    setupVehicleHighlightedPart();
    // Hide arrows
    showElementMovement(true, true, 0);
    showElementMovement(false, true, 0);
    showElementMovement(true, false, 0);
    showElementMovement(false, false, 0);
}

void OutputCalibrationPage::initializePage()
{
    if (m_vehicleScene) {
        setupVehicle();
        startWizard();
    }
}

bool OutputCalibrationPage::validatePage()
{
    if (!isFinished()) {
        m_currentWizardIndex++;
        while (!isFinished() && m_wizardIndexes[m_currentWizardIndex] == -1) {
            // Skip step, found a blank page
            // Dual servo setup, a '3' page is followed with a '-1' page
            m_currentWizardIndex++;
        }
        if (ui->calibrateAllMotors->isChecked() &&
            m_currentWizardIndex > 0 &&
            m_wizardIndexes[m_currentWizardIndex - 1] == 1) {
            while (!isFinished() && m_wizardIndexes[m_currentWizardIndex] == 1) {
                m_currentWizardIndex++;
            }
        }
    }

    if (isFinished()) {
        getWizard()->setActuatorSettings(m_actuatorSettings);
        return true;
    } else {
        setWizardPage();
        return false;
    }
}

void OutputCalibrationPage::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    if (m_vehicleBoundsItem) {
        ui->vehicleView->setSceneRect(m_vehicleBoundsItem->boundingRect());
        ui->vehicleView->fitInView(m_vehicleBoundsItem, Qt::KeepAspectRatio);
    }
}

void OutputCalibrationPage::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (m_vehicleBoundsItem) {
        ui->vehicleView->setSceneRect(m_vehicleBoundsItem->boundingRect());
        ui->vehicleView->fitInView(m_vehicleBoundsItem, Qt::KeepAspectRatio);
    }
}

void OutputCalibrationPage::customBackClicked()
{
    if (m_currentWizardIndex >= 0) {
        m_currentWizardIndex--;
        while (m_currentWizardIndex > 0 &&
               m_wizardIndexes[m_currentWizardIndex] == -1 &&
               m_wizardIndexes[m_currentWizardIndex - 1] == 3) {
            // Skip step, found a blank page
            // Dual servo setup, a '3' page is followed with a '-1' page
            m_currentWizardIndex--;
        }
        if (ui->calibrateAllMotors->isChecked()) {
            while (m_currentWizardIndex > 0 &&
                   m_wizardIndexes[m_currentWizardIndex] == 1 &&
                   m_wizardIndexes[m_currentWizardIndex - 1] == 1) {
                m_currentWizardIndex--;
            }
        }
    }

    if (m_currentWizardIndex >= 0) {
        setWizardPage();
    } else {
        getWizard()->back();
    }
}

void OutputCalibrationPage::getCurrentChannels(QList<quint16> &channels)
{
    if (ui->calibrateAllMotors->isChecked()) {
        for (int i = 1; i < m_channelIndex.size(); i++) {
            if (m_vehicleElementTypes[i + 1] == MOTOR) {
                channels << m_channelIndex[i];
            }
        }
    } else {
        channels << m_channelIndex[m_currentWizardIndex];
        // Add next channel for dual servo setup
        if (m_wizardIndexes[m_currentWizardIndex] == 3) {
            channels << m_channelIndex[m_currentWizardIndex + 1];
        }
    }
}

void OutputCalibrationPage::enableAllMotorsCheckBox(bool enable)
{
    if (getWizard()->getVehicleType() == SetupWizard::VEHICLE_MULTI) {
        ui->calibrateAllMotors->setVisible(true);
        ui->calibrateAllMotors->setEnabled(enable);
    } else {
        ui->calibrateAllMotors->setChecked(false);
        ui->calibrateAllMotors->setVisible(false);
    }
}

void OutputCalibrationPage::enableButtons(bool enable)
{
    getWizard()->button(QWizard::NextButton)->setEnabled(enable);
    getWizard()->button(QWizard::CustomButton1)->setEnabled(enable);
    getWizard()->button(QWizard::CancelButton)->setEnabled(enable);
    getWizard()->button(QWizard::BackButton)->setEnabled(enable);
    enableAllMotorsCheckBox(enable);
    QApplication::processEvents();
}

void OutputCalibrationPage::on_motorNeutralButton_toggled(bool checked)
{
    ui->motorNeutralButton->setText(checked ? tr("Stop") : tr("Start"));
    ui->motorNeutralSlider->setEnabled(checked);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    quint16 safeValue = m_actuatorSettings[currentChannel].channelMin;

    if (m_actuatorSettings[currentChannel].isReversableMotor) {
        safeValue = m_actuatorSettings[currentChannel].channelNeutral;
    }

    onStartButtonToggle(ui->motorNeutralButton, currentChannels, m_actuatorSettings[currentChannel].channelNeutral, safeValue, ui->motorNeutralSlider);
}

void OutputCalibrationPage::onStartButtonToggle(QAbstractButton *button, QList<quint16> &channels,
                                                quint16 value, quint16 safeValue, QSlider *slider)
{
    if (button->isChecked()) {
        // Start calibration
        if (checkAlarms()) {
            enableButtons(false);
            enableServoSliders(true);
            m_calibrationUtil->startChannelOutput(channels, safeValue);
            slider->setValue(value);
            m_calibrationUtil->setChannelOutputValue(value);
        } else {
            button->setChecked(false);
        }
    } else {
        // Stop calibration
        quint16 channel = channels[0];
        if ((button == ui->motorNeutralButton) && !m_actuatorSettings[channel].isReversableMotor) {
            // Normal motor
            m_calibrationUtil->startChannelOutput(channels, m_actuatorSettings[channel].channelMin);
        } else {
            // Servos and ReversableMotors
            m_calibrationUtil->startChannelOutput(channels, m_actuatorSettings[channel].channelNeutral);
        }

        m_calibrationUtil->stopChannelOutput();

        enableServoSliders(false);
        enableButtons(true);
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::onStartButtonToggleDual(QAbstractButton *button, QList<quint16> &channels,
                                                    quint16 value1, quint16 value2,
                                                    quint16 safeValue,
                                                    QSlider *slider1, QSlider *slider2)
{
    if (button->isChecked()) {
        // Start calibration
        if (checkAlarms()) {
            enableButtons(false);
            enableServoSliders(true);
            m_calibrationUtil->startChannelOutput(channels, safeValue);

            slider1->setValue(value1);
            slider2->setValue(value2);
            m_calibrationUtil->setChannelDualOutputValue(value1, value2);
        } else {
            button->setChecked(false);
        }
    } else {
        // Stop calibration
        quint16 channel1 = channels[0];
        quint16 channel2 = channels[1];

        m_calibrationUtil->startChannelOutput(channels, m_actuatorSettings[channel1].channelNeutral);
        m_calibrationUtil->stopChannelDualOutput(m_actuatorSettings[channel1].channelNeutral, m_actuatorSettings[channel2].channelNeutral);

        m_calibrationUtil->stopChannelOutput();

        enableServoSliders(false);
        enableButtons(true);
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::enableServoSliders(bool enabled)
{
    ui->servoCenterAngleSlider->setEnabled(enabled);
    ui->servoMinAngleSlider->setEnabled(enabled);
    ui->servoMaxAngleSlider->setEnabled(enabled);
    ui->reverseCheckbox->setEnabled(!enabled);

    ui->servoCenterAngleSlider1->setEnabled(enabled);
    ui->servoMinAngleSlider1->setEnabled(enabled);
    ui->servoMaxAngleSlider1->setEnabled(enabled);
    ui->reverseCheckbox1->setEnabled(!enabled);
    ui->servoCenterAngleSlider2->setEnabled(enabled);
    ui->servoMinAngleSlider2->setEnabled(enabled);
    ui->servoMaxAngleSlider2->setEnabled(enabled);
    ui->reverseCheckbox2->setEnabled(!enabled);
    // Hide arrows
    showElementMovement(true, true, 0);
    showElementMovement(false, true, 0);
}

bool OutputCalibrationPage::checkAlarms()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavObjectManager = pm->getObject<UAVObjectManager>();

    Q_ASSERT(uavObjectManager);
    SystemAlarms *systemAlarms    = SystemAlarms::GetInstance(uavObjectManager);
    Q_ASSERT(systemAlarms);
    SystemAlarms::DataFields data = systemAlarms->getData();

    if (data.Alarm[SystemAlarms::ALARM_ACTUATOR] != SystemAlarms::ALARM_OK) {
        QMessageBox mbox(this);
        mbox.setText(tr("The actuator module is in an error state.\n\n"
                        "Please make sure the correct firmware version is used then "
                        "restart the wizard and try again. If the problem persists please "
                        "consult the librepilot.org support forum."));
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.setIcon(QMessageBox::Critical);

        getWizard()->setWindowFlags(getWizard()->windowFlags() & ~Qt::WindowStaysOnTopHint);

        mbox.exec();

        getWizard()->setWindowFlags(getWizard()->windowFlags() | Qt::WindowStaysOnTopHint);
        getWizard()->setWindowIcon(qApp->windowIcon());
        getWizard()->show();
        return false;
    }
    return true;
}

void OutputCalibrationPage::debugLogChannelValues(bool showFirst)
{
    QList<quint16> currentChannels;
    quint16 currentChannel;

    getCurrentChannels(currentChannels);
    if (showFirst) {
        currentChannel = currentChannels[0];
    } else {
        currentChannel = currentChannels[1];
    }
    qDebug() << "ChannelMin    : " << m_actuatorSettings[currentChannel].channelMin;
    qDebug() << "ChannelNeutral: " << m_actuatorSettings[currentChannel].channelNeutral;
    qDebug() << "ChannelMax    : " << m_actuatorSettings[currentChannel].channelMax;
}

int OutputCalibrationPage::getLowOutputRate()
{
    if (getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT150 ||
        getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT600 ||
        getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT1200) {
        return LOW_OUTPUT_RATE_DSHOT;
    }
    return LOW_OUTPUT_RATE_PWM_MS;
}

int OutputCalibrationPage::getHighOutputRate()
{
    if (getWizard()->getEscType() == VehicleConfigurationSource::ESC_ONESHOT125 ||
        getWizard()->getEscType() == VehicleConfigurationSource::ESC_ONESHOT42 ||
        getWizard()->getEscType() == VehicleConfigurationSource::ESC_MULTISHOT) {
        return HIGH_OUTPUT_RATE_ONESHOT_MULTISHOT_MS;
    } else if (getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT150 ||
               getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT600 ||
               getWizard()->getEscType() == VehicleConfigurationSource::ESC_DSHOT1200) {
        return HIGH_OUTPUT_RATE_DSHOT;
    }
    return HIGH_OUTPUT_RATE_PWM_MS;
}

void OutputCalibrationPage::on_motorNeutralSlider_valueChanged(int value)
{
    Q_UNUSED(value);

    if (ui->motorNeutralSlider->minimum() == LOW_OUTPUT_RATE_DSHOT) {
        // DShot output
        ui->motorPWMValue->setText(tr("Digital output value : <b>%1</b>").arg(value));
    } else {
        ui->motorPWMValue->setText(tr("Output value : <b>%1</b> µs").arg(value));
    }
    if (ui->motorNeutralButton->isChecked()) {
        quint16 value = ui->motorNeutralSlider->value();
        m_calibrationUtil->setChannelOutputValue(value);

        QList<quint16> currentChannels;
        getCurrentChannels(currentChannels);
        foreach(quint16 channel, currentChannels) {
            m_actuatorSettings[channel].channelNeutral = value;
        }
        debugLogChannelValues(true);
    }
}

void OutputCalibrationPage::on_servoButton_toggled(bool checked)
{
    ui->servoButton->setText(checked ? tr("Stop") : tr("Start"));
    // Now we set servos, motors are done (Tricopter fix)
    ui->calibrateAllMotors->setChecked(false);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    quint16 safeValue = m_actuatorSettings[currentChannel].channelNeutral;
    onStartButtonToggle(ui->servoButton, currentChannels, safeValue, safeValue, ui->servoCenterAngleSlider);
}

void OutputCalibrationPage::on_dualservoButton_toggled(bool checked)
{
    ui->dualservoButton->setText(checked ? tr("Stop") : tr("Start"));
    // Now we set servos, motors are done (Tricopter fix)
    ui->calibrateAllMotors->setChecked(false);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];
    quint16 nextChannel    = currentChannels[1];

    quint16 safeValue1     = m_actuatorSettings[currentChannel].channelNeutral;
    quint16 safeValue2     = m_actuatorSettings[nextChannel].channelNeutral;
    onStartButtonToggleDual(ui->dualservoButton, currentChannels, safeValue1, safeValue2, safeValue1,
                            ui->servoCenterAngleSlider1, ui->servoCenterAngleSlider2);
}

//
// Single servo page (2)
//
void OutputCalibrationPage::on_servoCenterAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoCenterAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    ui->servoPWMValue->setText(tr("Output %1 value : <b>%2</b> µs").arg(currentChannel + 1).arg(value));

    bool showFirst = true;
    setSliderLimitsAndArrows(currentChannel, showFirst, value, ui->reverseCheckbox, ui->servoMinAngleSlider, ui->servoMaxAngleSlider);

    debugLogChannelValues(showFirst);
}

void OutputCalibrationPage::on_servoMinAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoMinAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];
    m_actuatorSettings[currentChannel].channelMin = value;
    ui->servoPWMValue->setText(tr("Output %1 value : <b>%2</b> µs (Min)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and max
    if (ui->reverseCheckbox->isChecked()) {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    } else {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::on_servoMaxAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoMaxAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];
    m_actuatorSettings[currentChannel].channelMax = value;
    ui->servoPWMValue->setText(tr("Output %1 value : <b>%2</b> µs (Max)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and min
    if (ui->reverseCheckbox->isChecked()) {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::on_reverseCheckbox_toggled(bool checked)
{
    Q_UNUSED(checked);
    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    reverseCheckBoxIsToggled(currentChannel, ui->reverseCheckbox,
                             ui->servoCenterAngleSlider, ui->servoMinAngleSlider, ui->servoMaxAngleSlider);

    ui->servoPWMValue->setText(tr("Output %1 value : <b>%2</b> µs (Max)")
                               .arg(currentChannel + 1).arg(m_actuatorSettings[currentChannel].channelMax));
}

//
// Dual servo page (3) - first channel
//
void OutputCalibrationPage::on_servoCenterAngleSlider1_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoCenterAngleSlider1->value();
    quint16 value2 = ui->servoCenterAngleSlider2->value();
    m_calibrationUtil->setChannelDualOutputValue(value, value2);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];


    ui->servoPWMValue1->setText(tr("Output %1 value : <b>%2</b> µs").arg(currentChannel + 1).arg(value));

    bool showFirst = true;
    setSliderLimitsAndArrows(currentChannel, showFirst, value, ui->reverseCheckbox1, ui->servoMinAngleSlider1, ui->servoMaxAngleSlider1);

    debugLogChannelValues(showFirst);
}

void OutputCalibrationPage::on_servoMinAngleSlider1_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoMinAngleSlider1->value();
    quint16 value2 = ui->servoCenterAngleSlider2->value();
    m_calibrationUtil->setChannelDualOutputValue(value, value2);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];
    m_actuatorSettings[currentChannel].channelMin = value;
    ui->servoPWMValue1->setText(tr("Output %1 value : <b>%2</b> µs (Min)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and max
    if (ui->reverseCheckbox1->isChecked()) {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider1->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider1->setValue(value);
        }
    } else {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider1->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider1->setValue(value);
        }
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::on_servoMaxAngleSlider1_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoMaxAngleSlider1->value();
    quint16 value2 = ui->servoCenterAngleSlider2->value();
    m_calibrationUtil->setChannelDualOutputValue(value, value2);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];
    m_actuatorSettings[currentChannel].channelMax = value;
    ui->servoPWMValue1->setText(tr("Output %1 value : <b>%2</b> µs (Max)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and min
    if (ui->reverseCheckbox1->isChecked()) {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider1->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider1->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider1->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider1->setValue(value);
        }
    }
    debugLogChannelValues(true);
}

void OutputCalibrationPage::on_reverseCheckbox1_toggled(bool checked)
{
    Q_UNUSED(checked);
    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    reverseCheckBoxIsToggled(currentChannel, ui->reverseCheckbox1,
                             ui->servoCenterAngleSlider1, ui->servoMinAngleSlider1, ui->servoMaxAngleSlider1);

    ui->servoPWMValue1->setText(tr("Output %1 value : <b>%2</b> µs (Max)")
                                .arg(currentChannel + 1).arg(m_actuatorSettings[currentChannel].channelMax));
}

//
// Dual servo page - second channel
//
void OutputCalibrationPage::on_servoCenterAngleSlider2_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoCenterAngleSlider2->value();
    quint16 value1 = ui->servoCenterAngleSlider1->value();
    m_calibrationUtil->setChannelDualOutputValue(value1, value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[1];

    ui->servoPWMValue2->setText(tr("Output %1 value : <b>%2</b> µs").arg(currentChannel + 1).arg(value));

    bool showFirst = false;
    setSliderLimitsAndArrows(currentChannel, showFirst, value, ui->reverseCheckbox2, ui->servoMinAngleSlider2, ui->servoMaxAngleSlider2);

    debugLogChannelValues(showFirst);
}

void OutputCalibrationPage::on_servoMinAngleSlider2_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoMinAngleSlider2->value();
    quint16 value1 = ui->servoCenterAngleSlider1->value();
    m_calibrationUtil->setChannelDualOutputValue(value1, value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[1];
    m_actuatorSettings[currentChannel].channelMin = value;
    ui->servoPWMValue2->setText(tr("Output %1 value : <b>%2</b> µs (Min)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and max
    if (ui->reverseCheckbox2->isChecked()) {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider2->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider2->setValue(value);
        }
    } else {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider2->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider2->setValue(value);
        }
    }
    debugLogChannelValues(false);
}

void OutputCalibrationPage::on_servoMaxAngleSlider2_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value  = ui->servoMaxAngleSlider2->value();
    quint16 value1 = ui->servoCenterAngleSlider1->value();
    m_calibrationUtil->setChannelDualOutputValue(value1, value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[1];
    m_actuatorSettings[currentChannel].channelMax = value;
    ui->servoPWMValue2->setText(tr("Output %1 value : <b>%2</b> µs (Max)").arg(currentChannel + 1).arg(value));

    // Adjust neutral and min
    if (ui->reverseCheckbox2->isChecked()) {
        if (value >= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider2->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider2->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[currentChannel].channelNeutral) {
            ui->servoCenterAngleSlider2->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider2->setValue(value);
        }
    }
    debugLogChannelValues(false);
}

void OutputCalibrationPage::on_reverseCheckbox2_toggled(bool checked)
{
    Q_UNUSED(checked);
    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[1];


    reverseCheckBoxIsToggled(currentChannel, ui->reverseCheckbox2,
                             ui->servoCenterAngleSlider2, ui->servoMinAngleSlider2, ui->servoMaxAngleSlider2);

    ui->servoPWMValue2->setText(tr("Output %1 value : <b>%2</b> µs (Max)")
                                .arg(currentChannel + 1).arg(m_actuatorSettings[currentChannel].channelMax));
}

void OutputCalibrationPage::on_calibrateAllMotors_toggled(bool checked)
{
    Q_UNUSED(checked);
    setupVehicleHighlightedPart();
}

void OutputCalibrationPage::resetOutputCalibrationUtil()
{
    if (m_calibrationUtil) {
        delete m_calibrationUtil;
        m_calibrationUtil = 0;
    }
    m_calibrationUtil = new OutputCalibrationUtil();
}

//
// Set Min/Max slider values and display servo movement with arrow
//
void OutputCalibrationPage::setSliderLimitsAndArrows(quint16 currentChannel, bool showFirst, quint16 value,
                                                     QCheckBox *revCheckbox, QSlider *minSlider, QSlider *maxSlider)
{
    m_actuatorSettings[currentChannel].channelNeutral = value;

    // Adjust min and max
    if (revCheckbox->isChecked()) {
        if (value >= m_actuatorSettings[currentChannel].channelMin) {
            minSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMax) {
            maxSlider->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[currentChannel].channelMin) {
            minSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMax) {
            maxSlider->setValue(value);
        }
    }

    quint16 minValue = (revCheckbox->isChecked()) ? maxSlider->value() : minSlider->value();
    quint16 maxValue = (revCheckbox->isChecked()) ? minSlider->value() : maxSlider->value();
    quint16 range    = maxValue - minValue;

    // Reset all arrows
    showElementMovement(true, showFirst, 0);
    showElementMovement(false, showFirst, 0);
    showElementMovement(true, !showFirst, 0);
    showElementMovement(false, !showFirst, 0);

    // 35% "Dead band" : no arrow display
    quint16 limitLow   = minValue + (range * 0.35);

    quint16 limitHigh  = maxValue - (range * 0.35);
    quint16 middle     = minValue + (range / 2);
    qreal arrowOpacity = 0;
    if (value < limitLow) {
        arrowOpacity = (qreal)(middle - value) / (qreal)(middle - minValue);

        showElementMovement(revCheckbox->isChecked(), showFirst, arrowOpacity);
    } else if (value > limitHigh) {
        arrowOpacity = (qreal)(value - middle) / (qreal)(maxValue - middle);
        showElementMovement(!revCheckbox->isChecked(), showFirst, arrowOpacity);
    }
}

//
// Set Center/Min/Max slider limits per reverse checkbox status
//
void OutputCalibrationPage::reverseCheckBoxIsToggled(quint16 currentChannel,
                                                     QCheckBox *checkBox, QSlider *centerSlider, QSlider *minSlider, QSlider *maxSlider)
{
    bool checked = checkBox->isChecked();

    if (checked && m_actuatorSettings[currentChannel].channelMax > m_actuatorSettings[currentChannel].channelMin) {
        quint16 oldMax = m_actuatorSettings[currentChannel].channelMax;
        m_actuatorSettings[currentChannel].channelMax = m_actuatorSettings[currentChannel].channelMin;
        m_actuatorSettings[currentChannel].channelMin = oldMax;
    } else if (!checkBox->isChecked() && m_actuatorSettings[currentChannel].channelMax < m_actuatorSettings[currentChannel].channelMin) {
        quint16 oldMax = m_actuatorSettings[currentChannel].channelMax;
        m_actuatorSettings[currentChannel].channelMax = m_actuatorSettings[currentChannel].channelMin;
        m_actuatorSettings[currentChannel].channelMin = oldMax;
    }
    centerSlider->setInvertedAppearance(checked);
    centerSlider->setInvertedControls(checked);
    minSlider->setInvertedAppearance(checked);
    minSlider->setInvertedControls(checked);
    maxSlider->setInvertedAppearance(checked);
    maxSlider->setInvertedControls(checked);

    if (checkBox->isChecked()) {
        maxSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
        centerSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
        minSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
    } else {
        minSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
        centerSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
        maxSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
    }
}
