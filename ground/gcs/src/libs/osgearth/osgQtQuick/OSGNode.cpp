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

#include <QDebug>

namespace osgQtQuick {
class OSGNode;
class Hidden;

struct OSGNode::NodeUpdateCallback : public osg::NodeCallback {
public:
    NodeUpdateCallback(OSGNode::Hidden *h) : h(h)
    {}

    void operator()(osg::Node *node, osg::NodeVisitor *nv);

private:
    OSGNode::Hidden *const h;
};

struct OSGNode::Hidden : public QObject {
    Q_OBJECT

    friend class OSGNode;

private:
    OSGNode *const self;

    osg::ref_ptr<osg::Node> node;

    osg::ref_ptr<osg::NodeCallback> nodeUpdateCallback;

    bool complete;
    int  dirty;

public:
    Hidden(OSGNode *self) : QObject(self), self(self), complete(false), dirty(0)
    {}

    bool isDirty(int mask) const
    {
        return (dirty & mask) != 0;
    }

    void setDirty(int mask)
    {
        if (!dirty) {
            if (node) {
                if (!nodeUpdateCallback.valid()) {
                    // lazy creation
                    nodeUpdateCallback = new NodeUpdateCallback(this);
                }
                node->addUpdateCallback(nodeUpdateCallback.get());
            }
        }
        dirty |= mask;
    }

    void clearDirty()
    {
        if (node && nodeUpdateCallback.valid()) {
            node->removeUpdateCallback(nodeUpdateCallback.get());
        }
        dirty = 0;
    }

    void update()
    {
        if (dirty) {
            self->update();
        }
        clearDirty();
    }

    bool acceptNode(osg::Node *aNode)
    {
        if (node == aNode) {
            return false;
        }
        if (node && dirty) {
            node->setUpdateCallback(NULL);
        }
        node = aNode;
        if (node) {
            if (dirty) {
                if (!nodeUpdateCallback.valid()) {
                    // lazy creation
                    nodeUpdateCallback = new NodeUpdateCallback(this);
                }
                node->setUpdateCallback(nodeUpdateCallback);
            }
        }
        return true;
    }
};

/* struct OSGNode::NodeUpdateCallback */

void OSGNode::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    // qDebug() << "OSGNode::NodeUpdateCallback";
    nv->traverse(*node);
    h->update();
}

/* class OSGNode */

OSGNode::OSGNode(QObject *parent) : QObject(parent), QQmlParserStatus(), h(new Hidden(this))
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
        emitNodeChanged();
    }
}

bool OSGNode::isDirty() const
{
    return h->isDirty(0xFFFFFFFF);
}

bool OSGNode::isDirty(int mask) const
{
    return h->isDirty(mask);
}

void OSGNode::setDirty(int mask)
{
    h->setDirty(mask);
}

void OSGNode::clearDirty()
{
    h->clearDirty();
}

void OSGNode::classBegin()
{
    // qDebug() << "OSGNode::classBegin" << this;
}

void OSGNode::componentComplete()
{
    qDebug() << "OSGNode::componentComplete" << this;
    update();
    clearDirty();
    h->complete = h->node.valid();
    if (!isComponentComplete()) {
        qWarning() << "OSGNode::componentComplete - not complete !!!" << this;
    }
}

bool OSGNode::isComponentComplete()
{
    return h->complete;
}


void OSGNode::emitNodeChanged()
{
    if (isComponentComplete()) {
        emit nodeChanged(node());
    }
}

void OSGNode::update()
{}
} // namespace osgQtQuick

#include "OSGNode.moc"
