/**
 ******************************************************************************
 *
 * @file       treeitem.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectBrowserPlugin UAVObject Browser Plugin
 * @{
 * @brief The UAVObject Browser gadget plugin
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

#ifndef TREEITEM_H
#define TREEITEM_H

#include "uavobject.h"
#include "uavmetaobject.h"
#include "uavobjectfield.h"
#include <QtCore/QList>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtCore/QDebug>

class TreeItem;

/*
 * Small utility class that handles the higlighting of
 * tree grid items.
 * Basicly it maintains all items due to be restored to
 * non highlighted state in a linked list.
 * A timer traverses this list periodically to find out
 * if any of the items should be restored. All items are
 * updated withan expiration timestamp when they expires.
 * An item that is beeing restored is removed from the
 * list and its removeHighlight() method is called. Items
 * that are not expired are left in the list til next time.
 * Items that are updated during the expiration time are
 * left untouched in the list. This reduces unwanted emits
 * of signals to the repaint/update function.
 */
class HighLightManager : public QObject {
    Q_OBJECT
public:
    // Constructor taking the checking interval in ms.
    HighLightManager(long checkingInterval);

    // This is called when an item has been set to
    // highlighted = true.
    bool add(TreeItem *itemToAdd);

    // This is called when an item is set to highlighted = false;
    bool remove(TreeItem *itemToRemove);

private slots:
    // Timer callback method.
    void checkItemsExpired();

private:
    // The timer checking highlight expiration.
    QTimer m_expirationTimer;

    // The collection holding all items due to be updated.
    QSet<TreeItem *> m_items;

    // Mutex to lock when accessing collection.
    QMutex m_mutex;
};

class TreeItem : public QObject {
    Q_OBJECT
public:
    static const int TITLE_COLUMN = 0;
    static const int DATA_COLUMN = 1;

    TreeItem(const QList<QVariant> &data, TreeItem *parent = 0);
    TreeItem(const QVariant &data, TreeItem *parent = 0);
    virtual ~TreeItem();

    void appendChild(TreeItem *child);
    void insertChild(TreeItem *child);

    TreeItem *getChild(int index);
    inline QList<TreeItem *> treeChildren() const
    {
        return m_children;
    }
    int childCount() const;
    int columnCount() const;
    virtual QVariant data(int column = 1) const;
    QString description()
    {
        return m_description;
    }
    void setDescription(QString d) // Split around 40 characters
    {
        int idx = d.indexOf(" ", 40);

        d.insert(idx, QString("<br>"));
        d.remove("@Ref", Qt::CaseInsensitive);
        m_description = d;
    }
    // only column 1 (TreeItem::dataColumn) is changed with setData currently
    // other columns are initialized in constructor
    virtual void setData(QVariant value, int column = 1);
    int row() const;
    TreeItem *parent()
    {
        return m_parent;
    }
    void setParentTree(TreeItem *parent)
    {
        m_parent = parent;
    }
    inline virtual bool isEditable()
    {
        return false;
    }
    virtual void update();
    virtual void apply();

    inline bool highlighted()
    {
        return m_highlight;
    }
    void setHighlight(bool highlight);
    static void setHighlightTime(int time)
    {
        m_highlightTimeMs = time;
    }

    inline bool changed()
    {
        return m_changed;
    }
    inline void setChanged(bool changed)
    {
        m_changed = changed;
    }

    virtual void setHighlightManager(HighLightManager *mgr);

    QTime getHiglightExpires();

    virtual void removeHighlight();

    int nameIndex(QString name)
    {
        for (int i = 0; i < childCount(); ++i) {
            if (name < getChild(i)->data(0).toString()) {
                return i;
            }
        }
        return childCount();
    }

    TreeItem *findChildByName(QString name)
    {
        foreach(TreeItem * child, m_children) {
            if (name == child->data(0).toString()) {
                return child;
            }
        }
        return 0;
    }

    static int maxHexStringLength(UAVObjectField::FieldType type)
    {
        switch (type) {
        case UAVObjectField::INT8:
            return 2;

        case UAVObjectField::INT16:
            return 4;

        case UAVObjectField::INT32:
            return 8;

        case UAVObjectField::UINT8:
            return 2;

        case UAVObjectField::UINT16:
            return 4;

        case UAVObjectField::UINT32:
            return 8;

        default:
            Q_ASSERT(false);
        }
        return 0;
    }
    void updateIsKnown(bool isKnown)
    {
        if (isKnown != this->isKnown()) {
            m_changed = false;
            foreach(TreeItem * child, m_children) {
                child->updateIsKnown(isKnown);
            }
            emit updateIsKnown(this);
        }
    }
    virtual bool isKnown()
    {
        return true;
    }

signals:
    void updateHighlight(TreeItem *item);
    void updateIsKnown(TreeItem *item);

private slots:

private:
    static int m_highlightTimeMs;
    QList<TreeItem *> m_children;

    // m_data contains: [0] property name, [1] value, [2] unit
    QList<QVariant> m_data;
    QString m_description;
    TreeItem *m_parent;
    bool m_highlight;
    bool m_changed;
    QTime m_highlightExpires;
    HighLightManager *m_highlightManager;
};

class DataObjectTreeItem;
class MetaObjectTreeItem;

class TopTreeItem : public TreeItem {
    Q_OBJECT
public:
    TopTreeItem(const QList<QVariant> &data, TreeItem *parent = 0) : TreeItem(data, parent) {}
    TopTreeItem(const QVariant &data, TreeItem *parent = 0) : TreeItem(data, parent) {}

    void addObjectTreeItem(quint32 objectId, DataObjectTreeItem *oti)
    {
        m_objectTreeItemsPerObjectIds[objectId] = oti;
    }

    DataObjectTreeItem *findDataObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_objectTreeItemsPerObjectIds.contains(objectId) ? m_objectTreeItemsPerObjectIds[objectId] : 0;
    }

    void addMetaObjectTreeItem(quint32 objectId, MetaObjectTreeItem *oti)
    {
        m_metaObjectTreeItemsPerObjectIds[objectId] = oti;
    }

    MetaObjectTreeItem *findMetaObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_metaObjectTreeItemsPerObjectIds.contains(objectId) ? m_metaObjectTreeItemsPerObjectIds[objectId] : 0;
    }

    QList<MetaObjectTreeItem *> getMetaObjectItems();

private:
    QHash<quint32, DataObjectTreeItem *> m_objectTreeItemsPerObjectIds;
    QHash<quint32, MetaObjectTreeItem *> m_metaObjectTreeItemsPerObjectIds;
};

class ObjectTreeItem : public TreeItem {
    Q_OBJECT
public:
    ObjectTreeItem(const QList<QVariant> &data, UAVObject *object, TreeItem *parent = 0) :
        TreeItem(data, parent), m_obj(object)
    {
        setDescription(m_obj->getDescription());
    }
    ObjectTreeItem(const QVariant &data, UAVObject *object, TreeItem *parent = 0) :
        TreeItem(data, parent), m_obj(object)
    {
        setDescription(m_obj->getDescription());
    }
    inline UAVObject *object()
    {
        return m_obj;
    }
    bool isKnown()
    {
        return !m_obj->isSettingsObject() || m_obj->isKnown();
    }

private:
    UAVObject *m_obj;
};

class MetaObjectTreeItem : public ObjectTreeItem {
    Q_OBJECT
public:
    MetaObjectTreeItem(UAVObject *object, const QList<QVariant> &data, TreeItem *parent = 0) :
        ObjectTreeItem(data, object, parent)
    {}
    MetaObjectTreeItem(UAVObject *object, const QVariant &data, TreeItem *parent = 0) :
        ObjectTreeItem(data, object, parent)
    {}

    bool isKnown()
    {
        return parent()->isKnown();
    }
};

class DataObjectTreeItem : public ObjectTreeItem {
    Q_OBJECT
public:
    DataObjectTreeItem(const QList<QVariant> &data, UAVObject *object, TreeItem *parent = 0) :
        ObjectTreeItem(data, object, parent) {}
    DataObjectTreeItem(const QVariant &data, UAVObject *object, TreeItem *parent = 0) :
        ObjectTreeItem(data, object, parent) {}
    virtual void apply()
    {
        foreach(TreeItem * child, treeChildren()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->apply();
            }
        }
    }
    virtual void update()
    {
        foreach(TreeItem * child, treeChildren()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->update();
            }
        }
    }
};

class InstanceTreeItem : public DataObjectTreeItem {
    Q_OBJECT
public:
    InstanceTreeItem(UAVObject *object, const QList<QVariant> &data, TreeItem *parent = 0) :
        DataObjectTreeItem(data, object, parent)
    {}
    InstanceTreeItem(UAVObject *object, const QVariant &data, TreeItem *parent = 0) :
        DataObjectTreeItem(data, object, parent)
    {}
    virtual void apply()
    {
        TreeItem::apply();
    }
    virtual void update()
    {
        TreeItem::update();
    }
};

class ArrayFieldTreeItem : public TreeItem {
    Q_OBJECT
public:
    ArrayFieldTreeItem(UAVObjectField *field, const QList<QVariant> &data, TreeItem *parent = 0) : TreeItem(data, parent), m_field(field)
    {}
    ArrayFieldTreeItem(UAVObjectField *field, const QVariant &data, TreeItem *parent = 0) : TreeItem(data, parent), m_field(field)
    {}
    QVariant data(int column) const;
    bool isKnown()
    {
        return parent()->isKnown();
    }

private:
    UAVObjectField *m_field;
};

#endif // TREEITEM_H
