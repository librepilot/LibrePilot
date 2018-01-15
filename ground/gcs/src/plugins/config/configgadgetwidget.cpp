/**
 ******************************************************************************
 *
 * @file       configgadgetwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015-2017.
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
#include "configautotunewidget.h"
#include "configoplinkwidget.h"
#include "configrevowidget.h"
#include "configrevonanohwwidget.h"
#include "configsparky2hwwidget.h"
#include "configspracingf3evohwwidget.h"
#include "configtinyfishhwwidget.h"
#include "configpikoblxhwwidget.h"
#include "defaultconfigwidget.h"

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutilmanager.h>
#include <uavtalk/telemetrymanager.h>
#include <uavtalk/oplinkmanager.h>

#include "utils/mytabbedstackwidget.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#define OPLINK_TIMEOUT 2000

ConfigGadgetWidget::ConfigGadgetWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    stackWidget = new MyTabbedStackWidget(this, true, true);
    stackWidget->setIconSize(64);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackWidget);
    setLayout(layout);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    QWidget *widget;
    QIcon *icon;

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/hardware_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/hardware_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultConfigWidget(this, tr("Hardware"));
    stackWidget->insertTab(ConfigGadgetWidget::Hardware, widget, *icon, QString("Hardware"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/vehicle_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/vehicle_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigVehicleTypeWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::Aircraft, widget, *icon, QString("Vehicle"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/input_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/input_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigInputWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::Input, widget, *icon, QString("Input"));
    QWidget *inputWidget = widget;

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/output_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/output_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigOutputWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::Output, widget, *icon, QString("Output"));
    QWidget *outputWidget = widget;

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/ins_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/ins_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultConfigWidget(this, tr("Attitude"));
    stackWidget->insertTab(ConfigGadgetWidget::Sensors, widget, *icon, QString("Attitude"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/stabilization_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/stabilization_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigStabilizationWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::Stabilization, widget, *icon, QString("Stabilization"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/camstab_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/camstab_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigCameraStabilizationWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::CameraStabilization, widget, *icon, QString("Gimbal"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/txpid_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/txpid_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new ConfigTxPIDWidget(this);
    static_cast<ConfigTaskWidget *>(widget)->bind();
    stackWidget->insertTab(ConfigGadgetWidget::TxPid, widget, *icon, QString("TxPID"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/autotune_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/autotune_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultConfigWidget(this, tr("AutoTune Configuration"));
    stackWidget->insertTab(ConfigGadgetWidget::AutoTune, widget, *icon, QString("AutoTune"));

    icon   = new QIcon();
    icon->addFile(":/configgadget/images/pipx-normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/pipx-selected.png", QSize(), QIcon::Selected, QIcon::Off);
    widget = new DefaultConfigWidget(this, tr("OPLink Configuration"));
    stackWidget->insertTab(ConfigGadgetWidget::OPLink, widget, *icon, QString("OPLink"));

    stackWidget->setCurrentIndex(ConfigGadgetWidget::Hardware);

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

    // Connect output tab and input tab
    // Input tab do not start calibration if Output tab is not safe
    // Output tab uses the signal from Input tab and freeze all output UI while calibrating inputs
    connect(outputWidget, SIGNAL(outputConfigSafeChanged(bool)), inputWidget, SLOT(setOutputConfigSafe(bool)));
    connect(inputWidget, SIGNAL(inputCalibrationStateChanged(bool)), outputWidget, SLOT(setInputCalibrationState(bool)));

    help = 0;
    connect(stackWidget, SIGNAL(currentAboutToShow(int, bool *)), this, SLOT(tabAboutToChange(int, bool *)));
}

ConfigGadgetWidget::~ConfigGadgetWidget()
{
    delete stackWidget;
}

void ConfigGadgetWidget::startInputWizard()
{
    stackWidget->setCurrentIndex(ConfigGadgetWidget::Input);
    ConfigInputWidget *inputWidget = dynamic_cast<ConfigInputWidget *>(stackWidget->getWidget(ConfigGadgetWidget::Input));
    Q_ASSERT(inputWidget);
    inputWidget->startInputWizard();
}

void ConfigGadgetWidget::saveState(QSettings &settings) const
{
    settings.setValue("currentIndex", stackWidget->currentIndex());
}

void ConfigGadgetWidget::restoreState(QSettings &settings)
{
    int index = settings.value("currentIndex", 0).toInt();

    stackWidget->setCurrentIndex(index);
}

void ConfigGadgetWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigGadgetWidget::onAutopilotConnect()
{
    // qDebug() << "ConfigGadgetWidget::onAutopilotConnect";

    // Check what Board type we are talking to, and if necessary, remove/add tabs in the config gadget
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();

    if (utilMngr) {
        int board = utilMngr->getBoardModel();
        if ((board & 0xff00) == 0x0400) {
            // CopterControl family

            ConfigTaskWidget *widget;

            if ((board & 0x00ff) == 0x03) {
                widget = new ConfigRevoWidget(this);
            } else {
                widget = new ConfigCCAttitudeWidget(this);
            }

            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Sensors, widget);

            widget = new ConfigCCHWWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Hardware, widget);
        } else if ((board & 0xff00) == 0x0900) {
            // Revolution family
            ConfigTaskWidget *widget;
            widget = new ConfigRevoWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Sensors, widget);
            widget = new ConfigAutoTuneWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::AutoTune, widget);
            if (board == 0x0903 || board == 0x0904) {
                widget = new ConfigRevoHWWidget(this);
            } else if (board == 0x0905) {
                widget = new ConfigRevoNanoHWWidget(this);
            }
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Hardware, widget);
        } else if ((board & 0xff00) == 0x9200) {
            // Sparky2
            ConfigTaskWidget *widget;
            widget = new ConfigRevoWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Sensors, widget);
            widget = new ConfigAutoTuneWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::AutoTune, widget);
            widget = new ConfigSparky2HWWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Hardware, widget);
        } else if ((board & 0xff00) == 0x1000) { // F3 boards
            ConfigTaskWidget *widget;
            widget = new ConfigRevoWidget(this);
            widget->bind();
            stackWidget->replaceTab(ConfigGadgetWidget::Sensors, widget);

            widget = 0;

            switch (board) {
            case 0x1001:
                // widget = new ConfigSPRacingF3HWWidget(this);
                break;
            case 0x1002:
            case 0x1003:
                widget = new ConfigSPRacingF3EVOHWWidget(this);
                break;
            case 0x1005:
                widget = new ConfigPikoBLXHWWidget(this);
                break;
            case 0x1006:
                widget = new ConfigTinyFISHHWWidget(this);
                break;
            }

            if (widget) {
                widget->bind();
                stackWidget->replaceTab(ConfigGadgetWidget::Hardware, widget);
            }
        } else {
            // Unknown board
            qWarning() << "Unknown board " << board;
        }
    }
}

void ConfigGadgetWidget::onAutopilotDisconnect()
{
    // qDebug() << "ConfigGadgetWidget::onAutopilotDiconnect";
    QWidget *widget;

    widget = new DefaultConfigWidget(this, tr("Attitude"));
    stackWidget->replaceTab(ConfigGadgetWidget::Sensors, widget);

    widget = new DefaultConfigWidget(this, tr("Hardware"));
    stackWidget->replaceTab(ConfigGadgetWidget::Hardware, widget);

    widget = new DefaultConfigWidget(this, tr("AutoTune"));
    stackWidget->replaceTab(ConfigGadgetWidget::AutoTune, widget);
}

void ConfigGadgetWidget::onOPLinkConnect()
{
    // qDebug() << "ConfigGadgetWidget::onOPLinkConnect";

    ConfigTaskWidget *widget = new ConfigOPLinkWidget(this);

    widget->bind();
    stackWidget->replaceTab(ConfigGadgetWidget::OPLink, widget);
}

void ConfigGadgetWidget::onOPLinkDisconnect()
{
    // qDebug() << "ConfigGadgetWidget::onOPLinkDisconnect";

    QWidget *widget = new DefaultConfigWidget(this, tr("OPLink Configuration"));

    stackWidget->replaceTab(ConfigGadgetWidget::OPLink, widget);
}

void ConfigGadgetWidget::tabAboutToChange(int index, bool *proceed)
{
    Q_UNUSED(index);
    *proceed = true;
    ConfigTaskWidget *wid = qobject_cast<ConfigTaskWidget *>(stackWidget->currentWidget());
    if (!wid) {
        return;
    }
    if (wid->isDirty()) {
        int ans = QMessageBox::warning(this, tr("Unsaved changes"), tr("The tab you are leaving has unsaved changes, "
                                                                       "if you proceed they will be lost.\n"
                                                                       "Do you still want to proceed?"), QMessageBox::Yes, QMessageBox::No);
        if (ans == QMessageBox::No) {
            *proceed = false;
        } else {
            wid->clearDirty();
        }
    }
}
