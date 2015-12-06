/**
 ******************************************************************************
 *
 * @file       pathutils.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 * @brief      String utilities
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
 */

#include "stringutils.h"

namespace Utils {
QString toLowerCamelCase(const QString & name)
{
    QString str = name;

    for (int i = 0; i < str.length(); ++i) {
        if (str[i].isLower() || !str[i].isLetter()) {
            break;
        }
        if (i > 0 && i < str.length() - 1) {
            // after first, look ahead one
            if (str[i + 1].isLower()) {
                break;
            }
        }
        str[i] = str[i].toLower();
    }

    return str;
}
}
