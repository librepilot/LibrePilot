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

// helper class to manage a simple registry
class QTCREATOR_UTILS_EXPORT Registry {
public:
    Registry(QSettings &settings) : settings(settings)
    {
        settings.beginGroup("registry");
        registry = settings.childKeys();
        settings.endGroup();
        // qDebug() << "read registry: " << settings.group() << registry;
    }

    void save() const
    {
        // qDebug() << "saving registry: " << settings.group() << registry;
        settings.beginGroup("registry");
        settings.remove("");
        foreach(QString entry, registry) {
            settings.setValue(key(entry), 1);
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
    QSettings &settings;
    QStringList registry;

    QString key(QString &entry) const
    {
        return entry;
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
            // qDebug() << "+++" << key <<  from.value(key);
            to.setValue(key, from.value(key));
        } else if (from.value(key) != to.value(key)) {
            // qDebug() << ">>>" << key <<  from.value(key) << to.value(key);
            to.setValue(key, from.value(key));
        }
    }
}

void mergeSettings(const QSettings &from, QSettings &to)
{
    const_cast<QSettings &>(from).beginGroup(to.group());

    // registry of all groups ever seen
    // used to avoid re-adding a group that was deleted by the user
    Registry registry(to);

    // iterate over factory defaults groups
    foreach(QString group, from.childGroups()) {
        const_cast<QSettings &>(from).beginGroup(group);
        to.beginGroup(group);
        if (!registry.contains(group)) {
            // registry keeps growing...
            registry.add(group);
            if (to.allKeys().count() <= 0) {
                // found new group, copy it to the destination
                copySettings(from, to);
            }
        }
        to.endGroup();
        const_cast<QSettings &>(from).endGroup();
    }

    registry.save();

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
        qDebug() << "Looking for factory defaults configuration files in" << directory.absolutePath();
        fileName = checkFile(directory.absoluteFilePath(DEFAULT_CONFIG_FILENAME));
    }

    if (fileName.isEmpty()) {
        // TODO should we exit violently?
        qCritical() << "No default configuration file found!";
        return;
    }

    QStringList files;

    // defaults
    files << fileName;

    foreach(QString file, files) {
        file = checkFile(file);

        QSettings const *settings = new QSettings(file, XmlConfig::XmlFormat);
        qDebug() << "Loaded factory defaults" << file;

        factorySettingsList.append(settings);
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
                qDebug() << "User setting" << key << "set to value" << value;
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

void mergeFactoryDefaults(QSettings &toSettings)
{
    applyToFactorySettings(
        [&](const QSettings &fromSettings) { mergeSettings(fromSettings, toSettings); }
        );
}
}
