/**
 ******************************************************************************
 *
 * @file       helpdialog.h
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

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QList>

class Ui_HelpDialog;

class HelpDialog : public QDialog {
    Q_OBJECT

public:
    HelpDialog(QWidget *parent, const QString &initialElement = QString());
    ~HelpDialog();

    // Run the dialog and return true if 'Ok' was choosen or 'Apply' was invoked
    // at least once
    bool execDialog();

private slots:
    void itemSelected();
    void close();

private:
    Ui_HelpDialog *m_helpDialog;

    QList<QString> m_elements;

    int m_windowWidth;
    int m_windowHeight;
};

#endif // HELPDIALOG_H
