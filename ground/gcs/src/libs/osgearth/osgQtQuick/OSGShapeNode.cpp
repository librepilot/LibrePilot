/**
 ******************************************************************************
 *
 * @file       OSGShapeNode.cpp
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

#include "OSGShapeNode.hpp"

#include "../shapeutils.h"

#include <osg/Geode>
#include <osg/Group>
#include <osg/Shape>
#include <osg/ShapeDrawable>

#include <QDebug>

namespace osgQtQuick {
struct OSGShapeNode::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGShapeNode *node) : QObject(node), self(node), shapeType(ShapeType::Sphere) {}

    void realize()
    {
        qDebug() << "OSGShapeNode::realize";

        osg::Node *node = NULL;

        switch (shapeType) {
        case ShapeType::Cube:
            node = ShapeUtils::createCube();
            break;
        case ShapeType::Sphere:
            node = ShapeUtils::createSphere(osg::Vec4(1, 0, 0, 1), 1.0);
            break;
        case ShapeType::Torus:
            node = ShapeUtils::createOrientatedTorus(0.8, 1.0);
            break;
        case ShapeType::Axis:
            node = ShapeUtils::create3DAxis();
            break;
        }

        // Add the node to the scene:
        self->setNode(node);
    }

    bool acceptShapeType(ShapeType::Enum type)
    {
        if (shapeType == type) {
            return false;
        }
        qDebug() << "OSGShapeNode::acceptShapeType" << type;

        shapeType = type;
        realize();

        return true;
    }

    OSGShapeNode    *self;

    ShapeType::Enum shapeType;
};

// TODO turn into generic shape node...
// see http://trac.openscenegraph.org/projects/osg//wiki/Support/Tutorials/TransformsAndStates
OSGShapeNode::OSGShapeNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGShapeNode::OSGShapeNode";
    h->realize();
}

OSGShapeNode::~OSGShapeNode()
{
    qDebug() << "OSGShapeNode::~OSGShapeNode";
    delete h;
}

ShapeType::Enum OSGShapeNode::shapeType() const
{
    return h->shapeType;
}

void OSGShapeNode::setShapeType(ShapeType::Enum type)
{
    if (h->acceptShapeType(type)) {
        emit shapeTypeChanged(shapeType());
    }
}
} // namespace osgQtQuick

#include "OSGShapeNode.moc"
