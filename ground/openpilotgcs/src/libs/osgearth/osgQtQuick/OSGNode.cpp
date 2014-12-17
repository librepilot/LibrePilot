#include "OSGNode.hpp"

#include <osg/Node>

#include <QListIterator>

namespace osgQtQuick {
struct OSGNode::Hidden {
    osg::ref_ptr<osg::Node> node;
};

OSGNode::OSGNode(QObject *parent) : QObject(parent), h(new Hidden)
{}

OSGNode::~OSGNode()
{
    delete h;
}

osg::Node *OSGNode::node() const
{
    return h->node.get();
}

void OSGNode::setNode(osg::Node *node)
{
    if (h->node != node) {
        h->node = node;
        emit nodeChanged(node);
    }
}

void OSGNode::realize()
{
    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        OSGNode *node = qobject_cast<OSGNode *>(i.next());
        if (node) {
            node->realize();
        }
    }
}
} // namespace osgQtQuick
