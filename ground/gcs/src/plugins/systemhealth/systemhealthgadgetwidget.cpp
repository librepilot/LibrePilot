/**
 ******************************************************************************
 *
 * @file       systemhealthgadgetwidget.cpp
 * @author     OpenPilot Team & Edouard Lafargue Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SystemHealthPlugin System Health Plugin
 * @{
 * @brief The System Health gadget plugin
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

#include "systemalarms.h"
#include "systemhealthgadgetwidget.h"

#include "utils/stylehelper.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include <uavtalk/telemetrymanager.h>

#include <QDebug>
#include <QWhatsThis>

/*
 * Initialize the widget
 */
SystemHealthGadgetWidget::SystemHealthGadgetWidget(QWidget *parent) : QGraphicsView(parent)
{
    setMinimumSize(128, 128);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setScene(new QGraphicsScene(this));


    m_renderer = new QSvgRenderer();
    background = new QGraphicsSvgItem();
    foreground = new QGraphicsSvgItem();
    nolink     = new QGraphicsSvgItem();
    logreplay  = new QGraphicsSvgItem();
    logreplay2 = new QGraphicsSvgItem();
    missingElements = new QStringList();
    paint();

    // Now connect the widget to the SystemAlarms UAVObject
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    SystemAlarms *obj = dynamic_cast<SystemAlarms *>(objManager->getObject(QString("SystemAlarms")));
    connect(obj, SIGNAL(objectUpdated(UAVObject *)), this, SLOT(updateAlarms(UAVObject *)));

    // Listen to autopilot connection events
    TelemetryManager *telMngr = pm->getObject<TelemetryManager>();
    connect(telMngr, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(telMngr, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));
    connect(telMngr, SIGNAL(telemetryUpdated(double, double)), this, SLOT(onTelemetryUpdated(double, double)));

    setToolTip(tr("Displays flight system errors. Click on an alarm for more information."));
}

/**
 * Hide/Show the "Log Replay" overlay
 */
void SystemHealthGadgetWidget::onTelemetryUpdated(double txRate, double rxRate)
{
    // Return if a real board is connected or log file end (no telemetry)
    if (boardConnected || ((txRate + rxRate) == 0)) {
        return;
    }

    logreplayDelay++;
    // With real board not connected, display Logreplay after a little delay
    // and avoid Logreplay display when real board is connected.
    if (logreplayDelay > 3) {
        // Blink sequence, tell user Log replay runs.
        if ((logreplayDelay % 2) == 0) {
            logreplay->setVisible(true);
            logreplay2->setVisible(false);
        } else {
            logreplay->setVisible(false);
            logreplay2->setVisible(true);
        }
        nolink->setVisible(false);
    }
}

/**
 * Hide the "No Link" overlay
 */
void SystemHealthGadgetWidget::onAutopilotConnect()
{
    nolink->setVisible(false);
    logreplay->setVisible(false);
    logreplay2->setVisible(false);
    boardConnected = true;
}

/**
 * Show the "No Link" overlay
 */
void SystemHealthGadgetWidget::onAutopilotDisconnect()
{
    nolink->setVisible(true);
    logreplay->setVisible(false);
    logreplay2->setVisible(false);
    boardConnected = false;
    logreplayDelay = 0;
}

void SystemHealthGadgetWidget::updateAlarms(UAVObject *systemAlarm)
{
    // This code does not know anything about alarms beforehand, and
    // I found no efficient way to locate items inside the scene by
    // name, so it's just as simple to reset the scene:
    // And add the one with the right name.
    QGraphicsScene *m_scene = scene();

    foreach(QGraphicsItem * item, background->childItems()) {
        m_scene->removeItem(item);
        delete item; // removeItem does _not_ delete the item.
    }

    QMatrix backgroundMatrix = (m_renderer->matrixForElement(background->elementId())).inverted();

    QString alarm = systemAlarm->getName();
    foreach(UAVObjectField * field, systemAlarm->getFields()) {
        for (uint i = 0; i < field->getNumElements(); ++i) {
            QString element = field->getElementNames()[i];
            QString value   = field->getValue(i).toString();
            if (!missingElements->contains(element)) {
                if (m_renderer->elementExists(element)) {
                    QString element2 = element + "-" + value;
                    if (!missingElements->contains(element2)) {
                        if (m_renderer->elementExists(element2)) {
                            // element2 is in global coordinates
                            // transform its matrix into the coordinates of background
                            QMatrix blockMatrix   = backgroundMatrix * m_renderer->matrixForElement(element2);
                            // use this composed projection to get the position in background coordinates
                            QRectF rectProjected  = blockMatrix.mapRect(m_renderer->boundsOnElement(element2));

                            QGraphicsSvgItem *ind = new QGraphicsSvgItem();
                            ind->setSharedRenderer(m_renderer);
                            ind->setElementId(element2);
                            ind->setParentItem(background);
                            QTransform matrix;
                            matrix.translate(rectProjected.x(), rectProjected.y());
                            ind->setTransform(matrix, false);
                        } else {
                            if (value.compare("Uninitialised") != 0) {
                                missingElements->append(element2);
                                qDebug() << "Warning: element " << element2 << " not found in SVG.";
                            }
                        }
                    }
                } else {
                    missingElements->append(element);
                    qDebug() << "Warning: Element " << element << " not found in SVG.";
                }
            }
        }
    }
}

SystemHealthGadgetWidget::~SystemHealthGadgetWidget()
{
    // Do nothing
}


void SystemHealthGadgetWidget::setSystemFile(QString dfn)
{
    // Clear the list of elements not found on svg
    missingElements->clear();
    setBackgroundBrush(QBrush(Utils::StyleHelper::baseColor()));
    if (QFile::exists(dfn)) {
        m_renderer->load(dfn);
        if (m_renderer->isValid()) {
            fgenabled = false;
            background->setSharedRenderer(m_renderer);
            background->setElementId("background");

            if (m_renderer->elementExists("foreground")) {
                foreground->setSharedRenderer(m_renderer);
                foreground->setElementId("foreground");
                foreground->setZValue(99);
                fgenabled = true;
            }
            if (m_renderer->elementExists("logreplay")) {
                logreplay->setSharedRenderer(m_renderer);
                logreplay->setElementId("logreplay");
                logreplay->setZValue(100);
            }
            if (m_renderer->elementExists("logreplay2")) {
                logreplay2->setSharedRenderer(m_renderer);
                logreplay2->setElementId("logreplay2");
                logreplay2->setZValue(100);
            }
            if (m_renderer->elementExists("nolink")) {
                nolink->setSharedRenderer(m_renderer);
                nolink->setElementId("nolink");
                nolink->setZValue(101);
            }

            QGraphicsScene *l_scene = scene();
            l_scene->setSceneRect(background->boundingRect());
            fitInView(background, Qt::KeepAspectRatio);

            // Check whether the autopilot is connected already, by the way:
            ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
            UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
            TelemetryManager *telMngr    = pm->getObject<TelemetryManager>();
            if (telMngr->isConnected()) {
                onAutopilotConnect();
                SystemAlarms *obj = dynamic_cast<SystemAlarms *>(objManager->getObject(QString("SystemAlarms")));
                updateAlarms(obj);
            }
        }
    } else { qDebug() << "SystemHealthGadget: no file"; }
}

void SystemHealthGadgetWidget::paint()
{
    QGraphicsScene *l_scene = scene();

    l_scene->clear();
    l_scene->addItem(background);
    l_scene->addItem(foreground);
    l_scene->addItem(logreplay);
    l_scene->addItem(logreplay2);
    l_scene->addItem(nolink);
    update();
}

void SystemHealthGadgetWidget::paintEvent(QPaintEvent *event)
{
    // Skip painting until the dial file is loaded
    if (!m_renderer->isValid()) {
        qDebug() << "SystemHealthGadget: System file not loaded, not rendering";
        return;
    }
    QGraphicsView::paintEvent(event);
}

// This event enables the dial to be dynamically resized
// whenever the gadget is resized, taking advantage of the vector
// nature of SVG dials.
void SystemHealthGadgetWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    fitInView(background, Qt::KeepAspectRatio);
}

void SystemHealthGadgetWidget::mousePressEvent(QMouseEvent *event)
{
    QGraphicsScene *graphicsScene = scene();

    if (graphicsScene) {
        QPoint point = event->pos();
        bool haveAlarmItem = false;
        foreach(QGraphicsItem * sceneItem, items(point)) {
            QGraphicsSvgItem *clickedItem = dynamic_cast<QGraphicsSvgItem *>(sceneItem);

            if (clickedItem) {
                if ((clickedItem != foreground) && (clickedItem != background)) {
                    // Clicked an actual alarm. We need to set haveAlarmItem to true
                    // as two of the items in this loop will always be foreground and
                    // background. Without this flag, at some point in the loop we
                    // would always call showAllAlarmDescriptions...
                    haveAlarmItem = true;
                    QString itemId = clickedItem->elementId();
                    if (itemId.contains("OK")) {
                        // No alarm set for this item
                        showAlarmDescriptionForItemId("AlarmOK", event->globalPos());
                    } else {
                        // Warning, error or critical alarm
                        showAlarmDescriptionForItemId(itemId, event->globalPos());
                    }
                } else if (!haveAlarmItem) {
                    // Clicked foreground or background
                    showAllAlarmDescriptions(event->globalPos());
                }
            }
        }
    }
}

void SystemHealthGadgetWidget::showAlarmDescriptionForItemId(const QString itemId, const QPoint & location)
{
    QFile alarmDescription(":/systemhealth/html/" + itemId + ".html");

    if (alarmDescription.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&alarmDescription);
        textStream.setCodec("UTF-8");
        QWhatsThis::showText(location, textStream.readAll());
    }
}

void SystemHealthGadgetWidget::showAllAlarmDescriptions(const QPoint & location)
{
    QGraphicsScene *graphicsScene = scene();

    if (graphicsScene) {
        QString alarmsText;

        // Loop through all items in the scene looking for svg items that represent alarms
        foreach(QGraphicsItem * curItem, graphicsScene->items()) {
            QGraphicsSvgItem *curSvgItem = dynamic_cast<QGraphicsSvgItem *>(curItem);

            if (curSvgItem && (curSvgItem != foreground) && (curSvgItem != background)) {
                QString elementId = curSvgItem->elementId();
                if (!elementId.contains("OK")) {
                    // Found an alarm, get its corresponding alarm html file contents
                    // and append to the cumulative string for all alarms.
                    QFile alarmDescription(":/systemhealth/html/" + elementId + ".html");
                    if (alarmDescription.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream textStream(&alarmDescription);
                        textStream.setCodec("UTF-8");
                        alarmsText.append(textStream.readAll());
                    }
                }
            }
        }

        // Show alarms text if we have any
        if (alarmsText.length() > 0) {
            QWhatsThis::showText(location, alarmsText);
        }
    }
}
