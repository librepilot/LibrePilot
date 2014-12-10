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

#ifndef PFDQMLGADGETWIDGET_H_
#define PFDQMLGADGETWIDGET_H_

#include "pfdqml.h"
#include "pfdqmlgadgetconfiguration.h"

#include <QQuickView>

class PfdQmlGadgetWidget : public QQuickView {
    Q_OBJECT
    Q_PROPERTY(QString speedUnit READ speedUnit WRITE setSpeedUnit NOTIFY speedUnitChanged)
    Q_PROPERTY(double speedFactor READ speedFactor WRITE setSpeedFactor NOTIFY speedFactorChanged)
    Q_PROPERTY(QString altitudeUnit READ altitudeUnit WRITE setAltitudeUnit NOTIFY altitudeUnitChanged)
    Q_PROPERTY(double altitudeFactor READ altitudeFactor WRITE setAltitudeFactor NOTIFY altitudeFactorChanged)

    Q_PROPERTY(Pfd::PositionMode positionMode READ positionMode WRITE setPositionMode NOTIFY positionModeChanged)
    // pre-defined fallback position
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double altitude READ altitude WRITE setAltitude NOTIFY altitudeChanged)

    // terrain
    Q_PROPERTY(bool terrainEnabled READ terrainEnabled WRITE setTerrainEnabled NOTIFY terrainEnabledChanged)
    Q_PROPERTY(QString terrainFile READ terrainFile WRITE setTerrainFile NOTIFY terrainFileChanged)
    Q_PROPERTY(QString modelFile READ modelFile WRITE setModelFile NOTIFY modelFileChanged)

public:
    PfdQmlGadgetWidget(QWindow *parent = 0);
    virtual ~PfdQmlGadgetWidget();

    void setQmlFile(QString fn);

    QString speedUnit() const
    {
        return m_speedUnit;
    }
    double speedFactor() const
    {
        return m_speedFactor;
    }
    QString altitudeUnit() const
    {
        return m_altitudeUnit;
    }
    double altitudeFactor() const
    {
        return m_altitudeFactor;
    }
    Pfd::PositionMode positionMode() const
    {
        return m_positionMode;
    }
    double latitude() const
    {
        return m_latitude;
    }
    double longitude() const
    {
        return m_longitude;
    }
    double altitude() const
    {
        return m_altitude;
    }
    bool terrainEnabled() const
    {
        return m_terrainEnabled;
    }
    QString terrainFile() const
    {
        return m_terrainFile;
    }
    QString modelFile() const
    {
        return m_modelFile;
    }

public slots:
    void setSpeedUnit(QString unit);
    void setSpeedFactor(double factor);
    void setAltitudeUnit(QString unit);
    void setAltitudeFactor(double factor);

    void setPositionMode(Pfd::PositionMode arg);
    void setLatitude(double arg);
    void setLongitude(double arg);
    void setAltitude(double arg);

    void setOpenGLEnabled(bool arg);
    void setTerrainEnabled(bool arg);
    void setTerrainFile(QString arg);
    void setModelFile(QString arg);


signals:
    void speedUnitChanged(QString arg);
    void speedFactorChanged(double arg);
    void altitudeUnitChanged(QString arg);
    void altitudeFactorChanged(double arg);

    void positionModeChanged(Pfd::PositionMode arg);
    void latitudeChanged(double arg);
    void longitudeChanged(double arg);
    void altitudeChanged(double arg);

    void terrainEnabledChanged(bool arg);
    void terrainFileChanged(QString arg);
    void modelFileChanged(QString arg);

private slots:
    void onStatusChanged(QQuickView::Status status);

private:
    QString m_qmlFileName;

    QString m_speedUnit;
    double m_speedFactor;
    QString m_altitudeUnit;
    double m_altitudeFactor;

    Pfd::PositionMode m_positionMode;
    double m_latitude;
    double m_longitude;
    double m_altitude;

    bool m_terrainEnabled;
    QString m_terrainFile;
    QString m_modelFile;
};

#endif /* PFDQMLGADGETWIDGET_H_ */
