/**
 ******************************************************************************
 *
 * @file       pfdqmlgadgetwidget.h
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

#ifndef PFDQMLGADGETWIDGET_H_
#define PFDQMLGADGETWIDGET_H_

#include "pfdqml.h"
#include "pfdqmlgadgetconfiguration.h"

#include <QWidget>

class QQmlEngine;
class QSettings;
class QuickWidgetProxy;
class PfdQmlContext;

class PfdQmlGadgetWidget : public QWidget {
    Q_OBJECT

public:
    PfdQmlGadgetWidget(QWidget *parent = 0);
    virtual ~PfdQmlGadgetWidget();

    void loadConfiguration(PfdQmlGadgetConfiguration *config);
    void saveState(QSettings &) const;
    void restoreState(QSettings &);

private:
    QuickWidgetProxy *m_quickWidgetProxy;

    PfdQmlContext *m_pfdQmlContext;
    QString m_qmlFileName;

    void setQmlFile(QString);
    void clear();

    void setSource(const QUrl &url);
    QQmlEngine *engine() const;
    QList<QQmlError> errors() const;
};

#endif /* PFDQMLGADGETWIDGET_H_ */
