/**
 ******************************************************************************
 *
 * @file       pfdqmlgadgetwidget.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************//*
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

#include "pfdqmlgadgetwidget.h"
#include "pfdqmlcontext.h"

#include "utils/quickwidgetproxy.h"
#include "utils/svgimageprovider.h"

#include <QLayout>
#include <QStackedLayout>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>

PfdQmlGadgetWidget::PfdQmlGadgetWidget(QWidget *parent) :
    QWidget(parent), m_quickWidgetProxy(NULL), m_pfdQmlContext(NULL), m_qmlFileName()
{
    setLayout(new QStackedLayout());
}

PfdQmlGadgetWidget::~PfdQmlGadgetWidget()
{
    if (m_pfdQmlContext) {
        delete m_pfdQmlContext;
    }
}

void PfdQmlGadgetWidget::setSource(const QUrl &url)
{
    m_quickWidgetProxy->setSource(url);
}

QQmlEngine *PfdQmlGadgetWidget::engine() const
{
    return m_quickWidgetProxy->engine();
}

QList<QQmlError> PfdQmlGadgetWidget::errors() const
{
    return m_quickWidgetProxy->errors();
}

void PfdQmlGadgetWidget::loadConfiguration(PfdQmlGadgetConfiguration *config)
{
    // qDebug() << "PfdQmlGadgetWidget::loadConfiguration" << config->name();

    if (!m_quickWidgetProxy) {
        m_quickWidgetProxy = new QuickWidgetProxy(this);

#if 0
        qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent OpenGL context" << isPersistentOpenGLContext();
        qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent scene graph" << isPersistentSceneGraph();
#endif

        // expose context
        m_pfdQmlContext = new PfdQmlContext(this);
        m_pfdQmlContext->apply(engine()->rootContext());

        // add widget
        layout()->addWidget(m_quickWidgetProxy->widget());
    }

    clear();

    m_pfdQmlContext->loadConfiguration(config);

    // go!
    setQmlFile(config->qmlFile());
}

void PfdQmlGadgetWidget::saveState(QSettings &settings) const
{
    m_pfdQmlContext->saveState(settings);
}

void PfdQmlGadgetWidget::restoreState(QSettings &settings)
{
    m_pfdQmlContext->restoreState(settings);
}

void PfdQmlGadgetWidget::setQmlFile(QString fn)
{
    qDebug() << "PfdQmlGadgetWidget::setQmlFile" << fn;

    m_qmlFileName = fn;

    if (fn.isEmpty()) {
        clear();
        return;
    }
    SvgImageProvider *svgProvider = new SvgImageProvider(fn);
    engine()->addImageProvider("svg", svgProvider);

    // it's necessary to allow qml side to query svg element position
    engine()->rootContext()->setContextProperty("svgRenderer", svgProvider);

    QUrl url = QUrl::fromLocalFile(fn);
    engine()->setBaseUrl(url);

    setSource(url);

    foreach(const QQmlError &error, errors()) {
        qDebug() << "PfdQmlGadgetWidget - " << error.description();
    }
}

void PfdQmlGadgetWidget::clear()
{
    // qDebug() << "PfdQmlGadgetWidget::clear";

    setSource(QUrl());

    engine()->removeImageProvider("svg");
    engine()->rootContext()->setContextProperty("svgRenderer", NULL);

    // calling clearComponentCache() causes crashes (see https://bugreports.qt-project.org/browse/QTBUG-41465)
    // but not doing it causes almost systematic crashes when switching PFD gadget to "Model View (Without Terrain)" configuration
    engine()->clearComponentCache();
}
