/**
 ******************************************************************************
 *
 * @file       vehicletemplateselectorwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup [Group]
 * @{
 * @addtogroup VehicleTemplateSelectorWidget
 * @{
 * @brief [Brief]
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

#ifndef VEHICLETEMPLATESELECTORWIDGET_H
#define VEHICLETEMPLATESELECTORWIDGET_H

#include <QGraphicsItem>
#include <QJsonObject>
#include <QWidget>

namespace Ui {
class VehicleTemplateSelectorWidget;
}

class VehicleTemplate {
public:
    VehicleTemplate(QJsonObject *templateObject, bool editable, QString templatePath) :
        m_templateObject(templateObject), m_editable(editable), m_templatePath(templatePath) {}

    ~VehicleTemplate()
    {
        if (m_templateObject) {
            delete m_templateObject;
        }
    }

    QJsonObject *templateObject()
    {
        return m_templateObject;
    }

    bool editable()
    {
        return m_editable;
    }

    QString templatePath()
    {
        return m_templatePath;
    }

private:
    QJsonObject *m_templateObject;
    bool m_editable;
    QString m_templatePath;
};

class VehicleTemplateSelectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit VehicleTemplateSelectorWidget(QWidget *parent = 0);
    ~VehicleTemplateSelectorWidget();
    void setTemplateInfo(int vehicleType, int vehicleSubType, bool showTemplateControls);
    QJsonObject *selectedTemplate() const;
public slots:
    void templateSelectionChanged();

protected:
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);

private:
    Ui::VehicleTemplateSelectorWidget *ui;
    int m_vehicleType;
    int m_vehicleSubType;

    QMap<QString, VehicleTemplate *> m_templates;
    QGraphicsPixmapItem *m_photoItem;

    void loadValidFiles();
    void loadFilesInDir(QString templateBasePath, bool local);
    void setupTemplateList();
    QString getTemplateKey(QJsonObject *templ);
    void updatePhoto(QJsonObject *templ);
    void updateDescription(QJsonObject *templ);
    bool airframeIsCompatible(int vehicleType, int vehicleSubType);
    QString getTemplatePath();
    bool selectedTemplateEditable() const;
    QString selectedTemplatePath() const;

private slots:
    void updateTemplates();
    void deleteSelectedTemplate();
    void addTemplate();
};

Q_DECLARE_METATYPE(QJsonObject *)

#endif // VEHICLETEMPLATESELECTORWIDGET_H
