/**
 ******************************************************************************
 *
 * @file       uavobjectbrowserwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             Tau Labs, http://taulabs.org, Copyright (C) 2013
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
#include "uavobjectbrowserwidget.h"

#include "ui_uavobjectbrowser.h"
#include "ui_viewoptions.h"

#include "uavobjecttreemodel.h"
#include "browseritemdelegate.h"
#include "treeitem.h"
#include "uavobjectmanager.h"
#include "extensionsystem/pluginmanager.h"
#include "utils/mustache.h"

#include <QTextStream>
#include <QDebug>

UAVObjectBrowserWidget::UAVObjectBrowserWidget(QWidget *parent) : QWidget(parent)
{
    m_viewoptionsDialog = new QDialog(this);

    m_viewoptions = new Ui_viewoptions();
    m_viewoptions->setupUi(m_viewoptionsDialog);

    m_model = createTreeModel();

    m_modelProxy = new TreeSortFilterProxyModel(this);
    m_modelProxy->setSourceModel(m_model);
    m_modelProxy->setDynamicSortFilter(true);

    m_browser = new Ui_UAVObjectBrowser();
    m_browser->setupUi(this);
    m_browser->treeView->setModel(m_modelProxy);
    m_browser->treeView->setColumnWidth(0, 300);

    m_browser->treeView->setItemDelegate(new BrowserItemDelegate());
    m_browser->treeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_browser->treeView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_mustacheTemplate = loadFileIntoString(QString(":/uavobjectbrowser/resources/uavodescription.mustache"));

    showDescription(m_viewoptions->cbDescription->isChecked());

    connect(m_browser->treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            this, SLOT(currentChanged(QModelIndex, QModelIndex)), Qt::UniqueConnection);
    connect(m_browser->saveSDButton, SIGNAL(clicked()), this, SLOT(saveObject()));
    connect(m_browser->readSDButton, SIGNAL(clicked()), this, SLOT(loadObject()));
    connect(m_browser->sendButton, SIGNAL(clicked()), this, SLOT(sendUpdate()));
    connect(m_browser->requestButton, SIGNAL(clicked()), this, SLOT(requestUpdate()));
    connect(m_browser->eraseSDButton, SIGNAL(clicked()), this, SLOT(eraseObject()));
    connect(m_browser->tbView, SIGNAL(clicked()), this, SLOT(viewSlot()));
    connect(m_browser->splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved()));

    connect(m_viewoptions->cbDescription, SIGNAL(toggled(bool)), this, SLOT(showDescription(bool)));

    connect(m_viewoptions->cbCategorized, SIGNAL(toggled(bool)), this, SLOT(updateViewOptions()));
    connect(m_viewoptions->cbMetaData, SIGNAL(toggled(bool)), this, SLOT(updateViewOptions()));
    connect(m_viewoptions->cbScientific, SIGNAL(toggled(bool)), this, SLOT(updateViewOptions()));

    // search field and button
    connect(m_browser->searchLine, SIGNAL(textChanged(QString)), this, SLOT(searchLineChanged(QString)));
    connect(m_browser->searchClearButton, SIGNAL(clicked(bool)), this, SLOT(searchTextCleared()));

    enableSendRequest(false);
}

UAVObjectBrowserWidget::~UAVObjectBrowserWidget()
{
    delete m_browser;
}

void UAVObjectBrowserWidget::setViewOptions(bool showCategories, bool showMetadata, bool useScientificNotation, bool showDescription)
{
    m_viewoptions->cbCategorized->setChecked(showCategories);
    m_viewoptions->cbMetaData->setChecked(showMetadata);
    m_viewoptions->cbScientific->setChecked(useScientificNotation);
    m_viewoptions->cbDescription->setChecked(showDescription);
}

void UAVObjectBrowserWidget::setSplitterState(QByteArray state)
{
    m_browser->splitter->restoreState(state);
}

void UAVObjectBrowserWidget::showDescription(bool show)
{
    m_browser->descriptionText->setVisible(show);

    // persist options
    emit viewOptionsChanged(m_viewoptions->cbCategorized->isChecked(), m_viewoptions->cbScientific->isChecked(),
                            m_viewoptions->cbMetaData->isChecked(), m_viewoptions->cbDescription->isChecked());
}

void UAVObjectBrowserWidget::sendUpdate()
{
    // TODO why steal focus ?
    this->setFocus();

    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem != NULL) {
        objItem->apply();
        UAVObject *obj = objItem->object();
        Q_ASSERT(obj);
        obj->updated();
    }
}

void UAVObjectBrowserWidget::requestUpdate()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem != NULL) {
        UAVObject *obj = objItem->object();
        Q_ASSERT(obj);
        obj->requestUpdate();
    }
}

ObjectTreeItem *UAVObjectBrowserWidget::findCurrentObjectTreeItem()
{
    QModelIndex current     = m_browser->treeView->currentIndex();
    TreeItem *item = static_cast<TreeItem *>(current.data(Qt::UserRole).value<void *>());
    ObjectTreeItem *objItem = 0;

    while (item) {
        objItem = dynamic_cast<ObjectTreeItem *>(item);
        if (objItem) {
            break;
        }
        item = item->parentItem();
    }
    return objItem;
}

QString UAVObjectBrowserWidget::loadFileIntoString(QString fileName)
{
    QFile file(fileName);

    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    QString line = stream.readAll();
    file.close();
    return line;
}

void UAVObjectBrowserWidget::saveObject()
{
    // TODO why steal focus ?
    this->setFocus();

    // Send update so that the latest value is saved
    sendUpdate();

    // Save object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem != NULL) {
        UAVObject *obj = objItem->object();
        Q_ASSERT(obj);
        updateObjectPersistence(ObjectPersistence::OPERATION_SAVE, obj);
    }
}

void UAVObjectBrowserWidget::loadObject()
{
    // Load object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem != NULL) {
        UAVObject *obj = objItem->object();
        Q_ASSERT(obj);
        updateObjectPersistence(ObjectPersistence::OPERATION_LOAD, obj);
        // Retrieve object so that latest value is displayed
        requestUpdate();
    }
}

void UAVObjectBrowserWidget::eraseObject()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem != NULL) {
        UAVObject *obj = objItem->object();
        Q_ASSERT(obj);
        updateObjectPersistence(ObjectPersistence::OPERATION_DELETE, obj);
        // Retrieve object so that correct default value is displayed
        requestUpdate();
    }
}

void UAVObjectBrowserWidget::updateObjectPersistence(ObjectPersistence::OperationOptions op, UAVObject *obj)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    ObjectPersistence *objper    = dynamic_cast<ObjectPersistence *>(objManager->getObject(ObjectPersistence::NAME));

    if (obj != NULL) {
        ObjectPersistence::DataFields data;
        data.Operation  = op;
        data.Selection  = ObjectPersistence::SELECTION_SINGLEOBJECT;
        data.ObjectID   = obj->getObjID();
        data.InstanceID = obj->getInstID();
        objper->setData(data);
        objper->updated();
    }
}

void UAVObjectBrowserWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    bool enable = true;
    if (!current.isValid()) {
        enable = false;
    }
    TreeItem *item       = static_cast<TreeItem *>(current.data(Qt::UserRole).value<void *>());
    TopTreeItem *top     = dynamic_cast<TopTreeItem *>(item);
    ObjectTreeItem *data = dynamic_cast<ObjectTreeItem *>(item);
    if (top || (data && !data->object())) {
        enable = false;
    }
    enableSendRequest(enable);
    updateDescription();
}

void UAVObjectBrowserWidget::viewSlot()
{
    if (m_viewoptionsDialog->isVisible()) {
        m_viewoptionsDialog->setVisible(false);
    } else {
        QPoint pos = QCursor::pos();
        pos.setX(pos.x() - m_viewoptionsDialog->width());
        m_viewoptionsDialog->move(pos);
        m_viewoptionsDialog->show();
    }
}

UAVObjectTreeModel *UAVObjectBrowserWidget::createTreeModel()
{
    UAVObjectTreeModel *model = new UAVObjectTreeModel(this);

    model->setShowCategories(m_viewoptions->cbCategorized->isChecked());
    model->setShowMetadata(m_viewoptions->cbMetaData->isChecked());
    model->setUseScientificNotation(m_viewoptions->cbScientific->isChecked());

    model->setRecentlyUpdatedColor(m_recentlyUpdatedColor);
    model->setManuallyChangedColor(m_manuallyChangedColor);
    model->setRecentlyUpdatedTimeout(m_recentlyUpdatedTimeout);
    model->setUnknownObjectColor(m_unknownObjectColor);
    model->setOnlyHighlightChangedValues(m_onlyHighlightChangedValues);

    return model;
}

void UAVObjectBrowserWidget::updateViewOptions()
{
    bool showCategories = m_viewoptions->cbCategorized->isChecked();
    bool useScientificNotation = m_viewoptions->cbScientific->isChecked();
    bool showMetadata   = m_viewoptions->cbMetaData->isChecked();
    bool showDesc = m_viewoptions->cbDescription->isChecked();

    m_model->setShowCategories(showCategories);
    m_model->setShowMetadata(showMetadata);
    m_model->setUseScientificNotation(useScientificNotation);

    // force an expand all if search text is not empty
    if (!m_browser->searchLine->text().isEmpty()) {
        searchLineChanged(m_browser->searchLine->text());
    }

    // persist options
    emit viewOptionsChanged(showCategories, useScientificNotation, showMetadata, showDesc);
}

void UAVObjectBrowserWidget::splitterMoved()
{
    emit splitterChanged(m_browser->splitter->saveState());
}

QString UAVObjectBrowserWidget::createObjectDescription(UAVObject *object)
{
    QString mustache(m_mustacheTemplate);

    QVariantHash uavoHash;

    uavoHash["OBJECT_NAME_TITLE"] = tr("Name");
    uavoHash["OBJECT_NAME"] = object->getName();
    uavoHash["CATEGORY_TITLE"]    = tr("Category");
    uavoHash["CATEGORY"]          = object->getCategory();
    uavoHash["TYPE_TITLE"]        = tr("Type");
    uavoHash["TYPE"] = object->isMetaDataObject() ? tr("Metadata") : object->isSettingsObject() ? tr("Setting") : tr("Data");
    uavoHash["SIZE_TITLE"]        = tr("Size");
    uavoHash["SIZE"] = object->getNumBytes();
    uavoHash["DESCRIPTION_TITLE"] = tr("Description");
    uavoHash["DESCRIPTION"]       = object->getDescription().replace("@ref", "");
    uavoHash["MULTI_INSTANCE_TITLE"] = tr("Multi");
    uavoHash["MULTI_INSTANCE"]    = object->isSingleInstance() ? tr("No") : tr("Yes");
    uavoHash["FIELDS_NAME_TITLE"] = tr("Fields");
    QVariantList fields;
    foreach(UAVObjectField * field, object->getFields()) {
        QVariantHash fieldHash;

        fieldHash["FIELD_NAME_TITLE"] = tr("Name");
        fieldHash["FIELD_NAME"] = field->getName();
        fieldHash["FIELD_TYPE_TITLE"] = tr("Type");
        fieldHash["FIELD_TYPE"] = QString("%1%2").arg(field->getTypeAsString(),
                                                      (field->getNumElements() > 1 ? QString("[%1]").arg(field->getNumElements()) : QString()));
        if (!field->getUnits().isEmpty()) {
            fieldHash["FIELD_UNIT_TITLE"] = tr("Unit");
            fieldHash["FIELD_UNIT"] = field->getUnits();
        }
        if (!field->getOptions().isEmpty()) {
            fieldHash["FIELD_OPTIONS_TITLE"] = tr("Options");
            QVariantList options;
            foreach(QString option, field->getOptions()) {
                QVariantHash optionHash;

                optionHash["FIELD_OPTION"] = option;
                if (!options.isEmpty()) {
                    optionHash["FIELD_OPTION_DELIM"] = ", ";
                }
                options.append(optionHash);
            }
            fieldHash["FIELD_OPTIONS"] = options;
        }
        if (field->getElementNames().count() > 1) {
            fieldHash["FIELD_ELEMENTS_TITLE"] = tr("Elements");
            QVariantList elements;
            for (int i = 0; i < field->getElementNames().count(); i++) {
                QString element = field->getElementNames().at(i);
                QVariantHash elementHash;
                elementHash["FIELD_ELEMENT"] = element;
                QString limitsString = field->getLimitsAsString(i);
                if (!limitsString.isEmpty()) {
                    elementHash["FIELD_ELEMENT_LIMIT"] = limitsString.prepend(" (").append(")");
                }
                if (!elements.isEmpty()) {
                    elementHash["FIELD_ELEMENT_DELIM"] = ", ";
                }
                elements.append(elementHash);
            }
            fieldHash["FIELD_ELEMENTS"] = elements;
        } else if (!field->getLimitsAsString(0).isEmpty()) {
            fieldHash["FIELD_LIMIT_TITLE"] = tr("Limits");
            fieldHash["FIELD_LIMIT"] = field->getLimitsAsString(0);
        }

        if (!field->getDescription().isEmpty()) {
            fieldHash["FIELD_DESCRIPTION_TITLE"] = tr("Description");
            fieldHash["FIELD_DESCRIPTION"] = field->getDescription();
        }

        fields.append(fieldHash);
    }
    uavoHash["FIELDS"] = fields;
    Mustache::QtVariantContext context(uavoHash);
    Mustache::Renderer renderer;
    return renderer.render(mustache, &context);
}

void UAVObjectBrowserWidget::enableSendRequest(bool enable)
{
    m_browser->sendButton->setEnabled(enable);
    m_browser->requestButton->setEnabled(enable);
    m_browser->saveSDButton->setEnabled(enable);
    m_browser->readSDButton->setEnabled(enable);
    m_browser->eraseSDButton->setEnabled(enable);
}

void UAVObjectBrowserWidget::updateDescription()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    if (objItem) {
        UAVObject *obj = objItem->object();
        if (obj) {
            m_browser->descriptionText->setText(createObjectDescription(obj));
            return;
        }
    }
    m_browser->descriptionText->setText("");
}

/**
 * @brief UAVObjectBrowserWidget::searchLineChanged Looks for matching text in the UAVO fields
 */
void UAVObjectBrowserWidget::searchLineChanged(QString searchText)
{
    m_modelProxy->setFilterRegExp(QRegExp(searchText, Qt::CaseInsensitive, QRegExp::FixedString));
    if (!searchText.isEmpty()) {
        int depth = m_viewoptions->cbCategorized->isChecked() ? 2 : 1;
        m_browser->treeView->expandToDepth(depth);
    } else {
        m_browser->treeView->collapseAll();
    }
}

QString UAVObjectBrowserWidget::indexToPath(const QModelIndex &index) const
{
    QString path = index.data(Qt::DisplayRole).toString();

    QModelIndex parent = index.parent();

    while (parent.isValid()) {
        path   = parent.data(Qt::DisplayRole).toString() + "/" + path;
        parent = parent.parent();
    }
    return path;
}

QModelIndex UAVObjectBrowserWidget::indexFromPath(const QString &path) const
{
    QStringList list  = path.split("/");

    QModelIndex index = m_modelProxy->index(0, 0);

    foreach(QString name, list) {
        QModelIndexList items = m_modelProxy->match(index, Qt::DisplayRole, name, 1, Qt::MatchFlags(Qt::MatchExactly | Qt::MatchRecursive));

        if (!items.isEmpty()) {
            index = items.first();
        } else {
            // bail out
            return QModelIndex();
        }
    }
    return index;
}

void UAVObjectBrowserWidget::saveState(QSettings &settings) const
{
    QStringList list;

    // prepare list
    foreach(QModelIndex index, m_modelProxy->getPersistentIndexList()) {
        if (m_browser->treeView->isExpanded(index)) {
            QString path = indexToPath(index);
            list << path;
        }
    }

    // save list
    settings.setValue("expandedItems", QVariant::fromValue(list));
}

void UAVObjectBrowserWidget::restoreState(QSettings &settings)
{
    // get list
    QStringList list = settings.value("expandedItems").toStringList();

    foreach(QString path, list) {
        QModelIndex index = indexFromPath(path);

        if (index.isValid()) {
            m_browser->treeView->setExpanded(index, true);
        }
    }
}

void UAVObjectBrowserWidget::searchTextCleared()
{
    m_browser->searchLine->clear();
}

TreeSortFilterProxyModel::TreeSortFilterProxyModel(QObject *p) :
    QSortFilterProxyModel(p)
{
    Q_ASSERT(p);
}

/**
 * @brief TreeSortFilterProxyModel::filterAcceptsRow  Taken from
 * http://qt-project.org/forums/viewthread/7782. This proxy model
 * will accept rows:
 *   - That match themselves, or
 *   - That have a parent that matches (on its own), or
 *   - That have a child that matches.
 * @param sourceRow
 * @param sourceParent
 * @return
 */
bool TreeSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (filterAcceptsRowItself(source_row, source_parent)) {
        return true;
    }

    // accept if any of the parents is accepted on it's own merits
    QModelIndex parent = source_parent;
    while (parent.isValid()) {
        if (filterAcceptsRowItself(parent.row(), parent.parent())) {
            return true;
        }
        parent = parent.parent();
    }

    // accept if any of the children is accepted on it's own merits
    if (hasAcceptedChildren(source_row, source_parent)) {
        return true;
    }

    return false;
}

bool TreeSortFilterProxyModel::filterAcceptsRowItself(int source_row, const QModelIndex &source_parent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool TreeSortFilterProxyModel::hasAcceptedChildren(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex item = sourceModel()->index(source_row, 0, source_parent);

    if (!item.isValid()) {
        return false;
    }

    // check if there are children
    int childCount = item.model()->rowCount(item);
    if (childCount == 0) {
        return false;
    }

    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRowItself(i, item)) {
            return true;
        }

        if (hasAcceptedChildren(i, item)) {
            return true;
        }
    }

    return false;
}
