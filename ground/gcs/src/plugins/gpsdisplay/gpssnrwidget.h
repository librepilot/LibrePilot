/**
 ******************************************************************************
 *
 * @file       gpssnrwidget.h
 * @author     The LibrePilot Team, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2014.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GpsSnrWidget GpsSnr Widget Plugin
 * @{
 * @brief A widget for visualizing Signal to Noise Ratio information for known SV
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPSSNRWIDGET_H
#define GPSSNRWIDGET_H

#include <QGraphicsView>
class QGraphicsRectItem;

class GpsSnrWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit GpsSnrWidget(QWidget *parent = 0);
    ~GpsSnrWidget();

signals:

public slots:
    void updateSat(int index, int prn, int elevation, int azimuth, int snr);

private:
    static const int MAX_SATELLITES = 24;
    int satellites[MAX_SATELLITES][4];
    QGraphicsScene *scene;
    QGraphicsRectItem *boxes[MAX_SATELLITES];
    QGraphicsSimpleTextItem *satTexts[MAX_SATELLITES];
    QGraphicsSimpleTextItem *satSNRs[MAX_SATELLITES];
    QRectF prnReferenceTextRect;

    void drawSat(int index);

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // GPSSNRWIDGET_H
