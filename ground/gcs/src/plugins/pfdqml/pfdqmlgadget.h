/**
 ******************************************************************************
 *
 * @file       pfdqmlgadget.h
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

#ifndef PFDQMLGADGET_H_
#define PFDQMLGADGET_H_

#include "pfdqmlgadget.h"

#include <coreplugin/iuavgadget.h>

class PfdQmlGadgetWidget;

using namespace Core;

class PfdQmlGadget : public Core::IUAVGadget {
    Q_OBJECT

public:
    PfdQmlGadget(QString classId, QWidget *parent = 0);
    virtual ~PfdQmlGadget();

    QWidget *widget();

    void loadConfiguration(IUAVGadgetConfiguration *config);
    void saveState(QSettings &) const;
    void restoreState(QSettings &);

private:
    PfdQmlGadgetWidget *m_qmlGadgetWidget;
};

#endif // PFDQMLGADGET_H_
