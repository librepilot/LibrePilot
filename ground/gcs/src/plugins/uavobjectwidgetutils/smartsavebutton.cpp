/**
 ******************************************************************************
 *
 * @file       smartsavebutton.cpp
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
#include "smartsavebutton.h"
#include "configtaskwidget.h"

SmartSaveButton::SmartSaveButton(ConfigTaskWidget *configTaskWidget) : configWidget(configTaskWidget)
{}

void SmartSaveButton::addApplyButton(QPushButton *apply)
{
    buttonList.insert(apply, apply_button);
    connect(apply, SIGNAL(clicked()), this, SLOT(processClick()));
}

void SmartSaveButton::addSaveButton(QPushButton *save)
{
    buttonList.insert(save, save_button);
    connect(save, SIGNAL(clicked()), this, SLOT(processClick()));
}

void SmartSaveButton::processClick()
{
    emit beginOp();
    bool save = false;
    QPushButton *button = qobject_cast<QPushButton *>(sender());

    if (!button) {
        return;
    }
    if (buttonList.value(button) == save_button) {
        save = true;
    }
    processOperation(button, save);
}

void SmartSaveButton::processOperation(QPushButton *button, bool save)
{
    emit preProcessOperations();

    if (button) {
        button->setEnabled(false);
        button->setIcon(QIcon(":/uploader/images/system-run.svg"));
    }
    QTimer timer;
    timer.setSingleShot(true);
    bool error = false;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr     = pm->getObject<UAVObjectUtilManager>();
    foreach(UAVDataObject * obj, objects) {
        if (!obj) {
            continue;
        }
        UAVObject::Metadata mdata = obj->getMetadata();

        // Should we really save this object to the board?
        if (!configWidget->shouldObjectBeSaved(obj) || UAVObject::GetGcsAccess(mdata) == UAVObject::ACCESS_READONLY) {
            qDebug() << obj->getName() << "was skipped.";
            continue;
        }

        up_result = false;
        current_object = obj;
        for (int i = 0; i < 3; ++i) {
            qDebug() << "Uploading" << obj->getName() << "to board.";
            connect(obj, SIGNAL(transactionCompleted(UAVObject *, bool)), this, SLOT(transaction_finished(UAVObject *, bool)));
            connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
            obj->updated();

            timer.start(3000);
            loop.exec();
            if (!timer.isActive()) {
                qDebug() << "Upload of" << obj->getName() << "timed out.";
            }
            timer.stop();

            disconnect(obj, SIGNAL(transactionCompleted(UAVObject *, bool)), this, SLOT(transaction_finished(UAVObject *, bool)));
            disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
            if (up_result) {
                qDebug() << "Upload of" << obj->getName() << "successful.";
                break;
            }
        }
        if (up_result == false) {
            qDebug() << "Upload of" << obj->getName() << "failed after 3 tries.";
            error = true;
            continue;
        }

        sv_result = false;
        current_objectID = obj->getObjID();
        if (save && (obj->isSettingsObject())) {
            for (int i = 0; i < 3; ++i) {
                qDebug() << "Saving" << obj->getName() << "to board.";
                connect(utilMngr, SIGNAL(saveCompleted(int, bool)), this, SLOT(saving_finished(int, bool)));
                connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                utilMngr->saveObjectToSD(obj);

                timer.start(3000);
                loop.exec();
                if (!timer.isActive()) {
                    qDebug() << "Saving of" << obj->getName() << "timed out.";
                }
                timer.stop();

                disconnect(utilMngr, SIGNAL(saveCompleted(int, bool)), this, SLOT(saving_finished(int, bool)));
                disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                if (sv_result) {
                    qDebug() << "Saving of" << obj->getName() << "successful.";
                    break;
                }
            }
            if (sv_result == false) {
                qDebug() << "Saving of" << obj->getName() << "failed after 3 tries.";
                error = true;
            }
        }
    }
    emit endOp();
    if (!error) {
        emit saveSuccessful();
    }
    if (button) {
        button->setEnabled(true);
        if (!error) {
            button->setIcon(QIcon(":/uploader/images/dialog-apply.svg"));
        } else {
            button->setIcon(QIcon(":/uploader/images/process-stop.svg"));
        }
    }
}

void SmartSaveButton::setObjects(QList<UAVDataObject *> list)
{
    objects = list;
}

void SmartSaveButton::addObject(UAVDataObject *obj)
{
    Q_ASSERT(obj);
    if (obj && !objects.contains(obj)) {
        objects.append(obj);
    }
}

void SmartSaveButton::removeObject(UAVDataObject *obj)
{
    Q_ASSERT(obj);
    if (obj && objects.contains(obj)) {
        objects.removeAll(obj);
    }
}

void SmartSaveButton::removeAllObjects()
{
    objects.clear();
}

void SmartSaveButton::clearObjects()
{
    objects.clear();
}

void SmartSaveButton::transaction_finished(UAVObject *obj, bool result)
{
    if (current_object == obj) {
        up_result = result;
        loop.quit();
    }
}

void SmartSaveButton::saving_finished(int id, bool result)
{
    if (id == (int)current_objectID) {
        sv_result = result;
        loop.quit();
    }
}

void SmartSaveButton::enableControls(bool value)
{
    foreach(QPushButton * button, buttonList.keys())
    button->setEnabled(value);
}

void SmartSaveButton::resetIcons()
{
    foreach(QPushButton * button, buttonList.keys())
    button->setIcon(QIcon());
}

void SmartSaveButton::apply()
{
    processOperation(NULL, false);
}

void SmartSaveButton::save()
{
    processOperation(NULL, true);
}
