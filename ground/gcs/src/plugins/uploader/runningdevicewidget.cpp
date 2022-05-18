/**
 ******************************************************************************
 *
 * @file       runningdevicewidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Uploader Serial and USB Uploader Plugin
 * @{
 * @brief The USB and Serial protocol uploader plugin
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
#include "runningdevicewidget.h"
#include "devicedescriptorstruct.h"
#include "uploadergadgetwidget.h"
#include "version_info/version_info.h"

RunningDeviceWidget::RunningDeviceWidget(QWidget *parent) :
    QWidget(parent)
{
    myDevice = new Ui_runningDeviceWidget();
    myDevice->setupUi(this);

    // Initialization of the Device icon display
    myDevice->devicePicture->setScene(new QGraphicsScene(this));
}

void RunningDeviceWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    // Thit fitInView method should only be called now, once the
    // widget is shown, otherwise it cannot compute its values and
    // the result is usually a ahrsbargraph that is way too small.
    myDevice->devicePicture->fitInView(devicePic.rect(), Qt::KeepAspectRatio);
}

void RunningDeviceWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    myDevice->devicePicture->fitInView(devicePic.rect(), Qt::KeepAspectRatio);
}

/**
   Fills the various fields for the device
 */
void RunningDeviceWidget::populate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();
    int id = utilMngr->getBoardModel();

    myDevice->lblDeviceID->setText(QString("Device ID: ") + QString::number(id, 16));
    myDevice->lblBoardName->setText(deviceDescriptorStruct::idToBoardName(id));
    myDevice->lblHWRev->setText(QString(tr("HW Revision: ")) + QString::number(id & 0x00FF, 16));
    // qDebug() << "CRC" << utilMngr->getFirmwareCRC();
    myDevice->lblCRC->setText(QString(tr("Firmware CRC: ")) + QVariant(utilMngr->getFirmwareCRC()).toString());
    // DeviceID tells us what sort of HW we have detected:
    // display a nice icon:
    myDevice->devicePicture->scene()->clear();

    switch (id) {
    case 0x0101:
    case 0x0201:
        devicePic.load("");
        break;
    case 0x0301:
        devicePic.load(":/uploader/images/gcs-board-oplink.png");
        break;
    case 0x0401:
        devicePic.load(":/uploader/images/gcs-board-cc.png");
        break;
    case 0x0402:
        devicePic.load(":/uploader/images/gcs-board-cc3d.png");
        break;
    case 0x0903:
    // Revo
    // fall through to DF4B
    case 0x0904:
        // DiscoveryF4Bare
        devicePic.load(":/uploader/images/gcs-board-revo.png");
        break;
    case 0x0905:
        // Nano
        devicePic.load(":/uploader/images/gcs-board-nano.png");
        break;
    case 0x9201:
        // Sparky2
        devicePic.load(":/uploader/images/gcs-board-sparky2.png");
        break;
    case 0x1001:
        // SPRacingF3
        devicePic.load(":/uploader/images/gcs-board-spracingf3.png");
        break;
    case 0x1003:
    // Nucleo F303RE
    case 0x1002:
        // SPRacingF3 EVO
        devicePic.load(":/uploader/images/gcs-board-spracingf3evo.png");
        break;
    case 0x1005:
        // pikoBLX
        devicePic.load(":/uploader/images/gcs-board-pikoblx.png");
        break;
    case 0x1006:
        // tinyFISH
        devicePic.load(":/uploader/images/gcs-board-tinyfish.png");
        break;
    default:
        // Clear
        devicePic.load("");
        break;
    }
    myDevice->devicePicture->scene()->addPixmap(devicePic);
    myDevice->devicePicture->setSceneRect(devicePic.rect());
    myDevice->devicePicture->fitInView(devicePic.rect(), Qt::KeepAspectRatio);

    QString serial = utilMngr->getBoardCPUSerial().toHex();
    myDevice->CPUSerial->setText(serial);
    myDevice->lblBLRev->setText(tr("BL version: ") + QString::number(utilMngr->getBootloaderRevision()));
    QByteArray description = utilMngr->getBoardDescription();

    deviceDescriptorStruct devDesc;
    if (UAVObjectUtilManager::descriptionToStructure(description, devDesc)) {
        // Convert current QString uavoHashArray stored in GCS to QByteArray
        QString uavoHash = VersionInfo::uavoHashArray();

        uavoHash.chop(2);
        uavoHash.remove(0, 2);
        uavoHash = uavoHash.trimmed();

        QByteArray uavoHashArray;
        bool ok;
        foreach(QString str, uavoHash.split(",")) {
            uavoHashArray.append(str.toInt(&ok, 16));
        }

        bool isCompatibleUavo = (uavoHashArray == devDesc.uavoHash);
        bool isTaggedGcs = (VersionInfo::tag() != "");
        bool isSameCommit     = (devDesc.gitHash == VersionInfo::hash8());
        bool isSameTag = (devDesc.gitTag == VersionInfo::fwTag());

        if (isTaggedGcs && isSameCommit && isSameTag && isCompatibleUavo) {
            // GCS tagged and firmware from same commit
            myDevice->lblCertified->setPixmap(QPixmap(":uploader/images/application-certificate.svg"));
            myDevice->lblCertified->setToolTip(tr("Matched firmware build with official tagged release"));
        } else if (!isTaggedGcs && isSameCommit && isSameTag && isCompatibleUavo) {
            // GCS untagged and firmware from same commit
            myDevice->lblCertified->setPixmap(QPixmap(":uploader/images/dialog-apply.svg"));
            myDevice->lblCertified->setToolTip(tr("Matched firmware build"));
        } else if ((!isSameCommit || !isSameTag) && isCompatibleUavo) {
            // firmware not matching current GCS but compatible UAVO
            myDevice->lblCertified->setPixmap(QPixmap(":uploader/images/warning.svg"));
            myDevice->lblCertified->setToolTip(tr("Untagged or custom firmware build"));
        } else {
            myDevice->lblCertified->setPixmap(QPixmap(":uploader/images/error.svg"));
            myDevice->lblCertified->setToolTip(tr("Uncompatible firmware build"));
        }
        myDevice->lblFWTag->setText(tr("Firmware tag: ") + devDesc.gitTag);
        myDevice->lblGitCommitTag->setText(tr("Git commit hash: ") + devDesc.gitHash);
        myDevice->lblFWDate->setText(tr("Firmware date: ") + devDesc.gitDate.insert(4, "-").insert(7, "-"));
    } else {
        QString desc = utilMngr->getBoardDescriptionString();
        myDevice->lblFWTag->setText(tr("Firmware tag: ") + (!desc.isEmpty() ? desc : tr("Unknown")));
        myDevice->lblCertified->setPixmap(QPixmap(":uploader/images/warning.svg"));
        myDevice->lblCertified->setToolTip(tr("Custom Firmware Build"));
        myDevice->lblGitCommitTag->setText(tr("Git commit hash: ") + tr("Unknown"));
        myDevice->lblFWDate->setText(tr("Firmware date: ") + tr("Unknown"));
    }
}
