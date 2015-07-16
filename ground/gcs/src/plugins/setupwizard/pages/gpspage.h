/**
 ******************************************************************************
 *
 * @file       gpspage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup
 * @{
 * @addtogroup GpsPage
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

#ifndef GPSPAGE_H
#define GPSPAGE_H

#include "selectionpage.h"

class GpsPage : public SelectionPage {
    Q_OBJECT

public:
    explicit GpsPage(SetupWizard *wizard, QWidget *parent = 0);
    ~GpsPage();

public:
    void initializePage(VehicleConfigurationSource *settings);
    bool validatePage(SelectionItem *selectedItem);
    void setupSelection(Selection *selection);
};

#endif // GPSPAGE_H
