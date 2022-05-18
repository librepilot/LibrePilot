/**
 ******************************************************************************
 *
 * @file       pfdqmlgadget.cpp
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

#include "pfdqmlgadget.h"
#include "pfdqmlgadgetwidget.h"
#include "pfdqmlgadgetconfiguration.h"

PfdQmlGadget::PfdQmlGadget(QString classId, QWidget *parent) :
    IUAVGadget(classId, parent)
{
    m_qmlGadgetWidget = new PfdQmlGadgetWidget(parent);
}

PfdQmlGadget::~PfdQmlGadget()
{
    delete m_qmlGadgetWidget;
}

QWidget *PfdQmlGadget::widget()
{
    return m_qmlGadgetWidget;
}

void PfdQmlGadget::loadConfiguration(IUAVGadgetConfiguration *config)
{
    PfdQmlGadgetConfiguration *m = qobject_cast<PfdQmlGadgetConfiguration *>(config);

    m_qmlGadgetWidget->loadConfiguration(m);
}

void PfdQmlGadget::saveState(QSettings &settings) const
{
    m_qmlGadgetWidget->saveState(settings);
}

void PfdQmlGadget::restoreState(QSettings &settings)
{
    m_qmlGadgetWidget->restoreState(settings);
}
