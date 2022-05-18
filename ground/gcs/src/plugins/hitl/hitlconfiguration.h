/**
 ******************************************************************************
 *
 * @file       hitlconfiguration.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin
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

#ifndef HITLCONFIGURATION_H
#define HITLCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>
#include <QString>

#include <simulator.h>

using namespace Core;

class HITLConfiguration : public IUAVGadgetConfiguration {
    Q_OBJECT Q_PROPERTY(SimulatorSettings settings READ Settings WRITE setSimulatorSettings)

public:
    explicit HITLConfiguration(QString classId, QSettings &settings, QObject *parent = 0);
    explicit HITLConfiguration(const HITLConfiguration &obj);

    IUAVGadgetConfiguration *clone() const;
    void saveConfig(QSettings &settings) const;

    SimulatorSettings Settings() const
    {
        return simSettings;
    }

public slots:
    void setSimulatorSettings(const SimulatorSettings & params)
    {
        simSettings = params;
    }


private:
    SimulatorSettings simSettings;
};

#endif // HITLCONFIGURATION_H
