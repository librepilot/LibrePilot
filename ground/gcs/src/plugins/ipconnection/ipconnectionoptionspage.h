/**
 ******************************************************************************
 *
 * @file       ipconnectionoptionspage.h
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

#ifndef IPCONNECTIONOPTIONSPAGE_H
#define IPCONNECTIONOPTIONSPAGE_H

#include "coreplugin/dialogs/ioptionspage.h"

class IPConnectionConfiguration;

namespace Ui {
class IPconnectionOptionsPage;
}

using namespace Core;

class IPConnectionOptionsPage : public IOptionsPage {
    Q_OBJECT
public:
    explicit IPConnectionOptionsPage(IPConnectionConfiguration *config, QObject *parent = 0);
    virtual ~IPConnectionOptionsPage();

    QString id() const
    {
        return QLatin1String("settings");
    }
    QString trName() const
    {
        return tr("settings");
    }
    QString category() const
    {
        return "Telemetry - IP Network";
    };
    QString trCategory() const
    {
        return "IP Network Telemetry";
    };

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

signals:
    void availableDevChanged();

private:
    Ui::IPconnectionOptionsPage *m_page;
    IPConnectionConfiguration *m_config;
};

#endif // IPCONNECTIONOPTIONSPAGE_H
