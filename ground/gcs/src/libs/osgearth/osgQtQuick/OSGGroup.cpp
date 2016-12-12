/**
 ******************************************************************************
 *
 * @file       OSGGroup.cpp
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

#include "OSGGroup.hpp"

#include <osg/Group>

#include <QQmlListProperty>
#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Children = 1 << 0 };

struct OSGGroup::Hidden : public QObject {
    Q_OBJECT

private:
    OSGGroup * const self;

    QMap<OSGNode *, osg::Node *> cache;

    QList<OSGNode *> children;

public:
    Hidden(OSGGroup *self) : QObject(self), self(self)
    {}

    osg::Node *createNode()
    {
        return new osg::Group();
    }

    void appendChild(OSGNode *childNode)
    {
        // qDebug() << "OSGGroup::appendChild" << childNode;
        children.append(childNode);
        connect(childNode, &OSGNode::nodeChanged, this, &osgQtQuick::OSGGroup::Hidden::onChildNodeChanged, Qt::UniqueConnection);
        self->setDirty(Children);
    }

    int countChild() const
    {
        return children.size();
    }

    OSGNode *atChild(int index) const
    {
        if (index >= 0 && index < children.size()) {
            return children[index];
        }
        return 0;
    }

    void clearChild()
    {
        while (!children.isEmpty()) {
            OSGNode *node = children.takeLast();
            disconnect(node);
        }
        children.clear();
        cache.clear();
        self->setDirty(Children);
    }

    void updateChildren()
    {
        // qDebug() << "OSGGroup::updateChildren";

        osg::Group *group = static_cast<osg::Group *>(self->node());

        if (!group) {
            qWarning() << "OSGGroup::updateChildren - null group";
            return;
        }

        bool updated = false;
        unsigned int index = 0;

        QListIterator<OSGNode *> i(children);
        while (i.hasNext()) {
            OSGNode *childNode = i.next();
            // qDebug() << "OSGGroup::updateChildren - child" << childNode;
            if (childNode->node()) {
                if (index < group->getNumChildren()) {
                    updated |= group->replaceChild(group->getChild(index), childNode->node());
                } else {
                    updated |= group->addChild(childNode->node());
                }
                cache.insert(childNode, childNode->node());
                index++;
            }
        }
        // removing eventual left overs
        if (index < group->getNumChildren()) {
            updated |= group->removeChild(index, group->getNumChildren() - index);
        }
        // if (updated) {
        // self->emitNodeChanged();
        // }
    }

    /* QQmlListProperty<OSGNode> */

    static void append_child(QQmlListProperty<OSGNode> *list, OSGNode *childNode)
    {
        Hidden *h = qobject_cast<Hidden *>(list->object);

        h->appendChild(childNode);
    }

    static int count_child(QQmlListProperty<OSGNode> *list)
    {
        Hidden *h = qobject_cast<Hidden *>(list->object);

        return h->countChild();
    }

    static OSGNode *at_child(QQmlListProperty<OSGNode> *list, int index)
    {
        Hidden *h = qobject_cast<Hidden *>(list->object);

        return h->atChild(index);
    }

    static void clear_child(QQmlListProperty<OSGNode> *list)
    {
        Hidden *h = qobject_cast<Hidden *>(list->object);

        h->clearChild();
    }

private slots:
    void onChildNodeChanged(osg::Node *child)
    {
        // qDebug() << "OSGGroup::onChildNodeChanged" << node;

        osg::Group *group = static_cast<osg::Group *>(self->node());

        if (!group) {
            qWarning() << "OSGGroup::onChildNodeChanged - null group";
            return;
        }

        OSGNode *childNode = qobject_cast<OSGNode *>(sender());
        // qDebug() << "OSGGroup::onChildNodeChanged - child node" << obj;
        if (!childNode) {
            qWarning() << "OSGGroup::onChildNodeChanged - sender is not an OSGNode" << sender();
            return;
        }
        if (childNode->node() != child) {
            qWarning() << "OSGGroup::onChildNodeChanged - child node is not valid" << childNode;
            return;
        }
        bool updated = false;
        osg::Node *current = cache.value(childNode, NULL);
        if (current) {
            updated |= group->replaceChild(current, child);
        } else {
            // should not happen...
            qWarning() << "OSGGroup::onChildNodeChanged - child node is not a child" << childNode;
        }
        cache[childNode] = childNode->node();
        // if (updated) {
        // emit self->nodeChanged(group.get());
        // }
    }
};

/* class OSGGGroupNode */

OSGGroup::OSGGroup(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGGroup::~OSGGroup()
{
    delete h;
}

QQmlListProperty<OSGNode> OSGGroup::children() const
{
    return QQmlListProperty<OSGNode>(h, 0,
                                     &Hidden::append_child,
                                     &Hidden::count_child,
                                     &Hidden::at_child,
                                     &Hidden::clear_child);
}

osg::Node *OSGGroup::createNode()
{
    return h->createNode();
}

void OSGGroup::updateNode()
{
    Inherited::updateNode();

    if (isDirty(Children)) {
        h->updateChildren();
    }
}
} // namespace osgQtQuick

#include "OSGGroup.moc"
