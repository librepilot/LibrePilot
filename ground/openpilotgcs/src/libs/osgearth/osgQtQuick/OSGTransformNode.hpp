#ifndef _H_OSGQTQUICK_TRANSFORMNODE_H_
#define _H_OSGQTQUICK_TRANSFORMNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGTransformNode : public OSGNode {
    Q_OBJECT
    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode *modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)

// Q_PROPERTY(qreal roll READ roll WRITE setRoll NOTIFY rollChanged)
// Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
// Q_PROPERTY(qreal yaw READ yaw WRITE setYaw NOTIFY yawChanged)

    Q_PROPERTY(double altitude READ altitude WRITE setAltitude NOTIFY altitudeChanged)

public:
    OSGTransformNode(QObject *parent = 0);
    virtual ~OSGTransformNode();

    OSGNode *modelData();
    void setModelData(OSGNode *node);

    double altitude() const;
    void setAltitude(double arg);

signals:
    void modelDataChanged(OSGNode *node);

    void altitudeChanged(double arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_TRANSFORMNODE_H_
