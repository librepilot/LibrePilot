#ifndef _H_OSGQTQUICK_SKYNODE_H_
#define _H_OSGQTQUICK_SKYNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGSkyNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(osgQtQuick::OSGNode *sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

public:
    OSGSkyNode(QObject *parent = 0);
    virtual ~OSGSkyNode();

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

signals:
    void sceneDataChanged(OSGNode *node);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_SKYNODE_H_
