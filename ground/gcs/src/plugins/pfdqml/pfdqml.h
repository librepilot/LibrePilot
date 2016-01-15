/**
 ******************************************************************************
 *
 * @file       pfdqml.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************//*
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

#ifndef PFDQML_H_
#define PFDQML_H_

#include <QObject>
#include <QtQml>

class ModelSelectionMode : public QObject {
    Q_OBJECT

public:
    enum Enum { Auto, Predefined };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5

    static void registerQMLTypes()
    {
        qmlRegisterType<ModelSelectionMode>("Pfd", 1, 0, "ModelSelectionMode");
    }
};

class TimeMode : public QObject {
    Q_OBJECT

public:
    enum Enum { Local, Predefined };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5

    static void registerQMLTypes()
    {
        qmlRegisterType<TimeMode>("Pfd", 1, 0, "TimeMode");
    }
};

#endif // PFDQML_H_
