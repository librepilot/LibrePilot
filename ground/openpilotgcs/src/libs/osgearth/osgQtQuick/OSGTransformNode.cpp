#include "OSGTransformNode.hpp"

#include <osg/Quat>
#include <osg/PositionAttitudeTransform>
#include <osg/Math>

#include <QDebug>

namespace osgQtQuick {

struct OSGTransformNode::Hidden : public QObject
{
    Q_OBJECT

public:

    Hidden(OSGTransformNode *parent) : QObject(parent), self(parent), modelData(NULL)
    {
    }

    ~Hidden()
    {
    }

    bool acceptModelData(OSGNode *node)
    {
        qDebug() << "OSGTransformNode - acceptModelData" << node;
        if (modelData == node) {
            return false;
        }

        if (modelData) {
            disconnect(modelData);
        }

        modelData = node;

        if (modelData) {
            acceptNode(modelData->node());
            connect(modelData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }


    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGTransformNode acceptNode" << node;
        if (!node) {
            qWarning() << "OSGTransformNode - acceptNode - node is null";
            return false;
        }

        transformNode = new osg::PositionAttitudeTransform();
        double s = 0.1;
        transformNode->setScale(osg::Vec3d(s, s, s));

        osg::Quat q = osg::Quat(osg::DegreesToRadians(180.0), osg::Vec3d(0, 0, 1));
        transformNode->setAttitude(q);

        //transformNode->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        transformNode->addChild(node);

        self->setNode(transformNode);

        return true;
    }

    OSGTransformNode *const self;

    OSGNode *modelData;

    osg::ref_ptr<osg::PositionAttitudeTransform> transformNode;

    double altitude;

private slots:

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGTransformNode - onNodeChanged" << node;
        acceptNode(node);
    }

};

OSGTransformNode::OSGTransformNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGTransformNode - <init>";
}

OSGTransformNode::~OSGTransformNode()
{
    qDebug() << "OSGTransformNode - <destruct>";
}

OSGNode *OSGTransformNode::modelData()
{
    return h->modelData;
}

void OSGTransformNode::setModelData(OSGNode *node)
{
    if (h->acceptModelData(node)) {
        emit modelDataChanged(node);
    }
}

double OSGTransformNode::altitude() const
{
    return h->altitude;
}

void OSGTransformNode::setAltitude(double arg)
{
    if (h->altitude!= arg) {
        h->altitude = arg;
        emit altitudeChanged(altitude());
    }
}

} // namespace osgQtQuick

#include "OSGTransformNode.moc"
