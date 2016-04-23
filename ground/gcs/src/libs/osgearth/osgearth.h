/**
 ******************************************************************************
 *
 * @file       osgearth.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup
 * @{
 * @addtogroup
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

#ifndef OSGEARTH_H
#define OSGEARTH_H

#include "osgearth_global.h"

class OSGEARTH_LIB_EXPORT OsgEarth {
public:
    static void registerQmlTypes();
    static void initialize();

private:
    static bool registered;
    static bool initialized;

    static void initializePathes();
    static void initializeCache();

    // Sets the WindowingSystem to Qt.
    static void initWindowingSystem();

    static void displayInfo();
};

#endif // OSGEARTH_H
