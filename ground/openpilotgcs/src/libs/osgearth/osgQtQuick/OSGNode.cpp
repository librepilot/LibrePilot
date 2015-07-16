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
    if (h->node.get() != node) {
        h->node = node;
        emit nodeChanged(node);
    }
}

bool OSGNode::attach(osgViewer::View *view)
{
    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        OSGNode *node = qobject_cast<OSGNode *>(i.next());
        if (node) {
            node->attach(view);
        }
    }
    return true;
}

bool OSGNode::detach(osgViewer::View *view)
{
    QListIterator<QObject *> i(children());
    while (i.hasNext()) {
        OSGNode *node = qobject_cast<OSGNode *>(i.next());
        if (node) {
            node->detach(view);
        }
    }
    return true;
}
} // namespace osgQtQuick
