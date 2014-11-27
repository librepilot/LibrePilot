/********************************************************************************

 * @file       osgviewerwidget.h
 * @author     The OpenPilot Team Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OsgEarthview Plugin
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

#ifndef OSGVIEWERWIDGET_H
#define OSGVIEWERWIDGET_H

#include "osgearthviewgadgetconfiguration.h"
//#include "extensionsystem/pluginmanager.h"
//#include "uavobjectmanager.h"
//#include "uavobject.h"

#include <QTimer>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

#include <osgEarth/MapNode>

#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/ObjectLocator>

#include <osgQt/GraphicsWindowQt>
#include <osgEarthQt/ViewerWidget>
#include <osgEarthQt/DataManager>

using namespace osgEarth::Util;
using namespace osgEarth::QtGui;

class MyViewerWidget : public ViewerWidget {
    Q_OBJECT
public:
    MyViewerWidget(osg::Node* scene=0L) : ViewerWidget(scene)
    {
        qDebug() << "************ construct";
    }

    virtual ~MyViewerWidget() {
        qDebug() << "************ delete";
    }
public slots:
    void update() {
        qDebug() << "************ update";
        ViewerWidget::update();
    }
protected:
    void paintEvent(QPaintEvent *event) {
//        qDebug() << "************ paint";
//        QElapsedTimer t;
//        t.start();
        ViewerWidget::paintEvent(event);
//        qDebug() << "************ paint took" << t.elapsed();
    }

    void installFrameTimer() {
        qDebug() << "************ install timer";
        ViewerWidget::installFrameTimer();
    }

    void createViewer() {
        qDebug() << "************ create viewer";
        ViewerWidget::createViewer();
    }

};

//class LoaderThread : public QThread
//{
//    Q_OBJECT
//    void run() Q_DECL_OVERRIDE {
//        osg::Node *sceneNode = loadScene(m_sceneFile);
//        emit sceneLoaded(m_sceneFile, sceneNode);
//    }
//public:
//    LoaderThread(QString &sceneFile, QObject *parent = 0) : QThread(parent), m_sceneFile(sceneFile) {}
//
//    QString& sceneFile()
//    {
//        return m_sceneFile;
//    }
//
//signals:
//    void sceneLoaded(QString sceneFile, osg::Node *sceneNode);
//
//private:
//    osg::Node *loadScene(QString &sceneFile);
//    QString m_sceneFile;
//};

class OsgViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit OsgViewerWidget(QWidget *parent = 0);
    ~OsgViewerWidget();

    void setSceneFile(QString &sceneFile);

protected:
    /* Create a osgQt::GraphicsWindowQt to add to the widget */
    QWidget *createViewWidget(osg::Camera *camera, osg::Node *scene);

    /* Create an osg::Camera which sets up the OSG view */
    osg::Camera *createCamera(int x, int y, int w, int h, const std::string & name, bool windowDecoration);

    /* Get the model to render */
    osg::Node *createAirplane();

private slots:
    void setScene(osg::Node *sceneNode);
    void setLoadedScene(QString sceneFile, osg::Node *sceneNode);


private:
    QString m_sceneFile;
    QTimer _timer;
    EarthManipulator *manip;
    osgEarth::Util::ObjectLocatorNode *uavPos;
    osg::MatrixTransform *uavAttitudeAndScale;
    osg::Group *rootNode;
    osg::Node *sceneNode;
    osgEarth::MapNode *mapNode;
    osgEarth::QtGui::DataManager *dataManager;
    osgEarth::QtGui::ViewerWidget *viewerWidget;
    osg::ref_ptr<osgViewer::ViewerBase> viewer;
};

#endif // OSGVIEWERWIDGET_H
