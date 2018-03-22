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
bool HighlightManager::add(TreeItem *itemToAdd)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Check so that the item isn't already in the list
    if (!m_items.contains(itemToAdd)) {
        m_items.insert(itemToAdd);
        emit updateHighlight(itemToAdd);
        return true;
    }
    return false;
}

/*
 * Called to remove item from list.
 * Returns true if item was removed, otherwise false.
 */
bool HighlightManager::remove(TreeItem *itemToRemove)
{
    // Lock to ensure thread safety
    QMutexLocker locker(&m_mutex);

    // Remove item and return result
    const bool removed = m_items.remove(itemToRemove);

    if (removed) {
        emit updateHighlight(itemToRemove);
    }
    return removed;
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

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parentItem) :
    m_itemData(data),
    m_parentItem(parentItem),
    m_changed(false),
    m_highlighted(false),
    m_highlightManager(0)
{}

TreeItem::TreeItem(const QVariant &data, TreeItem *parentItem) :
    m_parentItem(parentItem),
    m_changed(false),
    m_highlighted(false),
    m_highlightManager(0)
{
    m_itemData << data << "" << "";
}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *child)
{
    m_childItems.append(child);
}

void TreeItem::insertChild(TreeItem *child)
{
    int index = nameIndex(child->data(0).toString());

    m_childItems.insert(index, child);
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

QVariant TreeItem::data(int column) const
{
    return m_itemData.value(column);
}

void TreeItem::setData(QVariant value, int column)
{
    m_itemData.replace(column, value);
}

void TreeItem::update()
{
    foreach(TreeItem * child, children()) {
        child->update();
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
void TreeItem::setHighlighted(bool highlighted)
{
    m_changed = false;
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        if (highlighted) {
            // Add to highlight manager
            m_highlightManager->add(this);
            // Update expiration timeout
            m_highlightExpires = QTime::currentTime().addMSecs(m_highlightTimeMs);
            // start expiration timer if necessary
            m_highlightManager->startTimer(m_highlightExpires);
        } else {
            m_highlightManager->remove(this);
        }
    }

    // If we have a parent, call recursively to update highlight status of parents.
    // This will ensure that the root of a leaf that is changed also is highlighted.
    // Only updates that really changes values will trigger highlight of parents.
    if (m_parentItem) {
        // FIXME: need to pass m_highlightExpires
        m_parentItem->setHighlighted(highlighted);
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
