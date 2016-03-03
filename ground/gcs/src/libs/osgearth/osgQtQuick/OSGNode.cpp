/**
 ******************************************************************************
 *
 * @file       OSGNode.cpp
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

#include "OSGNode.hpp"

#include <osg/Node>

#include <QListIterator>

namespace osgQtQuick {
struct OSGNode::Hidden {
    osg::ref_ptr<osg::Node> node;
};

OSGNode::OSGNode(QObject *parent) : QObject(parent), h(new Hidden)
{}

OSGNode::~OSGNode()
{
    delete h;
}

osg::Node *OSGNode::node() const
{
    return h->node.get();
}

void OSGNode::setNode(osg::Node *node)
{
    if (h->node.get() != node) {
        h->node = node;
        emit nodeChanged(node);
    }
}

void OSGNode::attach(osgViewer::View *view)
{
}

void OSGNode::detach(osgViewer::View *view)
{
}
} // namespace osgQtQuick
