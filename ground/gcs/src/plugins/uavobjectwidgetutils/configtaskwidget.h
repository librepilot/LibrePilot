/**
 ******************************************************************************
 *
 * @file       configtaskwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectWidgetUtils Plugin
 * @{
 * @brief Utility plugin for UAVObject to Widget relation management
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
#ifndef CONFIGTASKWIDGET_H
#define CONFIGTASKWIDGET_H

#include "uavobjectwidgetutils_global.h"

#include <QWidget>
#include <QList>
#include <QVariant>

namespace ExtensionSystem {
class PluginManager;
}
class UAVObject;
class UAVObjectField;
class UAVObjectManager;
class UAVObjectUtilManager;
class SmartSaveButton;

class QComboBox;
class QPushButton;
class QEvent;

class ShadowWidgetBinding : public QObject {
    Q_OBJECT
public:
    ShadowWidgetBinding(QWidget *widget, double scale, bool isLimited);
    ~ShadowWidgetBinding();
    QWidget *widget() const;
    double scale() const;
    bool isLimited() const;

protected:
    QWidget *m_widget;
    double m_scale;
    bool m_isLimited;
};

class WidgetBinding : public ShadowWidgetBinding {
    Q_OBJECT
public:
    WidgetBinding(QWidget *widget, UAVObject *object, UAVObjectField *field, int index, double scale, bool isLimited);
    ~WidgetBinding();

    QString units() const;
    QString type() const;
    bool isInteger() const;
    UAVObject *object() const;
    UAVObjectField *field() const;
    int index() const;
    QList<ShadowWidgetBinding *> shadows() const;

    void addShadow(QWidget *widget, double scale, bool isLimited);
    bool matches(QString objectName, QString fieldName, int index, quint32 instanceId);

    bool isEnabled() const;
    void setIsEnabled(bool isEnabled);

    QVariant value() const;
    void setValue(const QVariant &value);

    void updateObjectFieldFromValue();
    void updateValueFromObjectField();

private:
    UAVObject *m_object;
    UAVObjectField *m_field;
    int m_index;
    bool m_isEnabled;
    QList<ShadowWidgetBinding *> m_shadows;
    QVariant m_value;
};

class UAVOBJECTWIDGETUTILS_EXPORT ConfigTaskWidget : public QWidget {
    Q_OBJECT

public:
    enum ConfigTaskType { AutoPilot, OPLink, Child };

    ConfigTaskWidget(QWidget *parent = 0, ConfigTaskType configType = AutoPilot);
    virtual ~ConfigTaskWidget();

    void bind();

    bool isDirty();
    void setDirty(bool value);
    void clearDirty();

    virtual bool shouldObjectBeSaved(UAVObject *object);

protected:
    // Combobox helper functions
    static bool isComboboxOptionSelected(QComboBox *combo, int optionValue);
    static int getComboboxSelectedOption(QComboBox *combo);
    static void setComboboxSelectedOption(QComboBox *combo, int optionValue);
    static int getComboboxIndexForOption(QComboBox *combo, int optionValue);
    static void enableComboBoxOptionItem(QComboBox *combo, int optionValue, bool enable);

    void disableMouseWheelEvents();
    bool eventFilter(QObject *obj, QEvent *evt);

    UAVObjectManager *getObjectManager();

    void addUAVObject(QString objectName, QList<int> *reloadGroups = NULL);
    void addUAVObject(UAVObject *objectName, QList<int> *reloadGroups = NULL);

// TODO should be protected (see VehicleConfig::registerWidgets(ConfigTaskWidget &parent)
public:
    void addWidget(QWidget *widget);

    void addWidgetBinding(QString objectName, QString fieldName, QWidget *widget, int index = 0, double scale = 1,
                          bool isLimited = false, QList<int> *reloadGroupIDs = 0, quint32 instID = 0);
    void addWidgetBinding(UAVObject *object, UAVObjectField *field, QWidget *widget, int index = 0, double scale = 1,
                          bool isLimited = false, QList<int> *reloadGroupIDs = 0, quint32 instID = 0);

protected:
    void addAutoBindings();

    void addWidgetBinding(QString objectName, QString fieldName, QWidget *widget, QString elementName, double scale,
                          bool isLimited = false, QList<int> *reloadGroupIDs = 0, quint32 instID = 0);
    void addWidgetBinding(UAVObject *object, UAVObjectField *field, QWidget *widget, QString elementName, double scale,
                          bool isLimited = false, QList<int> *reloadGroupIDs = 0, quint32 instID = 0);

    void addWidgetBinding(QString objectName, QString fieldName, QWidget *widget, QString elementName);
    void addWidgetBinding(UAVObject *object, UAVObjectField *field, QWidget *widget, QString elementName);

    void addWidgetToReloadGroups(QWidget *widget, QList<int> *reloadGroupIDs);

    bool addShadowWidgetBinding(QString objectName, QString fieldName, QWidget *widget, int index = 0, double scale = 1,
                                bool isLimited = false, QList<int> *m_reloadGroups = NULL, quint32 instID = 0);

    bool allObjectsUpdated();
    void setOutOfLimitsStyle(QString style)
    {
        m_outOfLimitsStyle = style;
    }
    void addHelpButton(QPushButton *button, QString url);
    void setWikiURL(QString url);

protected slots:
    void apply();
    void save();

signals:
    void connected();
    void disconnected();
    void widgetContentsChanged(QWidget *widget);
    void defaultRequested(int group);
    void enableControlsChanged(bool enable);

protected:
    int boardModel() const
    {
        return m_currentBoardId;
    }
    bool expertMode() const;
    virtual QString mapObjectName(const QString objectName);
    virtual UAVObject *getObject(const QString name, quint32 instId = 0);
    virtual void buildOptionComboBox(QComboBox *combo, UAVObjectField *field, int index, bool applyLimits);

    virtual void enableControls(bool enable);
    virtual void refreshWidgetsValuesImpl(UAVObject *) {};
    virtual void updateObjectsFromWidgetsImpl() {};

    bool isConnected() const;
    void updateEnableControls();

protected slots:
    void setWidgetBindingObjectEnabled(QString objectName, bool enabled);

    virtual void widgetsContentsChanged();
    void refreshWidgetsValues(UAVObject *obj = NULL);
    void updateObjectsFromWidgets();

private slots:
    void onConnect();
    void onDisconnect();

    void disableObjectUpdates();
    void enableObjectUpdates();
    void objectUpdated(UAVObject *object);
    void invalidateObjects();

    void saveSuccessful();

    void defaultButtonClicked();
    void reloadButtonClicked();
    void helpButtonPressed();

private:
    struct ObjectComparator {
        quint32 objid;
        quint32 objinstid;
        bool operator==(const ObjectComparator & lhs)
        {
            return lhs.objid == this->objid && lhs.objinstid == this->objinstid;
        }
    };

    enum ButtonTypeEnum { None, SaveButton, ApplyButton, ReloadButton, DefaultButton, HelpButton };
    struct BindingStruct {
        QString objectName;
        QString fieldName;
        QString elementName;
        int     index;
        QString url;
        ButtonTypeEnum buttonType;
        QList<int>     buttonGroup;
        double  scale;
        bool    haslimits;
    };

    // indicates if this is an "autopilot" widget (CC3D, Revolution, ...), an OPLink widget or a Child widget (for vehicle config)
    // TODO the logic that this flag controls should be moved to derived classes
    ConfigTaskType m_configType;

    // only valid for "autopilot" widgets
    int m_currentBoardId;

    bool m_isConnected;
    bool m_isWidgetUpdatesAllowed;
    bool m_isDirty;
    bool m_refreshing;

    QStringList m_objects;

    // Wiki address for help button (will be concatenated with WIKI_URL_ROOT)
    QString m_wikiURL;

    QMultiHash<int, WidgetBinding *> m_reloadGroups;
    QMultiHash<QWidget *, WidgetBinding *> m_widgetBindingsPerWidget;
    QMultiHash<UAVObject *, WidgetBinding *> m_widgetBindingsPerObject;

    ExtensionSystem::PluginManager *m_pluginManager;
    UAVObjectUtilManager *m_objectUtilManager;

    QHash<UAVObject *, bool> m_updatedObjects;

    SmartSaveButton *m_saveButton;
    QHash<QPushButton *, QString> m_helpButtons;
    QList<QPushButton *> m_reloadButtons;

    QString m_outOfLimitsStyle;
    QTimer *m_realtimeUpdateTimer;

    bool setWidgetFromField(QWidget *widget, UAVObjectField *field, WidgetBinding *binding);

    QVariant getVariantFromWidget(QWidget *widget, WidgetBinding *binding);
    bool setWidgetFromVariant(QWidget *widget, QVariant value, WidgetBinding *binding);

    void connectWidgetUpdatesToSlot(QWidget *widget, const char *function);
    void disconnectWidgetUpdatesToSlot(QWidget *widget, const char *function);

    void resetLimits();

    void loadWidgetLimits(QWidget *widget, UAVObjectField *field, int index, bool applyLimits, double scale);

    void checkWidgetsLimits(QWidget *widget, UAVObjectField *field, int index, bool hasLimits, QVariant value, double scale);

    void dumpBindings();

    int fieldIndexFromElementName(QString objectName, QString fieldName, QString elementName);

    void doAddWidgetBinding(QString objectName, QString fieldName, QWidget *widget, int index = 0, double scale = 1,
                            bool isLimited = false, QList<int> *reloadGroupIDs = 0, quint32 instID = 0);

    void addApplyButton(QPushButton *button);
    void addSaveButton(QPushButton *button);
    void addReloadButton(QPushButton *button, int buttonGroup);
    void addDefaultButton(QPushButton *button, int buttonGroup);
};

#endif // CONFIGTASKWIDGET_H
