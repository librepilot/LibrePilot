/**
 ******************************************************************************
 *
 * @file       aboutdialog.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
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

#include "aboutdialog.h"

#include "version_info/version_info.h"
#include "coreconstants.h"
#include "icore.h"

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QSysInfo>
#include <QDesktopServices>
#include <QLayout>
#include <QVBoxLayout>

#include <QtQuick>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>

using namespace Core::Constants;

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowIcon(QIcon(":/core/images/librepilot_logo_32.png"));
    setWindowTitle(tr("About %1").arg(ORG_BIG_NAME));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(600, 400);
    setMaximumSize(800, 600);

    const QString description = tr(
        "Revision: <b>%1</b><br/>"
        "UAVO Hash: <b>%2</b><br/>"
        "<br/>"
        "Built from %3<br/>"
        "Built on %4 at %5<br/>"
        "Based on Qt %6 (%7 bit)<br/>"
        "<br/>"
        "\u00A9 The %8 Project, 2015-%9. All rights reserved.<br/>"
        "\u00A9 The OpenPilot Project, 2010-2015. All rights reserved.<br/>"
        ).arg(
        VersionInfo::revision().left(60), // %1
        VersionInfo::uavoHash().left(8), // %2
        VersionInfo::origin(), // $3
        QLatin1String(__DATE__), // %4
        QLatin1String(__TIME__), // %5
        QLatin1String(QT_VERSION_STR), // %6
        QString::number(QSysInfo::WordSize), // %7
        QLatin1String(ORG_BIG_NAME), // %8
        VersionInfo::year() // %9
        );

    // %1 = name, %2 = description, %3 = url, %4 = image url (not used)
    // <td><img src=\"%4\" size=\"32\"></td>
    QString creditRow     = "<tr padding=10><td><b>%1</b>%2<br/></td><td><a href=\"%3\">%3</a></td></tr>";

    // uses Text.StyledText (see http://doc.qt.io/qt-5/qml-qtquick-text.html#textFormat-prop)
    const QString credits = "<table width=\"100%\">"
                            + creditRow.arg("Tau Labs", "", "http://www.taulabs.org")
                            + creditRow.arg("dRonin", "", "http://www.dronin.org")
                            + creditRow.arg("OpenSceneGraph", "<br/>Open source high performance 3D graphics toolkit", "http://www.openscenegraph.org")
                            + creditRow.arg("osgEarth", "<br/>Geospatial SDK for OpenSceneGraph", "http://osgearth.org")
                            + creditRow.arg("gstreamer", "<br/>Open source multimedia framework", "https://gstreamer.freedesktop.org/")
                            + creditRow.arg("MSYS2", "<br/>An independent rewrite of MSYS", "https://github.com/msys2/msys2/wiki")
                            + creditRow.arg("The Qt Company", "", "https://www.qt.io")
                            + "</table>";

    // uses Text.StyledText (see http://doc.qt.io/qt-5/qml-qtquick-text.html#textFormat-prop)
    const QString license = tr("This program is free software; you can redistribute it and/or "
                               "modify it under the terms of the GNU General Public License "
                               "as published by the Free Software Foundation; either version 3 "
                               "of the License, or (at your option) any later version.\n"
                               "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
                               "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."
                               );


    QQuickView *view = new QQuickView();
    view->rootContext()->setContextProperty("dialog", this);
    view->rootContext()->setContextProperty("version", description);
    view->rootContext()->setContextProperty("credits", credits);
    view->rootContext()->setContextProperty("license", license);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/core/qml/AboutDialog.qml"));

    QWidget *container = QWidget::createWindowContainer(view);
    container->setMinimumSize(600, 400);
    container->setMaximumSize(800, 600);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    setLayout(lay);
    layout()->addWidget(container);
}

void AboutDialog::openUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

AboutDialog::~AboutDialog()
{}
