/**
 ******************************************************************************
 *
 * @file       OSGCubeNode.cpp
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

#include "OSGCubeNode.hpp"

#include <osg/Geode>
#include <osg/Group>
#include <osg/Shape>
#include <osg/ShapeDrawable>

#include <QDebug>

namespace osgQtQuick {
struct OSGCubeNode::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGCubeNode *parent) : QObject(parent), self(parent) {}

    void realize()
    {
        qDebug() << "OSGCubeNode::realize";

        // Declare a group to act as root node of a scene:
        // osg::Group* root = new osg::Group();

        // Declare a box class (derived from shape class) instance
        // This constructor takes an osg::Vec3 to define the center
        // and a float to define the height, width and depth.
        // (an overloaded constructor allows you to specify unique
        // height, width and height values.)
        osg::Box *unitCube = new osg::Box(osg::Vec3(0, 0, 0), 1.0f);

        // Declare an instance of the shape drawable class and initialize
        // it with the unitCube shape we created above.
        // This class is derived from 'drawable' so instances of this
        // class can be added to Geode instances.
        osg::ShapeDrawable *unitCubeDrawable = new osg::ShapeDrawable(unitCube);

        // Declare a instance of the geode class:
        osg::Geode *basicShapesGeode = new osg::Geode();

        // Add the unit cube drawable to the geode:
        basicShapesGeode->addDrawable(unitCubeDrawable);

        // Add the geode to the scene:
        self->setNode(basicShapesGeode);
    }

    OSGCubeNode *self;
};

// TODO turn into generic shape node...
// see http://trac.openscenegraph.org/projects/osg//wiki/Support/Tutorials/TransformsAndStates
OSGCubeNode::OSGCubeNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGCubeNode::OSGCubeNode";
    h->realize();
}

OSGCubeNode::~OSGCubeNode()
{
    qDebug() << "OSGCubeNode::~OSGCubeNode";
}
} // namespace osgQtQuick

#include "OSGCubeNode.moc"
