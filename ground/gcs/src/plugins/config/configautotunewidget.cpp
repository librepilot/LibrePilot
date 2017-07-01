/**
 ****************************************************************************************
 *
 * @file       configautotunewidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to configure autotune module
 ***************************************************************************************/
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

#include "configautotunewidget.h"

#include "ui_autotune.h"

#include <uavobjectutilmanager.h>

#include <systemidentsettings.h>
#include <systemidentstate.h>
#include "hwsettings.h"

#include <QMessageBox>
#include <QDebug>


ConfigAutoTuneWidget::ConfigAutoTuneWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_autotune = new Ui_AutoTuneWidget();
    m_autotune->setupUi(this);

    // must be done before auto binding !
    setWikiURL("AutoTune+Configuration");

    addAutoBindings();

    addUAVObject("HwSettings");

    disableMouseWheelEvents();

    systemIdentStateObj    = dynamic_cast<SystemIdentState *>(getObject("SystemIdentState"));
    Q_ASSERT(systemIdentStateObj);

    systemIdentSettingsObj = dynamic_cast<SystemIdentSettings *>(getObject("SystemIdentSettings"));
    Q_ASSERT(systemIdentSettingsObj);

    addWidget(m_autotune->AutotuneEnable);
}

ConfigAutoTuneWidget::~ConfigAutoTuneWidget()
{
    // Do nothing
}

/*
 * This overridden function refreshes widgets which have no direct relation
 * to any of UAVObjects. It saves their dirty state first because update comes
 * from UAVObjects, and then restores it.
 */
void ConfigAutoTuneWidget::refreshWidgetsValuesImpl(UAVObject *obj)
{
    if (obj == systemIdentStateObj) {
        m_autotune->stateComplete->setText((systemIdentStateObj->getComplete() == SystemIdentState::COMPLETE_TRUE) ? tr("True") : tr("False"));
    } else {
        HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

        m_autotune->AutotuneEnable->setChecked(hwSettings->getOptionalModules(HwSettings::OPTIONALMODULES_AUTOTUNE) == HwSettings::OPTIONALMODULES_ENABLED);
    }
}

/*
 * This overridden function updates UAVObjects which have no direct relation
 * to any of widgets.
 */
void ConfigAutoTuneWidget::updateObjectsFromWidgetsImpl()
{
    // Save state of the module enable checkbox first.
    // Do not use setData() member on whole object, if possible, since it triggers unnecessary UAVObect update.
    quint8 enableModule    = m_autotune->AutotuneEnable->isChecked() ?
                             HwSettings::OPTIONALMODULES_ENABLED : HwSettings::OPTIONALMODULES_DISABLED;
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());

    hwSettings->setOptionalModules(HwSettings::OPTIONALMODULES_AUTOTUNE, enableModule);
}
