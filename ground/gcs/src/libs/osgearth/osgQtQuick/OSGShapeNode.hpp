/**
 ******************************************************************************
 *
 * @file       OSGShapeNode.hpp
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

#ifndef _H_OSGQTQUICK_SHAPENODE_H_
#define _H_OSGQTQUICK_SHAPENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

namespace osgQtQuick {
class ShapeType : public QObject {
    Q_OBJECT
public:
    enum Enum { Cube, Sphere, Torus, Axis, Rhombicuboctahedron };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

class OSGQTQUICK_EXPORT OSGShapeNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(osgQtQuick::ShapeType::Enum shapeType READ shapeType WRITE setShapeType NOTIFY shapeTypeChanged)

    typedef OSGNode Inherited;

public:
    OSGShapeNode(QObject *parent = 0);
    virtual ~OSGShapeNode();

    ShapeType::Enum shapeType() const;
    void setShapeType(ShapeType::Enum);

signals:
    void shapeTypeChanged(ShapeType::Enum);

protected:
    virtual osg::Node *createNode();
    virtual void updateNode();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_SHAPENODE_H_
