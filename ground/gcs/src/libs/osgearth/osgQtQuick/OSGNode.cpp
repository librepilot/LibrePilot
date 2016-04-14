/**
 ******************************************************************************
 *
 * @file       OSGNode.cpp
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

#include "OSGNode.hpp"

#include "DirtySupport.hpp"

#include <osg/Node>

#include <QDebug>

namespace osgQtQuick {
class OSGNode;

struct OSGNode::Hidden : public QObject, public DirtySupport {
    Q_OBJECT

    friend class OSGNode;

private:
    OSGNode *const self;

    osg::ref_ptr<osg::Node> node;

    bool complete;

public:
    Hidden(OSGNode *self) : QObject(self), self(self), complete(false) /*, dirty(0)*/
    {}

    osg::Node *nodeToUpdate() const
    {
        return self->node();
    }

    void update()
    {
        return self->updateNode();
    }

    bool acceptNode(osg::Node *aNode)
    {
        if (node == aNode) {
            return false;
        }

        int flags = dirty();
        if (flags) {
            clearDirty();
        }
        node = aNode;
        if (node) {
            if (flags) {
                setDirty(flags);
            }
        }
        return true;
    }
};

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

osg::Node *OSGNode::createNode()
{
    return NULL;
}

void OSGNode::updateNode()
{}

void OSGNode::emitNodeChanged()
{
    if (h->complete) {
        emit nodeChanged(node());
    }
}

void OSGNode::classBegin()
{
    // qDebug() << "OSGNode::classBegin" << this;

    setNode(createNode());
}

void OSGNode::componentComplete()
{
    // qDebug() << "OSGNode::componentComplete" << this;

    updateNode();
    clearDirty();
    h->complete = true;
    if (!h->node.valid()) {
        qWarning() << "OSGNode::componentComplete - node is not valid!" << this;
    }
}
} // namespace osgQtQuick

#include "OSGNode.moc"
