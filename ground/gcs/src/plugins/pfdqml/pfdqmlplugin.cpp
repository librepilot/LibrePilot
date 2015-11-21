/**
 ******************************************************************************
 *
 * @file       pfdqmlplugin.cpp
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

#include "pfdqml.h"
#include "pfdqmlplugin.h"
#include "pfdqmlgadgetfactory.h"
#include <extensionsystem/pluginmanager.h>

#ifdef USE_OSG
#include <osgearth/osgearth.h>
#endif

#include <QDebug>
#include <QStringList>

PfdQmlPlugin::PfdQmlPlugin()
{
    // Do nothing
}

PfdQmlPlugin::~PfdQmlPlugin()
{
    // Do nothing
}

bool PfdQmlPlugin::initialize(const QStringList & args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

#ifdef USE_OSG
    // TODO get rid of this call...
    // this is the only place that references osgearth
    // if this code goes away then the dependency to osgearth should be removed from pfdqml_dependencies.pri
    OsgEarth::registerQmlTypes();
#endif

    ModelSelectionMode::registerQMLTypes();
    TimeMode::registerQMLTypes();

    PfdQmlGadgetFactory *mf = new PfdQmlGadgetFactory(this);
    addAutoReleasedObject(mf);

    return true;
}

void PfdQmlPlugin::extensionsInitialized()
{
    // Do nothing
}

void PfdQmlPlugin::shutdown()
{
    // Do nothing
}
