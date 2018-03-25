/**
 ******************************************************************************
 *
 * @file       fieldtreeitem.h
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

#ifndef FIELDTREEITEM_H
#define FIELDTREEITEM_H

#include "treeitem.h"

#include <QStringList>
#include <QSettings>
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <qscispinbox/QScienceSpinBox.h>
#include <QComboBox>

#include <limits>

#define QINT8MIN   std::numeric_limits<qint8>::min()
#define QINT8MAX   std::numeric_limits<qint8>::max()
#define QUINTMIN   std::numeric_limits<quint8>::min()
#define QUINT8MAX  std::numeric_limits<quint8>::max()
#define QINT16MIN  std::numeric_limits<qint16>::min()
#define QINT16MAX  std::numeric_limits<qint16>::max()
#define QUINT16MAX std::numeric_limits<quint16>::max()
#define QINT32MIN  std::numeric_limits<qint32>::min()
#define QINT32MAX  std::numeric_limits<qint32>::max()
#define QUINT32MAX std::numeric_limits<qint32>::max()

class FieldTreeItem : public TreeItem {
public:

    FieldTreeItem(int index, const QList<QVariant> &data, UAVObjectField *field) :
        TreeItem(data), m_index(index), m_field(field)
    {}
    FieldTreeItem(int index, const QVariant &data, UAVObjectField *field) :
        TreeItem(data), m_index(index), m_field(field)
    {}

    bool isEditable() const
    {
        return true;
    }

    virtual QWidget *createEditor(QWidget *parent) const   = 0;
    virtual QVariant getEditorValue(QWidget *editor) const = 0;
    virtual void setEditorValue(QWidget *editor, QVariant value) const = 0;

    void setData(QVariant value, int column)
    {
        QVariant currentValue = fieldToData();

        setChanged(currentValue != value);
        TreeItem::setData(value, column);
    }

    void update(const QTime &ts)
    {
        bool updated = false;

        if (!changed()) {
            QVariant currentValue = fieldToData();
            if (data() != currentValue) {
                updated = true;
                TreeItem::setData(currentValue);
            }
        }
        if (changed() || updated) {
            setHighlighted(true, ts);
        }
    }

    void apply()
    {
        m_field->setValue(dataToField(), m_index);
        setChanged(false);
    }

protected:
    virtual QVariant fieldToData() const = 0;
    virtual QVariant dataToField() const = 0;

    int m_index;
    UAVObjectField *m_field;
};

class EnumFieldTreeItem : public FieldTreeItem {
public:
    EnumFieldTreeItem(UAVObjectField *field, int index, const QList<QVariant> &data) :
        FieldTreeItem(index, data, field), m_enumOptions(field->getOptions())
    {}

    EnumFieldTreeItem(UAVObjectField *field, int index, const QVariant &data) :
        FieldTreeItem(index, data, field), m_enumOptions(field->getOptions())
    {}

    QString enumOptions(int index)
    {
        if ((index < 0) || (index >= m_enumOptions.length())) {
            return QString("Invalid Value (") + QString().setNum(index) + QString(")");
        }
        return m_enumOptions.at(index);
    }

    QVariant fieldToData() const
    {
        QStringList options = m_field->getOptions();
        QVariant value = m_field->getValue(m_index);
        int valIndex   = options.indexOf(value.toString());

        return valIndex;
    }

    QVariant dataToField() const
    {
        int value = data().toInt();
        QStringList options = m_field->getOptions();

        return options[value];
    }

    QWidget *createEditor(QWidget *parent) const
    {
        QComboBox *editor = new QComboBox(parent);

        // Setting ClickFocus lets the ComboBox stay open on Mac OSX.
        editor->setFocusPolicy(Qt::ClickFocus);
        foreach(QString option, m_enumOptions) {
            editor->addItem(option);
        }
        return editor;
    }

    QVariant getEditorValue(QWidget *editor) const
    {
        QComboBox *comboBox = static_cast<QComboBox *>(editor);

        return comboBox->currentIndex();
    }

    void setEditorValue(QWidget *editor, QVariant value) const
    {
        QComboBox *comboBox = static_cast<QComboBox *>(editor);

        comboBox->setCurrentIndex(value.toInt());
    }

private:
    QStringList m_enumOptions;
};

class IntFieldTreeItem : public FieldTreeItem {
public:
    IntFieldTreeItem(UAVObjectField *field, int index, const QList<QVariant> &data) :
        FieldTreeItem(index, data, field)
    {
        setMinMaxValues();
    }
    IntFieldTreeItem(UAVObjectField *field, int index, const QVariant &data) :
        FieldTreeItem(index, data, field)
    {
        setMinMaxValues();
    }

    void setMinMaxValues()
    {
        switch (m_field->getType()) {
        case UAVObjectField::INT8:
            m_minValue = QINT8MIN;
            m_maxValue = QINT8MAX;
            break;
        case UAVObjectField::INT16:
            m_minValue = QINT16MIN;
            m_maxValue = QINT16MAX;
            break;
        case UAVObjectField::INT32:
            m_minValue = QINT32MIN;
            m_maxValue = QINT32MAX;
            break;
        case UAVObjectField::UINT8:
            m_minValue = QUINTMIN;
            m_maxValue = QUINT8MAX;
            break;
        case UAVObjectField::UINT16:
            m_minValue = QUINTMIN;
            m_maxValue = QUINT16MAX;
            break;
        case UAVObjectField::UINT32:
            m_minValue = QUINTMIN;
            m_maxValue = QUINT32MAX;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    }

    QVariant fieldToData() const
    {
        return m_field->getValue(m_index).toInt();
    }

    QVariant dataToField() const
    {
        return data().toInt();
    }

    QWidget *createEditor(QWidget *parent) const
    {
        QSpinBox *editor = new QSpinBox(parent);

        editor->setMinimum(m_minValue);
        editor->setMaximum(m_maxValue);
        return editor;
    }

    QVariant getEditorValue(QWidget *editor) const
    {
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);

        spinBox->interpretText();
        return spinBox->value();
    }

    void setEditorValue(QWidget *editor, QVariant value) const
    {
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);

        spinBox->setValue(value.toInt());
    }

private:
    int m_minValue;
    int m_maxValue;
};

class FloatFieldTreeItem : public FieldTreeItem {
public:
    FloatFieldTreeItem(UAVObjectField *field, int index, const QList<QVariant> &data, const QSettings &settings) :
        FieldTreeItem(index, data, field), m_settings(settings) {}

    FloatFieldTreeItem(UAVObjectField *field, int index, const QVariant &data, const QSettings &settings) :
        FieldTreeItem(index, data, field), m_settings(settings) {}

    QVariant fieldToData() const
    {
        return m_field->getValue(m_index).toDouble();
    }

    QVariant dataToField() const
    {
        return data().toDouble();
    }

    QWidget *createEditor(QWidget *parent) const
    {
        bool useScientificNotation = m_settings.value("useScientificNotation", false).toBool();

        if (useScientificNotation) {
            QScienceSpinBox *editor = new QScienceSpinBox(parent);
            editor->setDecimals(6);
            editor->setMinimum(-std::numeric_limits<float>::max());
            editor->setMaximum(std::numeric_limits<float>::max());
            return editor;
        } else {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setDecimals(8);
            editor->setMinimum(-std::numeric_limits<float>::max());
            editor->setMaximum(std::numeric_limits<float>::max());
            return editor;
        }
    }

    QVariant getEditorValue(QWidget *editor) const
    {
        bool useScientificNotation = m_settings.value("useScientificNotation", false).toBool();

        if (useScientificNotation) {
            QScienceSpinBox *spinBox = static_cast<QScienceSpinBox *>(editor);
            spinBox->interpretText();
            return spinBox->value();
        } else {
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox *>(editor);
            spinBox->interpretText();
            return spinBox->value();
        }
    }

    void setEditorValue(QWidget *editor, QVariant value) const
    {
        bool useScientificNotation = m_settings.value("useScientificNotation", false).toBool();

        if (useScientificNotation) {
            QScienceSpinBox *spinBox = static_cast<QScienceSpinBox *>(editor);
            spinBox->setValue(value.toDouble());
        } else {
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox *>(editor);
            spinBox->setValue(value.toDouble());
        }
    }

private:
    const QSettings &m_settings;
};

class HexFieldTreeItem : public FieldTreeItem {
public:
    HexFieldTreeItem(UAVObjectField *field, int index, const QList<QVariant> &data) :
        FieldTreeItem(index, data, field)
    {}

    HexFieldTreeItem(UAVObjectField *field, int index, const QVariant &data) :
        FieldTreeItem(index, data, field)
    {}

    QVariant fieldToData() const
    {
        return toHexString(m_field->getValue(m_index));
    }

    QVariant dataToField() const
    {
        return toUInt(data());
    }

    QWidget *createEditor(QWidget *parent) const
    {
        QLineEdit *lineEdit = new QLineEdit(parent);

        lineEdit->setInputMask(QString(TreeItem::maxHexStringLength(m_field->getType()), 'H'));

        return lineEdit;
    }

    QVariant getEditorValue(QWidget *editor) const
    {
        QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);

        return lineEdit->text();
    }

    void setEditorValue(QWidget *editor, QVariant value) const
    {
        QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);

        lineEdit->setText(value.toString());
    }

private:
    QVariant toHexString(QVariant value) const
    {
        QString str;
        bool ok;

        return str.setNum(value.toUInt(&ok), 16).toUpper();
    }

    QVariant toUInt(QVariant str) const
    {
        bool ok;

        return str.toString().toUInt(&ok, 16);
    }
};

class CharFieldTreeItem : public FieldTreeItem {
public:
    CharFieldTreeItem(UAVObjectField *field, int index, const QList<QVariant> &data) :
        FieldTreeItem(index, data, field)
    {}

    CharFieldTreeItem(UAVObjectField *field, int index, const QVariant &data) :
        FieldTreeItem(index, data, field)
    {}

    QVariant fieldToData() const
    {
        return toChar(m_field->getValue(m_index));
    }

    QVariant dataToField() const
    {
        return toUInt(data());
    }

    QWidget *createEditor(QWidget *parent) const
    {
        QLineEdit *lineEdit = new QLineEdit(parent);

        lineEdit->setInputMask(QString(1, 'N'));

        return lineEdit;
    }

    QVariant getEditorValue(QWidget *editor) const
    {
        QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);

        return lineEdit->text();
    }

    void setEditorValue(QWidget *editor, QVariant value) const
    {
        QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);

        lineEdit->setText(value.toString());
    }

private:
    QVariant toChar(QVariant value) const
    {
        return value.toChar();
    }

    QVariant toUInt(QVariant str) const
    {
        return QVariant(str.toString().at(0).toLatin1());
    }
};

#endif // FIELDTREEITEM_H
