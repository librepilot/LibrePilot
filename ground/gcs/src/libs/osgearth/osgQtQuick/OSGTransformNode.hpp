/**
 ******************************************************************************
 *
 * @file       OSGTransformNode.hpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
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
#include "OSGNode.hpp"

#include <QVector3D>

// TODO derive from OSGGroup...
namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGTransformNode : public OSGNode {
    Q_OBJECT
    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode *modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)

    Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(QVector3D rotate READ rotate WRITE setRotate NOTIFY rotateChanged)
    Q_PROPERTY(QVector3D translate READ translate WRITE setTranslate NOTIFY translateChanged)

public:
    OSGTransformNode(QObject *parent = 0);
    virtual ~OSGTransformNode();

    OSGNode *modelData();
    void setModelData(OSGNode *node);

    QVector3D scale() const;
    void setScale(QVector3D arg);

    QVector3D rotate() const;
    void setRotate(QVector3D arg);

    QVector3D translate() const;
    void setTranslate(QVector3D arg);

signals:
    void modelDataChanged(OSGNode *node);

    void scaleChanged(QVector3D arg);
    void rotateChanged(QVector3D arg);
    void translateChanged(QVector3D arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_TRANSFORMNODE_H_
