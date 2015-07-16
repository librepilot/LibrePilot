/**
 ******************************************************************************
 *
 * @file       uavgadgetdecorator.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#include "uavgadgetdecorator.h"
#include "iuavgadgetconfiguration.h"
#include <QComboBox>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>

using namespace Core;

UAVGadgetDecorator::UAVGadgetDecorator(IUAVGadget *gadget, QList<IUAVGadgetConfiguration *> *configurations) :
    IUAVGadget(gadget->classId(), gadget->parent()),
    m_gadget(gadget),
    m_toolbar(new QComboBox),
    m_activeConfiguration(0),
    m_configurations(configurations)
{
    m_gadget->setParent(this);
    m_toolbar->setMinimumContentsLength(15);
    foreach(IUAVGadgetConfiguration * config, *m_configurations)
    m_toolbar->addItem(config->name());
    connect(m_toolbar, SIGNAL(activated(int)), this, SLOT(loadConfiguration(int)));
    updateToolbar();
}

UAVGadgetDecorator::~UAVGadgetDecorator()
{
    delete m_configurations;
    delete m_toolbar;
}

void UAVGadgetDecorator::loadConfiguration(int index)
{
    IUAVGadgetConfiguration *config = m_configurations->at(index);

    loadConfiguration(config);
}

void UAVGadgetDecorator::loadConfiguration(IUAVGadgetConfiguration *config)
{
    if (m_activeConfiguration == config) {
        return;
    }
    m_activeConfiguration = config;
    int index = m_toolbar->findText(config->name());
    if (m_toolbar->currentIndex() != index) {
        m_toolbar->setCurrentIndex(index);
    }
    m_gadget->loadConfiguration(config);
}

void UAVGadgetDecorator::configurationChanged(IUAVGadgetConfiguration *config)
{
    if (config == m_activeConfiguration) {
        // force a configuration reload
        m_activeConfiguration = NULL;
        loadConfiguration(config);
    }
}

void UAVGadgetDecorator::configurationAdded(IUAVGadgetConfiguration *config)
{
    if (config->classId() != classId()) {
        return;
    }
    m_configurations->append(config);
    m_toolbar->addItem(config->name());
    updateToolbar();
}

void UAVGadgetDecorator::configurationToBeDeleted(IUAVGadgetConfiguration *config)
{
    if (config->classId() != classId()) {
        return;
    }
    int index = m_configurations->indexOf(config);
    if (index >= 0) {
        m_toolbar->removeItem(index);
        m_configurations->removeAt(index);
    }
    updateToolbar();
}

void UAVGadgetDecorator::configurationNameChanged(IUAVGadgetConfiguration *config, QString oldName, QString newName)
{
    if (config->classId() != classId()) {
        return;
    }
    for (int i = 0; i < m_toolbar->count(); ++i) {
        if (m_toolbar->itemText(i) == oldName) {
            m_toolbar->setItemText(i, newName);
        }
    }
}

void UAVGadgetDecorator::updateToolbar()
{
    m_toolbar->setEnabled(m_toolbar->count() > 1);
}

void UAVGadgetDecorator::saveState(QSettings *qSettings)
{
    if (m_activeConfiguration) {
        qSettings->setValue("activeConfiguration", m_activeConfiguration->name());
    }
    // save gadget individual state
    qSettings->beginGroup("state");
    m_gadget->saveState(qSettings);
    qSettings->endGroup();
}

void UAVGadgetDecorator::restoreState(QSettings *qSettings)
{
    QString configName = qSettings->value("activeConfiguration").toString();

    foreach(IUAVGadgetConfiguration * config, *m_configurations) {
        if (config->name() == configName) {
            loadConfiguration(config);
            break;
        }
    }
    // restore gadget individual state
    qSettings->beginGroup("state");
    m_gadget->restoreState(qSettings);
    qSettings->endGroup();
}
