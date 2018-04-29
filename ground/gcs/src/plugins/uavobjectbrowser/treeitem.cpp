/**
 ******************************************************************************
 *
 * @file       treeitem.cpp
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

#include "treeitem.h"

#include <QDebug>

/* Constructor */
HighlightManager::HighlightManager()
{
    // Initialize the timer and connect it to the callback
    m_expirationTimer.setTimerType(Qt::PreciseTimer);
    m_expirationTimer.setSingleShot(true);
    connect(&m_expirationTimer, &QTimer::timeout, this, &HighlightManager::checkItemsExpired);
}

/*
 * Called to add item to list. Item is only added if absent.
 * Returns true if item was added, otherwise false.
 */
bool HighlightManager::add(TreeItem *item)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Check so that the item isn't already in the list
    if (!m_items.contains(item)) {
        m_items.insert(item);
        emit updateHighlight(item);
        return true;
    }
    return false;
}

/*
 * Called to remove item from list.
 * Returns true if item was removed, otherwise false.
 */
bool HighlightManager::remove(TreeItem *item)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Remove item and return result
    const bool removed = m_items.remove(item);

    if (removed) {
        emit updateHighlight(item);
    }
    return removed;
}

/*
 * Called to remove item from list.
 * Will not emit a signal. Called when destroying an item
 * Returns true if item was removed, otherwise false.
 */
bool HighlightManager::reset(TreeItem *item)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Remove item and return result
    return m_items.remove(item);
}

bool HighlightManager::startTimer(QTime expirationTime)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    if (!m_expirationTimer.isActive()) {
        int msec = QTime::currentTime().msecsTo(expirationTime);
        // qDebug() << "start" << msec;
        m_expirationTimer.start((msec < 10) ? 10 : msec);
        return true;
    }
    return false;
}


void HighlightManager::reset()
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    m_expirationTimer.stop();

    m_items.clear();
}

/*
 * Callback called periodically by the timer.
 * This method checks for expired highlights and
 * removes them if they are expired.
 * Expired highlights are restored.
 */
void HighlightManager::checkItemsExpired()
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Get a mutable iterator for the list
    QMutableSetIterator<TreeItem *> iter(m_items);

    // This is the timestamp to compare with
    QTime now = QTime::currentTime();
    QTime next;

    // Loop over all items, check if they expired.
    while (iter.hasNext()) {
        TreeItem *item = iter.next();
        if (item->getHighlightExpires() <= now) {
            // expired, call removeHighlight
            item->resetHighlight();

            // Remove from list since it is restored.
            iter.remove();

            emit updateHighlight(item);
        } else {
            // not expired, check if next to expire
            if (!next.isValid() || (next > item->getHighlightExpires())) {
                next = item->getHighlightExpires();
            }
        }
    }
    if (next.isValid()) {
        int msec = QTime::currentTime().msecsTo(next);
        // qDebug() << "restart" << msec;
        m_expirationTimer.start((msec < 10) ? 10 : msec);
    }
}

int TreeItem::m_highlightTimeMs = 300;

TreeItem::TreeItem(const QList<QVariant> &data) :
    m_itemData(data),
    m_parentItem(0),
    m_changed(false),
    m_highlighted(false),
    m_highlightManager(0)
{}

TreeItem::TreeItem(const QVariant &data) :
    m_parentItem(0),
    m_changed(false),
    m_highlighted(false),
    m_highlightManager(0)
{
    m_itemData << data << "" << "";
}

TreeItem::~TreeItem()
{
    if (m_highlightManager) {
        m_highlightManager->reset(this);
    }
    qDeleteAll(m_childItems);
}

void TreeItem::setParentItem(TreeItem *parentItem)
{
    if (m_parentItem) {
        m_parentItem->removeChild(this, false);
    }
    m_parentItem = parentItem;
}

void TreeItem::appendChild(TreeItem *childItem)
{
    m_childItems.append(childItem);
    childItem->setParentItem(this);
}

void TreeItem::insertChild(TreeItem *childItem, int index)
{
    m_childItems.insert(index, childItem);
    childItem->setParentItem(this);
}

void TreeItem::removeChild(TreeItem *childItem, bool reparent)
{
    m_childItems.removeOne(childItem);
    if (reparent) {
        childItem->setParentItem(0);
    }
}

TreeItem *TreeItem::child(int index) const
{
    return m_childItems.value(index);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));
    }

    return 0;
}

int TreeItem::columnCount() const
{
    return m_itemData.count();
}

void TreeItem::setDescription(QString desc)
{
    // Split around 40 characters
    int idx = desc.indexOf(" ", 40);

    desc.insert(idx, QString("<br>"));
    desc.remove("@Ref", Qt::CaseInsensitive);
    m_description = desc;
}

QVariant TreeItem::data(int column) const
{
    return m_itemData.value(column);
}

void TreeItem::setData(QVariant value, int column)
{
    m_itemData.replace(column, value);
}

void TreeItem::update(const QTime &ts)
{
    foreach(TreeItem * child, children()) {
        child->update(ts);
    }
}

void TreeItem::apply()
{
    foreach(TreeItem * child, children()) {
        child->apply();
    }
}

/*
 * Called after a value has changed to trigger highlighting of tree item.
 */
void TreeItem::setHighlighted(bool highlighted, const QTime &ts)
{
    m_changed = false;
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        if (highlighted) {
            // Add to highlight manager
            m_highlightManager->add(this);
            // Update expiration timeout
            m_highlightExpires = ts.addMSecs(m_highlightTimeMs);
            // start expiration timer if necessary
            m_highlightManager->startTimer(m_highlightExpires);
        } else {
            m_highlightManager->remove(this);
        }
    }

    // If we have a parent, call recursively to update highlight status of parents.
    // This will ensure that the root of a leaf that is changed is also highlighted.
    // Only updates that really changes values will trigger highlight of parents.
    if (m_parentItem) {
        m_parentItem->setHighlighted(highlighted, ts);
    }
}

void TreeItem::resetHighlight()
{
    m_highlighted = false;
}

void TreeItem::setHighlightManager(HighlightManager *mgr)
{
    m_highlightManager = mgr;
}

QTime TreeItem::getHighlightExpires() const
{
    return m_highlightExpires;
}

int TreeItem::childIndex(QString name) const
{
    for (int i = 0; i < childCount(); ++i) {
        if (name == child(i)->data(0).toString()) {
            return i;
        }
    }
    return -1;
}

TreeItem *TreeItem::childByName(QString name) const
{
    int index = childIndex(name);

    return (index >= 0) ? m_childItems[index] : 0;
}

int TreeItem::insertionIndex(TreeItem *item) const
{
    QString name = item->data(0).toString();

    for (int i = 0; i < childCount(); ++i) {
        if (name < child(i)->data(0).toString()) {
            return i;
        }
    }
    return childCount();
}

int TreeItem::maxHexStringLength(UAVObjectField::FieldType type)
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

QVariant ArrayFieldTreeItem::data(int column) const
{
    if (column == 1) {
        if (m_field->getType() == UAVObjectField::UINT8 && m_field->getUnits().toLower() == "char") {
            QString dataString;
            dataString.reserve(2 + m_field->getNumElements());
            dataString.append("'");
            for (uint i = 0; i < m_field->getNumElements(); ++i) {
                dataString.append(m_field->getValue(i).toChar());
            }
            dataString.append("'");
            return dataString;
        } else if (m_field->getUnits().toLower() == "hex") {
            QString dataString;
            int len = TreeItem::maxHexStringLength(m_field->getType());
            QChar fillChar('0');
            dataString.reserve(2 + (len + 1) * m_field->getNumElements());
            dataString.append("{");
            for (uint i = 0; i < m_field->getNumElements(); ++i) {
                if (i > 0) {
                    dataString.append(' ');
                }
                bool ok;
                uint value  = m_field->getValue(i).toUInt(&ok);
                QString str = QString("%1").arg(value, len, 16, fillChar);
                str = str.toUpper();
                dataString.append(str);
            }
            dataString.append("}");
            return dataString;
        }
    }
    return TreeItem::data(column);
}
