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

#include "pfdqmlgadget.h"
#include "pfdqmlgadgetwidget.h"
#include "pfdqmlgadgetconfiguration.h"

#include <QWidget>
#include <QDebug>

PfdQmlGadget::PfdQmlGadget(QString classId, PfdQmlGadgetWidget *widget, QWidget *parent) :
    IUAVGadget(classId, parent),
    m_widget(widget)
{
}

PfdQmlGadget::~PfdQmlGadget()
{
    delete m_widget;
}

QWidget *PfdQmlGadget::widget()
{
    return m_widget;
}

/*
   This is called when a configuration is loaded, and updates the plugin's settings.
   Careful: the plugin is already drawn before the loadConfiguration method is called the
   first time, so you have to be careful not to assume all the plugin values are initialized
   the first time you use them
 */
void PfdQmlGadget::loadConfiguration(IUAVGadgetConfiguration *config)
{
    PfdQmlGadgetConfiguration *m = qobject_cast<PfdQmlGadgetConfiguration *>(config);

    qDebug() << "PfdQmlGadget - loading configuration :" << m->name();

    // clear widget
    m_widget->setQmlFile("");

    m_widget->setSpeedFactor(m->speedFactor());
    m_widget->setSpeedUnit(m->speedUnit());
    m_widget->setAltitudeFactor(m->altitudeFactor());
    m_widget->setAltitudeUnit(m->altitudeUnit());

    // terrain
    m_widget->setTerrainEnabled(m->terrainEnabled());
    m_widget->setTerrainFile(m->terrainFile());

    m_widget->setPositionMode(m->positionMode());
    m_widget->setLatitude(m->latitude());
    m_widget->setLongitude(m->longitude());
    m_widget->setAltitude(m->altitude());

    // sky
    m_widget->setTimeMode(m->timeMode());
    m_widget->setDateTime(m->dateTime());
    m_widget->setMinimumAmbientLight(m->minAmbientLight());

    // model
    m_widget->setModelFile(m->modelFile());

    // background image
    m_widget->setBackgroundImageFile(m->backgroundImageFile());

    // go!
    m_widget->setQmlFile(m->qmlFile());
}
