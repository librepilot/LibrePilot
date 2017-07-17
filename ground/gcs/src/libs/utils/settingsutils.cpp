/**
 ******************************************************************************
 *
 * @file       settingsutils.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      Settings utilities
 *
 * @see        The GNU Public License (GPL) Version 3
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

#include "settingsutils.h"

#include "pathutils.h"
#include "xmlconfig.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSettings>

namespace Utils {
static const QString DEFAULT_CONFIG_DIRNAME  = "configurations";

static const QString DEFAULT_CONFIG_FILENAME = "default.xml";

const QLatin1String CONFIG_OPTION("-D");

// list of read only QSettings objects containing factory defaults
QList<QSettings const *> factorySettingsList;

/*
 * Helper class to manage a simple registry
 * Each entry in the registry is a configuration path
 * For example : UAVGadgetConfigurations/DialGadget/Attitude
 * Note that entries are base64 encoded so QSettings does not mess with them
 */
class QTCREATOR_UTILS_EXPORT Registry {
public:
    Registry()
    {
        QSettings settings;

        settings.beginGroup("Registry");
        registry = settings.childKeys();
        settings.endGroup();
    }

    void save() const
    {
        QSettings settings;

        settings.beginGroup("Registry");
        settings.remove("");
        foreach(QString entry, registry) {
            settings.setValue(entry, 1);
        }
        settings.endGroup();
    }

    bool contains(QString &entry) const
    {
        return registry.contains(key(entry));
    }

    void add(QString &entry)
    {
        registry.append(key(entry));
    }

private:
    QStringList registry;

    QString key(QString &entry) const
    {
        return entry.toUtf8().toBase64();
    }
};

QString checkFile(QString fileName)
{
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        if (fi.isRelative()) {
            // file name has a relative path
            QDir directory(Utils::GetDataPath() + QString("configurations"));
            fileName = directory.absoluteFilePath(fileName);
        }
        if (!QFile::exists(fileName)) {
            qWarning() << "Configuration file" << fileName << "does not exist.";
            fileName = "";
        }
    }
    return fileName;
}

template<typename Func>
void applyToFactorySettings(Func func)
{
    foreach(QSettings const *settings, factorySettingsList) {
        func(*settings);
    }
}

void copySettings(const QSettings &from, QSettings &to)
{
    foreach(QString key, from.allKeys()) {
        if (!to.contains(key)) {
            // qDebug() << "++" << key <<  from.value(key);
            to.setValue(key, from.value(key));
        } else if (from.value(key) != to.value(key)) {
            // qDebug() << ">>" << key <<  from.value(key) << to.value(key);
            to.setValue(key, from.value(key));
        }
    }
}

void mergeSettings(Registry &registry, const QSettings &from, QSettings &to)
{
    to.beginGroup(from.group());

    // iterate over factory defaults groups
    // note that merging could be done in smarter way
    // currently we do a "all or nothing" merge but we could do something more granular
    // this, would allow, new configuration options to be added to existing configurations
    foreach(QString group, from.childGroups()) {
        const_cast<QSettings &>(from).beginGroup(group);
        to.beginGroup(group);
        QString id = from.group();
        if (!registry.contains(id)) {
            // registry keeps growing...
            registry.add(id);
            // copy settings only if destination group is totally empty (see comment above)
            if (to.allKeys().count() <= 0 && to.childGroups().count() <= 0) {
                // found new group, copy it to the destination
                qDebug() << "settings - adding new configuration" << id;
                copySettings(from, to);
            }
        }
        to.endGroup();
        const_cast<QSettings &>(from).endGroup();
    }

    to.endGroup();
}

void mergeFactorySettings(Registry &registry, const QSettings &from, QSettings &to)
{
    const_cast<QSettings &>(from).beginGroup("Plugins");
    mergeSettings(registry, from, to);
    const_cast<QSettings &>(from).endGroup();

    const_cast<QSettings &>(from).beginGroup("UAVGadgetConfigurations");
    foreach(QString childGroup, from.childGroups()) {
        const_cast<QSettings &>(from).beginGroup(childGroup);
        mergeSettings(registry, from, to);
        const_cast<QSettings &>(from).endGroup();
    }
    const_cast<QSettings &>(from).endGroup();
}

void initSettings(const QString &factoryDefaultsFileName)
{
    QSettings::setDefaultFormat(XmlConfig::XmlFormat);

    QDir directory(Utils::GetDataPath() + DEFAULT_CONFIG_DIRNAME);

    // check if command line option -config-file contains a file name
    QString fileName = checkFile(factoryDefaultsFileName);

    if (fileName.isEmpty()) {
        // check default file
        qDebug() << "settings - looking for factory defaults configuration files in" << directory.absolutePath();
        fileName = checkFile(directory.absoluteFilePath(DEFAULT_CONFIG_FILENAME));
    }

    if (fileName.isEmpty()) {
        // TODO should we exit violently?
        qCritical() << "No default configuration file found!";
        return;
    }

    QStringList files;

    // common default
    files << fileName;

    // OS specific default
#ifdef Q_OS_MAC
    files << directory.absoluteFilePath("default_macos.xml");
#elif defined(Q_OS_LINUX)
    files << directory.absoluteFilePath("default_linux.xml");
#else
    files << directory.absoluteFilePath("default_windows.xml");
#endif

    foreach(QString file, files) {
        file = checkFile(file);
        if (!file.isEmpty()) {
            QSettings const *settings = new QSettings(file, XmlConfig::XmlFormat);
            qDebug() << "settings - loaded factory defaults" << file;
            factorySettingsList.append(settings);
        }
    }
}

void overrideSettings(QSettings &settings, int argc, char * *argv)
{
    // Options like -D My/setting=test
    QRegExp rx("([^=]+)=(.*)");

    for (int i = 0; i < argc; ++i) {
        if (CONFIG_OPTION == QString(argv[i])) {
            if (rx.indexIn(argv[++i]) > -1) {
                QString key   = rx.cap(1);
                QString value = rx.cap(2);
                qDebug() << "settings - user setting" << key << "set to value" << value;
                settings.setValue(key, value);
            }
        }
    }
}

void resetToFactoryDefaults(QSettings &toSettings)
{
    toSettings.clear();
    applyToFactorySettings(
        [&](const QSettings &fromSettings) { copySettings(fromSettings, toSettings); }
        );
}

/**
 * Merge factory defaults into user settings.
 * Currently only Plugins and UAVGadgetConfigurations are merged
 */
void mergeFactoryDefaults(QSettings &toSettings)
{
    // registry of all configuration groups ever seen
    // used to avoid re-adding a group that was deleted by the user
    Registry registry;

    // merge all loaded factory defaults
    applyToFactorySettings(
        [&](const QSettings &fromSettings) { mergeFactorySettings(registry, fromSettings, toSettings); }
        );

    registry.save();
}
}
