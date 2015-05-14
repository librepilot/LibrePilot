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

#include <QWidget>
#include <QQuickWidget>
#include <QQuickView>

class PfdQmlGadgetWidget;
class QQmlEngine;

// TODO rename to PfdQmlContext
class PfdQmlProperties : public QObject {
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

    Q_PROPERTY(Pfd::TimeMode timeMode READ timeMode WRITE setTimeMode NOTIFY timeModeChanged)
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(double minimumAmbientLight READ minimumAmbientLight WRITE setMinimumAmbientLight NOTIFY minimumAmbientLightChanged)

    Q_PROPERTY(QString modelFile READ modelFile WRITE setModelFile NOTIFY modelFileChanged)
    Q_PROPERTY(QString backgroundImageFile READ backgroundImageFile WRITE setBackgroundImageFile NOTIFY backgroundImageFileChanged)

public:
    PfdQmlProperties(QObject *parent = 0);
    virtual ~PfdQmlProperties();

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

    Pfd::TimeMode timeMode() const;
    void setTimeMode(Pfd::TimeMode arg);
    QDateTime dateTime() const;
    void setDateTime(QDateTime arg);
    double minimumAmbientLight() const;
    void setMinimumAmbientLight(double arg);

    QString modelFile() const;
    void setModelFile(const QString &arg);
    QString backgroundImageFile() const;
    void setBackgroundImageFile(const QString &arg);

    Q_INVOKABLE void resetConsumedEnergy();

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

    void timeModeChanged(Pfd::TimeMode arg);
    void dateTimeChanged(QDateTime arge);
    void minimumAmbientLightChanged(double arg);

    void modelFileChanged(QString arg);
    void backgroundImageFileChanged(QString arg);

private:
    QString m_speedUnit;
    double m_speedFactor;
    QString m_altitudeUnit;
    double m_altitudeFactor;

    bool m_terrainEnabled;
    QString m_terrainFile;

    double m_latitude;
    double m_longitude;
    double m_altitude;

    Pfd::TimeMode m_timeMode;
    QDateTime m_dateTime;
    double m_minAmbientLight;

    QString m_modelFile;

    QString m_backgroundImageFile;
};

/*
 * Very crude proxy that allows to switch between QQuickView and QQuickWindow are runtime
 */
class QuickWidgetProxy : public QObject {
public:
    QuickWidgetProxy(PfdQmlGadgetWidget *parent = 0);
    virtual ~QuickWidgetProxy();

    QWidget *widget();

    void setSource(const QUrl &url);
    QQmlEngine *engine() const;
    QList<QQmlError> errors() const;

private:
    bool m_widget;

    QQuickWidget *m_quickWidget;

    QWidget *m_container;
    QQuickView *m_quickView;
};

class PfdQmlGadgetWidget : public QWidget {
    Q_OBJECT

public:
    PfdQmlGadgetWidget(QWidget *parent = 0);
    virtual ~PfdQmlGadgetWidget();

    void loadConfiguration(PfdQmlGadgetConfiguration *config);

public slots:
    void onStatusChanged(QQuickWidget::Status status);
    void onStatusChanged(QQuickView::Status status);
    void onSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

private:
    void init();

    void setQmlFile(QString);

    void setSource(const QUrl &url)
    {
        m_quickWidgetProxy->setSource(url);
    }

    QQmlEngine *engine() const
    {
        return m_quickWidgetProxy->engine();
    }

    QList<QQmlError> errors() const
    {
        return m_quickWidgetProxy->errors();
    }

    QuickWidgetProxy *m_quickWidgetProxy;

    PfdQmlProperties *m_pfdQmlProperties;
    QString m_qmlFileName;
};

#endif /* PFDQMLGADGETWIDGET_H_ */
