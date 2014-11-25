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
    Hidden(OSGSkyNode *parent) : QObject(parent), pub(parent), sceneData(NULL)
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

        sceneData = NULL;

        if (!acceptNode(node->node())) {
            //return false;
        }

        sceneData = node;

        if (sceneData) {
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGSkyNode - acceptNode" << node;
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (!mapNode) {
            qWarning() << "scene data does not contain a map node";
            return false;
        }
        if (!mapNode->getMap()->isGeocentric()) {
            qWarning() << "map node is not geocentric";
            return false;
        }
        //dataManager = new osgEarth::QtGui::DataManager(mapNode);

        //osg::Camera *camera = new osgEarth::Util::AutoClipPlaneCullCallback(mapNode);
        //camera->addChild(sceneData);
        //camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        const osgEarth::Config& externals = mapNode->externalConfig();

        // Sky model.
        osgEarth::Config skyConf = externals.child("sky");
        double hours = skyConf.value("hours", 12.0);

        //osgEarth::Util::SkyOptions *skyOptions = new osgEarth::Util::SkyOptions();

        skyNode = osgEarth::Util::SkyNode::create(mapNode);

        skyNode->setLighting(osg::StateAttribute::OFF);
        //skyNode->setAmbientBrightness(ambientBrightness);
        skyNode->setDateTime(osgEarth::DateTime(2011, 3, 6, hours));
        //                for (osgEarth::QtGui::ViewVector::iterator i = views.begin(); i != views.end(); ++i)
        //                    s_sky->attach(*i, 0);
        //rootNode->addChild(skyNode);

        this->node = node;
        skyNode->addChild(node);

        // Ocean surface.
        //                if (externals.hasChild("ocean")) {
        //                    s_ocean = osgEarth::Util::OceanNode::create(osgEarth::Util::OceanOptions(externals.child("ocean")),
        //                            mapNode);
        //
        //                    if (s_ocean)
        //                        root->addChild(s_ocean);
        //                }

        pub->setNode(skyNode);

        return true;
    }

    OSGSkyNode *pub;

    OSGNode *sceneData;

    osg::ref_ptr<osgEarth::Util::SkyNode> skyNode;
    osg::ref_ptr<osg::Node> node;

private slots:

    void onNodeChanged(osg::Node *node) {
        qDebug() << "OSGSkyNode - onNodeChanged" << node;
        skyNode->removeChild(this->node);
        this->node = node;
        skyNode->addChild(this->node);
        //acceptNode(node);
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
