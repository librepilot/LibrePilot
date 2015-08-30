/**
 ******************************************************************************
 *
 * @file       gcsdirs.h
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

#ifndef GCSDIRS_H
#define GCSDIRS_H

#include "utils_global.h"

class QString;

class QTCREATOR_UTILS_EXPORT GCSDirs {
public:
    GCSDirs();

    static QString rootDir();

    static QString libraryPath(QString provider);

    static QString pluginPath(QString provider);

    static QString sharePath(QString provider);

    static QString gcsSharePath();

    static QString gcsPluginPath();

    static void debug();

private:
    Q_DISABLE_COPY(GCSDirs)
    class Private;
    Private *const d;
};

#endif // GCSDIRS_H
