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

#ifndef PFDQMLGADGETCONFIGURATION_H
#define PFDQMLGADGETCONFIGURATION_H

#include "pfdqml.h"
#include <coreplugin/iuavgadgetconfiguration.h>

#include <QMap>
#include <QDateTime>

using namespace Core;

class PfdQmlGadgetConfiguration : public IUAVGadgetConfiguration {
    Q_OBJECT
public:
    explicit PfdQmlGadgetConfiguration(QString classId, QSettings *qSettings = 0, QObject *parent = 0);

    QString qmlFile() const
    {
        return m_qmlFile;
    }
    void setQmlFile(const QString &fileName)
    {
        m_qmlFile = fileName;
    }

    QString speedUnit() const
    {
        return m_speedMap[m_speedFactor];
    }

    double speedFactor() const
    {
        return m_speedFactor;
    }
    void setSpeedFactor(double factor)
    {
        m_speedFactor = factor;
    }

    QString altitudeUnit() const
    {
        return m_altitudeMap[m_altitudeFactor];
    }

    double altitudeFactor() const
    {
        return m_altitudeFactor;
    }
    void setAltitudeFactor(double factor)
    {
        m_altitudeFactor = factor;
    }

    bool terrainEnabled() const
    {
        return m_terrainEnabled;
    }
    void setTerrainEnabled(bool flag)
    {
        m_terrainEnabled = flag;
    }

    QString terrainFile() const
    {
        return m_terrainFile;
    }
    void setTerrainFile(const QString &fileName)
    {
        m_terrainFile = fileName;
    }


    double latitude() const
    {
        return m_latitude;
    }
    void setLatitude(double value)
    {
        m_latitude = value;
    }

    double longitude() const
    {
        return m_longitude;
    }
    void setLongitude(double value)
    {
        m_longitude = value;
    }

    double altitude() const
    {
        return m_altitude;
    }
    void setAltitude(double value)
    {
        m_altitude = value;
    }

    void setCacheOnly(bool flag)
    {
        m_cacheOnly = flag;
    }
    bool cacheOnly() const
    {
        return m_cacheOnly;
    }

    Pfd::TimeMode timeMode() const
    {
        return m_timeMode;
    }
    void setTimeMode(Pfd::TimeMode timeMode)
    {
        m_timeMode = timeMode;
    }

    QDateTime dateTime() const
    {
        return m_dateTime;
    }
    void setDateTime(QDateTime &dateTime)
    {
        m_dateTime = dateTime;
    }

    double minAmbientLight() const
    {
        return m_minAmbientLight;
    }
    void setMinAmbientLight(double minAmbientLight)
    {
        m_minAmbientLight = minAmbientLight;
    }

    bool modelEnabled() const
    {
        return m_modelEnabled;
    }
    void setModelEnabled(bool flag)
    {
        m_modelEnabled = flag;
    }

    QString modelFile() const
    {
        return m_modelFile;
    }
    void setModelFile(const QString &fileName)
    {
        m_modelFile = fileName;
    }

    Pfd::ModelSelectionMode modelSelectionMode() const
    {
        return m_modelSelectionMode;
    }
    void setModelSelectionMode(Pfd::ModelSelectionMode modelSelectionMode)
    {
        m_modelSelectionMode = modelSelectionMode;
    }

    QString backgroundImageFile() const
    {
        return m_backgroundImageFile;
    }
    void setBackgroundImageFile(const QString &fileName)
    {
        m_backgroundImageFile = fileName;
    }

    QMapIterator<double, QString> speedMapIterator()
    {
        return QMapIterator<double, QString>(m_speedMap);
    }

    QMapIterator<double, QString> altitudeMapIterator()
    {
        return QMapIterator<double, QString>(m_altitudeMap);
    }

    void saveConfig(QSettings *settings) const;
    IUAVGadgetConfiguration *clone();

private:
    QString m_qmlFile; // The name of the dial's SVG source file

    double m_speedFactor;
    double m_altitudeFactor;

    bool m_terrainEnabled;
    QString m_terrainFile; // The name of osgearth terrain file
    bool m_cacheOnly;

    double m_latitude;
    double m_longitude;
    double m_altitude;

    Pfd::TimeMode m_timeMode;
    QDateTime m_dateTime;
    double m_minAmbientLight;

    bool m_modelEnabled;
    QString m_modelFile; // The name of model file
    Pfd::ModelSelectionMode m_modelSelectionMode;

    QString m_backgroundImageFile;

    QMap<double, QString> m_speedMap;
    QMap<double, QString> m_altitudeMap;
};

#endif // PFDQMLGADGETCONFIGURATION_H
