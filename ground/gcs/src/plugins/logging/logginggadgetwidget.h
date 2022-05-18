/**
 ******************************************************************************
 *
 * @file       logginggadgetwidget.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2018.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup LoggingGadgetPlugin Logging Gadget Plugin
 * @{
 * @brief      A gadget to control playback of a GCS log.
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

#ifndef LoggingGADGETWIDGET_H_
#define LoggingGADGETWIDGET_H_

#include "loggingplugin.h"

#include "extensionsystem/pluginmanager.h"
#include "scope/scopeplugin.h"
#include "scope/scopegadgetfactory.h"

#include <QWidget>

class Ui_Logging;
class QTimer;

class LoggingGadgetWidget : public QWidget {
    Q_OBJECT

public:
    LoggingGadgetWidget(QWidget *parent = 0);
    ~LoggingGadgetWidget();
    void setPlugin(LoggingPlugin *p);

protected slots:
    void stateChanged(LoggingPlugin::State state);
    void setBeginAndEndTimes(quint32 startTimeStamp, quint32 endTimeStamp);
    void setPlaybackPosition(quint32 positionTimeStamp);
    void playButtonAction();
    void pauseButtonAction();
    void stopButtonAction();
    void enableWidgets();
    void disableWidgets();
    void sliderMoved(int);
    void sliderAction();

signals:
    void resumeReplay(quint32 positionTimeStamp);
    void pauseReplay();
    void pauseReplayAndResetPosition();

private:
    Ui_Logging *m_logging;
    LoggingPlugin *loggingPlugin;
    ScopeGadgetFactory *scpPlugin;
    QTimer sliderActionDelay;

    void updatePositionLabel(quint32 positionTimeStamp);
};

#endif /* LoggingGADGETWIDGET_H_ */
