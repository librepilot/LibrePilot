/**
 ******************************************************************************
 *
 * @file       outputcalibrationpage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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
            m_actuatorSettings[servoid].channelMin        = LOW_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelNeutral    = LOW_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelMax        = getHighOutputRate();
            m_actuatorSettings[servoid].isReversableMotor = false;
            // Car and Tank should use reversable Esc/motors
            if ((getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_CAR)
                || (getWizard()->getVehicleSubType() == SetupWizard::GROUNDVEHICLE_DIFFERENTIAL)) {
                m_actuatorSettings[servoid].channelNeutral    = NEUTRAL_OUTPUT_RATE_MILLISECONDS;
                m_actuatorSettings[servoid].isReversableMotor = true;
                // Set initial output value
                m_calibrationUtil->startChannelOutput(servoid, NEUTRAL_OUTPUT_RATE_MILLISECONDS);
                m_calibrationUtil->stopChannelOutput();
            }
        } else if (servoid < totalUsedChannels) {
            // Set to servo safe values
            m_actuatorSettings[servoid].channelMin     = NEUTRAL_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelNeutral = NEUTRAL_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelMax     = NEUTRAL_OUTPUT_RATE_MILLISECONDS;
            // Set initial servo output value
            m_calibrationUtil->startChannelOutput(servoid, NEUTRAL_OUTPUT_RATE_MILLISECONDS);
            m_calibrationUtil->stopChannelOutput();
        } else {
            // "Disable" these channels
            m_actuatorSettings[servoid].channelMin     = LOW_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelNeutral = LOW_OUTPUT_RATE_MILLISECONDS;
            m_actuatorSettings[servoid].channelMax     = LOW_OUTPUT_RATE_MILLISECONDS;
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
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2 << 2;
        m_vehicleElementIds << "aileron" << "aileron-frame" << "aileron-motor" << "aileron-ail-left" << "aileron-ail-right" << "aileron-elevator" << "aileron-rudder";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 2 << 0 << 5 << 1 << 3;

        setupActuatorMinMaxAndNeutral(2, 2, 6); // should be 5 instead 6 but output 5 is not used

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_AILERON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2;
        m_vehicleElementIds << "singleaileron" << "singleaileron-frame" << "singleaileron-motor" << "singleaileron-aileron" << "singleaileron-elevator" << "singleaileron-rudder";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4;
        m_channelIndex << 0 << 2 << 0 << 1 << 3;

        setupActuatorMinMaxAndNeutral(2, 2, 4);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_ELEVON:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2;
        m_vehicleElementIds << "elevon" << "elevon-frame" << "elevon-motor" << "elevon-left" << "elevon-right";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3;
        m_channelIndex << 0 << 2 << 0 << 1;

        setupActuatorMinMaxAndNeutral(2, 2, 3);

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;
    case SetupWizard::FIXED_WING_VTAIL:
        loadSVGFile(FIXEDWING_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2 << 2 << 2 << 2;
        m_vehicleElementIds << "vtail" << "vtail-frame" << "vtail-motor" << "vtail-ail-left" << "vtail-ail-right" << "vtail-rudder-left" << "vtail-rudder-right";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO << SERVO << SERVO << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2 << 3 << 4 << 5;
        m_channelIndex << 0 << 2 << 0 << 5 << 3 << 1;

        setupActuatorMinMaxAndNeutral(2, 2, 6); // should be 5 instead 6 but output 5 is not used

        getWizard()->setActuatorSettings(m_actuatorSettings);
        break;

    // Ground vehicles
    case SetupWizard::GROUNDVEHICLE_CAR:
        loadSVGFile(GROUND_SVG_FILE);
        m_wizardIndexes << 0 << 1 << 2;
        m_vehicleElementIds << "car" << "car-frame" << "car-motor" << "car-steering";
        m_vehicleElementTypes << FULL << FRAME << MOTOR << SERVO;
        m_vehicleHighlightElementIndexes << 0 << 1 << 2;
        m_channelIndex << 0 << 1 << 0;

        setupActuatorMinMaxAndNeutral(1, 1, 2);

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
        m_channelIndex << 0 << 1 << 0;

        setupActuatorMinMaxAndNeutral(1, 1, 2);

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

    for (int i = 0; i < m_vehicleItems.size(); i++) {
        QGraphicsSvgItem *item = m_vehicleItems[i];
        if (highlightedIndex == i || (ui->calibrateAllMotors->isChecked() && m_vehicleElementTypes[i + 1] == MOTOR)) {
            item->setOpacity(highlightOpaque);
        } else {
            item->setOpacity(dimOpaque);
        }
    }
}

void OutputCalibrationPage::showElementMovement(bool isUp, qreal value)
{
    QString highlightedItemName = m_vehicleItems[m_currentWizardIndex]->elementId();

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
            ui->motorNeutralSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
            ui->motorPWMValue->setText(QString(tr("Output value : <b>%1</b> µs")).arg(m_actuatorSettings[currentChannel].channelNeutral));
            // Reversable motor found
            if (m_actuatorSettings[currentChannel].isReversableMotor) {
                ui->motorNeutralSlider->setMinimum(m_actuatorSettings[currentChannel].channelMin);
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
        }
    }
    setupVehicleHighlightedPart();
    // Hide arrows
    showElementMovement(true, 0);
    showElementMovement(false, 0);
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
    debugLogChannelValues();
}

void OutputCalibrationPage::enableServoSliders(bool enabled)
{
    ui->servoCenterAngleSlider->setEnabled(enabled);
    ui->servoMinAngleSlider->setEnabled(enabled);
    ui->servoMaxAngleSlider->setEnabled(enabled);
    ui->reverseCheckbox->setEnabled(!enabled);
    // Hide arrows
    showElementMovement(true, 0);
    showElementMovement(false, 0);
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
        mbox.setText(QString(tr("The actuator module is in an error state.\n\n"
                                "Please make sure the correct firmware version is used then "
                                "restart the wizard and try again. If the problem persists please "
                                "consult the openpilot.org support forum.")));
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

void OutputCalibrationPage::debugLogChannelValues()
{
    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    qDebug() << "ChannelMin    : " << m_actuatorSettings[currentChannel].channelMin;
    qDebug() << "ChannelNeutral: " << m_actuatorSettings[currentChannel].channelNeutral;
    qDebug() << "ChannelMax    : " << m_actuatorSettings[currentChannel].channelMax;
}

int OutputCalibrationPage::getHighOutputRate()
{
    if (getWizard()->getEscType() == SetupWizard::ESC_ONESHOT) {
        return HIGH_OUTPUT_RATE_MILLISECONDS_ONESHOT125;
    } else {
        return HIGH_OUTPUT_RATE_MILLISECONDS_PWM;
    }
}

void OutputCalibrationPage::on_motorNeutralSlider_valueChanged(int value)
{
    Q_UNUSED(value);
    ui->motorPWMValue->setText(tr("Output value : <b>%1</b> µs").arg(value));

    if (ui->motorNeutralButton->isChecked()) {
        quint16 value = ui->motorNeutralSlider->value();
        m_calibrationUtil->setChannelOutputValue(value);

        QList<quint16> currentChannels;
        getCurrentChannels(currentChannels);
        foreach(quint16 channel, currentChannels) {
            m_actuatorSettings[channel].channelNeutral = value;
        }
        debugLogChannelValues();
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

void OutputCalibrationPage::on_servoCenterAngleSlider_valueChanged(int position)
{
    Q_UNUSED(position);
    quint16 value = ui->servoCenterAngleSlider->value();
    m_calibrationUtil->setChannelOutputValue(value);

    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    m_actuatorSettings[currentChannel].channelNeutral = value;
    ui->servoPWMValue->setText(tr("Output value : <b>%1</b> µs").arg(value));

    // Adjust min and max
    if (ui->reverseCheckbox->isChecked()) {
        if (value >= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
        if (value <= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    } else {
        if (value <= m_actuatorSettings[currentChannel].channelMin) {
            ui->servoMinAngleSlider->setValue(value);
        }
        if (value >= m_actuatorSettings[currentChannel].channelMax) {
            ui->servoMaxAngleSlider->setValue(value);
        }
    }

    quint16 minValue = (ui->reverseCheckbox->isChecked()) ? ui->servoMaxAngleSlider->value() : ui->servoMinAngleSlider->value();
    quint16 maxValue = (ui->reverseCheckbox->isChecked()) ? ui->servoMinAngleSlider->value() : ui->servoMaxAngleSlider->value();
    quint16 range    = maxValue - minValue;
    // Reset arows
    showElementMovement(true, 0);
    showElementMovement(false, 0);

    // 30% "Dead band" : no arrow display
    quint16 limitLow   = minValue + (range * 0.35);
    quint16 limitHigh  = maxValue - (range * 0.35);
    quint16 middle     = minValue + (range / 2);
    qreal arrowOpacity = 0;
    if (value < limitLow) {
        arrowOpacity = (qreal)(middle - value) / (qreal)(middle - minValue);
        showElementMovement(ui->reverseCheckbox->isChecked(), arrowOpacity);
    } else if (value > limitHigh) {
        arrowOpacity = (qreal)(value - middle) / (qreal)(maxValue - middle);
        showElementMovement(!ui->reverseCheckbox->isChecked(), arrowOpacity);
    }

    debugLogChannelValues();
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
    ui->servoPWMValue->setText(tr("Output value : <b>%1</b> µs (Min)").arg(value));

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
    debugLogChannelValues();
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
    ui->servoPWMValue->setText(tr("Output value : <b>%1</b> µs (Max)").arg(value));

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
    debugLogChannelValues();
}

void OutputCalibrationPage::on_reverseCheckbox_toggled(bool checked)
{
    QList<quint16> currentChannels;
    getCurrentChannels(currentChannels);
    quint16 currentChannel = currentChannels[0];

    if (checked && m_actuatorSettings[currentChannel].channelMax > m_actuatorSettings[currentChannel].channelMin) {
        quint16 oldMax = m_actuatorSettings[currentChannel].channelMax;
        m_actuatorSettings[currentChannel].channelMax = m_actuatorSettings[currentChannel].channelMin;
        m_actuatorSettings[currentChannel].channelMin = oldMax;
    } else if (!checked && m_actuatorSettings[currentChannel].channelMax < m_actuatorSettings[currentChannel].channelMin) {
        quint16 oldMax = m_actuatorSettings[currentChannel].channelMax;
        m_actuatorSettings[currentChannel].channelMax = m_actuatorSettings[currentChannel].channelMin;
        m_actuatorSettings[currentChannel].channelMin = oldMax;
    }
    ui->servoCenterAngleSlider->setInvertedAppearance(checked);
    ui->servoCenterAngleSlider->setInvertedControls(checked);
    ui->servoMinAngleSlider->setInvertedAppearance(checked);
    ui->servoMinAngleSlider->setInvertedControls(checked);
    ui->servoMaxAngleSlider->setInvertedAppearance(checked);
    ui->servoMaxAngleSlider->setInvertedControls(checked);

    if (ui->reverseCheckbox->isChecked()) {
        ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
        ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
        ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
    } else {
        ui->servoMinAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMin);
        ui->servoCenterAngleSlider->setValue(m_actuatorSettings[currentChannel].channelNeutral);
        ui->servoMaxAngleSlider->setValue(m_actuatorSettings[currentChannel].channelMax);
    }
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
