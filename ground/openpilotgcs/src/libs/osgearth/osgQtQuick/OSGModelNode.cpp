#include "OSGModelNode.hpp"

#include <osg/Quat>

#include <osgEarth/MapNode>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>

#include <osgEarthSymbology/Style>
#include <osgEarthSymbology/ModelSymbol>

#include <osgEarthAnnotation/ModelNode>

#include <QDebug>

namespace osgQtQuick {

struct OSGModelNode::Hidden : public QObject
{
    Q_OBJECT

    struct NodeUpdateCallback : public osg::NodeCallback
    {
    public:
        NodeUpdateCallback(Hidden *h);

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGModelNode *parent) : QObject(parent), self(parent), modelData(NULL), sceneData(NULL), dirty(false)
    {
        roll = pitch = yaw = 0.0;
        latitude = longitude = altitude = 0.0;
    }

    ~Hidden()
    {
    }

    bool acceptModelData(OSGNode *node)
    {
        qDebug() << "OSGModelNode - acceptModelData" << node;
        if (modelData == node) {
            return false;
        }

        if (modelData) {
            disconnect(modelData);
        }

        modelData = node;

        if (modelData) {
            acceptModelNode(modelData->node());
            connect(modelData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onModelNodeChanged(osg::Node*)));
        }

        return true;
    }


    bool acceptModelNode(osg::Node *node)
    {
        qDebug() << "OSGModelNode acceptModelNode" << node;
        if (!node) {
            qWarning() << "OSGModelNode - acceptModelNode - node is null";
            return false;
        }

        if (!sceneData || !sceneData->node()) {
            qWarning() << "OSGModelNode - acceptModelNode - no scene data";
            return false;
        }

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
        if (!mapNode) {
            qWarning() << "OSGModelNode - acceptModelNode - scene data does not contain a map node";
            return false;
        }

        // establish the coordinate system we wish to use:
        //const osgEarth::SpatialReference* latLong = osgEarth::SpatialReference::get("wgs84");

//        osgEarth::Config conf("style");
//        conf.set("auto_scale", true);
//        osgEarth::Symbology::Style style(conf);
        osgEarth::Symbology::Style style;

        // construct the symbology
        osgEarth::Symbology::ModelSymbol *modelSymbol = style.getOrCreate<osgEarth::Symbology::ModelSymbol>();
        modelSymbol->setModel(node);

        // make a ModelNode
        modelNode = new osgEarth::Annotation::ModelNode(mapNode, style);

        if (!nodeUpdateCB.valid()) {
            nodeUpdateCB = new NodeUpdateCallback(this);
        }
        modelNode->addUpdateCallback(nodeUpdateCB.get());

        mapNode->addChild(modelNode);

        self->setNode(modelNode);

        dirty = true;

        return true;
    }

    bool acceptSceneData(OSGNode *node)
    {
        qDebug() << "OSGModelNode - acceptSceneData" << node;
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            // TODO find a better way
            if (modelData && modelData->node()) {
                acceptModelNode(modelData->node());
            }
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onSceneNodeChanged(osg::Node*)));
        }

        return true;
    }

    // TODO model update gets jitter if camera is thrown (i.e animated)
    void updateNode()
    {
        if (!dirty || !modelNode.valid()) {
            return;
        }
        dirty = false;

        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
                longitude, latitude, altitude, osgEarth::ALTMODE_ABSOLUTE);
        modelNode->setPosition(geoPoint);

        osg::Quat q = osg::Quat(
                osg::DegreesToRadians(roll), osg::Vec3d(0, 1, 0),
                osg::DegreesToRadians(pitch), osg::Vec3d(1, 0, 0),
                osg::DegreesToRadians(yaw), osg::Vec3d(0, 0, -1));

        modelNode->setLocalRotation(q);

    }

    OSGModelNode *const self;

    OSGNode *modelData;
    OSGNode *sceneData;

    osg::ref_ptr<osgEarth::Annotation::ModelNode> modelNode;

    osg::ref_ptr<NodeUpdateCallback> nodeUpdateCB;

    qreal roll;
    qreal pitch;
    qreal yaw;

    double latitude;
    double longitude;
    double altitude;

    bool dirty;

private slots:

    void onModelNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode - onModelNodeChanged" << node;
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCB.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCB.get());
                }
            }
            if (node) {
                acceptModelNode(node);
            }
        }
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode - onSceneNodeChanged" << node;
        // TODO needs to be improved...
        if (modelData) {
            if (modelNode.valid()) {
                osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
                if (mapNode) {
                    mapNode->removeChild(modelNode);
                }
                if (nodeUpdateCB.valid()) {
                    modelNode->removeUpdateCallback(nodeUpdateCB.get());
                }
            }
            if (modelData->node()) {
                acceptModelNode(modelData->node());
            }
        }
    }

};

/* struct Hidden::NodeUpdateCallback */

OSGModelNode::Hidden::NodeUpdateCallback::NodeUpdateCallback(Hidden *h) : h(h) {}

void OSGModelNode::Hidden::NodeUpdateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    h->updateNode();
}

OSGModelNode::OSGModelNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGModelNode - <init>";
}

OSGModelNode::~OSGModelNode()
{
    qDebug() << "OSGModelNode - <destruct>";
}

OSGNode *OSGModelNode::modelData()
{
    return h->modelData;
}

void OSGModelNode::setModelData(OSGNode *node)
{
    if (h->acceptModelData(node)) {
        emit modelDataChanged(node);
    }
}

OSGNode *OSGModelNode::sceneData()
{
    return h->sceneData;
}

void OSGModelNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

qreal OSGModelNode::roll() const
{
    return h->roll;
}

void OSGModelNode::setRoll(qreal arg)
{
    if (h->roll!= arg) {
        h->roll = arg;
        h->dirty = true;
        emit rollChanged(roll());
    }
}

qreal OSGModelNode::pitch() const
{
    return h->pitch;
}

void OSGModelNode::setPitch(qreal arg)
{
    if (h->pitch!= arg) {
        h->pitch = arg;
        h->dirty = true;
        emit pitchChanged(pitch());
    }
}

qreal OSGModelNode::yaw() const
{
    return h->yaw;
}

void OSGModelNode::setYaw(qreal arg)
{
    if (h->yaw!= arg) {
        h->yaw = arg;
        h->dirty = true;
        emit yawChanged(yaw());
    }
}

double OSGModelNode::latitude() const
{
    return h->latitude;
}

void OSGModelNode::setLatitude(double arg)
{
    if (h->latitude != arg) {
        h->latitude = arg;
        h->dirty = true;
        emit latitudeChanged(latitude());
    }
}

double OSGModelNode::longitude() const
{
    return h->longitude;
}

void OSGModelNode::setLongitude(double arg)
{
    if (h->longitude != arg) {
        h->longitude = arg;
        h->dirty = true;
        emit longitudeChanged(longitude());
    }
}

double OSGModelNode::altitude() const
{
    return h->altitude;
}

void OSGModelNode::setAltitude(double arg)
{
    if (h->altitude!= arg) {
        h->altitude = arg;
        h->dirty = true;
        emit altitudeChanged(altitude());
    }
}


} // namespace osgQtQuick

#include "OSGModelNode.moc"
