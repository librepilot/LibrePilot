/**
 ******************************************************************************
 *
 * @file       uavobjectbrowserwidget.h
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

#ifndef UAVOBJECTBROWSERWIDGET_H_
#define UAVOBJECTBROWSERWIDGET_H_

#include "uavobjecttreemodel.h"

#include "objectpersistence.h"

#include <QWidget>
#include <QSortFilterProxyModel>

class QPushButton;
class ObjectTreeItem;
class Ui_UAVObjectBrowser;
class Ui_viewoptions;

class TreeSortFilterProxyModel : public QSortFilterProxyModel {
public:
    TreeSortFilterProxyModel(QObject *parent);

public:
    QModelIndexList getPersistentIndexList() const
    {
        return persistentIndexList();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool filterAcceptsRowItself(int source_row, const QModelIndex &source_parent) const;
    bool hasAcceptedChildren(int source_row, const QModelIndex &source_parent) const;
};

class UAVObjectBrowserWidget : public QWidget {
    Q_OBJECT

public:
    UAVObjectBrowserWidget(QWidget *parent = 0);
    ~UAVObjectBrowserWidget();

    void setUnknownObjectColor(QColor color)
    {
        m_unknownObjectColor = color;
        m_model->setUnknownObjectColor(color);
    }
    void setRecentlyUpdatedColor(QColor color)
    {
        m_recentlyUpdatedColor = color;
        m_model->setRecentlyUpdatedColor(color);
    }
    void setManuallyChangedColor(QColor color)
    {
        m_manuallyChangedColor = color;
        m_model->setManuallyChangedColor(color);
    }
    void setRecentlyUpdatedTimeout(int timeout)
    {
        m_recentlyUpdatedTimeout = timeout;
        m_model->setRecentlyUpdatedTimeout(timeout);
    }
    void setOnlyHighlightChangedValues(bool highlight)
    {
        m_onlyHighlightChangedValues = highlight;
        m_model->setOnlyHighlightChangedValues(highlight);
    }
    void setViewOptions(bool showCategories, bool showMetadata, bool useScientificNotation, bool showDescription);
    void setSplitterState(QByteArray state);

    void saveState(QSettings &settings) const;
    void restoreState(QSettings &settings);

    QString indexToPath(const QModelIndex &index) const;
    QModelIndex indexFromPath(const QString &path) const;

public slots:
    void showDescription(bool show);

private slots:
    void sendUpdate();
    void requestUpdate();
    void saveObject();
    void loadObject();
    void eraseObject();
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void viewSlot();
    void updateViewOptions();
    void searchLineChanged(QString searchText);
    void searchTextCleared();
    void splitterMoved();
    QString createObjectDescription(UAVObject *object);

signals:
    void viewOptionsChanged(bool categorized, bool scientific, bool metadata, bool description);
    void splitterChanged(QByteArray state);

private:
    Ui_UAVObjectBrowser *m_browser;
    Ui_viewoptions *m_viewoptions;
    QDialog *m_viewoptionsDialog;
    UAVObjectTreeModel *m_model;
    TreeSortFilterProxyModel *m_modelProxy;

    int m_recentlyUpdatedTimeout;
    QColor m_unknownObjectColor;
    QColor m_recentlyUpdatedColor;
    QColor m_manuallyChangedColor;
    bool m_onlyHighlightChangedValues;
    QString m_mustacheTemplate;

    UAVObjectTreeModel *createTreeModel();

    void updateObjectPersistence(ObjectPersistence::OperationOptions op, UAVObject *obj);
    void enableSendRequest(bool enable);
    void updateDescription();
    ObjectTreeItem *findCurrentObjectTreeItem();
    QString loadFileIntoString(QString fileName);
};

#endif /* UAVOBJECTBROWSERWIDGET_H_ */
