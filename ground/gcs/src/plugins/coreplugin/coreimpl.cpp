/**
 ******************************************************************************
 *
 * @file       coreimpl.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
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

#include "coreimpl.h"

#include "utils/pathutils.h"

#include <QDir>
#include <QSettings>

namespace Core {
namespace Internal {
// The Core Singleton
static CoreImpl *m_instance = 0;
} // namespace Internal
} // namespace Core


using namespace Core;
using namespace Core::Internal;


ICore *ICore::instance()
{
    return m_instance;
}

CoreImpl::CoreImpl(MainWindow *mainwindow)
{
    m_instance   = this;
    m_mainwindow = mainwindow;
}

bool CoreImpl::showOptionsDialog(const QString &group, const QString &page, QWidget *parent)
{
    return m_mainwindow->showOptionsDialog(group, page, parent);
}

bool CoreImpl::showWarningWithOptions(const QString &title, const QString &text,
                                      const QString &details,
                                      const QString &settingsCategory,
                                      const QString &settingsId,
                                      QWidget *parent)
{
    return m_mainwindow->showWarningWithOptions(title, text,
                                                details, settingsCategory,
                                                settingsId, parent);
}

ActionManager *CoreImpl::actionManager() const
{
    return m_mainwindow->actionManager();
}

UniqueIDManager *CoreImpl::uniqueIDManager() const
{
    return m_mainwindow->uniqueIDManager();
}

MessageManager *CoreImpl::messageManager() const
{
    return m_mainwindow->messageManager();
}

ConnectionManager *CoreImpl::connectionManager() const
{
    return m_mainwindow->connectionManager();
}

UAVGadgetInstanceManager *CoreImpl::uavGadgetInstanceManager() const
{
    return m_mainwindow->uavGadgetInstanceManager();
}

VariableManager *CoreImpl::variableManager() const
{
    return m_mainwindow->variableManager();
}

ThreadManager *CoreImpl::threadManager() const
{
    return m_mainwindow->threadManager();
}

ModeManager *CoreImpl::modeManager() const
{
    return m_mainwindow->modeManager();
}

MimeDatabase *CoreImpl::mimeDatabase() const
{
    return m_mainwindow->mimeDatabase();
}

SettingsDatabase *CoreImpl::settingsDatabase() const
{
    return m_mainwindow->settingsDatabase();
}

QString CoreImpl::resourcePath() const
{
    return QDir::cleanPath(Utils::GetDataPath());
}

IContext *CoreImpl::currentContextObject() const
{
    return m_mainwindow->currentContextObject();
}

QMainWindow *CoreImpl::mainWindow() const
{
    return m_mainwindow;
}

// adds and removes additional active contexts, this context is appended to the
// currently active contexts. call updateContext after changing
void CoreImpl::addAdditionalContext(int context)
{
    m_mainwindow->addAdditionalContext(context);
}

void CoreImpl::removeAdditionalContext(int context)
{
    m_mainwindow->removeAdditionalContext(context);
}

bool CoreImpl::hasContext(int context) const
{
    return m_mainwindow->hasContext(context);
}

void CoreImpl::addContextObject(IContext *context)
{
    m_mainwindow->addContextObject(context);
}

void CoreImpl::removeContextObject(IContext *context)
{
    m_mainwindow->removeContextObject(context);
}

void CoreImpl::updateContext()
{
    return m_mainwindow->updateContext();
}

void CoreImpl::openFiles(const QStringList &arguments)
{
    Q_UNUSED(arguments)
    // m_mainwindow->openFiles(arguments);
}

void CoreImpl::readSettings(IConfigurablePlugin *plugin)
{
    QSettings settings;

    readSettings(plugin, settings);
}

void CoreImpl::saveSettings(IConfigurablePlugin *plugin) const
{
    QSettings settings;

    saveSettings(plugin, settings);
}

void CoreImpl::readMainSettings(QSettings &settings, bool workspaceDiffOnly)
{
    m_mainwindow->readSettings(settings, workspaceDiffOnly);
}

void CoreImpl::saveMainSettings(QSettings &settings) const
{
    m_mainwindow->saveSettings(settings);
}

void CoreImpl::readSettings(IConfigurablePlugin *plugin, QSettings &settings)
{
    m_mainwindow->readSettings(plugin, settings);
}

void CoreImpl::saveSettings(IConfigurablePlugin *plugin, QSettings &settings) const
{
    m_mainwindow->saveSettings(plugin, settings);
}

void CoreImpl::deleteSettings()
{
    m_mainwindow->deleteSettings();
}
