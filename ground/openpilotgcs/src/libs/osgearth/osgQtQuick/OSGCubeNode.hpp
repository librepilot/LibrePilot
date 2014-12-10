#ifndef _H_OSGQTQUICK_CUBENODE_H_
#define _H_OSGQTQUICK_CUBENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGCubeNode : public OSGNode
{
    Q_OBJECT

public:
    OSGCubeNode(QObject *parent = 0);
    virtual ~OSGCubeNode();

    virtual void realize();

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_CUBENODE_H_
