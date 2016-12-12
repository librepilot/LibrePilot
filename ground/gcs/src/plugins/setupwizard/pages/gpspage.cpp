/**
 ******************************************************************************
 *
 * @file       fixedwingpage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup FixedWingPage
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

#include "gpspage.h"
#include "setupwizard.h"

GpsPage::GpsPage(SetupWizard *wizard, QWidget *parent) :
    SelectionPage(wizard, QString(":/setupwizard/resources/sensor-shapes.svg"), parent)
{}

GpsPage::~GpsPage()
{}

void GpsPage::initializePage(VehicleConfigurationSource *settings)
{
    // Enable all
    setItemDisabled(-1, false);

    bool isSparky = (getWizard()->getControllerType() == SetupWizard::CONTROLLER_SPARKY2);
    // All rcinputs are on rcvrport for sparky2, that leaves mainport/flexiport available for gps/auxmag
    if (!isSparky && (settings->getInputType() == VehicleConfigurationSource::INPUT_SBUS ||
                      settings->getInputType() == VehicleConfigurationSource::INPUT_DSM ||
                      settings->getInputType() == VehicleConfigurationSource::INPUT_SRXL ||
                      settings->getInputType() == VehicleConfigurationSource::INPUT_HOTT_SUMD ||
                      settings->getInputType() == VehicleConfigurationSource::INPUT_IBUS ||
                      settings->getInputType() == VehicleConfigurationSource::INPUT_EXBUS)) {
        // Disable GPS+I2C Mag
        setItemDisabled(VehicleConfigurationSource::GPS_UBX_FLEXI_I2CMAG, true);
        if (getSelectedItem()->id() == VehicleConfigurationSource::GPS_UBX_FLEXI_I2CMAG) {
            // If previously selected invalid GPS, reset to no GPS
            setSelectedItem(VehicleConfigurationSource::GPS_DISABLED);
        }
    }
}

bool GpsPage::validatePage(SelectionItem *selectedItem)
{
    getWizard()->setGpsType((SetupWizard::GPS_TYPE)selectedItem->id());
    if (getWizard()->getGpsType() == SetupWizard::GPS_DISABLED) {
        getWizard()->setAirspeedType(VehicleConfigurationSource::AIRSPEED_DISABLED);
    }
    return true;
}

void GpsPage::setupSelection(Selection *selection)
{
    selection->setTitle(tr("GPS Selection"));
    selection->setText(tr("Please select the type of GPS you wish to use. As well as OpenPilot hardware, "
                          "3rd party GPSs are supported also, although please note that performance could "
                          "be less than optimal as not all GPSs are created equal.\n\n"
                          "Note: NMEA only GPSs perform poorly on VTOL aircraft and are not recommended for Helis and MultiRotors.\n\n"
                          "Please select your GPS type data below:"));

    selection->addItem(tr("Disabled"),
                       tr("GPS Features are not to be enabled"),
                       "no-gps",
                       SetupWizard::GPS_DISABLED);

    selection->addItem(tr("OpenPilot Platinum"),
                       tr("Select this option to use the OpenPilot Platinum GPS with integrated Magnetometer "
                          "and Microcontroller.\n\n"
                          "Note: for the OpenPilot v8 GPS please select the U-Blox option."),
                       "OPGPS-v9",
                       SetupWizard::GPS_PLATINUM);

    selection->addItem(tr("Naza GPS"),
                       tr("Select this option to use the Naza GPS with integrated Magnetometer."),
                       "NazaGPS",
                       SetupWizard::GPS_NAZA);

    selection->addItem(tr("U-Blox Based + Magnetometer"),
                       tr("Select this option for the generic U-Blox chipset based GPS + I2C Magnetometer.\n\n"
                          "GPS is connected to MainPort and two wires I2C to FlexiPort."),
                       "generic-ublox-mag",
                       SetupWizard::GPS_UBX_FLEXI_I2CMAG);

    selection->addItem(tr("U-Blox Based"),
                       tr("Select this option for the OpenPilot V8 GPS or generic U-Blox chipset based GPS."),
                       "OPGPS-v8-ublox",
                       SetupWizard::GPS_UBX);

    selection->addItem(tr("NMEA Based"),
                       tr("Select this option for a generic NMEA based GPS."),
                       "generic-nmea",
                       SetupWizard::GPS_NMEA);
}
