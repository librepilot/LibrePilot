/**
 ******************************************************************************
 *
 * @file       navitem.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      A graphicsItem representing a WayPoint
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   OPMapWidget
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
#include "navitem.h"
#include <QGraphicsSceneMouseEvent>
namespace mapcontrol {
NavItem::NavItem(MapGraphicItem *map, OPMapWidget *parent) : map(map), mapwidget(parent),
    toggleRefresh(true), altitude(0)
{
    pic.load(QString::fromUtf8(":/markers/images/nav.svg"));
    pic = pic.scaled(30, 30, Qt::IgnoreAspectRatio);
    this->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    this->setFlag(QGraphicsItem::ItemIsMovable, false);
    this->setFlag(QGraphicsItem::ItemIsSelectable, true);
    localposition = map->FromLatLngToLocal(mapwidget->CurrentPosition());
    this->setPos(localposition.X(), localposition.Y());
    this->setZValue(4);
    coord = internals::PointLatLng(50, 50);
    RefreshToolTip();
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    connect(map, SIGNAL(childRefreshPosition()), this, SLOT(RefreshPos()));
    connect(map, SIGNAL(childSetOpacity(qreal)), this, SLOT(setOpacitySlot(qreal)));
}

void NavItem::RefreshToolTip()
{
    QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);

    setToolTip(QString("Waypoint: Nav\nCoordinate:%1\nAltitude:%2\n").arg(coord_str).arg(QString::number(altitude)));
}


void NavItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->drawPixmap(-pic.width() / 2, -pic.height() / 2, pic);
}
QRectF NavItem::boundingRect() const
{
    return QRectF(-pic.width() / 2, -pic.height() / 2, pic.width(), pic.height());
}


int NavItem::type() const
{
    return Type;
}

void NavItem::RefreshPos()
{
    prepareGeometryChange();
    localposition = map->FromLatLngToLocal(coord);
    this->setPos(localposition.X(), localposition.Y());

    RefreshToolTip();

    this->update();
    toggleRefresh = false;
}

void NavItem::setOpacitySlot(qreal opacity)
{
    setOpacity(opacity);
}


// Set clickable area as smaller than the bounding rect.
QPainterPath NavItem::shape() const
{
    QPainterPath path;

    path.addEllipse(QRectF(-12, -25, 24, 50));
    return path;
}
}
