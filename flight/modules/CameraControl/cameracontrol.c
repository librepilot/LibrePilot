/**
 ******************************************************************************
 *
 * @file       cameracontrol.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @brief      camera control module. triggers cameras with multiple options
 *
 * @see        The GNU Public License (GPL) Version 3
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <openpilot.h>
#include "inc/cameracontrol.h"
#include <cameradesired.h>
#include <cameracontrolsettings.h>
#include <cameracontrolactivity.h>
#include <hwsettings.h>

/*
// Private variables
static struct CameraControl_data {
    portTickType lastSysTime;
} *ccd;
*/

//static void setOutput();


/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CameraControlInitialize(void)
{
    return 0;
}

/* stub: module has no module thread */
int32_t CameraControlStart(void)
{
    return 0;
}

MODULE_INITCALL(CameraControlInitialize, CameraControlStart);
