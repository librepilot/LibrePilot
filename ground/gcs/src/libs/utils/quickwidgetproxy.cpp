/**
 ******************************************************************************
 *
 * @file       quickwidgetproxy.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
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

#include "quickwidgetproxy.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>

/*
 * Note: QQuickWidget is an alternative to using QQuickView and QWidget::createWindowContainer().
 * The restrictions on stacking order do not apply, making QQuickWidget the more flexible alternative,
 * behaving more like an ordinary widget. This comes at the expense of performance.
 * Unlike QQuickWindow and QQuickView, QQuickWidget involves rendering into OpenGL framebuffer objects.
 * This will naturally carry a minor performance hit.
 *
 * Note: Using QQuickWidget disables the threaded render loop on all platforms.
 * This means that some of the benefits of threaded rendering, for example Animator classes
 * and vsync driven animations, will not be available.
 *
 * Note: Avoid calling winId() on a QQuickWidget. This function triggers the creation of a native window,
 * resulting in reduced performance and possibly rendering glitches.
 * The entire purpose of QQuickWidget is to render Quick scenes without a separate native window,
 * hence making it a native widget should always be avoided.
 */
QuickWidgetProxy::QuickWidgetProxy(QWidget *parent) : QObject(parent)
{
    m_widget = true;

    m_quickWidget = NULL;

    m_container   = NULL;
    m_quickView   = NULL;

    if (m_widget) {
        m_quickWidget = new QQuickWidget(parent);
        m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

        void (QuickWidgetProxy::*f)(QQuickWidget::Status) = &QuickWidgetProxy::onStatusChanged;
        connect(m_quickWidget, &QQuickWidget::statusChanged, this, f);
        connect(m_quickWidget, &QQuickWidget::sceneGraphError, this, &QuickWidgetProxy::onSceneGraphError);
    } else {
        m_quickView = new QQuickView();
        m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
        m_container = QWidget::createWindowContainer(m_quickView, parent);
        m_container->setMinimumSize(64, 64);
        m_container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        void (QuickWidgetProxy::*f)(QQuickView::Status) = &QuickWidgetProxy::onStatusChanged;
        connect(m_quickView, &QQuickView::statusChanged, this, f);
        connect(m_quickView, &QQuickView::sceneGraphError, this, &QuickWidgetProxy::onSceneGraphError);
    }
}

QuickWidgetProxy::~QuickWidgetProxy()
{
    if (m_quickWidget) {
        delete m_quickWidget;
    }
    if (m_quickView) {
        delete m_quickView;
        delete m_container;
    }
}

QWidget *QuickWidgetProxy::widget()
{
    if (m_widget) {
        return m_quickWidget;
    } else {
        return m_container;
    }
}

QQmlEngine *QuickWidgetProxy::engine() const
{
    if (m_widget) {
        return m_quickWidget->engine();
    } else {
        return m_quickView->engine();
    }
}

void QuickWidgetProxy::setSource(const QUrl &url)
{
    if (m_widget) {
        m_quickWidget->setSource(url);
    } else {
        m_quickView->setSource(url);
    }
}

QList<QQmlError> QuickWidgetProxy::errors() const
{
    if (m_widget) {
        return m_quickWidget->errors();
    } else {
        return m_quickView->errors();
    }
}

void QuickWidgetProxy::onStatusChanged(QQuickWidget::Status status)
{
    switch (status) {
    case QQuickWidget::Null:
        qWarning() << "QuickWidgetProxy - status Null";
        break;
    case QQuickWidget::Ready:
        // qDebug() << "QuickWidgetProxy - status Ready";
        break;
    case QQuickWidget::Loading:
        // qDebug() << "QuickWidgetProxy - status Loading";
        break;
    case QQuickWidget::Error:
        qWarning() << "QuickWidgetProxy - status Error";
        foreach(const QQmlError &error, errors()) {
            qWarning() << error.description();
        }
        break;
    }
}

void QuickWidgetProxy::onStatusChanged(QQuickView::Status status)
{
    switch (status) {
    case QQuickView::Null:
        qWarning() << "QuickWidgetProxy - status Null";
        break;
    case QQuickView::Ready:
        // qDebug() << "QuickWidgetProxy - status Ready";
        break;
    case QQuickView::Loading:
        // qDebug() << "QuickWidgetProxy - status Loading";
        break;
    case QQuickView::Error:
        qWarning() << "QuickWidgetProxy - status Error";
        foreach(const QQmlError &error, errors()) {
            qWarning() << error.description();
        }
        break;
    }
}

void QuickWidgetProxy::onSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
{
    qWarning() << QString("Scenegraph error %1: %2").arg(error).arg(message);
}
