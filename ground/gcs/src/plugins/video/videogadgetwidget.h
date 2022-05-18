/**
 ******************************************************************************
 *
 * @file       videogadgetwidget.h
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

#ifndef VIDEOGADGETWIDGET_H_
#define VIDEOGADGETWIDGET_H_

#include "pipeline.h"
#include "ui_video.h"

#include <QFrame>
#include <QtCore/QEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QPaintEvent>

class VideoWidget;
class VideoGadgetConfiguration;

class VideoGadgetWidget : public QFrame {
    Q_OBJECT

public:
    VideoGadgetWidget(QWidget *parent = 0);
    ~VideoGadgetWidget();

    void setConfiguration(VideoGadgetConfiguration *config);

private slots:
    void start();
    void pause();
    void stop();
    void console();

    void onStateChanged(Pipeline::State oldState, Pipeline::State newState, Pipeline::State pendingState);

private:
    Ui_Form *m_ui;
    VideoGadgetConfiguration *config;

    void msg(const QString &str);

    VideoWidget *videoWidget();
};

#endif /* VIDEOGADGETWIDGET_H_ */
