/**
 ******************************************************************************
 *
 * @file       flatearthwidget.h
 * @author     The LibrePilot Team, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup FlatEarthWidget FlatEarth Widget Plugin
 * @{
 * @brief      A widget for visualizing a location on a flat map of the world.
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

#ifndef FLATEARTHWIDGET_H_
#define FLATEARTHWIDGET_H_

#include <QGraphicsView>
class QGraphicsSvgItem;

class FlatEarthWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit FlatEarthWidget(QWidget *parent = 0);
    ~FlatEarthWidget();
    void setPosition(double, double, bool forceUpdate = false);

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    bool zoomedin;
    double oldLat, oldLon;

    QGraphicsScene *earthScene;
    QGraphicsPixmapItem *earthPixmapItem;
    QGraphicsSvgItem *marker;

    void refreshMap(void);
};
#endif /* FLATEARTHWIDGET_H_ */
