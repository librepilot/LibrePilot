#include "OSGTransformNode.hpp"

#include <osg/Quat>
#include <osg/PositionAttitudeTransform>
#include <osg/Math>

#include <QDebug>

namespace osgQtQuick {
struct OSGTransformNode::Hidden : public QObject {
    Q_OBJECT

    struct NodeUpdateCallback : public osg::NodeCallback {
    public:
        NodeUpdateCallback(Hidden *h) : h(h) {}

        void operator()(osg::Node *node, osg::NodeVisitor *nv);

        mutable Hidden *h;
    };
    friend class NodeUpdateCallback;

public:

    Hidden(OSGTransformNode *parent) : QObject(parent), self(parent), modelData(NULL), dirty(false)
    {}

    ~Hidden()
    {}

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
            connect(modelData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onNodeChanged(osg::Node *)));
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

        osg::Transform *transform = getOrCreateTransform();
        if (!transform) {
            qWarning() << "OSGTransformNode - acceptNode - transform is null";
            return false;
        }

        transform->addChild(node);

        self->setNode(transform);

        return true;
    }

    osg::Transform *getOrCreateTransform()
    {
        if (transform.valid()) {
            return transform.get();
        }

        transform = new osg::PositionAttitudeTransform();

        transform->addUpdateCallback(new NodeUpdateCallback(this));

        dirty = true;
        updateNode();

        return transform.get();
    }

    void updateNode()
    {
        if (!dirty || !transform.valid()) {
            return;
        }
        dirty = false;

        // scale
        if ((scale.x() != 0.0) || (scale.y() != 0.0) || (scale.z() != 0.0)) {
            transform->setScale(osg::Vec3d(scale.x(), scale.y(), scale.z()));
            //transform->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
            transform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);
        }

        // rotate
        osg::Quat q = osg::Quat(
                osg::DegreesToRadians(rotate.x()), osg::Vec3d(1, 0, 0),
                osg::DegreesToRadians(rotate.y()), osg::Vec3d(0, 1, 0),
                osg::DegreesToRadians(rotate.z()), osg::Vec3d(0, 0, 1));
        transform->setAttitude(q);

        // translate
        transform->setPosition(osg::Vec3d(translate.x(), translate.y(), translate.z()));

    }

    OSGTransformNode *const self;

    OSGNode *modelData;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform;

    bool   dirty;

    QVector3D scale;
    QVector3D rotate;
    QVector3D translate;

private slots:

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGTransformNode - onNodeChanged" << node;
        acceptNode(node);
    }
};

/* struct Hidden::NodeUpdateCallback */

void OSGTransformNode::Hidden::NodeUpdateCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    h->updateNode();
    traverse(node, nv);
}

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

QVector3D OSGTransformNode::scale() const
{
    return h->scale;
}

void OSGTransformNode::setScale(QVector3D arg)
{
    if (h->scale != arg) {
        h->scale = arg;
        h->dirty = true;
        emit scaleChanged(scale());
    }
}

QVector3D OSGTransformNode::rotate() const
{
    return h->rotate;
}

void OSGTransformNode::setRotate(QVector3D arg)
{
    if (h->rotate != arg) {
        h->rotate = arg;
        h->dirty = true;
        emit rotateChanged(rotate());
    }
}

QVector3D OSGTransformNode::translate() const
{
    return h->translate;
}

void OSGTransformNode::setTranslate(QVector3D arg)
{
    if (h->translate != arg) {
        h->translate = arg;
        h->dirty = true;
        emit translateChanged(translate());
    }
}

} // namespace osgQtQuick

#include "OSGTransformNode.moc"
