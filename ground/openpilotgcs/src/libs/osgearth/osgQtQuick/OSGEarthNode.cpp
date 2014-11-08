#include "OSGEarthNode.hpp"

#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/Group>

#include <osgDB/ReadFile>

#include <osgEarth/Config>
#include <osgEarth/DateTime>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/ObjectLocator>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>
#include <osgEarthQt/DataManager>

#include <QUrl>
#include <QDebug>
#include <QElapsedTimer>

namespace osgQtQuick {

struct OSGEarthNode::Hidden
{
public:
    QUrl url;

    osg::Node *createNode() {
        QElapsedTimer t;
        t.start();
        qDebug() << "OSGEarthNode - scene URL" << url;
        QString s = url.toString();
        s = s.right(s.length() - QString("file://").length());
        qDebug() << "OSGEarthNode - scene file" << s;
        // TODO use Options to control caching...
        osg::Node *sceneNode = osgDB::readNodeFile(s.toStdString());
        qDebug() << "OSGEarthNode - sceneNode" << sceneNode;
        qDebug() << "OSGEarthNode - read took" << t.elapsed();

        osg::Group *rootNode = new osg::Group;
        rootNode->addChild(sceneNode);

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneNode);
        //dataManager = new osgEarth::QtGui::DataManager(mapNode);

        //osg::Camera *camera = new osgEarth::Util::AutoClipPlaneCullCallback(mapNode);
        //camera->addChild(sceneNode);
        //camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        if (mapNode) {
            const osgEarth::Config& externals = mapNode->externalConfig();

            if (mapNode->getMap()->isGeocentric()) {
                // Sky model.
                osgEarth::Config skyConf = externals.child("sky");

                double hours = skyConf.value("hours", 12.0);
                osgEarth::Util::SkyNode *skyNode = osgEarth::Util::SkyNode::create(mapNode);
                skyNode->setDateTime(osgEarth::DateTime(2011, 3, 6, hours));
//                for (osgEarth::QtGui::ViewVector::iterator i = views.begin(); i != views.end(); ++i)
//                    s_sky->attach(*i, 0);
                rootNode->addChild(skyNode);

                // Ocean surface.
//                if (externals.hasChild("ocean")) {
//                    s_ocean = osgEarth::Util::OceanNode::create(osgEarth::Util::OceanOptions(externals.child("ocean")),
//                            mapNode);
//
//                    if (s_ocean)
//                        root->addChild(s_ocean);
//                }
            }
        }

        return rootNode;
    }

private:
    //QTimer _timer;
    //osgEarth::Util::EarthManipulator *manip;
    //osgEarth::Util::ObjectLocatorNode *uavPos;
    //osg::MatrixTransform *uavAttitudeAndScale;
    //osg::Group *rootNode;
    //osg::Node *sceneNode;
    //osgEarth::MapNode *mapNode;
    // TODO needed?
    //osgEarth::QtGui::DataManager *dataManager;
    //osgEarth::QtGui::ViewerWidget *viewerWidget;
    //osg::ref_ptr<osgViewer::ViewerBase> viewer;

};

OSGEarthNode::OSGEarthNode(QObject *parent) :
            OSGNode(parent),
            h(new Hidden)
{
    qDebug() << "OSGEarthNode - <init>";
}

OSGEarthNode::~OSGEarthNode()
{
    qDebug() << "OSGEarthNode - <destruct>";
    delete h;
}

const QUrl OSGEarthNode::source() const
{
    return h->url;
}

void OSGEarthNode::setSource(const QUrl &url)
{
    qDebug() << "OSGEarthNode - setSource" << url;
    if (h->url != url)
    {
        h->url = url;
        setNode(h->createNode());
        emit sourceChanged(url);
    }
}


} // namespace osgQtQuick
