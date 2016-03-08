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
#include <osg/NodeVisitor>

#include <qDebug>

namespace osgQtQuick {
class OSGNode;
class Hidden;

struct NodeUpdateCallback : public osg::NodeCallback {
public:
    NodeUpdateCallback(OSGNode::Hidden *h) : h(h) {}

    void operator()(osg::Node *node, osg::NodeVisitor *nv);

private:
    OSGNode::Hidden *const h;
};


struct OSGNode::Hidden /*: public QObject*/ {
    // Q_OBJECT

    friend class OSGNode;
public:
    Hidden(OSGNode *node) : /*QObject(node),*/ self(node), dirty(0)
    {}

    bool isDirty(int flag)
    {
        return (dirty && (1 << flag)) != 0;
    }

    void setDirty(int flag)
    {
        // qDebug() << "OSGNode::setDirty BEGIN";
        if (!dirty) {
            if (node) {
                if (!nodeUpdateCallback.valid()) {
                    // lazy creation
                    // qDebug() << "OSGNode::setDirty CREATE";
                    nodeUpdateCallback = new NodeUpdateCallback(this);
                }
                // qDebug() << "OSGNode::setDirty ADD" << node;
                node->setUpdateCallback(nodeUpdateCallback);
            }
        }
        dirty |= 1 << flag;
        // qDebug() << "OSGNode::setDirty DONE";
    }

    void clearDirty()
    {
        dirty = 0;
        if (node && nodeUpdateCallback.valid()) {
            // qDebug() << "OSGNode::clearDirty REMOVE CALLBACK";
            node->setUpdateCallback(NULL);
        }
    }

    void update()
    {
        // qDebug() << "OSGNode::update BEGIN";
        if (dirty) {
            // qDebug() << "OSGNode::update UPDATE";
            self->update();
        }
        clearDirty();
        // qDebug() << "OSGNode::update DONE";
    }

    bool acceptNode(osg::Node *aNode)
    {
        if (node == aNode) {
            return false;
        }
        if (node && dirty) {
            // qDebug() << "OSGNode::acceptNode REMOVE CALLBACK" << node;
            node->setUpdateCallback(NULL);
        }
        node = aNode;
        if (node) {
            if (dirty) {
                if (!nodeUpdateCallback.valid()) {
                    // lazy creation
                    // qDebug() << "OSGNode::acceptNode CREATE CALLBACK";
                    nodeUpdateCallback = new NodeUpdateCallback(this);
                }
                // qDebug() << "OSGNode::acceptNode ADD CALLBACK";
                node->setUpdateCallback(nodeUpdateCallback);
            }
        }
        return true;
    }

private:
    OSGNode *const self;

    osg::ref_ptr<osg::Node> node;

    osg::ref_ptr<osg::NodeCallback> nodeUpdateCallback;

    int dirty;
};

void NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    // qDebug() << "NodeUpdateCallback::";
    nv->traverse(*node);
    h->update();
}

OSGNode::OSGNode(QObject *parent) : QObject(parent), h(new Hidden(this))
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
    if (h->acceptNode(node)) {
        emit nodeChanged(node);
    }
}

bool OSGNode::isDirty()
{
    return h->isDirty(1);
}

bool OSGNode::isDirty(int flag)
{
    return h->isDirty(flag);
}

void OSGNode::setDirty()
{
    h->setDirty(1);
}

void OSGNode::setDirty(int flag)
{
    h->setDirty(flag);
}

void OSGNode::attach(osgViewer::View *view)
{}

void OSGNode::detach(osgViewer::View *view)
{}
} // namespace osgQtQuick
