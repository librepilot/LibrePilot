/**
 ******************************************************************************
 *
 * @file       OSGGroup.cpp
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

#include "OSGGroup.hpp"

#include <osg/Group>

#include <QQmlListProperty>
#include <QDebug>

namespace osgQtQuick {
struct OSGGroup::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGGroup *parent) : QObject(parent), self(parent)
    {
        group = new osg::Group;
    }

    OSGGroup *self;

    osg::ref_ptr<osg::Group> group;

    QList<OSGNode *> children;

    QMap<OSGNode *, osg::Node *> cache;

    static void append_child(QQmlListProperty<OSGNode> *list, OSGNode *child)
    {
        OSGGroup *group = qobject_cast<OSGGroup *>(list->object);

        if (group && child) {
            group->h->cache[child] = child->node();
            group->h->children.append(child);
            if (child->node()) {
                group->h->group->addChild(child->node());
                emit group->nodeChanged(group->h->group);
            }
            QObject::connect(child, SIGNAL(nodeChanged(osg::Node *)), group->h, SLOT(onNodeChanged(osg::Node *)));
        }
    }

    static int count_child(QQmlListProperty<OSGNode> *list)
    {
        OSGGroup *group = qobject_cast<OSGGroup *>(list->object);

        if (group) {
            return group->h->children.size();
        }

        return 0;
    }

    static OSGNode *at_child(QQmlListProperty<OSGNode> *list, int index)
    {
        OSGGroup *group = qobject_cast<OSGGroup *>(list->object);

        if (group && index >= 0 && index < group->h->children.size()) {
            return group->h->children[index];
        }

        return 0;
    }

    static void clear_child(QQmlListProperty<OSGNode> *list)
    {
        OSGGroup *group = qobject_cast<OSGGroup *>(list->object);

        if (group) {
            while (!group->h->children.isEmpty()) {
                OSGNode *node = group->h->children.takeLast();
                if (node->parent() == group) {
                    node->deleteLater();
                }
                if (!node->parent()) {
                    node->deleteLater();
                }
            }
            group->h->group->removeChild(0, group->h->group->getNumChildren());
            group->h->cache.clear();
        }
    }

public slots:
    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGGroup::nodeChanged" << node;
        OSGNode *obj = qobject_cast<OSGNode *>(sender());
        if (obj) {
            osg::Node *cacheNode = cache.value(obj, NULL);
            if (cacheNode) {
                group->removeChild(cacheNode);
            }
            if (node) {
                group->addChild(node);
            }
            cache[obj] = node;
            emit self->nodeChanged(group.get());
        }
    }
};

OSGGroup::OSGGroup(QObject *parent) :
    OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGGroup::OSGGroup";
    setNode(h->group.get());
}

OSGGroup::~OSGGroup()
{
    qDebug() << "OSGGroup::~OSGGroup";
}

QQmlListProperty<OSGNode> OSGGroup::children()
{
    return QQmlListProperty<OSGNode>(this, 0,
                                     &Hidden::append_child,
                                     &Hidden::count_child,
                                     &Hidden::at_child,
                                     &Hidden::clear_child);
}

void OSGGroup::attach(osgViewer::View *view)
{
    // qDebug() << "OSGGroup::attach " << view;
    QListIterator<OSGNode *> i(h->children);
    while (i.hasNext()) {
        OSGNode *node = i.next();
        // qDebug() << "OSGGroup::attach - child" << node;
        node->attach(view);
    }
}

void OSGGroup::detach(osgViewer::View *view)
{
    // qDebug() << "OSGGroup::detach " << view;
    QListIterator<OSGNode *> i(h->children);
    while (i.hasNext()) {
        OSGNode *node = i.next();
        // qDebug() << "OSGGroup::detach - child" << node;
        node->detach(view);
    }
}
} // namespace osgQtQuick

#include "OSGGroup.moc"
