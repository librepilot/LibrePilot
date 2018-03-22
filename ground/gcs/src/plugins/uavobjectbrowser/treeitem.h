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

#include <QList>
#include <QMap>
#include <QVariant>
#include <QTime>
#include <QTimer>
#include <QObject>

class TreeItem;

/*
 * Small utility class that handles the higlighting of
 * tree grid items.
 * Basicly it maintains all items due to be restored to
 * non highlighted state in a linked list.
 * A timer traverses this list periodically to find out
 * if any of the items should be restored. All items are
 * updated with an expiration timestamp when they expires.
 * An item that is beeing restored is removed from the
 * list and its removeHighlight() method is called. Items
 * that are not expired are left in the list til next time.
 * Items that are updated during the expiration time are
 * left untouched in the list. This reduces unwanted emits
 * of signals to the repaint/update function.
 */
class HighlightManager : public QObject {
    Q_OBJECT
public:
    HighlightManager();

    // This is called when an item has been set to
    // highlighted = true.
    bool add(TreeItem *itemToAdd);

    // This is called when an item is set to highlighted = false;
    bool remove(TreeItem *itemToRemove);

    bool startTimer(QTime time);

    void reset();

signals:
    void updateHighlight(TreeItem *item);

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

class TreeItem {
public:
    static const int TITLE_COLUMN = 0;
    static const int DATA_COLUMN  = 1;

    TreeItem(const QList<QVariant> &data, TreeItem *parentItem = 0);
    TreeItem(const QVariant &data, TreeItem *parentItem = 0);
    virtual ~TreeItem();

    void appendChild(TreeItem *child);
    void insertChild(TreeItem *child);

    TreeItem *child(int index) const;
    QList<TreeItem *> children() const
    {
        return m_childItems;
    }
    int childCount() const;
    int columnCount() const;
    virtual QVariant data(int column = 1) const;
    QString description() const
    {
        return m_description;
    }
    void setDescription(QString d)
    {
        // Split around 40 characters
        int idx = d.indexOf(" ", 40);

        d.insert(idx, QString("<br>"));
        d.remove("@Ref", Qt::CaseInsensitive);
        m_description = d;
    }
    // only column 1 (TreeItem::dataColumn) is changed with setData currently
    // other columns are initialized in constructor
    virtual void setData(QVariant value, int column = 1);
    int row() const;
    TreeItem *parentItem() const
    {
        return m_parentItem;
    }
    virtual bool isEditable() const
    {
        return false;
    }
    virtual void update();
    virtual void apply();

    bool changed() const
    {
        return m_changed;
    }

    void setChanged(bool changed)
    {
        m_changed = changed;
    }

    bool isHighlighted() const
    {
        return m_highlighted;
    }

    void setHighlighted(bool highlighted);

    static void setHighlightTime(int time)
    {
        m_highlightTimeMs = time;
    }

    QTime getHighlightExpires() const;

    void resetHighlight();

    void setHighlightManager(HighlightManager *mgr);

    virtual bool isKnown() const
    {
        if (m_parentItem) {
            return m_parentItem->isKnown();
        }
        return true;
    }

    int nameIndex(QString name) const
    {
        for (int i = 0; i < childCount(); ++i) {
            if (name < child(i)->data(0).toString()) {
                return i;
            }
        }
        return childCount();
    }

    TreeItem *findChildByName(QString name) const
    {
        foreach(TreeItem * child, m_childItems) {
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

private:
    static int m_highlightTimeMs;

    QList<TreeItem *> m_childItems;
    // m_data contains: [0] property name, [1] value, [2] unit
    QList<QVariant> m_itemData;
    TreeItem *m_parentItem;

    QString m_description;

    bool m_changed;

    bool m_highlighted;
    QTime m_highlightExpires;
    HighlightManager *m_highlightManager;
};

class DataObjectTreeItem;
class MetaObjectTreeItem;

class TopTreeItem : public TreeItem {
public:
    TopTreeItem(const QList<QVariant> &data, TreeItem *parentItem) :
        TreeItem(data, parentItem)
    {}
    TopTreeItem(const QVariant &data, TreeItem *parentItem) :
        TreeItem(data, parentItem)
    {}

    void addObjectTreeItem(quint32 objectId, DataObjectTreeItem *oti)
    {
        m_objectTreeItemsPerObjectIds[objectId] = oti;
    }

    DataObjectTreeItem *findDataObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_objectTreeItemsPerObjectIds.value(objectId, 0);
    }

    void addMetaObjectTreeItem(quint32 objectId, MetaObjectTreeItem *oti)
    {
        m_metaObjectTreeItemsPerObjectIds[objectId] = oti;
    }

    MetaObjectTreeItem *findMetaObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_metaObjectTreeItemsPerObjectIds.value(objectId, 0);
    }

private:
    QHash<quint32, DataObjectTreeItem *> m_objectTreeItemsPerObjectIds;
    QHash<quint32, MetaObjectTreeItem *> m_metaObjectTreeItemsPerObjectIds;
};

class ObjectTreeItem : public TreeItem {
public:
    ObjectTreeItem(const QList<QVariant> &data, UAVObject *object, TreeItem *parentItem) :
        TreeItem(data, parentItem), m_obj(object)
    {
        setDescription(m_obj->getDescription());
    }
    ObjectTreeItem(const QVariant &data, UAVObject *object, TreeItem *parentItem) :
        TreeItem(data, parentItem), m_obj(object)
    {
        setDescription(m_obj->getDescription());
    }

    UAVObject *object() const
    {
        return m_obj;
    }

private:
    UAVObject *m_obj;
};

class MetaObjectTreeItem : public ObjectTreeItem {
public:
    MetaObjectTreeItem(UAVObject *object, const QList<QVariant> &data, TreeItem *parentItem) :
        ObjectTreeItem(data, object, parentItem)
    {}
    MetaObjectTreeItem(UAVObject *object, const QVariant &data, TreeItem *parentItem) :
        ObjectTreeItem(data, object, parentItem)
    {}
};

class DataObjectTreeItem : public ObjectTreeItem {
public:
    DataObjectTreeItem(const QList<QVariant> &data, UAVObject *object, TreeItem *parentItem) :
        ObjectTreeItem(data, object, parentItem)
    {}
    DataObjectTreeItem(const QVariant &data, UAVObject *object, TreeItem *parentItem) :
        ObjectTreeItem(data, object, parentItem)
    {}

    virtual void apply()
    {
        foreach(TreeItem * child, children()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->apply();
            }
        }
    }

    virtual void update()
    {
        foreach(TreeItem * child, children()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->update();
            }
        }
    }

    virtual bool isKnown() const
    {
        return !object()->isSettingsObject() || object()->isKnown();
    }
};

class InstanceTreeItem : public DataObjectTreeItem {
public:
    InstanceTreeItem(UAVObject *object, const QList<QVariant> &data, TreeItem *parentItem) :
        DataObjectTreeItem(data, object, parentItem)
    {}
    InstanceTreeItem(UAVObject *object, const QVariant &data, TreeItem *parentItem) :
        DataObjectTreeItem(data, object, parentItem)
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
public:
    ArrayFieldTreeItem(UAVObjectField *field, const QList<QVariant> &data, TreeItem *parentItem) :
        TreeItem(data, parentItem), m_field(field)
    {}
    ArrayFieldTreeItem(UAVObjectField *field, const QVariant &data, TreeItem *parentItem) :
        TreeItem(data, parentItem), m_field(field)
    {}

    QVariant data(int column) const;

private:
    UAVObjectField *m_field;
};

#endif // TREEITEM_H
