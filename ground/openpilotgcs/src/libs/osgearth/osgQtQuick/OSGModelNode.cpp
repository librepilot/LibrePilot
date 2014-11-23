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
    Hidden(OSGModelNode *parent) : QObject(parent), pub(parent), modelData(NULL), sceneData(NULL), dirty(false)
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

        modelData = NULL;

        if (!acceptModelNode(node->node())) {
            //return false;
        }

        modelData = node;

        if (modelData) {
            connect(modelData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onModelNodeChanged(osg::Node*)));
        }

        return true;
    }


    bool acceptModelNode(osg::Node *node)
    {
        qDebug() << "OSGModelNode - acceptModelNode" << node;
        if (!sceneData) {
            qWarning() << "no scene data";
            return false;
        }

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneData->node());
        if (!mapNode) {
            qWarning() << "scene data does not contain a map node";
            return false;
        }

        // establish the coordinate system we wish to use:
        //const osgEarth::SpatialReference* latLong = osgEarth::SpatialReference::get("wgs84");

        // construct the symbology
        osgEarth::Symbology::Style style;
        style.getOrCreate<osgEarth::Symbology::ModelSymbol>()->setModel(node);

        // make a ModelNode
        modelNode = new osgEarth::Annotation::ModelNode(mapNode, style);

        modelNode->addUpdateCallback(new NodeUpdateCallback(this));

        mapNode->addChild(modelNode);

        pub->setNode(modelNode);

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

        sceneData = NULL;

        //if (!acceptNode(node->node())) {
            //return false;
        //}

        sceneData = node;

        if (sceneData) {
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onSceneNodeChanged(osg::Node*)));
        }

        // TODO find a better way
        if (modelData) {
            acceptModelNode(modelData->node());
        }

        return true;
    }

    void updateNode()
    {
        if (!dirty) {
            return;
        }
        dirty = false;

        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"),
                longitude, latitude, altitude, osgEarth::ALTMODE_ABSOLUTE);
        modelNode->setPosition(geoPoint);

        osg::Quat q = osg::Quat(
                osg::DegreesToRadians(roll), osg::Vec3d(1, 0, 0),
                osg::DegreesToRadians(pitch), osg::Vec3d(0, 1, 0),
                osg::DegreesToRadians(yaw), osg::Vec3d(0, 0, 1));

        modelNode->setLocalRotation(q);
    }

    OSGModelNode *pub;

    OSGNode *modelData;
    OSGNode *sceneData;

    osg::ref_ptr<osgEarth::Annotation::ModelNode> modelNode;

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
        acceptModelNode(node);
    }

    void onSceneNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGModelNode - onSceneNodeChanged" << node;
        if (modelData) {
            acceptModelNode(modelData->node());
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
    // not sure qFuzzyCompare is accurate enough for geo coordinates
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
