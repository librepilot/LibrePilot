/**
 ******************************************************************************
 *
 * @file       ipconnectionoptionspage.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin implements telemetry over TCP/IP and UDP/IP
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

#include "ipconnectionoptionspage.h"

#include "ui_ipconnectionoptionspage.h"

#include "ipconnectionconfiguration.h"

IPConnectionOptionsPage::IPConnectionOptionsPage(IPConnectionConfiguration *config, QObject *parent) :
    IOptionsPage(parent), m_page(0), m_config(config)
{}

IPConnectionOptionsPage::~IPConnectionOptionsPage()
{}

QWidget *IPConnectionOptionsPage::createPage(QWidget *parent)
{
    m_page = new Ui::IPconnectionOptionsPage();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    m_page->Port->setValue(m_config->port());
    m_page->HostName->setText(m_config->hostName());
    m_page->UseTCP->setChecked(m_config->useTCP() ? true : false);
    m_page->UseUDP->setChecked(m_config->useTCP() ? false : true);

    return w;
}

void IPConnectionOptionsPage::apply()
{
    m_config->setPort(m_page->Port->value());
    m_config->setHostName(m_page->HostName->text());
    m_config->setUseTCP(m_page->UseTCP->isChecked() ? 1 : 0);

    // FIXME this signal is too low level (and duplicated all over the place)
    // FIXME this signal will trigger (amongst other things) the saving of the configuration !
    emit availableDevChanged();
}

void IPConnectionOptionsPage::finish()
{
    delete m_page;
}
