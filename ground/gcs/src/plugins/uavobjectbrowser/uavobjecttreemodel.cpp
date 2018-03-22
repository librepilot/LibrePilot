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

UAVObjectTreeModel::UAVObjectTreeModel(QObject *parent, bool categorize, bool showMetadata, bool useScientificNotation) :
    QAbstractItemModel(parent),
    m_categorize(categorize),
    m_showMetadata(showMetadata),
    m_useScientificFloatNotation(useScientificNotation),
    m_recentlyUpdatedTimeout(500), // ms
    m_recentlyUpdatedColor(QColor(255, 230, 230)),
    m_manuallyChangedColor(QColor(230, 230, 255)),
    m_unknownObjectColor(QColor(Qt::gray))
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    Q_ASSERT(objManager);

    connect(objManager, SIGNAL(newObject(UAVObject *)), this, SLOT(newObject(UAVObject *)));
    connect(objManager, SIGNAL(newInstance(UAVObject *)), this, SLOT(newObject(UAVObject *)));

    m_highlightManager = new HighlightManager();
    connect(m_highlightManager, SIGNAL(updateHighlight(TreeItem *)), this, SLOT(updateHighlight(TreeItem *)));

    TreeItem::setHighlightTime(m_recentlyUpdatedTimeout);
    setupModelData(objManager);
}

UAVObjectTreeModel::~UAVObjectTreeModel()
{
    delete m_rootItem;
    delete m_highlightManager;
}

void UAVObjectTreeModel::resetModelData()
{
    m_highlightManager->reset();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objManager);

    emit beginResetModel();

    delete m_rootItem;
    m_rootItem = NULL;
    setupModelData(objManager);

    emit endResetModel();
}

void UAVObjectTreeModel::setShowCategories(bool showCategories)
{
    if (showCategories == m_categorize) {
        return;
    }
    m_categorize = showCategories;
}

void UAVObjectTreeModel::setShowMetadata(bool showMetadata)
{
    if (showMetadata == m_showMetadata) {
        return;
    }
    m_showMetadata = showMetadata;
}

void UAVObjectTreeModel::setShowScientificNotation(bool showScientificNotation)
{
    if (showScientificNotation == m_useScientificFloatNotation) {
        return;
    }
    m_useScientificFloatNotation = showScientificNotation;
}


void UAVObjectTreeModel::setupModelData(UAVObjectManager *objManager)
{
    // root
    QList<QVariant> rootData;
    rootData << tr("Property") << tr("Value") << tr("Unit");
    m_rootItem        = new TreeItem(rootData);
    m_rootItem->setHighlightManager(m_highlightManager);

    m_settingsTree    = new TopTreeItem(tr("Settings"), m_rootItem);
    m_settingsTree->setHighlightManager(m_highlightManager);

    m_nonSettingsTree = new TopTreeItem(tr("Data Objects"), m_rootItem);
    m_nonSettingsTree->setHighlightManager(m_highlightManager);

    // tree item takes ownership of its children
    m_rootItem->appendChild(m_settingsTree);
    m_rootItem->appendChild(m_nonSettingsTree);

    QList< QList<UAVDataObject *> > objList = objManager->getDataObjects();
    foreach(QList<UAVDataObject *> list, objList) {
        foreach(UAVDataObject * obj, list) {
            disconnect(obj, 0, this, 0);
            addDataObject(obj);
        }
    }
}

void UAVObjectTreeModel::newObject(UAVObject *obj)
{
    UAVDataObject *dobj = qobject_cast<UAVDataObject *>(obj);

    if (dobj) {
        addDataObject(dobj);
    }
}

void UAVObjectTreeModel::addDataObject(UAVDataObject *obj)
{
    TopTreeItem *root = obj->isSettingsObject() ? m_settingsTree : m_nonSettingsTree;

    TreeItem *parent  = root;

    if (m_categorize && obj->getCategory() != 0 && !obj->getCategory().isEmpty()) {
        QStringList categoryPath = obj->getCategory().split('/');
        parent = createCategoryItems(categoryPath, root);
    }

    ObjectTreeItem *existing = root->findDataObjectTreeItemByObjectId(obj->getObjID());
    if (existing) {
        addInstance(obj, existing);
    } else {
        DataObjectTreeItem *dataTreeItem = new DataObjectTreeItem(obj->getName(), obj, parent);
        dataTreeItem->setHighlightManager(m_highlightManager);

        parent->insertChild(dataTreeItem);
        root->addObjectTreeItem(obj->getObjID(), dataTreeItem);
        if (m_showMetadata) {
            UAVMetaObject *meta = obj->getMetaObject();
            MetaObjectTreeItem *metaTreeItem = addMetaObject(meta, dataTreeItem);
            root->addMetaObjectTreeItem(meta->getObjID(), metaTreeItem);
        }
        addInstance(obj, dataTreeItem);
    }
}

TreeItem *UAVObjectTreeModel::createCategoryItems(QStringList categoryPath, TreeItem *root)
{
    TreeItem *parent = root;

    foreach(QString category, categoryPath) {
        TreeItem *existing = parent->findChildByName(category);

        if (!existing) {
            TreeItem *categoryItem = new TopTreeItem(category, parent);
            categoryItem->setHighlightManager(m_highlightManager);

            parent->insertChild(categoryItem);
            parent = categoryItem;
        } else {
            parent = existing;
        }
    }
    return parent;
}

MetaObjectTreeItem *UAVObjectTreeModel::addMetaObject(UAVMetaObject *obj, TreeItem *parent)
{
    connect(obj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateObject(UAVObject *)));

    MetaObjectTreeItem *meta = new MetaObjectTreeItem(obj, tr("Meta Data"), parent);
    meta->setHighlightManager(m_highlightManager);

    foreach(UAVObjectField * field, obj->getFields()) {
        if (field->getNumElements() > 1) {
            addArrayField(field, meta);
        } else {
            addSingleField(0, field, meta);
        }
    }
    parent->appendChild(meta);
    return meta;
}

void UAVObjectTreeModel::addInstance(UAVObject *obj, TreeItem *parent)
{
    connect(obj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateObject(UAVObject *)));
    connect(obj, SIGNAL(isKnownChanged(UAVObject *)), this, SLOT(updateIsKnown(UAVObject *)));

    TreeItem *item;
    if (obj->isSingleInstance()) {
        item = parent;
    } else {
        QString name = tr("Instance") + " " + QString::number(obj->getInstID());
        item = new InstanceTreeItem(obj, name, parent);
        item->setHighlightManager(m_highlightManager);
        parent->appendChild(item);
    }
    foreach(UAVObjectField * field, obj->getFields()) {
        if (field->getNumElements() > 1) {
            addArrayField(field, item);
        } else {
            addSingleField(0, field, item);
        }
    }
}

void UAVObjectTreeModel::addArrayField(UAVObjectField *field, TreeItem *parent)
{
    TreeItem *item = new ArrayFieldTreeItem(field, field->getName(), parent);

    item->setHighlightManager(m_highlightManager);
    parent->appendChild(item);

    for (int i = 0; i < (int)field->getNumElements(); ++i) {
        addSingleField(i, field, item);
    }
}

void UAVObjectTreeModel::addSingleField(int index, UAVObjectField *field, TreeItem *parent)
{
    QList<QVariant> data;
    if (field->getNumElements() == 1) {
        data.append(field->getName());
    } else {
        data.append(QString("[%1]").arg((field->getElementNames())[index]));
    }

    FieldTreeItem *item = NULL;
    UAVObjectField::FieldType type = field->getType();
    switch (type) {
    case UAVObjectField::BITFIELD:
    case UAVObjectField::ENUM:
    {
        QStringList options = field->getOptions();
        QVariant value = field->getValue(index);
        data.append(options.indexOf(value.toString()));
        data.append(field->getUnits());
        item = new EnumFieldTreeItem(field, index, data, parent);
        break;
    }
    case UAVObjectField::INT8:
    case UAVObjectField::INT16:
    case UAVObjectField::INT32:
    case UAVObjectField::UINT8:
    case UAVObjectField::UINT16:
    case UAVObjectField::UINT32:
        data.append(field->getValue(index));
        data.append(field->getUnits());
        if (field->getUnits().toLower() == "hex") {
            item = new HexFieldTreeItem(field, index, data, parent);
        } else if (field->getUnits().toLower() == "char") {
            item = new CharFieldTreeItem(field, index, data, parent);
        } else {
            item = new IntFieldTreeItem(field, index, data, parent);
        }
        break;
    case UAVObjectField::FLOAT32:
        data.append(field->getValue(index));
        data.append(field->getUnits());
        item = new FloatFieldTreeItem(field, index, data, m_useScientificFloatNotation, parent);
        break;
    default:
        Q_ASSERT(false);
    }

    item->setHighlightManager(m_highlightManager);
    item->setDescription(field->getDescription());

    parent->appendChild(item);
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

QModelIndex UAVObjectTreeModel::index(TreeItem *item, int column)
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
            return m_unknownObjectColor;
        }
        return QVariant();

    case Qt::BackgroundRole:
        if (index.column() == TreeItem::TITLE_COLUMN) {
            if (!dynamic_cast<TopTreeItem *>(item) && item->isHighlighted()) {
                return m_recentlyUpdatedColor;
            }
        } else if (index.column() == TreeItem::DATA_COLUMN) {
            FieldTreeItem *fieldItem = dynamic_cast<FieldTreeItem *>(item);
            if (fieldItem && fieldItem->isHighlighted()) {
                return m_recentlyUpdatedColor;
            }
            if (fieldItem && fieldItem->changed()) {
                return m_manuallyChangedColor;
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
    Q_UNUSED(role)
    TreeItem * item = static_cast<TreeItem *>(index.internalPointer());
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
    ObjectTreeItem *item = findObjectTreeItem(obj);
    Q_ASSERT(item);
    item->update();
    if (!m_onlyHighlightChangedValues) {
        item->setHighlighted(true);
    }
}

ObjectTreeItem *UAVObjectTreeModel::findObjectTreeItem(UAVObject *object)
{
    UAVDataObject *dataObject = qobject_cast<UAVDataObject *>(object);
    UAVMetaObject *metaObject = qobject_cast<UAVMetaObject *>(object);

    Q_ASSERT(dataObject || metaObject);
    if (dataObject) {
        return findDataObjectTreeItem(dataObject);
    } else {
        return findMetaObjectTreeItem(metaObject);
    }
    return 0;
}

DataObjectTreeItem *UAVObjectTreeModel::findDataObjectTreeItem(UAVDataObject *obj)
{
    TopTreeItem *root = obj->isSettingsObject() ? m_settingsTree : m_nonSettingsTree;

    return root->findDataObjectTreeItemByObjectId(obj->getObjID());
}

MetaObjectTreeItem *UAVObjectTreeModel::findMetaObjectTreeItem(UAVMetaObject *obj)
{
    UAVDataObject *dataObject = qobject_cast<UAVDataObject *>(obj->getParentObject());

    Q_ASSERT(dataObject);
    TopTreeItem *root = dataObject->isSettingsObject() ? m_settingsTree : m_nonSettingsTree;
    return root->findMetaObjectTreeItemByObjectId(obj->getObjID());
}

void UAVObjectTreeModel::updateHighlight(TreeItem *item)
{
    // performance note: here we emit data changes column by column
    // emitting a dataChanged that spans multiple columns kills performance (CPU shoots up)
    // this is probably because we configure the sort/filter proxy to be dynamic
    // this happens when calling setDynamicSortFilter(true) on it which we do

    QModelIndex itemIndex;

    itemIndex = index(item, TreeItem::TITLE_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);

    itemIndex = index(item, TreeItem::DATA_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);
}

void UAVObjectTreeModel::updateIsKnown(UAVObject *object)
{
    ObjectTreeItem *item = findObjectTreeItem(object);

    if (item) {
        updateIsKnown(item);
    }
}

void UAVObjectTreeModel::updateIsKnown(TreeItem *item)
{
    QModelIndex itemIndex;

    itemIndex = index(item, TreeItem::TITLE_COLUMN);
    Q_ASSERT(itemIndex != QModelIndex());
    emit dataChanged(itemIndex, itemIndex);

    foreach(TreeItem * child, item->children()) {
        updateIsKnown(child);
    }
}
