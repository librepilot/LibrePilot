/**
 ******************************************************************************
 *
 * @file       escpage.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup
 * @{
 * @addtogroup EscPage
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

#include "escpage.h"
#include "ui_escpage.h"
#include "setupwizard.h"

EscPage::EscPage(SetupWizard *wizard, QWidget *parent) :
    AbstractWizardPage(wizard, parent),

    ui(new Ui::EscPage)
{
    ui->setupUi(this);
}

EscPage::~EscPage()
{
    delete ui;
}

void EscPage::initializePage()
{
    bool enabled = isSynchOrOneShotAvailable();

    ui->oneshot125ESCButton->setEnabled(enabled);
    ui->oneshot42ESCButton->setEnabled(enabled);
    ui->multishotESCButton->setEnabled(enabled);

    if ((ui->oneshot125ESCButton->isChecked() ||
         ui->oneshot42ESCButton->isChecked() ||
         ui->multishotESCButton->isChecked()) && !enabled) {
        ui->oneshot125ESCButton->setChecked(false);
        ui->oneshot42ESCButton->setChecked(false);
        ui->multishotESCButton->setChecked(false);
        ui->rapidESCButton->setChecked(true);
    }

    enabled = isFastDShotAvailable();
    ui->dshot1200ESCButton->setEnabled(enabled);
    if (ui->dshot1200ESCButton->isChecked() && !enabled) {
        ui->dshot1200ESCButton->setChecked(false);
        ui->dshot600ESCButton->setChecked(true);
    }
}

bool EscPage::validatePage()
{
    if (ui->dshot1200ESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_DSHOT1200);
    } else if (ui->dshot600ESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_DSHOT600);
    } else if (ui->dshot150ESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_DSHOT150);
    } else if (ui->multishotESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_MULTISHOT);
    } else if (ui->oneshot42ESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_ONESHOT42);
    } else if (ui->oneshot125ESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_ONESHOT125);
    } else if (ui->rapidESCButton->isChecked()) {
        if (isSynchOrOneShotAvailable()) {
            getWizard()->setEscType(SetupWizard::ESC_SYNCHED);
        } else {
            getWizard()->setEscType(SetupWizard::ESC_RAPID);
        }
    } else if (ui->defaultESCButton->isChecked()) {
        getWizard()->setEscType(SetupWizard::ESC_STANDARD300);
    }

    return true;
}

bool EscPage::isSynchOrOneShotAvailable()
{
    bool available = true;

    switch (getWizard()->getControllerType()) {
    case SetupWizard::CONTROLLER_NANO:
    case SetupWizard::CONTROLLER_CC:
    case SetupWizard::CONTROLLER_CC3D:
        switch (getWizard()->getVehicleType()) {
        case SetupWizard::VEHICLE_MULTI:
            switch (getWizard()->getVehicleSubType()) {
            case SetupWizard::MULTI_ROTOR_TRI_Y:
            case SetupWizard::MULTI_ROTOR_QUAD_X:
            case SetupWizard::MULTI_ROTOR_QUAD_H:
            case SetupWizard::MULTI_ROTOR_QUAD_PLUS:
                available = getWizard()->getInputType() != SetupWizard::INPUT_PWM;
                break;
            default:
                available = false;
                break;
            }
            break;
        default:
            break;
        }
        break;
    case SetupWizard::CONTROLLER_REVO:
    case SetupWizard::CONTROLLER_SPARKY2:
        available = true;
        break;
    default:
        break;
    }

    return available;
}

bool EscPage::isFastDShotAvailable()
{
    bool available = true;

    switch (getWizard()->getControllerType()) {
    case SetupWizard::CONTROLLER_CC3D:
    case SetupWizard::CONTROLLER_SPRACINGF3:
    case SetupWizard::CONTROLLER_SPRACINGF3EVO:
    case SetupWizard::CONTROLLER_PIKOBLX:
    case SetupWizard::CONTROLLER_TINYFISH:
        available = false;
        break;
    default:
        break;
    }

    return available;
}
