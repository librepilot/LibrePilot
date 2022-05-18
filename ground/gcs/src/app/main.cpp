/**
 ******************************************************************************
 *
 * @file       main.cpp
 * @author     The LibrePilot Team http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
 *
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

/*
   The GCS application name is defined in the top level makefile - GCS_BIG_NAME / GCS_SMALL_NAME, and
   set for the build in ../../gcs.pri and ./app.pro

   The GCS locale is set to the system locale by default unless the "hidden" setting General/Locale has a value.
   The user can not change General/Locale from the Options dialog.

   The GCS language will default to the GCS locale unless the General/OverrideLanguage has a value.
   The user can change General/OverrideLanguage to any available language from the Options dialog.

   Both General/Locale and General/OverrideLanguage can be set from the command line or through the the factory defaults file.

   The -D option is used to permanently set a user setting.

   The -reset switch will clear all the user settings and will trigger a reload of the factory defaults.

   You can combine it with the -config-file=<file> command line argument to quickly switch between multiple settings files.

   [code]
   gcs -reset -config-file ./MyGCS.xml
   [/code]

   Relative paths are relative to <install dir>/share/$(GCS_SMALL_NAME)/configurations/

   The specified file will be used to load the factory defaults from but only when the user settings are empty.
   If the user settings are not empty the file will not be used.
   This switch is useful on the 1st run when the user settings are empty or in combination with -reset.


   Quickly switch configurations

   [code]
   gcs -reset -config-file <relative or absolute path>
   [/code]

   Configuring GCS from installer

   The -D option is used to permanently set a user setting.

   If the user chooses to start GCS at the end of the installer:

   [code]
   gcs -D General/OverrideLanguage=de
   [/code]

   If the user chooses not to start GCS at the end of the installer, you still need to configure GCS.
   In that case you can use -exit-after-config

   [code]
   gcs -D General/OverrideLanguage=de -exit-after-config
   [/code]

 */

#include "qtsingleapplication.h"
#include "gcssplashscreen.h"
#include "utils/pathutils.h"
#include "utils/settingsutils.h"
#include "utils/filelogger.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QSettings>

#include <QMessageBox>
#include <QApplication>
#include <QSurfaceFormat>

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;
typedef QMap<QString, bool> AppOptions;
typedef QMap<QString, QString> AppOptionValues;

const int OptionIndent = 4;
const int DescriptionIndent = 24;

const QLatin1String APP_NAME(GCS_BIG_NAME);
const QLatin1String ORG_NAME(ORG_BIG_NAME);

const QLatin1String CORE_PLUGIN_NAME("Core");

const char *DEFAULT_CONFIG_FILENAME = "default.xml";

const char *fixedOptionsC = " [OPTION]... [FILE]...\n"
                            "Options:\n"
                            "    -help               Display this help\n"
                            "    -version            Display application version\n"
                            "    -no-splash          Don't display splash screen\n"
                            "    -log <file>         Log to specified file\n"
                            "    -client             Attempt to connect to already running instance\n"
                            "    -D <key>=<value>    Permanently set a user setting, e.g: -D General/OverrideLanguage=de\n"
                            "    -reset              Reset user settings to factory defaults.\n"
                            "    -config-file <file> Specify alternate factory defaults settings file (used with -reset)\n"
                            "    -exit-after-config  Exit after manipulating configuration settings\n";

const QLatin1String HELP1_OPTION("-h");
const QLatin1String HELP2_OPTION("-help");
const QLatin1String HELP3_OPTION("/h");
const QLatin1String HELP4_OPTION("--help");
const QLatin1String VERSION_OPTION("-version");
const QLatin1String NO_SPLASH_OPTION("-no-splash");
const QLatin1String CLIENT_OPTION("-client");
const QLatin1String CONFIG_OPTION("-D");
const QLatin1String RESET_OPTION("-reset");
const QLatin1String CONFIG_FILE_OPTION("-config-file");
const QLatin1String EXIT_AFTER_CONFIG_OPTION("-exit-after-config");
const QLatin1String LOG_FILE_OPTION("-log");

// Helpers for displaying messages. Note that there is no console on Windows.
void displayHelpText(QString t)
{
#ifdef Q_OS_WIN
    // No console on Windows. (???)
    // TODO there is a console on windows and popups are not always desired
    t.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    t.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    t.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    t.insert(0, QLatin1String("<html><pre>"));
    t.append(QLatin1String("</pre></html>"));
    QMessageBox::information(0, APP_NAME, t);
#else
    qWarning("%s", qPrintable(t));
#endif
}

void displayError(const QString &t)
{
#ifdef Q_OS_WIN
    // No console on Windows. (???)
    // TODO there is a console on windows and popups are not always desired
    QMessageBox::critical(0, APP_NAME, t);
#else
    qCritical("%s", qPrintable(t));
#endif
}

void printVersion(const ExtensionSystem::PluginSpec *corePlugin, const ExtensionSystem::PluginManager &pm)
{
    QString version;
    QTextStream str(&version);

    str << '\n' << APP_NAME << ' ' << corePlugin->version() << " based on Qt " << qVersion() << "\n\n";
    pm.formatPluginVersions(str);
    str << '\n' << corePlugin->copyright() << '\n';
    displayHelpText(version);
}

void printHelp(const QString &appExecName, const ExtensionSystem::PluginManager &pm)
{
    QString help;
    QTextStream str(&help);

    str << "Usage: " << appExecName << fixedOptionsC;
    ExtensionSystem::PluginManager::formatOptions(str, OptionIndent, DescriptionIndent);
    pm.formatPluginOptions(str, OptionIndent, DescriptionIndent);
    displayHelpText(help);
}

inline QString msgCoreLoadFailure(const QString &reason)
{
    return QCoreApplication::translate("Application", "Failed to load core plug-in, reason is: %1").arg(reason);
}

inline QString msgSendArgumentFailed()
{
    return QCoreApplication::translate("Application",
                                       "Unable to send command line arguments to the already running instance. It appears to be not responding.");
}

inline QString msgLogfileOpenFailed(const QString &fileName)
{
    return QCoreApplication::translate("Application", "Failed to open log file %1").arg(fileName);
}

// Prepare a remote argument: If it is a relative file, add the current directory
// since the the central instance might be running in a different directory.
inline QString prepareRemoteArgument(const QString &arg)
{
    QFileInfo fi(arg);

    if (!fi.exists()) {
        return arg;
    }
    if (fi.isRelative()) {
        return fi.absoluteFilePath();
    }
    return arg;
}

// Send the arguments to an already running instance of application
bool sendArguments(SharedTools::QtSingleApplication &app, const QStringList &arguments)
{
    if (!arguments.empty()) {
        // Send off arguments
        const QStringList::const_iterator acend = arguments.constEnd();
        for (QStringList::const_iterator it = arguments.constBegin(); it != acend; ++it) {
            if (!app.sendMessage(prepareRemoteArgument(*it))) {
                displayError(msgSendArgumentFailed());
                return false;
            }
        }
    }
    // Special empty argument means: Show and raise (the slot just needs to be triggered)
    if (!app.sendMessage(QString())) {
        displayError(msgSendArgumentFailed());
        return false;
    }
    return true;
}

void systemInit()
{
#ifdef Q_OS_MAC
    // increase the number of file that can be opened in application
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rl);
#endif

    QApplication::setAttribute(Qt::AA_X11InitThreads, true);

    // protect QQuickWidget from native widgets like GLC ModelView
    // TODO revisit this...
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);

#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    // see https://doc-snapshots.qt.io/qt5-5.6/highdpi.html
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#else
    // see http://doc.qt.io/qt-5/highdpi.html
    qputenv("QT_DEVICE_PIXEL_RATIO", "auto");
#endif
#endif

    // Force "basic" render loop
    // Only Mac uses "threaded" by default and that mode currently does not work well with OSGViewport
    qputenv("QSG_RENDER_LOOP", "basic");

    // disable vsync
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    // see https://bugreports.qt.io/browse/QTBUG-40332
    int timeout = std::numeric_limits<int>::max();
    qputenv("QT_BEARER_POLL_TIMEOUT", QString::number(timeout).toLatin1());
}

static FileLogger *logger = NULL;

void mainMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    logger->log(type, context, msg);
}

Q_DECLARE_METATYPE(QtMsgType)

void logInit(QString &fileName)
{
    qRegisterMetaType<QtMsgType>();
    logger = new FileLogger(fileName);
    if (!logger->start()) {
        delete logger;
        logger = NULL;
        displayError(msgLogfileOpenFailed(fileName));
        return;
    }
    qInstallMessageHandler(mainMessageOutput);
}

void logDeinit()
{
    if (!logger) {
        return;
    }
    qInstallMessageHandler(0);
    delete logger;
    logger = NULL;
}

AppOptions options()
{
    AppOptions appOptions;

    appOptions.insert(HELP1_OPTION, false);
    appOptions.insert(HELP2_OPTION, false);
    appOptions.insert(HELP3_OPTION, false);
    appOptions.insert(HELP4_OPTION, false);
    appOptions.insert(VERSION_OPTION, false);
    appOptions.insert(NO_SPLASH_OPTION, false);
    appOptions.insert(LOG_FILE_OPTION, true);
    appOptions.insert(CLIENT_OPTION, false);
    appOptions.insert(CONFIG_OPTION, true);
    appOptions.insert(RESET_OPTION, false);
    appOptions.insert(CONFIG_FILE_OPTION, true);
    appOptions.insert(EXIT_AFTER_CONFIG_OPTION, false);
    return appOptions;
}

AppOptionValues parseCommandLine(SharedTools::QtSingleApplication &app,
                                 ExtensionSystem::PluginManager &pluginManager, QString &errorMessage)
{
    AppOptionValues appOptionValues;
    const QStringList arguments = app.arguments();

    if (arguments.size() > 1) {
        AppOptions appOptions = options();
        pluginManager.parseOptions(arguments, appOptions, &appOptionValues, &errorMessage);
    }
    return appOptionValues;
}

void loadTranslators(QString language, QTranslator &translator, QTranslator &qtTranslator)
{
    const QString &creatorTrPath = Utils::GetDataPath() + QLatin1String("translations");

    if (translator.load(QLatin1String("gcs_") + language, creatorTrPath)) {
        // Install gcs_xx.qm translation file
        QCoreApplication::installTranslator(&translator);

        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &qtTrFile = QLatin1String("qt_") + language;
        // Binary installer puts Qt tr files into creatorTrPath
        if (qtTranslator.load(qtTrFile, qtTrPath) || qtTranslator.load(qtTrFile, creatorTrPath)) {
            // Install main qt_xx.qm translation file
            QCoreApplication::installTranslator(&qtTranslator);
        }
    } else {
        // unload(), no gcs translation found
        translator.load(QString());
    }
}

int runApplication(int argc, char * *argv)
{
    QElapsedTimer timer;

    timer.start();

    // create application
    SharedTools::QtSingleApplication app(APP_NAME, argc, argv);

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(ORG_NAME);

    // initialize the plugin manager
    ExtensionSystem::PluginManager pluginManager;
    pluginManager.setFileExtension(QLatin1String("pluginspec"));
    pluginManager.setPluginPaths(Utils::GetPluginPaths());

    // parse command line
    qDebug() << "main - command line" << app.arguments();
    QString errorMessage;
    AppOptionValues appOptionValues = parseCommandLine(app, pluginManager, errorMessage);
    if (!errorMessage.isEmpty()) {
        // this will display two popups : one error popup + one usage string popup
        // TODO merge two popups into one.
        displayError(errorMessage);
        printHelp(QFileInfo(app.applicationFilePath()).baseName(), pluginManager);
        return -1;
    }

    // start logging to file if requested
    if (appOptionValues.contains(LOG_FILE_OPTION)) {
        QString logFileName = appOptionValues.value(LOG_FILE_OPTION);
        logInit(logFileName);
        // relog command line arguments for the benefit of the file logger...
        qDebug() << "main - command line" << app.arguments();
    }

    // init settings
    Utils::initSettings(appOptionValues.value(CONFIG_FILE_OPTION));

    // load user settings
    QSettings settings;
    qDebug() << "main - loading user settings from" << settings.fileName();

    // need to reset user settings?
    if (settings.allKeys().isEmpty() || appOptionValues.contains(RESET_OPTION)) {
        qDebug() << "main - resetting user settings";
        Utils::resetToFactoryDefaults(settings);
    }
    Utils::mergeFactoryDefaults(settings);

    // override settings with command line provided values
    // take notice that the overridden values will be saved in the user settings and will continue to be effective
    // in subsequent GCS runs
    Utils::overrideSettings(settings, argc, argv);

    // initialize GCS locale
    // use the value defined by the General/Locale setting or default to system Locale.
    // the General/Locale setting is not available in the Options dialog, it is a hidden setting but can still be changed:
    // - through the command line
    // - editing the factory defaults XML file before 1st launch
    // - editing the user XML file
    QString localeName = settings.value("General/Locale", QLocale::system().name()).toString();
    QLocale::setDefault(localeName);

    // some debugging
    qDebug() << "main - system locale:" << QLocale::system().name();
    qDebug() << "main - GCS locale:" << QLocale().name();

    // load translation file
    // the language used is defined by the General/OverrideLanguage setting (defaults to GCS locale)
    // if the translation file for the given language is not found, GCS will default to built in English.
    QString language = settings.value("General/OverrideLanguage", localeName).toString();
    qDebug() << "main - language:" << language;
    QTranslator translator;
    QTranslator qtTranslator;
    loadTranslators(language, translator, qtTranslator);

    app.setProperty("qtc_locale", localeName); // Do we need this?

    if (appOptionValues.contains(EXIT_AFTER_CONFIG_OPTION)) {
        qDebug() << "main - exiting after config!";
        return 0;
    }

    // open the splash screen
    GCSSplashScreen *splash = 0;
    if (!appOptionValues.contains(NO_SPLASH_OPTION)) {
        splash = new GCSSplashScreen();
        // show splash
        splash->showProgressMessage(QObject::tr("Application starting..."));
        splash->show();
        // connect to track progress of plugin manager
        QObject::connect(&pluginManager, SIGNAL(pluginAboutToBeLoaded(ExtensionSystem::PluginSpec *)), splash,
                         SLOT(showPluginLoadingProgress(ExtensionSystem::PluginSpec *)));
    }

    // find and load core plugin
    const PluginSpecSet plugins = pluginManager.plugins();
    ExtensionSystem::PluginSpec *coreplugin = 0;
    foreach(ExtensionSystem::PluginSpec * spec, plugins) {
        if (spec->name() == CORE_PLUGIN_NAME) {
            coreplugin = spec;
            break;
        }
    }
    if (!coreplugin) {
        QString nativePaths  = QDir::toNativeSeparators(Utils::GetPluginPaths().join(QLatin1String(",")));
        const QString reason = QCoreApplication::translate("Application", "Could not find '%1.pluginspec' in %2")
                               .arg(CORE_PLUGIN_NAME).arg(nativePaths);
        displayError(msgCoreLoadFailure(reason));
        return 1;
    }
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }

    if (appOptionValues.contains(VERSION_OPTION)) {
        printVersion(coreplugin, pluginManager);
        return 0;
    }
    if (appOptionValues.contains(HELP1_OPTION) || appOptionValues.contains(HELP2_OPTION)
        || appOptionValues.contains(HELP3_OPTION) || appOptionValues.contains(HELP4_OPTION)) {
        printHelp(QFileInfo(app.applicationFilePath()).baseName(), pluginManager);
        return 0;
    }

    const bool isFirstInstance = !app.isRunning();
    if (!isFirstInstance && appOptionValues.contains(CLIENT_OPTION)) {
        return sendArguments(app, pluginManager.arguments()) ? 0 : -1;
    }

    pluginManager.loadPlugins();

    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }

    QStringList errors;
    foreach(ExtensionSystem::PluginSpec * p, pluginManager.plugins()) {
        if (p->hasError()) {
            errors.append(p->errorString());
        }
    }
    if (!errors.isEmpty()) {
        QMessageBox::warning(0,
                             QCoreApplication::translate("Application", "%1 - Plugin loader messages").arg(GCS_BIG_NAME),
                             errors.join(QString::fromLatin1("\n\n")));
    }

    if (isFirstInstance) {
        // Set up lock and remote arguments for the first instance only.
        // Silently fallback to unconnected instances for any subsequent instances.
        app.initialize();
        QObject::connect(&app, SIGNAL(messageReceived(QString)), coreplugin->plugin(), SLOT(remoteArgument(QString)));
    }
    QObject::connect(&app, SIGNAL(fileOpenRequest(QString)), coreplugin->plugin(), SLOT(remoteArgument(QString)));

    // Do this after the event loop has started
    QTimer::singleShot(100, &pluginManager, SLOT(startTests()));

    if (splash) {
        // close and delete splash
        splash->close();
        delete splash;
    }

    qDebug() << "main - starting GCS took" << timer.elapsed() << "ms";
    int ret = app.exec();
    qDebug() << "main - GCS ran for" << timer.elapsed() << "ms";

    return ret;
}

int main(int argc, char * *argv)
{
    // low level init
    systemInit();

    int ret = runApplication(argc, argv);

    // close log file if needed
    logDeinit();

    return ret;
}
