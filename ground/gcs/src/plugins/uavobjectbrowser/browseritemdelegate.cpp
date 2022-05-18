/**
 ******************************************************************************
 *
 * @file       browseritemdelegate.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#include "browseritemdelegate.h"
#include "fieldtreeitem.h"

BrowserItemDelegate::BrowserItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
    _sizeHint = QSpinBox().sizeHint();
}

QWidget *BrowserItemDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem & option,
                                           const QModelIndex &index) const
{
    Q_UNUSED(option)
    FieldTreeItem * item = static_cast<FieldTreeItem *>(index.data(Qt::UserRole).value<void *>());
    QWidget *editor = item->createEditor(parent);
    Q_ASSERT(editor);
    return editor;
}

void BrowserItemDelegate::setEditorData(QWidget *editor,
                                        const QModelIndex &index) const
{
    FieldTreeItem *item = static_cast<FieldTreeItem *>(index.data(Qt::UserRole).value<void *>());
    QVariant value = index.model()->data(index, Qt::EditRole);

    item->setEditorValue(editor, value);
}

void BrowserItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                       const QModelIndex &index) const
{
    FieldTreeItem *item = static_cast<FieldTreeItem *>(index.data(Qt::UserRole).value<void *>());
    QVariant value = item->getEditorValue(editor);

    bool ret = model->setData(index, value, Qt::EditRole);

    Q_ASSERT(ret);
}

void BrowserItemDelegate::updateEditorGeometry(QWidget *editor,
                                               const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

QSize BrowserItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return _sizeHint;
}
