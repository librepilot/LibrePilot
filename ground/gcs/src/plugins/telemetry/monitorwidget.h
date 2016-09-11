/**
 ******************************************************************************
 *
 * @file       monitorwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   telemetryplugin
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
#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QObject>
#include <QGraphicsView>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include <QtCore/QPointer>

class MonitorWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MonitorWidget(QWidget *parent = 0);
    ~MonitorWidget();

    void setMin(double min)
    {
        minValue = min;
    }

    double getMin()
    {
        return minValue;
    }

    void setMax(double max)
    {
        maxValue = max;
    }

    double getMax()
    {
        return maxValue;
    }

public slots:
    void telemetryConnected();
    void telemetryDisconnected();
    void telemetryUpdated(double txRate, double rxRate);

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    bool connected;

    double minValue;
    double maxValue;

    QGraphicsSvgItem *graph;

    QPointer<QGraphicsTextItem> txSpeed;
    QPointer<QGraphicsTextItem> rxSpeed;

    QList<QGraphicsSvgItem *> txNodes;
    QList<QGraphicsSvgItem *> rxNodes;

    Qt::AspectRatioMode aspectRatioMode;
};

#endif // MONITORWIDGET_H
