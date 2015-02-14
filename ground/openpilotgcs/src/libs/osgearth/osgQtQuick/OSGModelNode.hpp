#ifndef _H_OSGQTQUICK_MODELNODE_H_
#define _H_OSGQTQUICK_MODELNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QVector3D>
#include <QUrl>

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGModelNode : public OSGNode {
    Q_OBJECT
    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode *modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    Q_PROPERTY(osgQtQuick::OSGNode *sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

    Q_PROPERTY(bool clampToTerrain READ clampToTerrain WRITE setClampToTerrain NOTIFY clampToTerrainChanged)

    Q_PROPERTY(QVector3D attitude READ attitude WRITE setAttitude NOTIFY attitudeChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)

    Q_PROPERTY(bool intoTerrain READ intoTerrain NOTIFY intoTerrainChanged)

public:
    OSGModelNode(QObject *parent = 0);
    virtual ~OSGModelNode();

    OSGNode *modelData();
    void setModelData(OSGNode *node);

    bool clampToTerrain() const;
    void setClampToTerrain(bool arg);

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    QVector3D attitude() const;
    void setAttitude(QVector3D arg);

    QVector3D position() const;
    void setPosition(QVector3D arg);

    bool intoTerrain() const;

signals:
    void modelDataChanged(OSGNode *node);
    void sceneDataChanged(OSGNode *node);

    void clampToTerrainChanged(bool arg);

    void attitudeChanged(QVector3D arg);
    void positionChanged(QVector3D arg);

    void intoTerrainChanged(bool arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_MODELNODE_H_
