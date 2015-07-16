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

void PfdQmlGadgetWidget::init()
{
    m_quickWidgetProxy = new QuickWidgetProxy(this);

#if 0
    qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent OpenGL context" << isPersistentOpenGLContext();
    qDebug() << "PfdQmlGadgetWidget::PfdQmlGadgetWidget - persistent scene graph" << isPersistentSceneGraph();
#endif

    // to expose settings values
    m_pfdQmlContext = new PfdQmlContext(this);
    m_pfdQmlContext->apply(engine()->rootContext());

// connect(this, &QQuickWidget::statusChanged, this, &PfdQmlGadgetWidget::onStatusChanged);
// connect(this, &QQuickWidget::sceneGraphError, this, &PfdQmlGadgetWidget::onSceneGraphError);
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
    qDebug() << "PfdQmlGadgetWidget::loadConfiguration" << config->name();

    QuickWidgetProxy *oldQuickWidgetProxy = NULL;
    PfdQmlContext *oldPfdQmlContext = NULL;
    if (m_quickWidgetProxy) {
        oldQuickWidgetProxy = m_quickWidgetProxy;
        oldPfdQmlContext = m_pfdQmlContext;
        m_quickWidgetProxy = NULL;
        m_pfdQmlContext = NULL;
    }

    if (!m_quickWidgetProxy) {
        init();
    }

    // here we first clear the Qml file
    // then set all the properties
    // and finally set the desired Qml file
    // TODO this is a work around... some OSG Quick items don't yet handle properties updates well

    // clear widget
    // setQmlFile("");

    // no need to go further is Qml file is empty
    // if (config->qmlFile().isEmpty()) {
    // return;
    // }

    m_pfdQmlContext->loadConfiguration(config);

    // go!
    setQmlFile(config->qmlFile());

    // deleting and recreating the PfdQmlGadgetWidget is workaround to avoid crashes in osgearth when
    // switching between configurations. Please remove this workaround once osgearth is stabilized
    if (oldQuickWidgetProxy) {
        layout()->removeWidget(oldQuickWidgetProxy->widget());
        delete oldQuickWidgetProxy;
        delete oldPfdQmlContext;
    }
    layout()->addWidget(m_quickWidgetProxy->widget());
}

void PfdQmlGadgetWidget::setQmlFile(QString fn)
{
// if (m_qmlFileName == fn) {
// return;
// }
    qDebug() << Q_FUNC_INFO << fn;

    m_qmlFileName = fn;

    if (fn.isEmpty()) {
        setSource(QUrl());

        engine()->removeImageProvider("svg");
        engine()->rootContext()->setContextProperty("svgRenderer", NULL);

        // calling clearComponentCache() causes crashes (see https://bugreports.qt-project.org/browse/QTBUG-41465)
        // but not doing it causes almost systematic crashes when switching PFD gadget to "Model View (Without Terrain)" configuration
        engine()->clearComponentCache();
    } else {
        SvgImageProvider *svgProvider = new SvgImageProvider(fn);
        engine()->addImageProvider("svg", svgProvider);

        // it's necessary to allow qml side to query svg element position
        engine()->rootContext()->setContextProperty("svgRenderer", svgProvider);

        QUrl url = QUrl::fromLocalFile(fn);
        engine()->setBaseUrl(url);
        setSource(url);
    }

    foreach(const QQmlError &error, errors()) {
        qDebug() << "PfdQmlGadgetWidget - " << error.description();
    }
}

