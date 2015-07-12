/**
 ******************************************************************************
 *
 * @file       vehicletemplateexportdialog.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup [Group]
 * @{
 * @addtogroup VehicleTemplateExportDialog
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

#include "vehicletemplateexportdialog.h"
#include "ui_vehicletemplateexportdialog.h"
#include <extensionsystem/pluginmanager.h>
#include "systemsettings.h"
#include <QBuffer>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>
#include <QMessageBox>
#include <uavobjecthelper.h>
#include <objectpersistence.h>
#include "stabilizationsettings.h"
#include "stabilizationsettingsbank1.h"
#include "stabilizationsettingsbank2.h"
#include "stabilizationsettingsbank3.h"
#include "mixersettings.h"
#include "ekfconfiguration.h"
#include <uavtalk/telemetrymanager.h>
#include <utils/pathutils.h>

const char *VehicleTemplateExportDialog::EXPORT_FIXEDWING_NAME = "fixedwing";
const char *VehicleTemplateExportDialog::EXPORT_MULTI_NAME     = "multirotor";
const char *VehicleTemplateExportDialog::EXPORT_HELI_NAME      = "helicopter";
const char *VehicleTemplateExportDialog::EXPORT_SURFACE_NAME   = "surface";
const char *VehicleTemplateExportDialog::EXPORT_CUSTOM_NAME    = "custom";

VehicleTemplateExportDialog::VehicleTemplateExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VehicleTemplateExportDialog), m_autopilotConnected(false)
{
    ui->setupUi(this);
    connect(ui->ImportButton, SIGNAL(clicked()), this, SLOT(importImage()));
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    m_uavoManager = pm->getObject<UAVObjectManager>();
    ui->Photo->setScene(new QGraphicsScene(this));
    ui->Type->setText(setupVehicleType());
    ui->selectionWidget->setTemplateInfo(m_type, m_subType);

    connect(ui->Name, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->Owner, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->ForumNick, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->Size, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->Weight, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));

    connect(ui->exportBtn, SIGNAL(clicked()), this, SLOT(exportTemplate()));
    connect(ui->saveAsBtn, SIGNAL(clicked()), this, SLOT(saveAsTemplate()));
    connect(ui->importBtn, SIGNAL(clicked()), this, SLOT(importTemplate()));
    connect(ui->cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->cancelBtn_2, SIGNAL(clicked()), this, SLOT(reject()));

    TelemetryManager *telemManager = pm->getObject<TelemetryManager>();
    if (telemManager->isConnected()) {
        onAutoPilotConnect();
    } else {
        onAutoPilotDisconnect();
    }

    connect(telemManager, SIGNAL(connected()), this, SLOT(onAutoPilotConnect()));
    connect(telemManager, SIGNAL(disconnected()), this, SLOT(onAutoPilotDisconnect()));
}

VehicleTemplateExportDialog::~VehicleTemplateExportDialog()
{
    delete ui;
}

QString VehicleTemplateExportDialog::setupVehicleType()
{
    SystemSettings *systemSettings = SystemSettings::GetInstance(m_uavoManager);

    Q_ASSERT(systemSettings);
    SystemSettings::DataFields systemSettingsData = systemSettings->getData();

    switch (systemSettingsData.AirframeType) {
    case SystemSettings::AIRFRAMETYPE_FIXEDWING:
        m_type    = VehicleConfigurationSource::VEHICLE_FIXEDWING;
        m_subType = VehicleConfigurationSource::FIXED_WING_AILERON;
        return tr("Fixed Wing - Aileron");

    case SystemSettings::AIRFRAMETYPE_FIXEDWINGELEVON:
        m_type    = VehicleConfigurationSource::VEHICLE_FIXEDWING;
        m_subType = VehicleConfigurationSource::FIXED_WING_ELEVON;
        return tr("Fixed Wing - Elevon");

    case SystemSettings::AIRFRAMETYPE_FIXEDWINGVTAIL:
        m_type    = VehicleConfigurationSource::VEHICLE_FIXEDWING;
        m_subType = VehicleConfigurationSource::FIXED_WING_VTAIL;
        return tr("Fixed Wing - V-Tail");

    case SystemSettings::AIRFRAMETYPE_HELICP:
        m_type    = VehicleConfigurationSource::VEHICLE_HELI;
        m_subType = VehicleConfigurationSource::HELI_CCPM;
        return tr("Helicopter");

    case SystemSettings::AIRFRAMETYPE_TRI:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_TRI_Y;
        return tr("Multirotor - Tricopter");

    case SystemSettings::AIRFRAMETYPE_QUADX:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_QUAD_X;
        return tr("Multirotor - Quadrocopter X");

    case SystemSettings::AIRFRAMETYPE_QUADP:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_QUAD_PLUS;
        return tr("Multirotor - Quadrocopter +");

    case SystemSettings::AIRFRAMETYPE_OCTOV:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_OCTO_V;
        return tr("Multirotor - Octocopter V");

    case SystemSettings::AIRFRAMETYPE_OCTOCOAXX:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_X;
        return tr("Multirotor - Octocopter X8X");

    case SystemSettings::AIRFRAMETYPE_OCTOCOAXP:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_OCTO_COAX_PLUS;
        return tr("Multirotor - Octocopter X8+");

    case SystemSettings::AIRFRAMETYPE_OCTO:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_OCTO;
        return tr("Multirotor - Octocopter +");

    case SystemSettings::AIRFRAMETYPE_OCTOX:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_OCTO_X;
        return tr("Multirotor - Octocopter X");

    case SystemSettings::AIRFRAMETYPE_HEXAX:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_HEXA_X;
        return tr("Multirotor - Hexacopter X");

    case SystemSettings::AIRFRAMETYPE_HEXAH:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_HEXA_H;
        return tr("Multirotor - Hexacopter H");

    case SystemSettings::AIRFRAMETYPE_HEXACOAX:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_HEXA_COAX_Y;
        return tr("Multirotor - Hexacopter Y6");

    case SystemSettings::AIRFRAMETYPE_HEXA:
        m_type    = VehicleConfigurationSource::VEHICLE_MULTI;
        m_subType = VehicleConfigurationSource::MULTI_ROTOR_HEXA;
        return tr("Multirotor - Hexacopter +");

    default:
        m_type = VehicleConfigurationSource::VEHICLE_UNKNOWN;
        return tr("Unsupported");
    }
}

void VehicleTemplateExportDialog::saveTemplate(QString path)
{
    QJsonObject exportObject;

    QList<UAVObject *> objectsToExport;
    objectsToExport << StabilizationSettings::GetInstance(m_uavoManager);
    objectsToExport << StabilizationSettingsBank1::GetInstance(m_uavoManager);
    objectsToExport << StabilizationSettingsBank2::GetInstance(m_uavoManager);
    objectsToExport << StabilizationSettingsBank3::GetInstance(m_uavoManager);
    objectsToExport << MixerSettings::GetInstance(m_uavoManager);
    objectsToExport << EKFConfiguration::GetInstance(m_uavoManager);
    m_uavoManager->toJson(exportObject, objectsToExport);

    exportObject["type"]       = m_type;
    exportObject["subtype"]    = m_subType;
    exportObject["name"]       = ui->Name->text();
    exportObject["owner"]      = ui->Owner->text();
    exportObject["nick"]       = ui->ForumNick->text();
    exportObject["size"]       = ui->Size->text();
    exportObject["weight"]     = ui->Weight->text();
    exportObject["motor"]      = ui->Motor->text();
    exportObject["esc"]        = ui->Esc->text();
    exportObject["servo"]      = ui->Servo->text();
    exportObject["battery"]    = ui->Battery->text();
    exportObject["propeller"]  = ui->Propeller->text();
    exportObject["controller"] = ui->Controllers->currentText();
    exportObject["comment"]    = ui->Comment->document()->toPlainText();
    QUuid uuid = QUuid::createUuid();
    exportObject["uuid"]       = uuid.toString();

    if (!m_image.isNull()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        m_image.scaled(IMAGE_SCALE_WIDTH, IMAGE_SCALE_HEIGHT, Qt::KeepAspectRatio,
                       Qt::SmoothTransformation).save(&buffer, "PNG");
        exportObject["photo"] = QString::fromLatin1(bytes.toBase64().data());
    }

    QJsonDocument saveDoc(exportObject);

    const char *fileType = ".optmpl";

    QString fileName     = QString("%1-%2-%3%4")
                           .arg(fixFilenameString(ui->Name->text(), 20))
                           .arg(fixFilenameString(ui->Type->text(), 30))
                           .arg(fixFilenameString(uuid.toString().right(12)))
                           .arg(fileType);

    QString fullPath;
    if (path.isEmpty()) {
        fullPath = QString("%1%2%3").arg(QDir::homePath()).arg(QDir::separator()).arg(fileName);
        fullPath = QFileDialog::getSaveFileName(this, tr("Export settings"), fullPath, QString("%1 (*%2)").arg(tr("OPTemplates"), fileType));
    } else {
        fullPath = QString("%1%2%3").arg(path).arg(QDir::separator()).arg(fileName);
    }

    if (!fullPath.isEmpty()) {
        if (!fullPath.endsWith(fileType)) {
            fullPath.append(fileType);
        }
        QFile saveFile(fullPath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            saveFile.write(saveDoc.toJson());
            saveFile.close();
        } else {
            QMessageBox::information(this, "Export", tr("Settings could not be exported to \n%1(%2).\nPlease try again.")
                                     .arg(QFileInfo(saveFile).absoluteFilePath(), saveFile.error()), QMessageBox::Ok);
        }
    }
}

QString VehicleTemplateExportDialog::fixFilenameString(QString input, int truncate)
{
    return input.replace(' ', "").replace('|', "").replace('/', "")
           .replace('\\', "").replace(':', "").replace('"', "")
           .replace('\'', "").replace('?', "").replace('*', "")
           .replace('>', "").replace('<', "")
           .replace('}', "").replace('{', "")
           .left(truncate);
}

void VehicleTemplateExportDialog::exportTemplate()
{
    QString path = QString("%1%2%3%4").arg(Utils::InsertStoragePath("%%STOREPATH%%cloudconfig"))
                   .arg(QDir::separator()).arg(getTypeDirectory()).arg(QDir::separator());
    QDir dir;

    dir.mkpath(path);
    saveTemplate(path);
}

void VehicleTemplateExportDialog::saveAsTemplate()
{
    saveTemplate(QString(""));
}

void VehicleTemplateExportDialog::importTemplate()
{
    QJsonObject *tmpl = ui->selectionWidget->selectedTemplate();

    if (tmpl != NULL) {
        QList<UAVObject *> updatedObjects;
        m_uavoManager->fromJson(*tmpl, &updatedObjects);
        UAVObjectUpdaterHelper helper;
        foreach(UAVObject * object, updatedObjects) {
            UAVDataObject *dataObj = dynamic_cast<UAVDataObject *>(object);

            if (dataObj != NULL && dataObj->isKnown()) {
                helper.doObjectAndWait(dataObj);

                ObjectPersistence *objper = ObjectPersistence::GetInstance(m_uavoManager);
                ObjectPersistence::DataFields data;
                data.Operation  = ObjectPersistence::OPERATION_SAVE;
                data.Selection  = ObjectPersistence::SELECTION_SINGLEOBJECT;
                data.ObjectID   = dataObj->getObjID();
                data.InstanceID = dataObj->getInstID();
                objper->setData(data);

                helper.doObjectAndWait(objper);
            }
        }
    }
}

void VehicleTemplateExportDialog::importImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Image"), "", tr("Images (*.png *.jpg)"));

    if (!fileName.isEmpty()) {
        m_image.load(fileName);
        ui->Photo->scene()->addPixmap(m_image);
        ui->Photo->setSceneRect(ui->Photo->scene()->itemsBoundingRect());
        ui->Photo->fitInView(ui->Photo->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void VehicleTemplateExportDialog::onAutoPilotConnect()
{
    ui->importBtn->setEnabled(true);
    m_autopilotConnected = true;
    updateStatus();
}

void VehicleTemplateExportDialog::onAutoPilotDisconnect()
{
    ui->importBtn->setEnabled(false);
    m_autopilotConnected = false;
    updateStatus();
}

QString VehicleTemplateExportDialog::getTypeDirectory()
{
    switch (m_type) {
    case VehicleConfigurationSource::VEHICLE_FIXEDWING:
        return EXPORT_FIXEDWING_NAME;

    case VehicleConfigurationSource::VEHICLE_MULTI:
        return EXPORT_MULTI_NAME;

    case VehicleConfigurationSource::VEHICLE_HELI:
        return EXPORT_HELI_NAME;

    case VehicleConfigurationSource::VEHICLE_SURFACE:
        return EXPORT_SURFACE_NAME;

    default:
        return EXPORT_CUSTOM_NAME;
    }
}

void VehicleTemplateExportDialog::updateStatus()
{
    bool enabled = m_autopilotConnected && ui->Name->text().length() > 3 && ui->Owner->text().length() > 2 &&
                   ui->ForumNick->text().length() > 2 && ui->Size->text().length() > 0 &&
                   ui->Weight->text().length() > 0;

    ui->exportBtn->setEnabled(enabled);
    ui->saveAsBtn->setEnabled(enabled);
}
