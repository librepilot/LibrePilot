#ifndef _H_OSGQTQUICK_CUBENODE_H_
#define _H_OSGQTQUICK_CUBENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGCubeNode : public OSGNode
{
    Q_OBJECT

//    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
//    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged)

public:
    OSGCubeNode(QObject *parent = 0);
    ~OSGCubeNode();

//    const QUrl source() const;
//    void setSource(const QUrl &url);

//    bool async() const;
//    void setAsync(const bool async);

//signals:
//    void sourceChanged(const QUrl &url);
//    void asyncChanged(const bool async);

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_CUBENODE_H_
