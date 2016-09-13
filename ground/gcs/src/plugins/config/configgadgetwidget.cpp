/**
 ******************************************************************************
 *
 * @file       configgadgetwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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

#include "configgadgetwidget.h"

#include "configvehicletypewidget.h"
#include "configccattitudewidget.h"
#include "configinputwidget.h"
#include "configoutputwidget.h"
#include "configstabilizationwidget.h"
#include "configcamerastabilizationwidget.h"
#include "configtxpidwidget.h"
#include "configrevohwwidget.h"
#include "config_cc_hw_widget.h"
#include "configoplinkwidget.h"
#include "configrevowidget.h"
#include "configrevonanohwwidget.h"
#include "configsparky2hwwidget.h"
#include "defaultattitudewidget.h"
#include "defaulthwsettingswidget.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutilmanager.h>
#include <uavtalk/telemetrymanager.h>
#include <uavtalk/oplinkmanager.h>

#include "utils/mytabbedstackwidget.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

ConfigGadgetWidget::ConfigGadgetWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    stackWidget = new MyTabbedStackWidget(this, true, true);
    stackWidget->setIconSize(64);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackWidget);
    setLayout(layout);


    QWidget *widget;
    QIcon *icon;

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/hardware_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/hardware_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultHwSettingsWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::hardware, widget, *icon, QString("Hardware"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/vehicle_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/vehicle_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigVehicleTypeWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::aircraft, widget, *icon, QString("Vehicle"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/input_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/input_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigInputWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::input, widget, *icon, QString("Input"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/output_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/output_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigOutputWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::output, widget, *icon, QString("Output"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/ins_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/ins_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultAttitudeWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::sensors, widget, *icon, QString("Attitude"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/stabilization_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/stabilization_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigStabilizationWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::stabilization, widget, *icon, QString("Stabilization"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/camstab_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/camstab_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigCameraStabilizationWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::camerastabilization, widget, *icon, QString("Gimbal"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/txpid_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/txpid_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigTxPIDWidget(this);
    stackWidget->insertTab(ConfigGadgetWidget::txpid, widget, *icon, QString("TxPID"));

    stackWidget->setCurrentIndex(ConfigGadgetWidget::hardware);

    // connect to autopilot connection events
    TelemetryManager *tm = pm->getObject<TelemetryManager>();
    connect(tm, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(tm, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));
    // check if we are already connected
    if (tm->isConnected()) {
        onAutopilotConnect();
    }

    // connect to oplink manager
    OPLinkManager *om = pm->getObject<OPLinkManager>();
    connect(om, SIGNAL(connected()), this, SLOT(onOPLinkConnect()));
    connect(om, SIGNAL(disconnected()), this, SLOT(onOPLinkDisconnect()));
    // check if we are already connected
    if (om->isConnected()) {
        onOPLinkConnect();
    }

    help = 0;
    connect(stackWidget, SIGNAL(currentAboutToShow(int, bool *)), this, SLOT(tabAboutToChange(int, bool *)));
}

ConfigGadgetWidget::~ConfigGadgetWidget()
{
    delete stackWidget;
}

void ConfigGadgetWidget::startInputWizard()
{
    stackWidget->setCurrentIndex(ConfigGadgetWidget::input);
    ConfigInputWidget *inputWidget = dynamic_cast<ConfigInputWidget *>(stackWidget->getWidget(ConfigGadgetWidget::input));
    Q_ASSERT(inputWidget);
    inputWidget->startInputWizard();
}

void ConfigGadgetWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigGadgetWidget::onAutopilotConnect()
{
    qDebug() << "ConfigGadgetWidget::onAutopilotConnect";

    // Check what Board type we are talking to, and if necessary, remove/add tabs in the config gadget
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();
    if (utilMngr) {
        int board = utilMngr->getBoardModel();
        if ((board & 0xff00) == 0x0400) {
            // CopterControl family
            ConfigTaskWidget *widget;
            widget = new ConfigCCAttitudeWidget(this);
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::sensors, widget);
            widget = new ConfigCCHWWidget(this);
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::hardware, widget);
        } else if ((board & 0xff00) == 0x0900) {
            // Revolution family
            ConfigTaskWidget *widget;
            widget = new ConfigRevoWidget(this);
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::sensors, widget);
            if (board == 0x0903 || board == 0x0904) {
                widget = new ConfigRevoHWWidget(this);
            } else if (board == 0x0905) {
                widget = new ConfigRevoNanoHWWidget(this);
            }
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::hardware, widget);
        } else if ((board & 0xff00) == 0x9200) {
            // Sparky2
            ConfigTaskWidget *widget;
            widget = new ConfigRevoWidget(this);
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::sensors, widget);
            widget = new ConfigSparky2HWWidget(this);
            widget->forceConnectedState();
            stackWidget->replaceTab(ConfigGadgetWidget::hardware, widget);
        } else {
            // Unknown board
            qDebug() << "Unknown board " << board;
        }
    }
}

void ConfigGadgetWidget::onAutopilotDisconnect()
{
    qDebug() << "ConfigGadgetWidget::onAutopilotDiconnect";
    QWidget *widget;

    widget = new DefaultAttitudeWidget(this);
    stackWidget->replaceTab(ConfigGadgetWidget::sensors, widget);

    widget = new DefaultHwSettingsWidget(this);
    stackWidget->replaceTab(ConfigGadgetWidget::hardware, widget);
}

void ConfigGadgetWidget::onOPLinkConnect()
{
    qDebug() << "ConfigGadgetWidget::onOPLinkConnect";

    ConfigTaskWidget *widget;
    QIcon *icon;

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/pipx-normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/pipx-selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigOPLinkWidget(this);
    widget->forceConnectedState();
    stackWidget->insertTab(ConfigGadgetWidget::oplink, widget, *icon, QString("OPLink"));
}

void ConfigGadgetWidget::onOPLinkDisconnect()
{
    qDebug() << "ConfigGadgetWidget::onOPLinkDisconnect";

    if (stackWidget->currentIndex() == ConfigGadgetWidget::oplink) {
        stackWidget->setCurrentIndex(0);
    }
    stackWidget->removeTab(ConfigGadgetWidget::oplink);
}

void ConfigGadgetWidget::tabAboutToChange(int i, bool *proceed)
{
    Q_UNUSED(i);
    *proceed = true;
    ConfigTaskWidget *wid = qobject_cast<ConfigTaskWidget *>(stackWidget->currentWidget());
    if (!wid) {
        return;
    }
    if (wid->isDirty()) {
        int ans = QMessageBox::warning(this, tr("Unsaved changes"), tr("The tab you are leaving has unsaved changes,"
                                                                       "if you proceed they will be lost.\n"
                                                                       "Do you still want to proceed?"), QMessageBox::Yes, QMessageBox::No);
        if (ans == QMessageBox::No) {
            *proceed = false;
        } else {
            wid->setDirty(false);
        }
    }
}
