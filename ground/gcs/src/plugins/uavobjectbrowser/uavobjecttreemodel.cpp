/**
 ******************************************************************************
 *
 * @file       uavobjecttreemodel.cpp
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

#include "uavobjecttreemodel.h"

#include "fieldtreeitem.h"
#include "uavobjectmanager.h"
#include "uavdataobject.h"
#include "uavmetaobject.h"
#include "uavobjectfield.h"
#include "extensionsystem/pluginmanager.h"

#include <QColor>

UAVObjectTreeModel::UAVObjectTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_highlightManager = new HighlightManager();
    connect(m_highlightManager, &HighlightManager::updateHighlight, this, &UAVObjectTreeModel::refreshHighlight);

    TreeItem::setHighlightTime(recentlyUpdatedTimeout());

    setupModelData();
}

UAVObjectTreeModel::~UAVObjectTreeModel()
{
    delete m_rootItem;
    delete m_highlightManager;
}

bool UAVObjectTreeModel::showCategories() const
{
    return m_settings.value("showCategories", false).toBool();
}

void UAVObjectTreeModel::setShowCategories(bool show)
{
    if (show == showCategories()) {
        return;
    }
    m_settings.setValue("showCategories", show);
    toggleCategoryItems();
}

bool UAVObjectTreeModel::showMetadata() const
{
    return m_settings.value("showMetadata", false).toBool();
}

void UAVObjectTreeModel::setShowMetadata(bool show)
{
    if (show == showMetadata()) {
        return;
    }
    m_settings.setValue("showMetadata", show);
    toggleMetaItems();
}

bool UAVObjectTreeModel::useScientificNotation()
{
    return m_settings.value("useScientificNotation", false).toBool();
}

void UAVObjectTreeModel::setUseScientificNotation(bool useScientificNotation)
{
    m_settings.setValue("useScientificNotation", useScientificNotation);
}

QColor UAVObjectTreeModel::unknownObjectColor() const
{
    return m_settings.value("unknownObjectColor", QColor(Qt::gray)).value<QColor>();
}

void UAVObjectTreeModel::setUnknownObjectColor(QColor color)
{
    m_settings.setValue("unknownObjectColor", color);
}

QColor UAVObjectTreeModel::recentlyUpdatedColor() const
{
    return m_settings.value("recentlyUpdatedColor", QColor(255, 230, 230)).value<QColor>();
}

void UAVObjectTreeModel::setRecentlyUpdatedColor(QColor color)
{
    m_settings.setValue("recentlyUpdatedColor", color);
}

QColor UAVObjectTreeModel::manuallyChangedColor() const
{
    return m_settings.value("manuallyChangedColor", QColor(230, 230, 255)).value<QColor>();
}

void UAVObjectTreeModel::setManuallyChangedColor(QColor color)
{
    m_settings.setValue("manuallyChangedColor", color);
}

int UAVObjectTreeModel::recentlyUpdatedTimeout() const
{
    return m_settings.value("recentlyUpdatedTimeout", 300).toInt();
}

void UAVObjectTreeModel::setRecentlyUpdatedTimeout(int timeout)
{
    m_settings.setValue("recentlyUpdatedTimeout", timeout);
    TreeItem::setHighlightTime(timeout);
}

bool UAVObjectTreeModel::onlyHighlightChangedValues() const
{
    return m_settings.value("onlyHighlightChangedValues", false).toBool();
}

void UAVObjectTreeModel::setOnlyHighlightChangedValues(bool highlight)
{
    m_settings.setValue("onlyHighlightChangedValues", highlight);
}

bool UAVObjectTreeModel::highlightTopTreeItems() const
{
    return m_settings.value("highlightTopTreeItems", false).toBool();
}

void UAVObjectTreeModel::setHighlightTopTreeItems(bool highlight)
{
    m_settings.setValue("highlightTopTreeItems", highlight);
}

void UAVObjectTreeModel::setupModelData()
{
    QList<QVariant> rootData;
    rootData << tr("Property") << tr("Value") << tr("Unit");
    m_rootItem        = new TreeItem(rootData);
    m_rootItem->setHighlightManager(m_highlightManager);

    m_settingsTree    = new TopTreeItem(tr("Settings"));
    m_settingsTree->setHighlightManager(m_highlightManager);

    m_nonSettingsTree = new TopTreeItem(tr("Data Objects"));
    m_nonSettingsTree->setHighlightManager(m_highlightManager);

    // tree item takes ownership of their children
    appendItem(m_rootItem, m_settingsTree);
    appendItem(m_rootItem, m_nonSettingsTree);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objManager);

    connect(objManager, &UAVObjectManager::newObject, this, &UAVObjectTreeModel::newObject, Qt::UniqueConnection);
    connect(objManager, &UAVObjectManager::newInstance, this, &UAVObjectTreeModel::newObject, Qt::UniqueConnection);

    QList< QList<UAVDataObject *> > objList = objManager->getDataObjects();
    foreach(QList<UAVDataObject *> list, objList) {
        foreach(UAVDataObject * obj, list) {
            addObject(obj);
        }
    }
}

void UAVObjectTreeModel::resetModelData()
{
    m_highlightManager->reset();

    emit beginResetModel();

    delete m_rootItem;
    m_rootItem = NULL;
    setupModelData();

    emit endResetModel();
}

void UAVObjectTreeModel::newObject(UAVObject *obj)
{
    UAVDataObject *dataObj = qobject_cast<UAVDataObject *>(obj);

    if (dataObj) {
        addObject(dataObj);
    }
}

void UAVObjectTreeModel::addObject(UAVDataObject *obj)
{
    connect(obj, &UAVDataObject::objectUpdated, this, &UAVObjectTreeModel::updateObject, Qt::UniqueConnection);
    connect(obj, &UAVDataObject::isKnownChanged, this, &UAVObjectTreeModel::updateIsKnown, Qt::UniqueConnection);
    if (obj->getInstID() == 0) {
        UAVMetaObject *metaObj = obj->getMetaObject();
        connect(metaObj, &UAVDataObject::objectUpdated, this, &UAVObjectTreeModel::updateObject, Qt::UniqueConnection);
    }
    if (obj->isSingleInstance()) {
        DataObjectTreeItem *dataObjectItem = createDataObject(obj);
        TreeItem *parentItem = getParentItem(obj, showCategories());
        insertItem(parentItem, dataObjectItem);
    } else {
        TreeItem *dataObjectItem;
        if (obj->getInstID() == 0) {
            dataObjectItem = createDataObject(obj);
            TreeItem *parentItem = getParentItem(obj, showCategories());
            insertItem(parentItem, dataObjectItem);
        } else {
            dataObjectItem = findObjectTreeItem(obj->getObjID());
        }
        InstanceTreeItem *instanceItem = createDataObjectInstance(obj);
        appendItem(dataObjectItem, instanceItem);
    }
}

DataObjectTreeItem *UAVObjectTreeModel::createDataObject(UAVDataObject *obj)
{
    DataObjectTreeItem *item = new DataObjectTreeItem(obj, obj->getName());

    addObjectTreeItem(obj->getObjID(), item);
    item->setHighlightManager(m_highlightManager);

    // metadata items are created up front and are added/removed as needed
    // see toggleMetaItems()
    MetaObjectTreeItem *metaItem = createMetaObject(obj->getMetaObject());
    if (showMetadata()) {
        appendItem(item, metaItem);
    }

    if (obj->isSingleInstance()) {
        foreach(UAVObjectField * field, obj->getFields()) {
            if (field->getNumElements() > 1) {
                addArrayField(field, item);
            } else {
                addSingleField(0, field, item);
            }
        }
    }

    return item;
}

InstanceTreeItem *UAVObjectTreeModel::createDataObjectInstance(UAVDataObject *obj)
{
    QString name = obj->getName() + " " + QString::number((int)obj->getInstID());
    InstanceTreeItem *item = new InstanceTreeItem(obj, name);

    item->setHighlightManager(m_highlightManager);

    foreach(UAVObjectField * field, obj->getFields()) {
        if (field->getNumElements() > 1) {
            addArrayField(field, item);
        } else {
            addSingleField(0, field, item);
        }
    }

    return item;
}

MetaObjectTreeItem *UAVObjectTreeModel::createMetaObject(UAVMetaObject *obj)
{
    MetaObjectTreeItem *item = new MetaObjectTreeItem(obj, tr("Meta Data"));

    addObjectTreeItem(obj->getObjID(), item);
    item->setHighlightManager(m_highlightManager);

    foreach(UAVObjectField * field, obj->getFields()) {
        if (field->getNumElements() > 1) {
            addArrayField(field, item);
        } else {
            addSingleField(0, field, item);
        }
    }
    return item;
}

TreeItem *UAVObjectTreeModel::getParentItem(UAVDataObject *obj, bool categorize)
{
    TreeItem *parentItem = obj->isSettingsObject() ? m_settingsTree : m_nonSettingsTree;

    if (categorize) {
        QString category = obj->getCategory();
        if (obj->getCategory().isEmpty()) {
            category = tr("Uncategorized");
        }
        QStringList categoryPath = category.split('/');

        foreach(QString category, categoryPath) {
            // metadata items are created and destroyed as needed
            // see toggleCategoryItems()
            TreeItem *categoryItem = parentItem->childByName(category);

            if (!categoryItem) {
                categoryItem = new TopTreeItem(category);
                categoryItem->setHighlightManager(m_highlightManager);
                insertItem(parentItem, categoryItem);
            }
            parentItem = categoryItem;
        }
    }
    return parentItem;
}

void UAVObjectTreeModel::addArrayField(UAVObjectField *field, TreeItem *parent)
{
    TreeItem *item = new ArrayFieldTreeItem(field, field->getName());

    item->setHighlightManager(m_highlightManager);

    appendItem(parent, item);

    for (int i = 0; i < (int)field->getNumElements(); ++i) {
        addSingleField(i, field, item);
    }
}

void UAVObjectTreeModel::addSingleField(int i, UAVObjectField *field, TreeItem *parent)
{
    QList<QVariant> data;
    if (field->getNumElements() == 1) {
        data.append(field->getName());
    } else {
        data.append(QString("[%1]").arg((field->getElementNames())[i]));
    }

    FieldTreeItem *item = NULL;
    UAVObjectField::FieldType type = field->getType();
    switch (type) {
    case UAVObjectField::BITFIELD:
    case UAVObjectField::ENUM:
    {
        QStringList options = field->getOptions();
        QVariant value = field->getValue(i);
        data.append(options.indexOf(value.toString()));
        data.append(field->getUnits());
        item = new EnumFieldTreeItem(field, i, data);
        break;
    }
    case UAVObjectField::INT8:
    case UAVObjectField::INT16:
    case UAVObjectField::INT32:
    case UAVObjectField::UINT8:
    case UAVObjectField::UINT16:
    case UAVObjectField::UINT32:
        data.append(field->getValue(i));
        data.append(field->getUnits());
        if (field->getUnits().toLower() == "hex") {
            item = new HexFieldTreeItem(field, i, data);
        } else if (field->getUnits().toLower() == "char") {
            item = new CharFieldTreeItem(field, i, data);
        } else {
            item = new IntFieldTreeItem(field, i, data);
        }
        break;
    case UAVObjectField::FLOAT32:
        data.append(field->getValue(i));
        data.append(field->getUnits());
        item = new FloatFieldTreeItem(field, i, data, m_settings);
        break;
    default:
        Q_ASSERT(false);
    }

    item->setHighlightManager(m_highlightManager);
    item->setDescription(field->getDescription());

    appendItem(parent, item);
}

void UAVObjectTreeModel::appendItem(TreeItem *parentItem, TreeItem *childItem)
{
    int row = parentItem->childCount();

    beginInsertRows(index(parentItem), row, row);
    parentItem->appendChild(childItem);
    endInsertRows();
}

void UAVObjectTreeModel::insertItem(TreeItem *parentItem, TreeItem *childItem, int row)
{
    if (row < 0) {
        row = parentItem->insertionIndex(childItem);
    }
    beginInsertRows(index(parentItem), row, row);
    parentItem->insertChild(childItem, row);
    endInsertRows();
}

void UAVObjectTreeModel::removeItem(TreeItem *parentItem, TreeItem *childItem)
{
    int row = childItem->row();

    beginRemoveRows(index(parentItem), row, row);
    parentItem->removeChild(childItem);
    endRemoveRows();
}

void UAVObjectTreeModel::moveItem(TreeItem *newParentItem, TreeItem *oldParentItem, TreeItem *childItem)
{
    int destinationRow = newParentItem->insertionIndex(childItem);
    int sourceRow = childItem->row();

    beginMoveRows(index(oldParentItem), sourceRow, sourceRow, index(newParentItem), destinationRow);
    oldParentItem->removeChild(childItem);
    newParentItem->insertChild(childItem, destinationRow);
    endMoveRows();
}

void UAVObjectTreeModel::toggleCategoryItems()
{
    foreach(ObjectTreeItem * item, m_objectTreeItems.values()) {
        DataObjectTreeItem *dataItem = dynamic_cast<DataObjectTreeItem *>(item);

        if (dataItem) {
            TreeItem *oldParentItem = dataItem->parentItem();
            TreeItem *newParentItem = getParentItem(dataItem->dataObject(), showCategories());
            if (oldParentItem == newParentItem) {
                // should not happen
                continue;
            }

            moveItem(newParentItem, oldParentItem, dataItem);

            if (!showCategories()) {
                // remove empty category items
                TreeItem *item = oldParentItem;
                while (item->childCount() == 0 && item != newParentItem) {
                    TreeItem *tmp = item;
                    item = item->parentItem();
                    removeItem(tmp->parentItem(), tmp);
                    delete tmp;
                }
            }
        }
    }
}

void UAVObjectTreeModel::toggleMetaItems()
{
    foreach(ObjectTreeItem * item, m_objectTreeItems.values()) {
        MetaObjectTreeItem *metaItem = dynamic_cast<MetaObjectTreeItem *>(item);

        if (metaItem) {
            DataObjectTreeItem *dataItem = findDataObjectTreeItem(metaItem->object());
            if (showMetadata()) {
                insertItem(dataItem, metaItem, 0);
            } else {
                removeItem(dataItem, metaItem);
            }
        }
    }
}

QModelIndex UAVObjectTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    TreeItem *parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    }

    TreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }
    return QModelIndex();
}

QModelIndex UAVObjectTreeModel::index(TreeItem *item, int column) const
{
    if (item == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(item->row(), column, item);
}

QModelIndex UAVObjectTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    TreeItem *childItem  = static_cast<TreeItem *>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (!parentItem) {
        return QModelIndex();
    }

    if (parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int UAVObjectTreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    }

    return parentItem->childCount();
}

int UAVObjectTreeModel::columnCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (parent.isValid()) {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    } else {
        parentItem = m_rootItem;
    }

    return parentItem->columnCount();
}

QVariant UAVObjectTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == TreeItem::DATA_COLUMN) {
            EnumFieldTreeItem *fieldItem = dynamic_cast<EnumFieldTreeItem *>(item);
            if (fieldItem) {
                int enumIndex = fieldItem->data(index.column()).toInt();
                return fieldItem->enumOptions(enumIndex);
            }
        }
        return item->data(index.column());

    case Qt::EditRole:
        if (index.column() == TreeItem::DATA_COLUMN) {
            return item->data(index.column());
        }
        return QVariant();

    case Qt::ToolTipRole:
        return item->description();

    case Qt::ForegroundRole:
        if (!dynamic_cast<TopTreeItem *>(item) && !item->isKnown()) {
            return unknownObjectColor();
        }
        return QVariant();

    case Qt::BackgroundRole:
        if (index.column() == TreeItem::TITLE_COLUMN) {
            // TODO filtering here on highlightTopTreeItems() should not be necessary
            // top tree items should not be highlighted at all in the first place
            // when highlightTopTreeItems() is false
            bool highlight = (highlightTopTreeItems() || !dynamic_cast<TopTreeItem *>(item));
            if (highlight && item->isHighlighted()) {
                return recentlyUpdatedColor();
            }
        } else if (index.column() == TreeItem::DATA_COLUMN) {
            FieldTreeItem *fieldItem = dynamic_cast<FieldTreeItem *>(item);
            if (fieldItem && fieldItem->isHighlighted()) {
                return recentlyUpdatedColor();
            }
            if (fieldItem && fieldItem->changed()) {
                return manuallyChangedColor();
            }
        }
        return QVariant();

    case Qt::UserRole:
        // UserRole gives access to TreeItem
        // cast to void* is necessary
        return qVariantFromValue((void *)item);

    default:
        return QVariant();
    }
}

bool UAVObjectTreeModel::setData(const QModelIndex &index, const QVariant & value, int role)
{
    Q_UNUSED(role);
    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    item->setData(value, index.column());
    return true;
}

Qt::ItemFlags UAVObjectTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    if (index.column() == TreeItem::DATA_COLUMN) {
        TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
        if (item->isEditable()) {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        }
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant UAVObjectTreeModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_rootItem->data(section);
    }
    return QVariant();
}

void UAVObjectTreeModel::updateObject(UAVObject *obj)
{
    Q_ASSERT(obj);
    ObjectTreeItem *item = findObjectTreeItem(obj->getObjID());
    Q_ASSERT(item);
    // TODO don't update meta object if they are not shown
    QTime ts = QTime::currentTime();
    item->update(ts);
    if (!onlyHighlightChangedValues()) {
        item->setHighlighted(true, ts);
    }
}

void UAVObjectTreeModel::updateIsKnown(UAVObject *object)
{
    DataObjectTreeItem *item = findDataObjectTreeItem(object);

    if (item) {
        refreshIsKnown(item);
    }
}

void UAVObjectTreeModel::refreshHighlight(TreeItem *item)
{
    // performance note: here we emit data changes column by column
    // emitting a dataChanged that spans multiple columns kills performance (CPU shoots up)
    // this is probably caused by the sort/filter proxy...

    QModelIndex itemIndex;

    itemIndex = index(item, TreeItem::TITLE_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);

    itemIndex = index(item, TreeItem::DATA_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);
}

void UAVObjectTreeModel::refreshIsKnown(TreeItem *item)
{
    QModelIndex itemIndex;

    itemIndex = index(item, TreeItem::TITLE_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);

    foreach(TreeItem * child, item->children()) {
        refreshIsKnown(child);
    }
}

void UAVObjectTreeModel::addObjectTreeItem(quint32 objectId, ObjectTreeItem *oti)
{
    m_objectTreeItems[objectId] = oti;
}

ObjectTreeItem *UAVObjectTreeModel::findObjectTreeItem(quint32 objectId)
{
    return m_objectTreeItems.value(objectId, 0);
}

DataObjectTreeItem *UAVObjectTreeModel::findDataObjectTreeItem(UAVObject *object)
{
    UAVDataObject *dataObject;

    UAVMetaObject *metaObject = qobject_cast<UAVMetaObject *>(object);

    if (metaObject) {
        dataObject = qobject_cast<UAVDataObject *>(metaObject->getParentObject());
    } else {
        dataObject = qobject_cast<UAVDataObject *>(object);
    }

    Q_ASSERT(dataObject);
    return static_cast<DataObjectTreeItem *>(findObjectTreeItem(dataObject->getObjID()));
}
