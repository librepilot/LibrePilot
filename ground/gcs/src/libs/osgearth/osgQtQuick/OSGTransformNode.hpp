/**
 ******************************************************************************
 *
 * @file       OSGTransformNode.hpp
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

#ifndef _H_OSGQTQUICK_TRANSFORMNODE_H_
#define _H_OSGQTQUICK_TRANSFORMNODE_H_

#include "Export.hpp"
#include "OSGGroup.hpp"

#include <QVector3D>

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGTransformNode : public OSGGroup {
    Q_OBJECT Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(QVector3D attitude READ attitude WRITE setAttitude NOTIFY attitudeChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)

    typedef OSGGroup Inherited;

public:
    OSGTransformNode(QObject *parent = 0);
    virtual ~OSGTransformNode();

    QVector3D scale() const;
    void setScale(QVector3D arg);

    QVector3D attitude() const;
    void setAttitude(QVector3D arg);

    QVector3D position() const;
    void setPosition(QVector3D arg);

signals:
    void scaleChanged(QVector3D arg);
    void attitudeChanged(QVector3D arg);
    void positionChanged(QVector3D arg);

protected:
    virtual osg::Node *createNode();
    virtual void updateNode();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_TRANSFORMNODE_H_
