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

#include <coreplugin/iuavgadgetconfiguration.h>
#include <QMap>

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

    bool actualPositionUsed() const
    {
        return m_actualPositionUsed;
    }
    void setActualPositionUsed(bool flag)
    {
        m_actualPositionUsed = flag;
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

    bool openGLEnabled() const
    {
        return m_openGLEnabled;
    }
    void setOpenGLEnabled(bool flag)
    {
        m_openGLEnabled = flag;
    }

    bool terrainEnabled() const
    {
        return m_terrainEnabled;
    }
    void setTerrainEnabled(bool flag)
    {
        m_terrainEnabled = flag;
    }

    QString earthFile() const
    {
        return m_earthFile;
    }
    void setEarthFile(const QString &fileName)
    {
        m_earthFile = fileName;
    }

    void setCacheOnly(bool flag)
    {
        m_cacheOnly = flag;
    }
    bool cacheOnly() const
    {
        return m_cacheOnly;
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

    bool m_actualPositionUsed;
    double m_latitude;
    double m_longitude;
    double m_altitude;

    bool m_openGLEnabled;
    bool m_terrainEnabled;
    QString m_earthFile; // The name of osgearth terrain file
    bool m_cacheOnly;

    QMap<double, QString> m_speedMap;
    QMap<double, QString> m_altitudeMap;
};

#endif // PfdQmlGADGETCONFIGURATION_H
