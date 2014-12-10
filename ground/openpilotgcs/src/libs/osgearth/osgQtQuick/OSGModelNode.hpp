#ifndef _H_OSGQTQUICK_MODELNODE_H_
#define _H_OSGQTQUICK_MODELNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGModelNode : public OSGNode
{
    Q_OBJECT

    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode* modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    Q_PROPERTY(osgQtQuick::OSGNode* sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

    Q_PROPERTY(qreal roll READ roll WRITE setRoll NOTIFY rollChanged)
    Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(qreal yaw READ yaw WRITE setYaw NOTIFY yawChanged)

    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(double altitude READ altitude WRITE setAltitude NOTIFY altitudeChanged)

public:
    OSGModelNode(QObject *parent = 0);
    virtual ~OSGModelNode();

    OSGNode* modelData();
    void setModelData(OSGNode *node);

    OSGNode* sceneData();
    void setSceneData(OSGNode *node);

    qreal roll() const;
    void setRoll(qreal arg);

    qreal pitch() const;
    void setPitch(qreal arg);

    qreal yaw() const;
    void setYaw(qreal arg);

    double latitude() const;
    void setLatitude(double arg);

    double longitude() const;
    void setLongitude(double arg);

    double altitude() const;
    void setAltitude(double arg);

    virtual void realize();

signals:
    void modelDataChanged(OSGNode *node);
    void sceneDataChanged(OSGNode *node);

    void rollChanged(qreal arg);
    void pitchChanged(qreal arg);
    void yawChanged(qreal arg);

    void latitudeChanged(double arg);
    void longitudeChanged(double arg);
    void altitudeChanged(double arg);

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_MODELNODE_H_
