/**
 ******************************************************************************
 *
 * @file       osgearth_global.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
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

#ifndef OSGEARTH_GLOBAL_H
#define OSGEARTH_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(OSGEARTH_LIBRARY)
#  define OSGEARTH_LIB_EXPORT Q_DECL_EXPORT
#elif  defined(OSGEARTH_STATIC_LIBRARY) // Abuse single files for manual tests
#  define OSGEARTH_LIB_EXPORT
#else
#  define OSGEARTH_LIB_EXPORT Q_DECL_IMPORT
#endif

#endif // OSGEARTH_GLOBAL_H
