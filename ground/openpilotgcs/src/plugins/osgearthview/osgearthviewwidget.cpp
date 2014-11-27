/********************************************************************************
 * @file       osgearthviewwidget.cpp
 * @author     The OpenPilot Team Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OsgEarthview Plugin Widget
 * @{
 * @brief Osg Earth view of UAV
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

#include "osgearthviewwidget.h"
#include "ui_osgearthview.h"

#include <QPainter>

OsgEarthviewWidget::OsgEarthviewWidget(QWidget *parent) : QWidget(parent)
{
    m_widget = new Ui_OsgEarthview();
    m_widget->setupUi(this);

    /*viewWidget = new OsgViewerWidget(this);
       viewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

       setLayout(new QVBoxLayout());
       layout()->addWidget(viewWidget);*/
}

OsgEarthviewWidget::~OsgEarthviewWidget()
{}

void OsgEarthviewWidget::setSceneFile(QString sceneFile)
{
    m_widget->widget->setSceneFile(sceneFile);
}

void OsgEarthviewWidget::paintEvent(QPaintEvent *event)
{}

void OsgEarthviewWidget::resizeEvent(QResizeEvent *event)
{}
