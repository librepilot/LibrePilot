/**
 ******************************************************************************
 *
 * @file       uavobjecttreemodel.h
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

#ifndef UAVOBJECTTREEMODEL_H
#define UAVOBJECTTREEMODEL_H

#include "treeitem.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QList>
#include <QColor>
#include <QSettings>

class TopTreeItem;
class ObjectTreeItem;
class DataObjectTreeItem;
class UAVObject;
class UAVDataObject;
class UAVMetaObject;
class UAVObjectField;
class UAVObjectManager;
class QSignalMapper;
class QTimer;

class UAVObjectTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit UAVObjectTreeModel(QObject *parent);
    ~UAVObjectTreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant & value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    bool showCategories() const;
    void setShowCategories(bool show);

    bool showMetadata() const;
    void setShowMetadata(bool show);

    bool useScientificNotation();
    void setUseScientificNotation(bool useScientificNotation);

    QColor unknownObjectColor() const;
    void setUnknownObjectColor(QColor color);

    QColor recentlyUpdatedColor() const;
    void setRecentlyUpdatedColor(QColor color);

    QColor manuallyChangedColor() const;
    void setManuallyChangedColor(QColor color);

    int recentlyUpdatedTimeout() const;
    void setRecentlyUpdatedTimeout(int timeout);

    bool onlyHighlightChangedValues() const;
    void setOnlyHighlightChangedValues(bool highlight);

    bool highlightTopTreeItems() const;
    void setHighlightTopTreeItems(bool highlight);

private slots:
    void newObject(UAVObject *obj);
    void updateObject(UAVObject *obj);
    void updateIsKnown(UAVObject *obj);
    void refreshHighlight(TreeItem *item);
    void refreshIsKnown(TreeItem *item);

private:
    QSettings m_settings;

    HighlightManager *m_highlightManager;

    TreeItem *m_rootItem;
    TopTreeItem *m_settingsTree;
    TopTreeItem *m_nonSettingsTree;

    QHash<quint32, ObjectTreeItem *> m_objectTreeItems;

    QModelIndex index(TreeItem *item, int column = 0) const;

    void setupModelData();
    void resetModelData();

    void addObject(UAVDataObject *obj);
    void addArrayField(UAVObjectField *field, TreeItem *parent);
    void addSingleField(int index, UAVObjectField *field, TreeItem *parent);

    DataObjectTreeItem *createDataObject(UAVDataObject *obj);
    InstanceTreeItem *createDataObjectInstance(UAVDataObject *obj);
    MetaObjectTreeItem *createMetaObject(UAVMetaObject *obj);
    TreeItem *getParentItem(UAVDataObject *obj, bool categorize);

    void appendItem(TreeItem *parentItem, TreeItem *childItem);
    void insertItem(TreeItem *parentItem, TreeItem *childItem, int row = -1);
    void removeItem(TreeItem *parentItem, TreeItem *childItem);
    void moveItem(TreeItem *newParentItem, TreeItem *oldParentItem, TreeItem *childItem);

    void toggleCategoryItems();
    void toggleMetaItems();

    void addObjectTreeItem(quint32 objectId, ObjectTreeItem *oti);
    ObjectTreeItem *findObjectTreeItem(quint32 objectId);

    DataObjectTreeItem *findDataObjectTreeItem(UAVObject *obj);
};

#endif // UAVOBJECTTREEMODEL_H
