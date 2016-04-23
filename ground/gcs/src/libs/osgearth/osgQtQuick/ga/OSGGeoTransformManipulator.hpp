/**
 ******************************************************************************
 *
 * @file       OSGGeoTransformManipulator.hpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
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

#ifndef _H_OSGQTQUICK_OSGGEOTRANSFORMMANIPULATOR_H_
#define _H_OSGQTQUICK_OSGGEOTRANSFORMMANIPULATOR_H_

#include "../Export.hpp"
#include "OSGCameraManipulator.hpp"

#include <QObject>
#include <QVector3D>

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGGeoTransformManipulator : public OSGCameraManipulator {
    Q_OBJECT Q_PROPERTY(QVector3D attitude READ attitude WRITE setAttitude NOTIFY attitudeChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool clampToTerrain READ clampToTerrain WRITE setClampToTerrain NOTIFY clampToTerrainChanged)
    Q_PROPERTY(bool intoTerrain READ intoTerrain NOTIFY intoTerrainChanged)

    typedef OSGCameraManipulator Inherited;

public:
    explicit OSGGeoTransformManipulator(QObject *parent = 0);
    virtual ~OSGGeoTransformManipulator();

    QVector3D attitude() const;
    void setAttitude(QVector3D arg);

    QVector3D position() const;
    void setPosition(QVector3D arg);

    bool clampToTerrain() const;
    void setClampToTerrain(bool arg);

    bool intoTerrain() const;

signals:
    void attitudeChanged(QVector3D arg);
    void positionChanged(QVector3D arg);
    void clampToTerrainChanged(bool arg);
    void intoTerrainChanged(bool arg);

private:
    struct Hidden;
    Hidden *const h;

    virtual void update();
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGGEOTRANSFORMMANIPULATOR_H_
