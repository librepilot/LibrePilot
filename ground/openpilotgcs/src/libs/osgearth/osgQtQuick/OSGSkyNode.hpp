#ifndef _H_OSGQTQUICK_SKYNODE_H_
#define _H_OSGQTQUICK_SKYNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QDateTime>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGSkyNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(osgQtQuick::OSGNode *sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

    Q_PROPERTY(bool sunLightEnabled READ sunLightEnabled WRITE setSunLightEnabled NOTIFY sunLightEnabledChanged)
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(double minimumAmbientLight READ minimumAmbientLight WRITE setMinimumAmbientLight NOTIFY minimumAmbientLightChanged)

public:
    OSGSkyNode(QObject *parent = 0);
    virtual ~OSGSkyNode();

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    bool sunLightEnabled();
    void setSunLightEnabled(bool arg);

    QDateTime dateTime();
    void setDateTime(QDateTime arg);

    double minimumAmbientLight();
    void setMinimumAmbientLight(double arg);

signals:
    void sceneDataChanged(OSGNode *node);

    void sunLightEnabledChanged(bool arg);
    void dateTimeChanged(QDateTime arg);
    void minimumAmbientLightChanged(double arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_SKYNODE_H_
