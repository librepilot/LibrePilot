/**
 ******************************************************************************
 *
 * @file       flatearthwidget.cpp
 * @author     The LibrePilot Team, http://www.librepilot.org Copyright (C) 2017.
 * @author     Edouard Lafargue Copyright (C) 2010.
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

#include "flatearthwidget.h"
#include <math.h> /* fabs */

#define MARKER_SIZE 40.0

FlatEarthWidget::FlatEarthWidget(QWidget *parent) : QGraphicsView(parent)
{
    // Disable scrollbars to prevent an "always updated loop"
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Show the whole world by default
    zoomedin   = false;

    // Set initial location
    oldLat     = oldLon = 0;

    // Draw the earth
    earthScene = new QGraphicsScene(this);
    QPixmap earthpix(":/gpsgadget/images/flatEarth.png");
    earthPixmapItem = earthScene->addPixmap(earthpix);
    this->setScene(earthScene);

    // Draw the marker
    marker = new QGraphicsSvgItem();
    QSvgRenderer *renderer = new QSvgRenderer();
    renderer->load(QString(":/gpsgadget/images/marker.svg"));
    marker->setSharedRenderer(renderer);
    earthScene->addItem(marker);
}

FlatEarthWidget::~FlatEarthWidget()
{}

/*
 * Refresh the map on first showing
 */
void FlatEarthWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    refreshMap();
}

/*
 * Refresh the map while resizing the widget
 */
void FlatEarthWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    refreshMap();
}

/*
 * Toggle zoom state on mouse click event
 */
void FlatEarthWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    // toggle zoom state
    if (zoomedin) {
        zoomedin = false;
    } else {
        zoomedin = true;
    }
    refreshMap();
}

/*
 * Update the position of the marker on the map
 */
void FlatEarthWidget::setPosition(double lat, double lon)
{
    /* Only perform this expensive graphics operation if the position has changed enough
       to be visible on the map.
     */
    if (fabs(oldLat - lat) > 0.1 || fabs(oldLon - lon) > 0.1) {
        double wscale = this->sceneRect().width() / 360;
        double hscale = this->sceneRect().height() / 180;
        QPointF opd   = QPointF((lon + 180) * wscale - marker->boundingRect().width() * marker->scale() / 2,
                                (90 - lat) * hscale - marker->boundingRect().height() * marker->scale() / 2);
        marker->setTransform(QTransform::fromTranslate(opd.x(), opd.y()), false);

        // Center the map on the uav position
        if (zoomedin) {
            this->centerOn(marker);
        }
    }
    oldLat = lat;
    oldLon = lon;
}

/*
 * Update the scale of the map
 */
void FlatEarthWidget::refreshMap(void)
{
    double markerScale;

    if (zoomedin) {
        // Zoom the map to it's native resolution
        this->resetTransform();

        // Center the map on the uav position
        this->centerOn(marker);

        // Rescale the marker dimensions to keep the same appearance in both zoom levels
        markerScale = MARKER_SIZE / 100.0 * (double)this->rect().width() / (double)(earthPixmapItem->boundingRect().width());
        marker->setScale(markerScale);
    } else {
        // Fit the whole world
        this->fitInView(earthPixmapItem, Qt::KeepAspectRatio);

        // Rescale the marker dimensions to keep the same appearance in both zoom levels
        markerScale = MARKER_SIZE / 100.0;
        marker->setScale(markerScale);
    }
    // force an update of the marker position by giving the position a nudge
    this->setPosition(oldLat - 0.2, oldLon);
}
