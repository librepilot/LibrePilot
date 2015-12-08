/**
 ******************************************************************************
 *
 * @file       stringutils.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
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
 */

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "utils_global.h"

#include <QString>

namespace Utils {
/**
 * Convert a string to lower camel case.
 * Handles following cases :
 * - Property -> property
 * - MyProperty -> myProperty
 * - MYProperty -> myProperty
 * - MY_Property -> my_Property
 * - MY -> my
 */
QTCREATOR_UTILS_EXPORT QString toLowerCamelCase(const QString &);
}

#endif /* STRINGUTILS_H */
