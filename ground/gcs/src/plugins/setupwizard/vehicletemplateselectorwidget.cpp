/**
 ******************************************************************************
 *
 * @file       vehicletemplateselectorwidget.cpp
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

#include "vehicletemplateselectorwidget.h"
#include "ui_vehicletemplateselectorwidget.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include "vehicletemplateexportdialog.h"
#include "utils/pathutils.h"

VehicleTemplateSelectorWidget::VehicleTemplateSelectorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VehicleTemplateSelectorWidget), m_photoItem(NULL)
{
    ui->setupUi(this);
    ui->templateImage->setScene(new QGraphicsScene());
    connect(ui->templateList, SIGNAL(itemSelectionChanged()), this, SLOT(templateSelectionChanged()));
    connect(ui->deleteTemplateButton, SIGNAL(clicked()), this, SLOT(deleteSelectedTemplate()));
    connect(ui->addTemplateButton, SIGNAL(clicked()), this, SLOT(addTemplate()));
}

VehicleTemplateSelectorWidget::~VehicleTemplateSelectorWidget()
{
    ui->templateList->clear();
    foreach(VehicleTemplate * templ, m_templates.values()) {
        delete templ;
    }
    m_templates.clear();

    delete ui;
}

void VehicleTemplateSelectorWidget::setTemplateInfo(int vehicleType, int vehicleSubType, bool showTemplateControls)
{
    ui->buttonFrame->setVisible(showTemplateControls);
    m_vehicleType    = vehicleType;
    m_vehicleSubType = vehicleSubType;
    updateTemplates();
}

QJsonObject *VehicleTemplateSelectorWidget::selectedTemplate() const
{
    if (ui->templateList->currentRow() >= 0) {
        return ui->templateList->item(ui->templateList->currentRow())->data(Qt::UserRole + 1).value<QJsonObject *>();
    }
    return NULL;
}

bool VehicleTemplateSelectorWidget::selectedTemplateEditable() const
{
    if (ui->templateList->currentRow() >= 0) {
        return ui->templateList->item(ui->templateList->currentRow())->data(Qt::UserRole + 2).value<bool>();
    }
    return false;
}

QString VehicleTemplateSelectorWidget::selectedTemplatePath() const
{
    if (ui->templateList->currentRow() >= 0) {
        return ui->templateList->item(ui->templateList->currentRow())->data(Qt::UserRole + 3).value<QString>();
    }
    return "";
}

void VehicleTemplateSelectorWidget::updateTemplates()
{
    loadValidFiles();
    setupTemplateList();
}

void VehicleTemplateSelectorWidget::deleteSelectedTemplate()
{
    if (selectedTemplateEditable()) {
        if (QMessageBox::question(this, tr("Delete Vehicle Template"),
                                  "Are you sure you want to delete the selected template?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QFile fileToDelete(selectedTemplatePath());
            if (fileToDelete.remove()) {
                QJsonObject *templObj = selectedTemplate();
                if (templObj) {
                    VehicleTemplate *templ = m_templates[templObj->value("uuid").toString()];
                    m_templates.remove(templObj->value("uuid").toString());
                    delete templ;
                }
                delete ui->templateList->item(ui->templateList->currentRow());
            }
        }
    }
}

void VehicleTemplateSelectorWidget::addTemplate()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Add settings"), QDir::homePath(),
                                                tr("Vehicle Template Files (*.vtmpl *.optmpl)"));

    if (path != NULL) {
        QFile file(path);

        if (file.open(QFile::ReadOnly)) {
            QByteArray jsonData = file.readAll();
            QJsonParseError error;
            QJsonDocument templateDoc = QJsonDocument::fromJson(jsonData, &error);
            if (error.error == QJsonParseError::NoError) {
                QJsonObject json = templateDoc.object();
                if (airframeIsCompatible(json["type"].toInt(), json["subtype"].toInt())) {
                    QFileInfo fInfo(file);
                    QString destinationFilePath = QString("%1/%2").arg(Utils::InsertStoragePath("%%STOREPATH%%vehicletemplates"))
                                                  .arg(getTemplatePath());
                    QDir dir;
                    if (dir.mkpath(destinationFilePath) && file.copy(QString("%1/%2").arg(destinationFilePath).arg(fInfo.fileName()))) {
                        updateTemplates();
                    } else {
                        QMessageBox::critical(this, tr("Error"), tr("The selected template file could not be added."));
                    }
                } else {
                    QMessageBox::critical(this, tr("Error"), tr("The selected template file is not compatible with the current vehicle type."));
                }
            } else {
                QMessageBox::critical(this, tr("Error"), tr("The selected template file is corrupt or of an unknown version."));
            }
        } else {
            QMessageBox::critical(this, tr("Error"), tr("The selected template file could not be opened."));
        }
    }
}

void VehicleTemplateSelectorWidget::updatePhoto(QJsonObject *templ)
{
    QPixmap photo;

    if (m_photoItem != NULL) {
        ui->templateImage->scene()->removeItem(m_photoItem);
    }
    if (templ != NULL && !templ->value("photo").isUndefined()) {
        QByteArray imageData = QByteArray::fromBase64(templ->value("photo").toString().toLatin1());
        photo.loadFromData(imageData, "PNG");
    } else {
        photo.load(":/core/images/librepilot_logo_500.png");
    }
    m_photoItem = ui->templateImage->scene()->addPixmap(photo);
    ui->templateImage->setSceneRect(ui->templateImage->scene()->itemsBoundingRect());
    ui->templateImage->fitInView(ui->templateImage->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void VehicleTemplateSelectorWidget::updateDescription(QJsonObject *templ)
{
    if (templ != NULL) {
        QString description;
        description.append("<b>").append(tr("Name of Vehicle: ")).append("</b>").append(templ->value("name").toString()).append("<br>");
        description.append("<b>").append(tr("Name of Owner: ")).append("</b>").append(templ->value("owner").toString());
        if (templ->value("nick") != QStringLiteral("")) {
            description.append(" (").append(templ->value("nick").toString()).append(")");
        }
        description.append("<br>");
        description.append("<b>").append(tr("Size: ")).append("</b>").append(templ->value("size").toString()).append("<br>");
        description.append("<b>").append(tr("Weight: ")).append("</b>").append(templ->value("weight").toString()).append("<br>");
        description.append("<b>").append(tr("Motor(s): ")).append("</b>").append(templ->value("motor").toString()).append("<br>");
        description.append("<b>").append(tr("ESC(s): ")).append("</b>").append(templ->value("esc").toString()).append("<br>");
        description.append("<b>").append(tr("Servo(s): ")).append("</b>").append(templ->value("servo").toString()).append("<br>");
        description.append("<b>").append(tr("Battery: ")).append("</b>").append(templ->value("battery").toString()).append("<br>");
        description.append("<b>").append(tr("Propellers(s): ")).append("</b>").append(templ->value("propeller").toString()).append("<br>");
        description.append("<b>").append(tr("Controller: ")).append("</b>").append(templ->value("controller").toString()).append("<br>");
        description.append("<b>").append(tr("Comments: ")).append("</b>").append(templ->value("comment").toString());
        ui->templateDescription->setText(description);
    } else {
        ui->templateDescription->setText(tr("This option will use the current tuning settings saved on the controller, if your controller "
                                            "is currently unconfigured, then the pre-configured firmware defaults will be used.\n\n"
                                            "It is suggested that if this is a first time configuration of your controller, rather than "
                                            "use this option, instead select a tuning set that matches your own airframe as close as "
                                            "possible from the list above or if you are not able to fine one, then select the generic item "
                                            "from the list."));
    }
}

void VehicleTemplateSelectorWidget::templateSelectionChanged()
{
    if (ui->templateList->currentRow() >= 0) {
        QJsonObject *templ = selectedTemplate();
        updatePhoto(templ);
        updateDescription(templ);
        ui->deleteTemplateButton->setEnabled(selectedTemplateEditable());
    }
}

bool VehicleTemplateSelectorWidget::airframeIsCompatible(int vehicleType, int vehicleSubType)
{
    if (vehicleType != m_vehicleType) {
        return false;
    }

    return vehicleSubType == m_vehicleSubType;
}

QString VehicleTemplateSelectorWidget::getTemplatePath()
{
    switch (m_vehicleType) {
    case VehicleConfigurationSource::VEHICLE_FIXEDWING:
        return VehicleTemplateExportDialog::EXPORT_FIXEDWING_NAME;

    case VehicleConfigurationSource::VEHICLE_MULTI:
        return VehicleTemplateExportDialog::EXPORT_MULTI_NAME;

    case VehicleConfigurationSource::VEHICLE_HELI:
        return VehicleTemplateExportDialog::EXPORT_HELI_NAME;

    case VehicleConfigurationSource::VEHICLE_SURFACE:
        return VehicleTemplateExportDialog::EXPORT_SURFACE_NAME;

    default:
        return NULL;
    }
}

void VehicleTemplateSelectorWidget::loadFilesInDir(QString templateBasePath, bool local)
{
    QDir templateDir(templateBasePath);

    qDebug() << "Loading templates from base path:" << templateBasePath;
    QStringList names;
    names << "*.optmpl";
    names << "*.vtmpl"; // Vehicle template
    templateDir.setNameFilters(names);
    templateDir.setSorting(QDir::Name);
    QStringList files = templateDir.entryList();
    foreach(QString fileName, files) {
        QString fullPathName = QString("%1/%2").arg(templateDir.absolutePath()).arg(fileName);
        QFile file(fullPathName);

        if (file.open(QFile::ReadOnly)) {
            QByteArray jsonData = file.readAll();
            QJsonParseError error;
            QJsonDocument templateDoc = QJsonDocument::fromJson(jsonData, &error);
            if (error.error == QJsonParseError::NoError) {
                QJsonObject json = templateDoc.object();
                if (airframeIsCompatible(json["type"].toInt(), json["subtype"].toInt())) {
                    QString uuid = json["uuid"].toString();
                    if (!m_templates.contains(uuid)) {
                        m_templates[json["uuid"].toString()] = new VehicleTemplate(new QJsonObject(json), local, fullPathName);
                    }
                }
            } else {
                qDebug() << "Error parsing json file: "
                         << fileName << ". Error was:" << error.errorString();
            }
        }
        file.close();
    }
}

void VehicleTemplateSelectorWidget::loadValidFiles()
{
    ui->templateList->clear();
    foreach(VehicleTemplate * templ, m_templates.values()) {
        delete templ;
    }
    m_templates.clear();
    QString path = getTemplatePath();
    loadFilesInDir(QString("%1/%2/").arg(Utils::InsertDataPath("%%DATAPATH%%vehicletemplates")).arg(path), false);
    loadFilesInDir(QString("%1/%2/").arg(Utils::InsertStoragePath("%%STOREPATH%%vehicletemplates")).arg(path), true);
}

void VehicleTemplateSelectorWidget::setupTemplateList()
{
    QListWidgetItem *item;

    foreach(QString templ, m_templates.keys()) {
        VehicleTemplate *vtemplate = m_templates[templ];

        item = new QListWidgetItem(vtemplate->templateObject()->value("name").toString(), ui->templateList);
        item->setData(Qt::UserRole + 1, QVariant::fromValue(vtemplate->templateObject()));
        item->setData(Qt::UserRole + 2, QVariant::fromValue(vtemplate->editable()));
        if (vtemplate->editable()) {
            item->setData(Qt::ForegroundRole, QVariant::fromValue(QColor(Qt::darkGreen)));
            item->setData(Qt::ToolTipRole, QVariant::fromValue(tr("Local template.")));
        } else {
            item->setData(Qt::ToolTipRole, QVariant::fromValue(tr("Built-in template.")));
        }
        item->setData(Qt::UserRole + 3, QVariant::fromValue(vtemplate->templatePath()));
    }
    ui->templateList->sortItems(Qt::AscendingOrder);

    item = new QListWidgetItem(tr("Current Tuning"));
    item->setData(Qt::UserRole + 1, QVariant::fromValue((QJsonObject *)NULL));
    ui->templateList->insertItem(0, item);
    ui->templateList->setCurrentRow(0);
    // TODO Add generics to top under item Current tuning
}

QString VehicleTemplateSelectorWidget::getTemplateKey(QJsonObject *templ)
{
    return QString(templ->value("name").toString());
}

void VehicleTemplateSelectorWidget::resizeEvent(QResizeEvent *)
{
    ui->templateImage->setSceneRect(ui->templateImage->scene()->itemsBoundingRect());
    ui->templateImage->fitInView(ui->templateImage->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void VehicleTemplateSelectorWidget::showEvent(QShowEvent *)
{
    ui->templateImage->setSceneRect(ui->templateImage->scene()->itemsBoundingRect());
    ui->templateImage->fitInView(ui->templateImage->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}
