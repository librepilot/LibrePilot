/**
 ******************************************************************************
 *
 * @file       mainwindow.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#include "mainwindow.h"
#include "actioncontainer.h"
#include "actionmanager_p.h"
#include "basemode.h"
#include "connectionmanager.h"
#include "coreimpl.h"
#include "coreconstants.h"
#include "utils/mytabwidget.h"
#include "generalsettings.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "mimedatabase.h"
#include "outputpane.h"
#include "plugindialog.h"
#include "shortcutsettings.h"
#include "uavgadgetmanager.h"
#include "uavgadgetinstancemanager.h"
#include "workspacesettings.h"

#include "aboutdialog.h"
#include "baseview.h"
#include "ioutputpane.h"
#include "icorelistener.h"
#include "iconfigurableplugin.h"

#include <qstylefactory.h>

#include "settingsdialog.h"
#include "threadmanager.h"
#include "uniqueidmanager.h"
#include "variablemanager.h"

#include <coreplugin/settingsdatabase.h>
#include <extensionsystem/pluginmanager.h>
#include "dialogs/iwizard.h"
#include <utils/pathchooser.h>
#include <utils/pathutils.h>
#include <utils/stylehelper.h>
#include "version_info/version_info.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QtPlugin>
#include <QtCore/QUrl>

#include <QtWidgets/QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QPixmap>
#include <QShortcut>
#include <QStatusBar>
#include <QWizard>
#include <QToolButton>
#include <QMessageBox>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QDir>
#include <QMimeData>

#ifdef Q_OS_WIN
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif

using namespace Core;
using namespace Core::Internal;

static const char *uriListMimeFormatC = "text/uri-list";

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    EventFilteringMainWindow(),
    m_coreImpl(new CoreImpl(this)),
    m_uniqueIDManager(new UniqueIDManager()),
    m_globalContext(QList<int>() << Constants::C_GLOBAL_ID),
    m_additionalContexts(m_globalContext),
    m_dontSaveSettings(false),
    m_actionManager(new ActionManagerPrivate(this)),
    m_variableManager(new VariableManager(this)),
    m_threadManager(new ThreadManager(this)),
    m_modeManager(0),
    m_connectionManager(0),
    m_mimeDatabase(new MimeDatabase),
    m_aboutDialog(0),
    m_activeContext(0),
    m_generalSettings(new GeneralSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_workspaceSettings(new WorkspaceSettings),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
#ifdef Q_WS_MAC
    m_minimizeAction(0),
    m_zoomAction(0),
#endif
    m_toggleFullScreenAction(0)
{
    setWindowTitle(QLatin1String(GCS_BIG_NAME) + " " + VersionInfo::label());
#ifndef Q_WS_MAC
    qApp->setWindowIcon(QIcon(":/core/images/librepilot_logo_128.png"));
#endif
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QSettings settings;
    m_settingsDatabase = new SettingsDatabase(QFileInfo(settings.fileName()).path(),
                                              QFileInfo(settings.fileName()).baseName(),
                                              this);

    setDockNestingEnabled(true);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    registerDefaultContainers();
    registerDefaultActions();

    m_modeStack = new MyTabWidget(this);
    m_modeStack->setIconSize(QSize(24, 24));
    m_modeStack->setTabPosition(QTabWidget::South);
    m_modeStack->setMovable(false);
    m_modeStack->setMinimumWidth(512);
    m_modeStack->setElideMode(Qt::ElideRight);
    m_modeManager = new ModeManager(this, m_modeStack);

    m_connectionManager = new ConnectionManager(this);
    m_modeStack->setCornerWidget(m_connectionManager, Qt::TopRightCorner);

    m_messageManager    = new MessageManager;
    setCentralWidget(m_modeStack);

    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)),
            this, SLOT(updateFocusWidget(QWidget *, QWidget *)));
    connect(m_workspaceSettings, SIGNAL(tabBarSettingsApplied(QTabWidget::TabPosition, bool)),
            this, SLOT(applyTabBarSettings(QTabWidget::TabPosition, bool)));
    connect(m_modeManager, SIGNAL(newModeOrder(QVector<IMode *>)), m_workspaceSettings, SLOT(newModeOrder(QVector<IMode *>)));
    statusBar()->setProperty("p_styled", true);
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    if (m_connectionManager) {
        // Pip
        m_connectionManager->disconnectDevice();
        m_connectionManager->suspendPolling();
    }

    hide();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    if (m_uavGadgetManagers.count() > 0) {
        foreach(UAVGadgetManager * mode, m_uavGadgetManagers) {
            pm->removeObject(mode);
            delete mode;
        }
    }

    pm->removeObject(m_shortcutSettings);
    pm->removeObject(m_generalSettings);
    pm->removeObject(m_workspaceSettings);
    delete m_messageManager;
    m_messageManager    = 0;
    delete m_shortcutSettings;
    m_shortcutSettings  = 0;
    delete m_generalSettings;
    m_generalSettings   = 0;
    delete m_workspaceSettings;
    m_workspaceSettings = 0;
    delete m_uniqueIDManager;
    m_uniqueIDManager   = 0;

    pm->removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl     = 0;

    delete m_modeManager;
    m_modeManager  = 0;
    delete m_mimeDatabase;
    m_mimeDatabase = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    ExtensionSystem::PluginManager * pm = ExtensionSystem::PluginManager::instance();
    pm->addObject(m_coreImpl);
    m_modeManager->init();
    m_connectionManager->init();

    pm->addObject(m_generalSettings);
    pm->addObject(m_shortcutSettings);
    pm->addObject(m_workspaceSettings);

    return true;
}

void MainWindow::modeChanged(Core::IMode * /*mode*/)
{}

void MainWindow::extensionsInitialized()
{
    QSettings settings;

    settings.beginGroup("General");

    m_config_description = settings.value("Description", "none").toString();
    m_config_details     = settings.value("Details", "none").toString();
    m_config_stylesheet  = settings.value("StyleSheet", "none").toString();

    qDebug() << "Configured style sheet:" << m_config_stylesheet;
    if (m_config_stylesheet == "wide") {
        // OP-869 just in case some user configuration still references the now obsolete "wide" style sheet
        m_config_stylesheet = "default";
    }

    // Load common style sheet
    QString style    = loadStyleSheet(m_config_stylesheet + ".qss");

    // Load and concatenate platform specific style sheet
    QString fileName = m_config_stylesheet;
#ifdef Q_OS_MAC
    fileName += "_macos.qss";
#elif defined(Q_OS_LINUX)
    fileName += "_linux.qss";
#else
    fileName += "_windows.qss";
#endif
    style    += loadStyleSheet(fileName);

    // We'll use qApp macro to get the QApplication pointer
    // and set the style sheet application wide.
    // qDebug() << "Setting application style sheet to:" << style;
    qApp->setStyleSheet(style);

    settings.endGroup();

    m_uavGadgetInstanceManager = new UAVGadgetInstanceManager(this);

    m_uavGadgetInstanceManager->readSettings(settings);

    m_messageManager->init();
    readSettings(settings);

    updateContext();

    emit m_coreImpl->coreAboutToOpen();
    show();
    emit m_coreImpl->coreOpened();
}

QString MainWindow::loadStyleSheet(QString fileName)
{
    QString style;
    // ...to open the file
    QFile file(Utils::GetDataPath() + QString("stylesheets/") + fileName);

    qDebug() << "Loading style sheet file" << file.fileName();
    if (file.open(QFile::ReadOnly)) {
        // QTextStream...
        QTextStream textStream(&file);
        // ...read file to a string.
        style = textStream.readAll();
        file.close();
    } else {
        qDebug() << "Failed to open style sheet file" << file.fileName();
    }
    return style;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_generalSettings->saveSettingsOnExit()) {
        m_dontSaveSettings = true;
    }
    if (!m_dontSaveSettings) {
        emit m_coreImpl->saveSettingsRequested();
    }

    const QList<ICoreListener *> listeners = ExtensionSystem::PluginManager::instance()->getObjects<ICoreListener>();
    foreach(ICoreListener * listener, listeners) {
        if (!listener->coreAboutToClose()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    if (!m_dontSaveSettings) {
        QSettings settings;
        saveSettings(settings);
        m_uavGadgetInstanceManager->saveSettings(settings);
    }

    qApp->closeAllWindows();

    event->accept();
}

// Check for desktop file manager file drop events
static bool isDesktopFileManagerDrop(const QMimeData *d, QStringList *files = 0)
{
    if (files) {
        files->clear();
    }
    // Extract dropped files from Mime data.
    if (!d->hasFormat(QLatin1String(uriListMimeFormatC))) {
        return false;
    }
    const QList<QUrl> urls = d->urls();
    if (urls.empty()) {
        return false;
    }
    // Try to find local files
    bool hasFiles = false;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        const QString fileName = it->toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files) {
                files->push_back(fileName);
            } else {
                break; // No result list, sufficient for checking
            }
        }
    }
    return hasFiles;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (isDesktopFileManagerDrop(event->mimeData())) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;

    if (isDesktopFileManagerDrop(event->mimeData(), &files)) {
        event->accept();
        // openFiles(files);
    } else {
        event->ignore();
    }
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext;
}

QStatusBar *MainWindow::statusBar() const
{
    return new QStatusBar(); // m_modeStack->statusBar();
}

void MainWindow::registerDefaultContainers()
{
    ActionManagerPrivate *am = m_actionManager;

    ActionContainer *menubar = am->createMenuBar(Constants::MENU_BAR);

#ifndef Q_WS_MAC // System menu bar on Mac
    setMenuBar(menubar->menuBar());
#endif
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = am->createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE);
    filemenu->menu()->setTitle(tr("&File"));
    filemenu->appendGroup(Constants::G_FILE_NEW);
    filemenu->appendGroup(Constants::G_FILE_OPEN);
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowRecentFiles()));


    // Edit Menu
    ActionContainer *medit = am->createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu
    ActionContainer *ac = am->createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = am->createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_HIDE_TOOLBAR);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = am->createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_ABOUT);
}

static Command *createSeparator(ActionManager *am, QObject *parent, const QString &name, const QList<int> &context)
{
    QAction *tmpaction = new QAction(parent);

    tmpaction->setSeparator(true);
    Command *cmd = am->registerAction(tmpaction, name, context);
    return cmd;
}

void MainWindow::registerDefaultActions()
{
    ActionManagerPrivate *am = m_actionManager;
    ActionContainer *mfile   = am->actionContainer(Constants::M_FILE);
    ActionContainer *medit   = am->actionContainer(Constants::M_EDIT);
    ActionContainer *mtools  = am->actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp   = am->actionContainer(Constants::M_HELP);

    // File menu separators
    Command *cmd = createSeparator(am, this, "QtCreator.File.Sep.Save", m_globalContext);

    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    cmd = createSeparator(am, this, "QtCreator.File.Sep.Close", m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);

    cmd = createSeparator(am, this, "QtCreator.File.Sep.Other", m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OTHER);

    // Edit menu separators
    cmd = createSeparator(am, this, "QtCreator.Edit.Sep.CopyPaste", m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    cmd = createSeparator(am, this, "QtCreator.Edit.Sep.SelectAll", m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);

    cmd = createSeparator(am, this, "QtCreator.Edit.Sep.Find", m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_FIND);

    cmd = createSeparator(am, this, "QtCreator.Edit.Sep.Advanced", m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_ADVANCED);

    // Tools menu separators
    cmd = createSeparator(am, this, "QtCreator.Tools.Sep.Options", m_globalContext);
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);

    // Help menu separators

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QShortcut(this);
    cmd = am->registerShortcut(m_focusToEditor, Constants::S_RETURNTOEDITOR, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, SIGNAL(activated()), this, SLOT(setFocusToEditor()));

    // New File Action

    /*
       m_newAction = new QAction(QIcon(Constants::ICON_NEWFILE), tr("&New File or Project..."), this);
       cmd = am->registerAction(m_newAction, Constants::NEW, m_globalContext);
       cmd->setDefaultKeySequence(QKeySequence::New);
       mfile->addAction(cmd, Constants::G_FILE_NEW);
       connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile()));
     */

    // Open Action
/*
    m_openAction = new QAction(QIcon(Constants::ICON_OPENFILE), tr("&Open File or Project..."), this);
    cmd = am->registerAction(m_openAction, Constants::OPEN, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));
 */

    // Open With Action
/*
    m_openWithAction = new QAction(tr("&Open File With..."), this);
    cmd = am->registerAction(m_openWithAction, Constants::OPEN_WITH, m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, SIGNAL(triggered()), this, SLOT(openFileWith()));
 */

    // File->Recent Files Menu
/*
    ActionContainer *ac = am->createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent Files"));
 */
/*
    // Save Action
    QAction *tmpaction = new QAction(QIcon(Constants::ICON_SAVEFILE), tr("&Save"), this);
    cmd = am->registerAction(tmpaction, Constants::SAVE, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    tmpaction = new QAction(tr("Save &As..."), this);
    cmd = am->registerAction(tmpaction, Constants::SAVEAS, m_globalContext);
   #ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
   #endif
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("Save &As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
 */

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save &GCS Default Settings"), this);
    cmd = am->registerAction(m_saveAllAction, Constants::SAVEALL, m_globalContext);
#ifndef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
#endif
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, SIGNAL(triggered()), this, SLOT(saveAll()));

    // Exit Action
    m_exitAction = new QAction(QIcon(Constants::ICON_EXIT), tr("E&xit"), this);
    cmd = am->registerAction(m_exitAction, Constants::EXIT, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(exit()));

    // Undo Action
    QAction *tmpaction = new QAction(QIcon(Constants::ICON_UNDO), tr("&Undo"), this);
    cmd = am->registerAction(tmpaction, Constants::UNDO, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    tmpaction = new QAction(QIcon(Constants::ICON_REDO), tr("&Redo"), this);
    cmd = am->registerAction(tmpaction, Constants::REDO, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    tmpaction = new QAction(QIcon(Constants::ICON_CUT), tr("Cu&t"), this);
    cmd = am->registerAction(tmpaction, Constants::CUT, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    tmpaction = new QAction(QIcon(Constants::ICON_COPY), tr("&Copy"), this);
    cmd = am->registerAction(tmpaction, Constants::COPY, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    tmpaction = new QAction(QIcon(Constants::ICON_PASTE), tr("&Paste"), this);
    cmd = am->registerAction(tmpaction, Constants::PASTE, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    tmpaction = new QAction(tr("&Select All"), this);
    cmd = am->registerAction(tmpaction, Constants::SELECTALL, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Options Action
    m_optionsAction = new QAction(QIcon(Constants::ICON_OPTIONS), tr("&Options..."), this);
    cmd = am->registerAction(m_optionsAction, Constants::OPTIONS, m_globalContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+,"));
    cmd->action()->setMenuRole(QAction::PreferencesRole);
#endif
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);
    connect(m_optionsAction, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));

#ifdef Q_WS_MAC
    // Minimize Action
    m_minimizeAction = new QAction(tr("Minimize"), this);
    cmd = am->registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+M"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

    // Zoom Action
    m_zoomAction = new QAction(tr("Zoom"), this);
    cmd = am->registerAction(m_zoomAction, Constants::ZOOM_WINDOW, m_globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_zoomAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    // Window separator
    cmd = createSeparator(am, this, "QtCreator.Window.Sep.Size", m_globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
#endif

#ifndef Q_WS_MAC
    // Full Screen Action
    m_toggleFullScreenAction = new QAction(tr("Full Screen"), this);
    m_toggleFullScreenAction->setCheckable(true);
    cmd = am->registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+Shift+F11"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_toggleFullScreenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
#endif

    /*
     * UavGadgetManager Actions
     */
    const QList<int> uavGadgetManagerContext =
        QList<int>() << CoreImpl::instance()->uniqueIDManager()->uniqueIdentifier(Constants::C_UAVGADGETMANAGER);
    // Window menu separators
    QAction *tmpaction1 = new QAction(this);
    tmpaction1->setSeparator(true);
    cmd = am->registerAction(tmpaction1, "OpenPilot.Window.Sep.Split", uavGadgetManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_HIDE_TOOLBAR);

    m_showToolbarsAction = new QAction(tr("Edit Gadgets Mode"), this);
    m_showToolbarsAction->setCheckable(true);
    cmd = am->registerAction(m_showToolbarsAction, Constants::HIDE_TOOLBARS, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F10")));
    mwindow->addAction(cmd, Constants::G_WINDOW_HIDE_TOOLBAR);

    // Window menu separators
    QAction *tmpaction2 = new QAction(this);
    tmpaction2->setSeparator(true);
    cmd = am->registerAction(tmpaction2, "OpenPilot.Window.Sep.Split2", uavGadgetManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

#ifdef Q_WS_MAC
    QString prefix = tr("Meta+Shift");
#else
    QString prefix = tr("Ctrl+Shift");
#endif

    m_splitAction = new QAction(tr("Split"), this);
    cmd = am->registerAction(m_splitAction, Constants::SPLIT, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1+Down").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    m_splitSideBySideAction = new QAction(tr("Split Side by Side"), this);
    cmd = am->registerAction(m_splitSideBySideAction, Constants::SPLIT_SIDE_BY_SIDE, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1+Right").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    m_removeCurrentSplitAction = new QAction(tr("Close Current View"), this);
    cmd = am->registerAction(m_removeCurrentSplitAction, Constants::REMOVE_CURRENT_SPLIT, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1+C").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    m_removeAllSplitsAction = new QAction(tr("Close All Other Views"), this);
    cmd = am->registerAction(m_removeAllSplitsAction, Constants::REMOVE_ALL_SPLITS, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1+A").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    m_gotoOtherSplitAction = new QAction(tr("Goto Next View"), this);
    cmd = am->registerAction(m_gotoOtherSplitAction, Constants::GOTO_OTHER_SPLIT, uavGadgetManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("%1+N").arg(prefix)));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    // Help Action
    tmpaction = new QAction(QIcon(Constants::ICON_HELP), tr("&Help..."), this);
    cmd = am->registerAction(tmpaction, Constants::G_HELP_HELP, m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_HELP);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this, SLOT(showHelp()));

    // About GCS Action
    // Mac doesn't have the "About" actions in the Help menu
#ifndef Q_WS_MAC
    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, "QtCreator.Help.Sep.About", m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
#endif

    // About Plugins Action
    tmpaction = new QAction(QIcon(Constants::ICON_PLUGIN), tr("About &Plugins..."), this);
    cmd = am->registerAction(tmpaction, Constants::ABOUT_PLUGINS, m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
#ifdef Q_WS_MAC
    cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    connect(tmpaction, SIGNAL(triggered()), this, SLOT(aboutPlugins()));

    // Credits Action
    tmpaction = new QAction(QIcon(Constants::ICON_PLUGIN), tr("About &%1...").arg(ORG_BIG_NAME), this);
    cmd = am->registerAction(tmpaction, Constants::ABOUT_AUTHORS, m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
#ifdef Q_WS_MAC
    cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    connect(tmpaction, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
}

void MainWindow::newFile()
{}

void MainWindow::openFile()
{}

void MainWindow::setFocusToEditor()
{}

bool MainWindow::showOptionsDialog(const QString &category, const QString &page, QWidget *parent)
{
    emit m_coreImpl->optionsDialogRequested();

    if (!parent) {
        parent = this;
    }
    SettingsDialog dlg(parent, category, page);
    return dlg.execDialog();
}

void MainWindow::saveAll()
{
    if (m_dontSaveSettings) {
        return;
    }

    emit m_coreImpl->saveSettingsRequested();
    QSettings settings;
    saveSettings(settings);
}

void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this, SLOT(close()));
}

void MainWindow::openFileWith()
{}

void MainWindow::applyTabBarSettings(QTabWidget::TabPosition pos, bool movable)
{
    if (m_modeStack->tabPosition() != pos) {
        m_modeStack->setTabPosition(pos);
    }
    m_modeStack->setMovable(movable);
}

void MainWindow::showHelp()
{
    QDesktopServices::openUrl(QUrl(Constants::GCS_HELP, QUrl::StrictMode));
}

ActionManager *MainWindow::actionManager() const
{
    return m_actionManager;
}

UniqueIDManager *MainWindow::uniqueIDManager() const
{
    return m_uniqueIDManager;
}

MessageManager *MainWindow::messageManager() const
{
    return m_messageManager;
}

VariableManager *MainWindow::variableManager() const
{
    return m_variableManager;
}

ThreadManager *MainWindow::threadManager() const
{
    return m_threadManager;
}

ConnectionManager *MainWindow::connectionManager() const
{
    return m_connectionManager;
}

QList<UAVGadgetManager *> MainWindow::uavGadgetManagers() const
{
    return m_uavGadgetManagers;
}

UAVGadgetInstanceManager *MainWindow::uavGadgetInstanceManager() const
{
    return m_uavGadgetInstanceManager;
}


ModeManager *MainWindow::modeManager() const
{
    return m_modeManager;
}

MimeDatabase *MainWindow::mimeDatabase() const
{
    return m_mimeDatabase;
}

GeneralSettings *MainWindow::generalSettings() const
{
    return m_generalSettings;
}

IContext *MainWindow::contextObject(QWidget *widget)
{
    return m_contextWidgets.value(widget, NULL);
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context) {
        return;
    }
    QWidget *widget = context->widget();
    if (m_contextWidgets.contains(widget)) {
        return;
    }

    m_contextWidgets.insert(widget, context);
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context) {
        return;
    }

    QWidget *widget = context->widget();
    if (!m_contextWidgets.contains(widget)) {
        return;
    }

    m_contextWidgets.remove(widget);
    if (m_activeContext == context) {
        updateContextObject(0);
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);

    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (debugMainWindow) {
                qDebug() << "main window activated";
            }
            emit windowActivated();
        }
    } else if (e->type() == QEvent::WindowStateChange) {
#ifdef Q_WS_MAC
        bool minimized = isMinimized();
        if (debugMainWindow) {
            qDebug() << "main window state changed to minimized=" << minimized;
        }
        m_minimizeAction->setEnabled(!minimized);
        m_zoomAction->setEnabled(!minimized);
#else
        bool isFullScreen = (windowState() & Qt::WindowFullScreen) != 0;
        m_toggleFullScreenAction->setChecked(isFullScreen);
#endif
    }
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu is activated
    if (qobject_cast<QMenuBar *>(now)) {
        return;
    }

    IContext *newContext = 0;
    if (focusWidget()) {
        IContext *context = 0;
        QWidget *p = focusWidget();
        while (p) {
            context = m_contextWidgets.value(p, NULL);
            if (context) {
                newContext = context;
                break;
            }
            p = p->parentWidget();
        }
    }
    updateContextObject(newContext);
}

void MainWindow::updateContextObject(IContext *context)
{
    if (context == m_activeContext) {
        return;
    }
    IContext *oldContext = m_activeContext;
    m_activeContext = context;
    if (!context || oldContext != m_activeContext) {
        emit m_coreImpl->contextAboutToChange(context);
        updateContext();
        if (debugMainWindow) {
            qDebug() << "new context object =" << context << (context ? context->widget() : 0)
                     << (context ? context->widget()->metaObject()->className() : 0);
        }
        emit m_coreImpl->contextChanged(context);
    }
}

void MainWindow::resetContext()
{
    updateContextObject(0);
}

void MainWindow::shutdown()
{
    disconnect(QApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)),
               this, SLOT(updateFocusWidget(QWidget *, QWidget *)));
    m_activeContext = 0;

    // We have to remove all the existing gagdets at his point, not
    // later!
    uavGadgetInstanceManager()->removeAllGadgets();
}

/* Enable/disable menus for uavgadgets */
void MainWindow::showUavGadgetMenus(bool show, bool hasSplitter)
{
    m_showToolbarsAction->setChecked(show);
    m_splitAction->setEnabled(show);
    m_splitSideBySideAction->setEnabled(show);
    m_removeCurrentSplitAction->setEnabled(show && hasSplitter);
    m_removeAllSplitsAction->setEnabled(show && hasSplitter);
    m_gotoOtherSplitAction->setEnabled(show && hasSplitter);
}

inline int takeLeastPriorityUavGadgetManager(const QList<Core::UAVGadgetManager *> m_uavGadgetManagers)
{
    int index = 0;
    int prio  = m_uavGadgetManagers.at(0)->priority();

    for (int i = 0; i < m_uavGadgetManagers.count(); i++) {
        int prio2 = m_uavGadgetManagers.at(i)->priority();
        if (prio2 < prio) {
            prio  = prio2;
            index = i;
        }
    }
    return index;
}

void MainWindow::createWorkspaces(QSettings &settings, bool diffOnly)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    Core::UAVGadgetManager *uavGadgetManager;

    // If diffOnly is true, we only add/remove the number of workspaces
    // that has changed,
    // otherwise a complete reload of workspaces is done
    int toRemoveFirst   = m_uavGadgetManagers.count();
    int newWorkspacesNo = m_workspaceSettings->numberOfWorkspaces();

    if (diffOnly && m_uavGadgetManagers.count() > newWorkspacesNo) {
        toRemoveFirst = m_uavGadgetManagers.count() - newWorkspacesNo;
    } else {
        toRemoveFirst = 0;
    }

    int removed = 0;

    while (!m_uavGadgetManagers.isEmpty() && (toRemoveFirst > removed)) {
        int index = takeLeastPriorityUavGadgetManager(m_uavGadgetManagers);
        uavGadgetManager = m_uavGadgetManagers.takeAt(index);
        uavGadgetManager->removeAllSplits();
        pm->removeObject(uavGadgetManager);
        delete uavGadgetManager;
        removed++;
    }

    int start = 0;
    if (diffOnly) {
        start = m_uavGadgetManagers.count();
    } else {
        m_uavGadgetManagers.clear();
    }

    QElapsedTimer totalTimer;
    totalTimer.start();
    for (int i = start; i < newWorkspacesNo; ++i) {
        QElapsedTimer timer;
        timer.start();

        const QString name     = m_workspaceSettings->name(i);
        const QString iconName = m_workspaceSettings->iconName(i);
        const QString modeName = m_workspaceSettings->modeName(i);
        uavGadgetManager = new Core::UAVGadgetManager(CoreImpl::instance(), name, QIcon(iconName), 90 - i + 1, modeName,
                                                      this);

        connect(uavGadgetManager, SIGNAL(showUavGadgetMenus(bool, bool)), this, SLOT(showUavGadgetMenus(bool, bool)));

        connect(m_showToolbarsAction, SIGNAL(triggered(bool)), uavGadgetManager, SLOT(showToolbars(bool)));
        connect(m_splitAction, SIGNAL(triggered()), uavGadgetManager, SLOT(split()));
        connect(m_splitSideBySideAction, SIGNAL(triggered()), uavGadgetManager, SLOT(splitSideBySide()));
        connect(m_removeCurrentSplitAction, SIGNAL(triggered()), uavGadgetManager, SLOT(removeCurrentSplit()));
        connect(m_removeAllSplitsAction, SIGNAL(triggered()), uavGadgetManager, SLOT(removeAllSplits()));
        connect(m_gotoOtherSplitAction, SIGNAL(triggered()), uavGadgetManager, SLOT(gotoOtherSplit()));

        pm->addObject(uavGadgetManager);
        m_uavGadgetManagers.append(uavGadgetManager);
        uavGadgetManager->readSettings(settings);
        qDebug() << "MainWindow::createWorkspaces - creating workspace" << name << "took" << timer.elapsed() << "ms";
    }
    qDebug() << "MainWindow::createWorkspaces - creating workspaces took" << totalTimer.elapsed() << "ms";
}

static const QString settingsGroup  = "MainWindow";
static const QString geometryKey    = "Geometry";
static const QString colorKey = "Color";
static const QString maxKey = "Maximized";
static const QString fullScreenKey  = "FullScreen";
static const QString modePriorities = "ModePriorities";

void MainWindow::readSettings(QSettings &settings, bool workspaceDiffOnly)
{
    if (workspaceDiffOnly) {
        createWorkspaces(settings, workspaceDiffOnly);
        return;
    }

    m_generalSettings->readSettings(settings);
    m_actionManager->readSettings(settings);

    settings.beginGroup(settingsGroup);

    Utils::StyleHelper::setBaseColor(settings.value(colorKey).value<QColor>());

    const QVariant geom = settings.value(geometryKey);
    if (geom.isValid()) {
        setGeometry(geom.toRect());
    } else {
        resize(750, 400);
    }
    if (settings.value(maxKey, false).toBool()) {
        setWindowState(Qt::WindowMaximized);
    }
    setFullScreen(settings.value(fullScreenKey, false).toBool());

    settings.endGroup();

    m_workspaceSettings->readSettings(settings);

    createWorkspaces(settings);

    // Restore tab ordering
    settings.beginGroup(modePriorities);

    QStringList modeNames = settings.childKeys();
    QMap<QString, int> map;
    foreach(QString modeName, modeNames) {
        map.insert(modeName, settings.value(modeName).toInt());
    }
    m_modeManager->reorderModes(map);

    settings.endGroup();

    // Restore selected tab
    if (m_workspaceSettings->restoreSelectedOnStartup()) {
        int index = settings.value("SelectedWorkspace").toInt();
        m_modeStack->setCurrentIndex(index);
    }
}


void MainWindow::saveSettings(QSettings &settings) const
{
    if (m_dontSaveSettings) {
        return;
    }

    m_workspaceSettings->saveSettings(settings);

    settings.beginGroup(settingsGroup);

    settings.setValue(colorKey, Utils::StyleHelper::baseColor());

    if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        settings.setValue(maxKey, (bool)(windowState() & Qt::WindowMaximized));
        settings.setValue(fullScreenKey, (bool)(windowState() & Qt::WindowFullScreen));
    } else {
        settings.setValue(maxKey, false);
        settings.setValue(fullScreenKey, false);
        settings.setValue(geometryKey, geometry());
    }

    settings.endGroup();

    // Write tab ordering
    settings.beginGroup(modePriorities);
    QVector<IMode *> modes = m_modeManager->modes();
    foreach(IMode * mode, modes) {
        settings.setValue(mode->uniqueModeName(), mode->priority());
    }
    settings.endGroup();

    // Write selected tab
    settings.setValue("SelectedWorkspace", m_modeStack->currentIndex());

    foreach(UAVGadgetManager * manager, m_uavGadgetManagers) {
        manager->saveSettings(settings);
    }

    m_actionManager->saveSettings(settings);
    m_generalSettings->saveSettings(settings);

    settings.beginGroup("General");
    settings.setValue("Description", m_config_description);
    settings.setValue("Details", m_config_details);
    settings.setValue("StyleSheet", m_config_stylesheet);
    settings.endGroup();
}

void MainWindow::readSettings(IConfigurablePlugin *plugin, QSettings &settings)
{
    UAVConfigInfo configInfo;
    QObject *qo = reinterpret_cast<QObject *>(plugin);
    QString configName = qo->metaObject()->className();

    settings.beginGroup("Plugins");
    settings.beginGroup(configName);
    configInfo.read(settings);
    configInfo.setNameOfConfigurable("Plugin-" + configName);
    settings.beginGroup("data");
    plugin->readConfig(settings, &configInfo);

    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
}

void MainWindow::saveSettings(IConfigurablePlugin *plugin, QSettings &settings) const
{
    if (m_dontSaveSettings) {
        return;
    }

    UAVConfigInfo configInfo;
    QString configName = plugin->metaObject()->className();

    settings.beginGroup("Plugins");
    settings.beginGroup(configName);
    settings.beginGroup("data");
    plugin->saveConfig(settings, &configInfo);
    settings.endGroup();
    configInfo.save(settings);
    settings.endGroup();
    settings.endGroup();
}

void MainWindow::deleteSettings()
{
    QSettings settings;

    settings.clear();
    settings.sync();
    m_dontSaveSettings = true;
}

void MainWindow::addAdditionalContext(int context)
{
    if (context == 0) {
        return;
    }

    if (!m_additionalContexts.contains(context)) {
        m_additionalContexts.prepend(context);
    }
}

void MainWindow::removeAdditionalContext(int context)
{
    if (context == 0) {
        return;
    }

    int index = m_additionalContexts.indexOf(context);
    if (index != -1) {
        m_additionalContexts.removeAt(index);
    }
}

bool MainWindow::hasContext(int context) const
{
    return m_actionManager->hasContext(context);
}

void MainWindow::updateContext()
{
    QList<int> contexts;

    if (m_activeContext) {
        contexts += m_activeContext->context();
    }

    contexts += m_additionalContexts;

    QList<int> uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const int c = contexts.at(i);
        if (!uniquecontexts.contains(c)) {
            uniquecontexts << c;
        }
    }

    m_actionManager->setContext(uniquecontexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci =
        m_actionManager->actionContainer(Constants::M_FILE_RECENTFILES);

    if (aci) {
        aci->menu()->clear();

        bool hasRecentFiles = false;

        aci->menu()->setEnabled(hasRecentFiles);
    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }
    QString fileName = action->data().toString();
    if (!fileName.isEmpty()) {}
}

void MainWindow::showAboutDialog()
{
    if (!m_aboutDialog) {
        m_aboutDialog = new AboutDialog(this);
        connect(m_aboutDialog, SIGNAL(finished(int)),
                this, SLOT(destroyAboutDialog()));
    }
    m_aboutDialog->show();
}

void MainWindow::destroyAboutDialog()
{
    if (m_aboutDialog) {
        m_aboutDialog->deleteLater();
        m_aboutDialog = 0;
    }
}

void MainWindow::aboutPlugins()
{
    PluginDialog dialog(this);

    dialog.exec();
}

void MainWindow::setFullScreen(bool on)
{
    if (bool(windowState() & Qt::WindowFullScreen) == on) {
        return;
    }

    if (on) {
#ifdef Q_OS_WIN
        QWindowsWindowFunctions::setHasBorderInFullScreen(windowHandle(), true);
#endif
        setWindowState(windowState() | Qt::WindowFullScreen);
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
}

// Display a warning with an additional button to open
// the debugger settings dialog if settingsId is nonempty.

bool MainWindow::showWarningWithOptions(const QString &title,
                                        const QString &text,
                                        const QString &details,
                                        const QString &settingsCategory,
                                        const QString &settingsId,
                                        QWidget *parent)
{
    if (parent == 0) {
        parent = this;
    }
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    if (details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    QAbstractButton *settingsButton = 0;
    if (!settingsId.isEmpty() || !settingsCategory.isEmpty()) {
        settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    }
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton) {
        return showOptionsDialog(settingsCategory, settingsId);
    }
    return false;
}
