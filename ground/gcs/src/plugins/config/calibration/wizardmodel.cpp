/**
 ******************************************************************************
 *
 * @file       wizardmodel.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 *
 * @brief      A fsm state machine model used in wizard like UIs
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
#include "wizardmodel.h"
#include "QDebug"
WizardModel::WizardModel(QObject *parent) :
    QStateMachine(parent)
{}

WizardState *WizardModel::currentState()
{
    foreach(QAbstractState * value, this->configuration()) {
        WizardState *state = static_cast<WizardState *>(value);

        if (state->children().count() <= 1) {
            return state;
        }
    }
    return NULL;
}
