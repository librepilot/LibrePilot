#include "OSGSkyNode.hpp"

#include <osgEarth/Config>
#include <osgEarth/DateTime>
#include <osgEarth/MapNode>
#include <osgEarthUtil/Sky>

#include <QDebug>

namespace osgQtQuick {

struct OSGSkyNode::Hidden : public QObject
{
    Q_OBJECT

public:
    Hidden(OSGSkyNode *parent) : QObject(parent), self(parent), sceneData(NULL)
    {
    }

    ~Hidden()
    {
    }

    bool acceptSceneNode(OSGNode *node)
    {
        qDebug() << "OSGSkyNode - acceptSceneNode" << node;
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;
        if (sceneData) {
            acceptNode(sceneData->node());
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGSkyNode - acceptNode" << node;

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (!mapNode) {
            qWarning() << "OSGSkyNode - scene data does not contain a map node";
            return false;
        }
        if (!mapNode->getMap()->isGeocentric()) {
            qWarning() << "OSGSkyNode - map node is not geocentric";
            return false;
        }

        // create sky node
        const osgEarth::Config& externals = mapNode->externalConfig();

        osgEarth::Config skyConf = externals.child("sky");
        double hours = skyConf.value("hours", 12.0);

        //osgEarth::Util::SkyOptions *skyOptions = new osgEarth::Util::SkyOptions();

        skyNode = osgEarth::Util::SkyNode::create(mapNode);

        skyNode->getSunLight()->setAmbient(osg::Vec4(0.8,0.8,0.8,1));

        //skyNode->setLighting(osg::StateAttribute::OFF);
        //skyNode->setAmbientBrightness(ambientBrightness);
        skyNode->setDateTime(osgEarth::DateTime(2011, 3, 6, hours));

        // Ocean
        //if (externals.hasChild("ocean")) {
        //s_ocean = osgEarth::Util::OceanNode::create(osgEarth::Util::OceanOptions(externals.child("ocean")), mapNode);
        //if (s_ocean) root->addChild(s_ocean);

        skyNode->addChild(node);

        self->setNode(skyNode);

        return true;
    }

    OSGSkyNode *const self;

    OSGNode *sceneData;
    osg::ref_ptr<osgEarth::Util::SkyNode> skyNode;

private slots:

    void onNodeChanged(osg::Node *node) {
        qDebug() << "OSGSkyNode - onNodeChanged" << node;
        acceptNode(node);
    }

};

OSGSkyNode::OSGSkyNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGSkyNode - <init>";
}

OSGSkyNode::~OSGSkyNode()
{
    qDebug() << "OSGSkyNode - <destruct>";
}

OSGNode *OSGSkyNode::sceneData()
{
    return h->sceneData;
}

void OSGSkyNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        emit sceneDataChanged(node);
    }
}

} // namespace osgQtQuick

#include "OSGSkyNode.moc"
