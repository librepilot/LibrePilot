#ifndef _H_OSGQTQUICK_TRANSFORMNODE_H_
#define _H_OSGQTQUICK_TRANSFORMNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include "QVector3D"

// TODO derive from OSGGroup...
namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGTransformNode : public OSGNode {
    Q_OBJECT
    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode *modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)

    Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(QVector3D rotate READ rotate WRITE setRotate NOTIFY rotateChanged)
    Q_PROPERTY(QVector3D translate READ translate WRITE setTranslate NOTIFY translateChanged)

public:
    OSGTransformNode(QObject *parent = 0);
    virtual ~OSGTransformNode();

    OSGNode *modelData();
    void setModelData(OSGNode *node);

    QVector3D scale() const;
    void setScale(QVector3D arg);

    QVector3D rotate() const;
    void setRotate(QVector3D arg);

    QVector3D translate() const;
    void setTranslate(QVector3D arg);

signals:
    void modelDataChanged(OSGNode *node);

    void scaleChanged(QVector3D arg);
    void rotateChanged(QVector3D arg);
    void translateChanged(QVector3D arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_TRANSFORMNODE_H_
