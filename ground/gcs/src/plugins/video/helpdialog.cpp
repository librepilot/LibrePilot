/**
 ******************************************************************************
 *
 * @file       helpdialog.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup VideoGadgetPlugin Video Gadget Plugin
 * @{
 * @brief A video gadget plugin
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

#include "helpdialog.h"

#include "ui_helpdialog.h"

#include <QDebug>
#include <QPushButton>

HelpDialog::HelpDialog(QWidget *parent, const QString &elementId)
    : QDialog(parent),
    m_windowWidth(0),
    m_windowHeight(0)
{
    Q_UNUSED(elementId)

    m_helpDialog = new Ui_HelpDialog();
    m_helpDialog->setupUi(this);

    setWindowTitle(tr("GStreamer Help"));

    if (m_windowWidth > 0 && m_windowHeight > 0) {
        resize(m_windowWidth, m_windowHeight);
    }

    m_helpDialog->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    connect(m_helpDialog->buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close()));

    m_helpDialog->splitter->setCollapsible(0, false);
    m_helpDialog->splitter->setCollapsible(1, false);

    connect(m_helpDialog->elementListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(pageSelected()));

    QList<QString> plugins; // = gst::pluginList();

// foreach(QString pluginName, plugins) {
// new QListWidgetItem(pluginName, m_helpDialog->elementListWidget);
// }
}

HelpDialog::~HelpDialog()
{
// foreach(QString category, m_categoryItemsMap.keys()) {
// QList<QTreeWidgetItem *> *categoryItemList = m_categoryItemsMap.value(category);
// delete categoryItemList;
// }
}

void HelpDialog::itemSelected()
{}

void HelpDialog::close()
{}

bool HelpDialog::execDialog()
{
    exec();
    return true;
}
