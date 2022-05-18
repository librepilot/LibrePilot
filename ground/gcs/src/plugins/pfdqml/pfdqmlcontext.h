/**
 ******************************************************************************
 *
 * @file       pfdqmlcontext.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************//*
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

#ifndef PFDQMLCONTEXT_H_
#define PFDQMLCONTEXT_H_

#include "pfdqml.h"
#include "pfdqmlgadgetconfiguration.h"

class QQmlContext;
class QSettings;

class PfdQmlContext : public QObject {
    Q_OBJECT Q_PROPERTY(QString speedUnit READ speedUnit WRITE setSpeedUnit NOTIFY speedUnitChanged)
    Q_PROPERTY(double speedFactor READ speedFactor WRITE setSpeedFactor NOTIFY speedFactorChanged)
    Q_PROPERTY(QString altitudeUnit READ altitudeUnit WRITE setAltitudeUnit NOTIFY altitudeUnitChanged)
    Q_PROPERTY(double altitudeFactor READ altitudeFactor WRITE setAltitudeFactor NOTIFY altitudeFactorChanged)

    // terrain
    Q_PROPERTY(bool terrainEnabled READ terrainEnabled WRITE setTerrainEnabled NOTIFY terrainEnabledChanged)
    Q_PROPERTY(QString terrainFile READ terrainFile WRITE setTerrainFile NOTIFY terrainFileChanged)

    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double altitude READ altitude WRITE setAltitude NOTIFY altitudeChanged)

    Q_PROPERTY(TimeMode::Enum timeMode READ timeMode WRITE setTimeMode NOTIFY timeModeChanged)
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(double minimumAmbientLight READ minimumAmbientLight WRITE setMinimumAmbientLight NOTIFY minimumAmbientLightChanged)

    // model
    Q_PROPERTY(QString modelFile READ modelFile NOTIFY modelFileChanged)
    Q_PROPERTY(QStringList modelFileList READ modelFileList CONSTANT FINAL)

    // background
    Q_PROPERTY(QString backgroundImageFile READ backgroundImageFile WRITE setBackgroundImageFile NOTIFY backgroundImageFileChanged)

    // gstreamer pipeline
    Q_PROPERTY(QString gstPipeline READ gstPipeline WRITE setGstPipeline NOTIFY gstPipelineChanged)

public:
    PfdQmlContext(QObject *parent = 0);
    virtual ~PfdQmlContext();

    QString speedUnit() const;
    void setSpeedUnit(QString unit);
    double speedFactor() const;
    void setSpeedFactor(double factor);
    QString altitudeUnit() const;
    void setAltitudeUnit(QString unit);
    double altitudeFactor() const;
    void setAltitudeFactor(double factor);

    bool terrainEnabled() const;
    void setTerrainEnabled(bool arg);
    QString terrainFile() const;
    void setTerrainFile(const QString &arg);

    double latitude() const;
    void setLatitude(double arg);
    double longitude() const;
    void setLongitude(double arg);
    double altitude() const;
    void setAltitude(double arg);

    TimeMode::Enum timeMode() const;
    void setTimeMode(TimeMode::Enum arg);
    QDateTime dateTime() const;
    void setDateTime(QDateTime arg);
    double minimumAmbientLight() const;
    void setMinimumAmbientLight(double arg);

    // model
    QString modelFile() const;
    void setModelFile(const QString &arg);
    QStringList modelFileList() const;
    Q_INVOKABLE void nextModel();
    Q_INVOKABLE void previousModel();

    // background
    QString backgroundImageFile() const;
    void setBackgroundImageFile(const QString &arg);

    // gstreamer pipeline
    QString gstPipeline() const;
    void setGstPipeline(const QString &arg);

    Q_INVOKABLE void resetConsumedEnergy();

    void loadConfiguration(PfdQmlGadgetConfiguration *config);
    void saveState(QSettings &) const;
    void restoreState(QSettings &);

    void apply(QQmlContext *context);

signals:
    void speedUnitChanged(QString arg);
    void speedFactorChanged(double arg);
    void altitudeUnitChanged(QString arg);
    void altitudeFactorChanged(double arg);

    void terrainEnabledChanged(bool arg);
    void terrainFileChanged(QString arg);

    void latitudeChanged(double arg);
    void longitudeChanged(double arg);
    void altitudeChanged(double arg);

    void timeModeChanged(TimeMode::Enum arg);
    void dateTimeChanged(QDateTime arge);
    void minimumAmbientLightChanged(double arg);

    void modelFileChanged(QString arg);
    void backgroundImageFileChanged(QString arg);

    void gstPipelineChanged(QString arg);

private:
    // constants
    static const QString CONTEXT_PROPERTY_NAME;

    QString m_speedUnit;
    double m_speedFactor;
    QString m_altitudeUnit;
    double m_altitudeFactor;

    bool m_terrainEnabled;
    QString m_terrainFile;

    double m_latitude;
    double m_longitude;
    double m_altitude;

    TimeMode::Enum m_timeMode;
    QDateTime m_dateTime;
    double m_minAmbientLight;

    QString m_modelFile;
    int m_modelIndex;
    QStringList m_modelFileList;

    QString m_backgroundImageFile;

    QString m_gstPipeline;

    void addModelDir(QString dir);
};
#endif /* PFDQMLCONTEXT_H_ */
