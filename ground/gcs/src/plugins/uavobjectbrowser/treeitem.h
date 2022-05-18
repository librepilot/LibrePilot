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
#include "uavdataobject.h"
#include "uavmetaobject.h"
#include "uavobjectfield.h"

#include <QList>
#include <QSet>
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

    // This is called when an item is set to highlighted = true.
    bool add(TreeItem *item);

    // This is called when an item is set to highlighted = false;
    bool remove(TreeItem *item);

    // This is called when an item is destroyed
    bool reset(TreeItem *item);

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

    TreeItem(const QList<QVariant> &data);
    TreeItem(const QVariant &data);
    virtual ~TreeItem();

    TreeItem *parentItem() const
    {
        return m_parentItem;
    }
    void setParentItem(TreeItem *parentItem);

    void appendChild(TreeItem *childItem);
    void insertChild(TreeItem *childItem, int index);
    void removeChild(TreeItem *childItem, bool reparent = true);

    TreeItem *child(int index) const;

    QList<TreeItem *> children() const
    {
        return m_childItems;
    }

    int childCount() const;

    int columnCount() const;

    QString description() const
    {
        return m_description;
    }

    void setDescription(QString desc);

    virtual QVariant data(int column = 1) const;

    virtual void setData(QVariant value, int column = 1);

    int row() const;

    virtual bool isEditable() const
    {
        return false;
    }

    virtual void update(const QTime &ts);
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

    void setHighlighted(bool highlighted, const QTime &ts);

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

    int childIndex(QString name) const;

    TreeItem *childByName(QString name) const;

    int insertionIndex(TreeItem *item) const;

    static int maxHexStringLength(UAVObjectField::FieldType type);

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

class TopTreeItem : public TreeItem {
public:
    TopTreeItem(const QList<QVariant> &data) :
        TreeItem(data)
    {}
    TopTreeItem(const QVariant &data) :
        TreeItem(data)
    {}
};

class ObjectTreeItem : public TreeItem {
public:
    ObjectTreeItem(UAVObject *object, const QList<QVariant> &data) :
        TreeItem(data), m_obj(object)
    {
        setDescription(m_obj->getDescription());
    }
    ObjectTreeItem(UAVObject *object, const QVariant &data) :
        TreeItem(data), m_obj(object)
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
    MetaObjectTreeItem(UAVMetaObject *object, const QList<QVariant> &data) :
        ObjectTreeItem(object, data)
    {}
    MetaObjectTreeItem(UAVMetaObject *object, const QVariant &data) :
        ObjectTreeItem(object, data)
    {}

    UAVMetaObject *metaObject() const
    {
        return static_cast<UAVMetaObject *>(object());
    }
};

class DataObjectTreeItem : public ObjectTreeItem {
public:
    DataObjectTreeItem(UAVDataObject *object, const QList<QVariant> &data) :
        ObjectTreeItem(object, data)
    {}
    DataObjectTreeItem(UAVDataObject *object, const QVariant &data) :
        ObjectTreeItem(object, data)
    {}

    UAVDataObject *dataObject() const
    {
        return static_cast<UAVDataObject *>(object());
    }

    virtual void apply()
    {
        foreach(TreeItem * child, children()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->apply();
            }
        }
    }

    virtual void update(const QTime &ts)
    {
        foreach(TreeItem * child, children()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);

            if (!metaChild) {
                child->update(ts);
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
    InstanceTreeItem(UAVDataObject *object, const QList<QVariant> &data) :
        DataObjectTreeItem(object, data)
    {}
    InstanceTreeItem(UAVDataObject *object, const QVariant &data) :
        DataObjectTreeItem(object, data)
    {}

    virtual void update(const QTime &ts)
    {
        TreeItem::update(ts);
    }

    virtual void apply()
    {
        TreeItem::apply();
    }
};

class ArrayFieldTreeItem : public TreeItem {
public:
    ArrayFieldTreeItem(UAVObjectField *field, const QList<QVariant> &data) :
        TreeItem(data), m_field(field)
    {}
    ArrayFieldTreeItem(UAVObjectField *field, const QVariant &data) :
        TreeItem(data), m_field(field)
    {}

    QVariant data(int column) const;

private:
    UAVObjectField *m_field;
};

#endif // TREEITEM_H
