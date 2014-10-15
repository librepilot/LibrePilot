#include "OSGGroup.hpp"

#include <osg/Group>

#include <QQmlListProperty>
#include <QDebug>

namespace osgQtQuick {

struct OSGGroup::Hidden : public QObject {

    Q_OBJECT

public:
    Hidden(OSGGroup *parent) : QObject(parent), pub(parent) {
        group = new osg::Group;
    }   

    OSGGroup *pub;
    osg::ref_ptr<osg::Group> group;
    QList<OSGNode*> child;
    QMap<OSGNode*, osg::Node*> caced;

    static void append_child(QQmlListProperty<OSGNode> *list, OSGNode *child)
    {
        OSGGroup *group = qobject_cast<OSGGroup*>(list->object);
        if (group && child) {
            group->h->caced[child] = child->node();
            group->h->child.append(child);
            if (child->node()) {
                group->h->group->addChild(child->node());
                emit group->nodeChanged(group->h->group);
            }
            QObject::connect(child, SIGNAL(nodeChanged(osg::Node*)),
                             group->h, SLOT(onNodeChanged(osg::Node*)));
        }
    }

    static int count_child(QQmlListProperty<OSGNode> *list)
    {
        OSGGroup *group = qobject_cast<OSGGroup*>(list->object);
        if (group) {
            return group->h->child.size();
        }

        return 0;
    }

    static OSGNode* at_child(QQmlListProperty<OSGNode> *list, int index)
    {
        OSGGroup *group = qobject_cast<OSGGroup*>(list->object);
        if (group && index >= 0 && index < group->h->child.size()) {
            return group->h->child[index];
        }

        return 0;
    }

    static void clear_child(QQmlListProperty<OSGNode> *list)
    {
        OSGGroup *group = qobject_cast<OSGGroup*>(list->object);
        if (group) {
            while(!group->h->child.isEmpty())
            {
                OSGNode *node = group->h->child.takeLast();
                if (node->parent() == group) node->deleteLater();
                if (!node->parent()) node->deleteLater();
            }
            group->h->group->removeChild(0, group->h->group->getNumChildren());
            group->h->caced.clear();
        }
    }

public slots:
    // Учитываем возможность изменения узла
    void onNodeChanged(osg::Node *node) {
        if (OSGNode *obj = qobject_cast<OSGNode*>(sender())) {
            osg::Node *cacheNode = caced.value(obj, 0);
            if (cacheNode) group->removeChild(cacheNode);
            if (node) {
                group->addChild(node);
            }
            caced[obj] = node;
            emit pub->nodeChanged(group.get());
        }
    }
};

OSGGroup::OSGGroup(QObject *parent) :
    OSGNode(parent), h(new Hidden(this))
{
    setNode(h->group.get());
}

OSGGroup::~OSGGroup()
{
}

QQmlListProperty<OSGNode> OSGGroup::child()
{
    return QQmlListProperty<OSGNode>(this, 0,
                                     &Hidden::append_child,
                                     &Hidden::count_child,
                                     &Hidden::at_child,
                                     &Hidden::clear_child);
}

} // namespace osgQtQuick

#include "OSGGroup.moc"
