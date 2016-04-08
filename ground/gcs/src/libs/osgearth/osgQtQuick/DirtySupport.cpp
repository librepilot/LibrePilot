/**
 ******************************************************************************
 *
 * @file       DirtySupport.cpp
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

#include "DirtySupport.hpp"

#include <osg/Node>
#include <osg/NodeVisitor>

#include <QDebug>

namespace osgQtQuick {
class Hidden;

struct DirtySupport::NodeUpdateCallback : public osg::NodeCallback {
public:
    NodeUpdateCallback(DirtySupport::Hidden *h) : h(h)
    {}

    void operator()(osg::Node *node, osg::NodeVisitor *nv);

private:
    DirtySupport::Hidden *const h;
};

struct DirtySupport::Hidden {
private:
    DirtySupport *const self;

    osg::ref_ptr<osg::NodeCallback> nodeUpdateCallback;

    int dirtyFlags;

public:
    Hidden(DirtySupport *self) : self(self), dirtyFlags(0)
    {}

    bool isDirty(int mask) const
    {
        return (dirtyFlags & mask) != 0;
    }

    int dirty() const
    {
        return dirtyFlags;
    }

    void setDirty(int mask)
    {
        // qDebug() << "DirtySupport::setDirty" << mask;
        if (!dirtyFlags) {
            osg::Node *node = self->nodeToUpdate();
            if (node) {
                if (!nodeUpdateCallback.valid()) {
                    // lazy creation
                    nodeUpdateCallback = new NodeUpdateCallback(this);
                }
                node->addUpdateCallback(nodeUpdateCallback.get());
            } else {
                // qWarning() << "DirtySupport::setDirty - node to update is null";
            }
        }
        dirtyFlags |= mask;
    }

    void clearDirty()
    {
        osg::Node *node = self->nodeToUpdate();

        if (node && nodeUpdateCallback.valid()) {
            node->removeUpdateCallback(nodeUpdateCallback.get());
        }
        dirtyFlags = 0;
    }

    void update()
    {
        // qDebug() << "DirtySupport::update";
        if (dirtyFlags) {
            // qDebug() << "DirtySupport::update - updating...";
            self->update();
        }
        clearDirty();
    }
};

/* struct DirtySupport::NodeUpdateCallback */

void DirtySupport::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    // qDebug() << "DirtySupport::NodeUpdateCallback";
    nv->traverse(*node);
    h->update();
}

/* class DirtySupport */

DirtySupport::DirtySupport() : h(new Hidden(this))
{}

DirtySupport::~DirtySupport()
{
    delete h;
}

int DirtySupport::dirty() const
{
    return h->dirty();
}

bool DirtySupport::isDirty(int mask) const
{
    return h->isDirty(mask);
}

void DirtySupport::setDirty(int mask)
{
    h->setDirty(mask);
}

void DirtySupport::clearDirty()
{
    h->clearDirty();
}
} // namespace osgQtQuick
